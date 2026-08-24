// Copyright (c) Red Alert 4 project. See RA4AutoPlayDriver.h.
#include "RA4AutoPlayDriver.h"
#include "RA4SimWorldSubsystem.h"
#include "Engine/World.h"
#include "Misc/CString.h"

#include "RA4Simulation/Public/RA4Simulation/SimWorld.h"
#include "RA4Simulation/Public/RA4Simulation/SimTypes.h"
#include "RA4Simulation/Public/RA4Simulation/MapFile.h"
#include "RA4Content/Public/RA4Content/ContentTypes.h"
#include "RA4Core/Public/RA4Core/Command.h"
#include "RA4Core/Public/RA4Core/Ids.h"

namespace
{
	enum class Step
	{
		Idle,
		FindMcv,
		WaitConYard,
		QueuePower,
		WaitPowerPlaced,
		QueueRefinery,
		WaitRefineryPlaced,
		WaitHarvest,
		QueueFactory,
		WaitFactoryPlaced,
		TrainUnits,
		WaitUnits,
		Attack,
		Done,
		Failed,
	};

	// Forward declarations: helpers defined later in this namespace.
	RA4::TileCoord FindPlaceTile(const RA4::SimWorld& World, const RA4::EntityDef* Def);
	void Enqueue(URA4SimWorldSubsystem& Subsystem, const RA4::Command& C);
	int32 CountOwnedBuildings(const RA4::SimWorld& World, const RA4::EntityDef* Def, RA4::PlayerId Owner);

	Step CurrentStep = Step::Idle;
	float StepElapsed = 0.0f;
	float PollCooldown = 0.0f;
	int32 CreditsAtHarvestStart = 0;
	bool bAttackOrdered = false;
	bool bGuardOrdered = false;
	int32 RefineryDiagPolls = 0;
	int32 QueueDiagPolls = 0;
	int32 RequeueCooldown = 0;
	int32 LastLoggedStep = -1;

	constexpr float kPollInterval = 0.5f;      // poll the world twice a second
	constexpr float kStepTimeout = 300.0f;     // a step that takes 3 minutes is stuck
	constexpr int32  kHarvestCreditDelta = 300; // refinery cycle that proves the economy
	constexpr int32_t kMyProgressScale = 100;   // matches SimWorld.cpp's kProgressScale

	const TCHAR* StepName(Step S)
	{
		switch (S)
		{
		case Step::Idle: return TEXT("Idle");
		case Step::FindMcv: return TEXT("FindMcv");
		case Step::WaitConYard: return TEXT("WaitConYard");
		case Step::QueuePower: return TEXT("QueuePower");
		case Step::WaitPowerPlaced: return TEXT("WaitPowerPlaced");
		case Step::QueueRefinery: return TEXT("QueueRefinery");
		case Step::WaitRefineryPlaced: return TEXT("WaitRefineryPlaced");
		case Step::WaitHarvest: return TEXT("WaitHarvest");
		case Step::QueueFactory: return TEXT("QueueFactory");
		case Step::WaitFactoryPlaced: return TEXT("WaitFactoryPlaced");
		case Step::TrainUnits: return TEXT("TrainUnits");
		case Step::WaitUnits: return TEXT("WaitUnits");
		case Step::Attack: return TEXT("Attack");
		case Step::Done: return TEXT("Done");
		case Step::Failed: return TEXT("Failed");
		}
		return TEXT("?");
	}

	void Log(const TCHAR* Message)
	{
		UE_LOG(LogTemp, Display, TEXT("RA4AutoPlay: %s"), Message);
	}

	RA4::PlayerId LocalPlayer()
	{
		return 0;
	}

	const RA4::EntityDef* FindDef(const RA4::SimWorld& World, const char* Name)
	{
		return World.GetContent() ? World.GetContent()->FindEntityByName(Name) : nullptr;
	}

	RA4::EntityId FindEntityByDef(const RA4::SimWorld& World, RA4::PlayerId Owner,
		const RA4::EntityDef* Def, RA4::EntityKind Kind, bool bRequireComplete)
	{
		if (Def == nullptr)
		{
			return RA4::EntityId::Invalid();
		}
		const auto& Cores = World.GetAllCores();
		for (uint32_t I = 0; I < Cores.size(); ++I)
		{
			const auto& C = Cores[I];
			if (!C.bAlive || C.Owner != Owner || C.Kind != Kind || C.Def != Def->Id)
			{
				continue;
			}
			if (bRequireComplete && Kind == RA4::EntityKind::Building)
			{
				// A queued building is not a working one: the completion state
				// lives on the building component.
				const RA4::BuildingComp* B = World.GetBuilding(World.MakeId(I));
				if (B == nullptr || B->State != RA4::ConstructionState::Complete)
				{
					continue;
				}
			}
			return World.MakeId(I);
		}
		return RA4::EntityId::Invalid();
	}

	// Self-healing economy step: make sure `DefName` ends up standing in the
	// world. Whatever happened -- the item never queued, the queue emptied, or
	// the placed building got destroyed -- re-issue what is missing. Returns
	// true once the building exists.
	bool EnsureBuilding(const RA4::SimWorld& World, URA4SimWorldSubsystem& Subsystem,
		const char* DefName)
	{
		const RA4::EntityDef* Def = FindDef(World, DefName);
		if (Def == nullptr)
		{
			return false;
		}
		const RA4::PlayerId Issuer = LocalPlayer();
		if (CountOwnedBuildings(World, Def, Issuer) > 0)
		{
			return true;
		}
		const RA4::EntityId Yard = FindEntityByDef(World, Issuer,
			FindDef(World, "building.sov.construction_yard"), RA4::EntityKind::Building, true);
		if (!Yard.IsValid())
		{
			return false;   // no yard: nothing to build with
		}
		const RA4::BuildingComp* B = World.GetBuilding(Yard);
		bool bQueued = false;
		bool bReadyToPlace = false;
		if (B != nullptr)
		{
			for (const RA4::ProductionItem& Item : B->Queue)
			{
				if (Item.Content != Def->Id)
				{
					continue;
				}
				bQueued = true;
				if (Item.ProgressTicks >= Item.TotalTicks * kMyProgressScale)
				{
					bReadyToPlace = true;
				}
				break;
			}
		}
		if (bReadyToPlace)
		{
			const RA4::TileCoord T = FindPlaceTile(World, Def);
			if (T.X > 0)
			{
				RA4::Command C;
				C.Type = RA4::CommandType::PlaceBuilding;
				C.Issuer = Issuer;
				C.Content = Def->Id;
				C.Tile = T;
				Enqueue(Subsystem, C);
			}
			return false;
		}
		if (!bQueued)
		{
			// Backoff: a rejected StartProduction (prerequisites still building,
			// queue full) stays rejected for a while; re-issuing every poll
			// spams the log and the command bus for nothing.
			if (++RequeueCooldown < 4)
			{
				return false;
			}
			RequeueCooldown = 0;
			RA4::Command C;
			C.Type = RA4::CommandType::StartProduction;
			C.Issuer = Issuer;
			C.Content = Def->Id;
			Enqueue(Subsystem, C);
			UE_LOG(LogTemp, Display, TEXT("RA4AutoPlay: re-queued %hs"), DefName);
		}
		else if (B != nullptr && (++QueueDiagPolls % 10) == 0)
		{
			// Stuck-queue diagnostics: the item state says starved vs throttled
			// vs paused, which is the difference between a money bug and a
			// power bug and neither is guessable from outside.
			for (const RA4::ProductionItem& Item : B->Queue)
			{
				UE_LOG(LogTemp, Display,
					TEXT("RA4AutoPlay: queue diag progress=%d/%d state=%d paid=%d cost=%d"),
					Item.ProgressTicks, Item.TotalTicks,
					(int32)Item.State, Item.PaidCredits, Item.TotalCost);
			}
			const RA4::PlayerState& P = World.GetPlayer(Issuer);
			UE_LOG(LogTemp, Display,
				TEXT("RA4AutoPlay: credits=%d power +%d/-%d"),
				P.Credits, P.PowerProduced, P.PowerConsumed);
		}
		return false;
	}

	// Authoritative check: does the player own a placed building of this type?
	// (Queued-in-yard does not count -- the item must exist in the world.)
	int32 CountOwnedBuildings(const RA4::SimWorld& World, const RA4::EntityDef* Def, RA4::PlayerId Owner)
	{
		if (Def == nullptr)
		{
			return 0;
		}
		int32 Count = 0;
		const auto& Cores = World.GetAllCores();
		for (uint32_t I = 0; I < Cores.size(); ++I)
		{
			const auto& C = Cores[I];
			if (C.bAlive && C.Owner == Owner && C.Kind == RA4::EntityKind::Building && C.Def == Def->Id)
			{
				++Count;
			}
		}
		return Count;
	}

	// Issue PlaceBuilding only once the yard has finished the item; an early
	// placement command is rejected by the sim and the building never lands.
	void PlaceWhenYardReady(const RA4::SimWorld& World, URA4SimWorldSubsystem& Subsystem,
		const RA4::EntityDef* Def)
	{
		if (Def == nullptr)
		{
			return;
		}
		const RA4::EntityId Yard = FindEntityByDef(World, LocalPlayer(),
			FindDef(World, "building.sov.construction_yard"), RA4::EntityKind::Building, true);
		if (!Yard.IsValid())
		{
			return;
		}
		const RA4::BuildingComp* B = World.GetBuilding(Yard);
		if (B == nullptr)
		{
			return;
		}
		for (const RA4::ProductionItem& Item : B->Queue)
		{
			if (Item.Content == Def->Id &&
				Item.ProgressTicks >= Item.TotalTicks * kMyProgressScale)
			{
				const RA4::TileCoord T = FindPlaceTile(World, Def);
				if (T.X > 0)
				{
					RA4::Command C;
					C.Type = RA4::CommandType::PlaceBuilding;
					C.Issuer = LocalPlayer();
					C.Content = Def->Id;
					C.Tile = T;
					Enqueue(Subsystem, C);
				}
				return;
			}
		}
	}

	RA4::TileCoord FindPlaceTile(const RA4::SimWorld& World, const RA4::EntityDef* Def)
	{
		// Spiral out from the construction yard until the sim says the footprint fits.
		const RA4::EntityId Yard = FindEntityByDef(World, LocalPlayer(),
			FindDef(World, "building.sov.construction_yard"), RA4::EntityKind::Building, true);
		if (!Yard.IsValid())
		{
			return RA4::TileCoord{ -1, -1 };
		}
		const RA4::TileCoord Centre = World.GetTransform(Yard) != nullptr
			? World.GetMap().WorldToTile(World.GetTransform(Yard)->Position)
			: RA4::TileCoord{ -1, -1 };

		for (int32 Radius = 0; Radius <= 12; ++Radius)
		{
			for (int32 DY = -Radius; DY <= Radius; ++DY)
			{
				for (int32 DX = -Radius; DX <= Radius; ++DX)
				{
					if (FMath::Max(FMath::Abs(DX), FMath::Abs(DY)) != Radius)
					{
						continue;
					}
					RA4::TileCoord T{ Centre.X + DX, Centre.Y + DY };
					if (World.IsPlacementValid(Def->Id, LocalPlayer(), T))
					{
						return T;
					}
				}
			}
		}
		return RA4::TileCoord{ -1, -1 };
	}

	void Enqueue(URA4SimWorldSubsystem& Subsystem, const RA4::Command& C)
	{
		Subsystem.EnqueueCommand(C);
	}

	// Authoritative check: does the player own a building of this type at all
	// (queued-in-yard does not count -- it must have been placed in the world)?

	bool FindEnemyBasePosition(const RA4::SimWorld& World, RA4::Vec2& OutPos)
	{
		const auto& Cores = World.GetAllCores();
		// Prefer buildings (the win condition), fall back to any enemy entity:
		// a razed-in-buildings AI still fields armed units, and the match only
		// ends when its last military capability dies too.
		for (uint32_t I = 0; I < Cores.size(); ++I)
		{
			const auto& C = Cores[I];
			if (C.bAlive && C.Kind == RA4::EntityKind::Building && C.Owner != LocalPlayer() &&
				C.Owner < RA4::kMaxPlayers && World.GetPlayer(C.Owner).bActive)
			{
				const auto* T = World.GetTransform(World.MakeId(I));
				if (T != nullptr)
				{
					OutPos = T->Position;
					return true;
				}
			}
		}
		for (uint32_t I = 0; I < Cores.size(); ++I)
		{
			const auto& C = Cores[I];
			if (C.bAlive && C.Kind == RA4::EntityKind::Unit && C.Owner != LocalPlayer() &&
				C.Owner < RA4::kMaxPlayers && World.GetPlayer(C.Owner).bActive)
			{
				const auto* T = World.GetTransform(World.MakeId(I));
				if (T != nullptr)
				{
					OutPos = T->Position;
					return true;
				}
			}
		}
		return false;
	}
} // namespace

void RA4AutoPlay::Reset()
{
	CurrentStep = Step::Idle;
	StepElapsed = 0.0f;
	PollCooldown = 0.0f;
	CreditsAtHarvestStart = 0;
	bAttackOrdered = false;
	bGuardOrdered = false;
	LastLoggedStep = -1;
}

void RA4AutoPlay::Tick(URA4SimWorldSubsystem& Subsystem, float DeltaTime)
{
	if (CurrentStep == Step::Done || CurrentStep == Step::Failed)
	{
		return;
	}

	const RA4::SimWorld* IdleWorld = Subsystem.GetSimWorld();
	if (CurrentStep == Step::Idle)
	{
		// Engage as soon as a match holds the player's starting MCV.
		if (IdleWorld != nullptr &&
			FindEntityByDef(*IdleWorld, LocalPlayer(), FindDef(*IdleWorld, "unit.sov.mcv"),
				RA4::EntityKind::Unit, false).IsValid())
		{
			Log(TEXT("engaged -- starting MCV detected"));
			CurrentStep = Step::FindMcv;
			PollCooldown = 0.0f;
		}
		return;
	}

	PollCooldown -= DeltaTime;
	StepElapsed += DeltaTime;
	if (PollCooldown > 0.0f)
	{
		return;
	}
	PollCooldown = kPollInterval;

	const RA4::SimWorld* World = Subsystem.GetSimWorld();
	if (World == nullptr)
	{
		return;
	}

	// Match over? That outranks whatever step we were on.
	const RA4::PlayerId Winner = World->GetWinner();
	const bool bLocalDefeated = World->GetPlayer(LocalPlayer()).bDefeated;
	if (Winner != RA4::kInvalidPlayer || bLocalDefeated)
	{
		UE_LOG(LogTemp, Display, TEXT("RA4AutoPlay: RESULT %s (winner=%d) after step %s"),
			Winner == LocalPlayer() ? TEXT("WIN") : TEXT("LOSE"), (int32)Winner, StepName(CurrentStep));
		CurrentStep = Step::Done;
		return;
	}

	if (StepElapsed > kStepTimeout)
	{
		UE_LOG(LogTemp, Error, TEXT("RA4AutoPlay: TIMEOUT in step %s after %.0fs"),
			StepName(CurrentStep), StepElapsed);
		CurrentStep = Step::Failed;
		return;
	}

	// Announce the step once.
	if ((int32)CurrentStep != LastLoggedStep)
	{
		FString Msg = FString::Printf(TEXT("step -> %s"), StepName(CurrentStep));
		UE_LOG(LogTemp, Display, TEXT("RA4AutoPlay: %s"), *Msg);
		LastLoggedStep = (int32)CurrentStep;
		StepElapsed = 0.0f;
	}

	const RA4::PlayerId Issuer = LocalPlayer();

	switch (CurrentStep)
	{
	case Step::FindMcv:
	{
		// The AI rushes with its starting army; a player who never touches
		// their own starting army dies in two minutes. Park it on guard at
		// the MCV so the rush meets a wall.
		const RA4::EntityDef* McvDef0 = FindDef(*World, "unit.sov.mcv");
		const RA4::EntityId McvForGuard = FindEntityByDef(*World, Issuer, McvDef0, RA4::EntityKind::Unit, false);
		if (McvForGuard.IsValid())
		{
			const auto* GuardAt = World->GetTransform(McvForGuard);
			if (GuardAt != nullptr && !bGuardOrdered)
			{
				const auto& Cores = World->GetAllCores();
				for (uint32_t I = 0; I < Cores.size(); ++I)
				{
					const auto& C = Cores[I];
					if (!C.bAlive || C.Owner != Issuer || C.Kind != RA4::EntityKind::Unit || World->MakeId(I) == McvForGuard)
					{
						continue;
					}
					const auto* Def = World->GetContent() ? World->GetContent()->FindEntity(C.Def) : nullptr;
					if (Def == nullptr || Def->Unit.bIsBuilder || Def->Name.find("harvester") != std::string::npos)
					{
						continue;
					}
					RA4::Command G;
					G.Type = RA4::CommandType::Guard;
					G.Issuer = Issuer;
					G.Primary = World->MakeId(I);
					G.Location = GuardAt->Position;
					Enqueue(Subsystem, G);
				}
				bGuardOrdered = true;
				Log(TEXT("starting army ordered to guard the base"));
			}
		}

		const RA4::EntityDef* Mcv = FindDef(*World, "unit.sov.mcv");
		const RA4::EntityId McvId = FindEntityByDef(*World, Issuer, Mcv, RA4::EntityKind::Unit, false);
		if (!McvId.IsValid())
		{
			Log(TEXT("no starting MCV found -- FAIL"));
			CurrentStep = Step::Failed;
			return;
		}
		RA4::Command C;
		C.Type = RA4::CommandType::Deploy;
		C.Issuer = Issuer;
		C.Primary = McvId;
		if (const auto* T = World->GetTransform(McvId))
		{
			C.Tile = World->GetMap().WorldToTile(T->Position);
		}
		Enqueue(Subsystem, C);
		Log(TEXT("Deploy issued on starting MCV"));
		CurrentStep = Step::WaitConYard;
		return;
	}
	case Step::WaitConYard:
	{
		if (FindEntityByDef(*World, Issuer, FindDef(*World, "building.sov.construction_yard"),
			RA4::EntityKind::Building, true).IsValid())
		{
			Log(TEXT("Construction Yard complete"));
			CurrentStep = Step::QueuePower;
		}
		return;
	}
	case Step::QueuePower:
	{
		const RA4::EntityDef* Def = FindDef(*World, "building.sov.tesla_reactor");
		if (Def == nullptr) { Log(TEXT("tesla_reactor def missing -- FAIL")); CurrentStep = Step::Failed; return; }
		RA4::Command C;
		C.Type = RA4::CommandType::StartProduction;
		C.Issuer = Issuer;
		C.Content = Def->Id;
		Enqueue(Subsystem, C);
		Log(TEXT("Power plant queued in ConYard"));
		CurrentStep = Step::WaitPowerPlaced;
		return;
	}
	case Step::WaitPowerPlaced:
	{
		if (EnsureBuilding(*World, Subsystem, "building.sov.tesla_reactor"))
		{
			Log(TEXT("Power plant in world"));
			CurrentStep = Step::QueueRefinery;
		}
		return;
	}
	case Step::QueueRefinery:
	{
		const RA4::EntityDef* Def = FindDef(*World, "building.sov.ore_refinery");
		if (Def == nullptr) { Log(TEXT("ore_refinery def missing -- FAIL")); CurrentStep = Step::Failed; return; }
		RA4::Command C;
		C.Type = RA4::CommandType::StartProduction;
		C.Issuer = Issuer;
		C.Content = Def->Id;
		Enqueue(Subsystem, C);
		Log(TEXT("Refinery queued"));
		CurrentStep = Step::WaitRefineryPlaced;
		return;
	}
	case Step::WaitRefineryPlaced:
	{
		// EnsureBuilding, not one-shot placement: the first StartProduction can
		// be rejected (the just-placed power plant is still under construction,
		// so the refinery's prerequisite is unmet), and the placed refinery can
		// later be destroyed by a raid. Either way, re-issue what is missing.
		if (EnsureBuilding(*World, Subsystem, "building.sov.ore_refinery"))
		{
			Log(TEXT("Refinery in world -- waiting for the first harvest cycle"));
			CreditsAtHarvestStart = World->GetPlayer(Issuer).Credits;
			CurrentStep = Step::WaitHarvest;
		}
		return;
	}
	case Step::WaitHarvest:
	{
		const int32 Now = World->GetPlayer(Issuer).Credits;
		if (Now >= CreditsAtHarvestStart + kHarvestCreditDelta)
		{
			UE_LOG(LogTemp, Display, TEXT("RA4AutoPlay: harvest cycle proven (+%d credits)"),
				Now - CreditsAtHarvestStart);
			CurrentStep = Step::QueueFactory;
		}
		return;
	}
	case Step::QueueFactory:
	{
		const RA4::EntityDef* Def = FindDef(*World, "building.sov.war_factory");
		if (Def == nullptr) { Log(TEXT("war_factory def missing -- FAIL")); CurrentStep = Step::Failed; return; }
		RA4::Command C;
		C.Type = RA4::CommandType::StartProduction;
		C.Issuer = Issuer;
		C.Content = Def->Id;
		Enqueue(Subsystem, C);
		Log(TEXT("War factory queued"));
		CurrentStep = Step::WaitFactoryPlaced;
		return;
	}
	case Step::WaitFactoryPlaced:
	{
		if (EnsureBuilding(*World, Subsystem, "building.sov.war_factory"))
		{
			Log(TEXT("War factory in world"));
			CurrentStep = Step::TrainUnits;
		}
		return;
	}
	case Step::TrainUnits:
	{
		const char* Units[] = { "unit.sov.conscript", "unit.sov.conscript", "unit.sov.heavy_tank" };
		for (const char* Name : Units)
		{
			const RA4::EntityDef* Def = FindDef(*World, Name);
			if (Def == nullptr) { continue; }
			RA4::Command C;
			C.Type = RA4::CommandType::StartProduction;
			C.Issuer = Issuer;
			C.Content = Def->Id;
			Enqueue(Subsystem, C);
		}
		Log(TEXT("Combat units queued (2x conscript, 1x heavy tank)"));
		CurrentStep = Step::WaitUnits;
		return;
	}
	case Step::WaitUnits:
	{
		int32 CombatUnits = 0;
		const auto& Cores = World->GetAllCores();
		for (uint32_t I = 0; I < Cores.size(); ++I)
		{
			const auto& C = Cores[I];
			if (!C.bAlive || C.Owner != Issuer || C.Kind != RA4::EntityKind::Unit)
			{
				continue;
			}
			const auto* Def = World->GetContent() ? World->GetContent()->FindEntity(C.Def) : nullptr;
			if (Def != nullptr && !Def->Unit.bIsBuilder && Def->Name.find("harvester") == std::string::npos)
			{
				++CombatUnits;
			}
		}
		if (CombatUnits >= 3)
		{
			UE_LOG(LogTemp, Display, TEXT("RA4AutoPlay: %d combat units ready"), CombatUnits);
			CurrentStep = Step::Attack;
		}
		return;
	}
	case Step::Attack:
	{
		// Order once; re-order only when the enemy has nothing left at all.
		// Spamming attack-move every poll buried the log and re-pathed the
		// army constantly.
		RA4::Vec2 EnemyPos;
		if (!FindEnemyBasePosition(*World, EnemyPos))
		{
			Log(TEXT("no enemy entity left -- awaiting the victory verdict"));
			CurrentStep = Step::Done;
			return;
		}
		if (!bAttackOrdered)
		{
			int32 Ordered = 0;
			const auto& Cores = World->GetAllCores();
			for (uint32_t I = 0; I < Cores.size(); ++I)
			{
				const auto& C = Cores[I];
				if (!C.bAlive || C.Owner != Issuer || C.Kind != RA4::EntityKind::Unit)
				{
					continue;
				}
				const auto* Def = World->GetContent() ? World->GetContent()->FindEntity(C.Def) : nullptr;
				if (Def == nullptr || Def->Unit.bIsBuilder || Def->Name.find("harvester") != std::string::npos)
				{
					continue;
				}
				RA4::Command Cmd;
				Cmd.Type = RA4::CommandType::AttackMove;
				Cmd.Issuer = Issuer;
				Cmd.Primary = World->MakeId(I);
				Cmd.Location = EnemyPos;
				Enqueue(Subsystem, Cmd);
				++Ordered;
			}
			UE_LOG(LogTemp, Display, TEXT("RA4AutoPlay: attack-move ordered on %d units toward the enemy"), Ordered);
			bAttackOrdered = true;
		}
		// The result check at the top of Tick closes the run when the sim
		// declares the AI defeated (last military capability gone).
		return;
	}
	default:
		return;
	}
}
