// Copyright (c) Red Alert 4 project.

#include "RA4SimWorldSubsystem.h"
#include "RA4NetworkManager.h"
#include "RA4AI/AICommander.h"
#include "RA4AI/AIStrategy.h"
#include "RA4Simulation/SimWorld.h"
#include "RA4Content/ContentDatabase.h"
#include "CampaignDatabase.h"
#include "MissionRuntime.h"
#include "RA4Presentation/HudSnapshot.h"
#include "RA4Core/Command.h"
#include "RA4Core/SimConfig.h"
#include "RA4MatchBootstrap.h"
#include "RA4AudioSubsystem.h"
#include "RA4SimCoords.h"
#include "RA4UIDataProviderSubsystem.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "LandscapeProxy.h"
#include "HAL/IConsoleManager.h"

// Off by default: this is a debugging aid, and with it on every shot draws a bright
// yellow tracer over the battlefield.
static TAutoConsoleVariable<int32> CVarRA4DebugCombatDraw(
    TEXT("ra4.DebugCombatDraw"),
    0,
    TEXT("Draw debug lines for weapon fire and impacts (0 = off, 1 = on)."),
    ECVF_Cheat);

URA4SimWorldSubsystem::URA4SimWorldSubsystem()
    : SimWorld(nullptr)
    , Content(nullptr)
    , TimeSinceLastSimTick(0.0f)
{
    // A Blueprint may replace this with a faction-specific presentation actor, but
    // the native skirmish must remain visible with no editor-authored setup.
    EntityActorClass = ARA4EntityActor::StaticClass();
}

URA4SimWorldSubsystem::~URA4SimWorldSubsystem()
{
}

bool URA4SimWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    const FString PackageName = World != nullptr ? World->GetOutermost()->GetName() : FString();
    const bool bMenuWorld = PackageName.EndsWith(TEXT("/Entry"));
    return !bMenuWorld
        && !FParse::Param(FCommandLine::Get(), TEXT("RA4UIOnly"))
        && Super::ShouldCreateSubsystem(Outer);
}

void URA4SimWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Content is created first and outlives the simulation, which keeps a raw
    // pointer to it for the whole match.
    Content = new RA4::ContentDatabase();
    SimWorld = new RA4::SimWorld();
    SnapshotBuilder = new RA4::Presentation::HudSnapshotBuilder();
    SnapshotBuilder->Initialize(/*LocalPlayer*/ 0);
    LatestSnapshot = new RA4::Presentation::HudSnapshot();

    // Simulation startup deferred until StartSkirmishMatch is called by GameMode.
    RegisterDefaultBlockoutMeshes();
    // Not called here: at this point in world startup the persistent level's own
    // baked actors (a landscape included) are not registered yet, so the check for
    // "does the map already have real terrain" would always miss and add a
    // redundant flat plane on top of it. Deferred to the first Tick instead.
}

namespace
{
// The setup widget emits Difficulty as a plain int (0/1/2); clamp anything out of
// range to Hard rather than leave a commander unconfigured.
RA4::AI::AIDifficulty MapIntToAIDifficulty(int32 Difficulty)
{
    if (Difficulty <= 0)
    {
        return RA4::AI::AIDifficulty::Easy;
    }
    if (Difficulty == 1)
    {
        return RA4::AI::AIDifficulty::Normal;
    }
    return RA4::AI::AIDifficulty::Hard;
}
} // namespace

void URA4SimWorldSubsystem::AttachAICommanders(int32 Difficulty, uint64 Seed, RA4::PlayerId LocalPlayer)
{
    const RA4::AI::AIDifficulty MappedDifficulty = MapIntToAIDifficulty(Difficulty);

    for (RA4::PlayerId Player = 0; Player < RA4::kMaxPlayers; ++Player)
    {
        if (Player == LocalPlayer || !SimWorld->GetPlayer(Player).bActive)
        {
            continue;
        }
        RA4::AI::AICommander* Commander = new RA4::AI::AICommander();
        Commander->Initialize(Player, RA4::AI::AIProfile::Balanced, Seed ^ (uint64(Player) * 0x9E3779B9ull));
        // Initialize builds the Normal-difficulty config; the URL-selected difficulty
        // refines it (decision cadence, memory cadence, Hard income bonus).
        Commander->SetConfig(RA4::AI::MakeProfileConfig(RA4::AI::AIProfile::Balanced, MappedDifficulty));
        AICommanders.push_back(Commander);
        UE_LOG(LogTemp, Display, TEXT("RA4 AI commander attached to player %d (profile=%s difficulty=%s)"),
               int32(Player), UTF8_TO_TCHAR(RA4::AI::ToString(RA4::AI::AIProfile::Balanced)),
               UTF8_TO_TCHAR(RA4::AI::ToString(MappedDifficulty)));
    }
}

void URA4SimWorldSubsystem::StartSkirmishMatch(uint8 PlayerFaction, uint8 EnemyFaction, int32 Difficulty, int32 NumAI, int32 AISpot)
{
    // The lobby or GameMode supplies the match setup.
    FRA4MatchBootstrap::BuildSkirmish(*Content, *SimWorld, /*Seed*/ 20260728,
        static_cast<RA4::FactionId>(PlayerFaction), static_cast<RA4::FactionId>(EnemyFaction),
        NumAI, AISpot);
    bWasLocalPowerShortage = false;

    AttachAICommanders(Difficulty, 20260728ull, /*LocalPlayer*/ 0);

    UE_LOG(LogTemp, Display, TEXT("RA4 skirmish initialized with %llu simulation entities"),
           static_cast<uint64>(SimWorld->GetAllCores().size()));
}

bool URA4SimWorldSubsystem::StartCampaignMission(const FString& MissionId, int32 Difficulty)
{
    if (SimWorld == nullptr || Content == nullptr)
    {
        return false;
    }

    if (Campaign == nullptr)
    {
        Campaign = new RA4::CampaignDatabase();
    }

    const std::string Id(TCHAR_TO_UTF8(*MissionId));
    const RA4::CampaignMissionDef* Def = Campaign->FindMission(Id);
    if (Def == nullptr)
    {
        // Deliberately not falling back to a default mission. Loading something the
        // player did not pick is worse than not loading: the briefing, the objectives
        // and the debrief would all describe a different mission than the one running.
        UE_LOG(LogTemp, Error, TEXT("RA4 campaign: no mission '%s' in the database; no match started."),
               *MissionId);
        return false;
    }

    RA4::BuildDefaultContent(*Content);

    std::vector<std::string> Errors;
    if (!Content->Validate(Errors))
    {
        for (const std::string& Error : Errors)
        {
            UE_LOG(LogTemp, Error, TEXT("RA4 content validation: %s"), UTF8_TO_TCHAR(Error.c_str()));
        }
    }

    // The mission describes its own match. This is the same pair of calls the headless
    // mission tests make, so what plays here is what those tests proved playable.
    SimWorld->Initialize(Content, RA4::BuildMissionMatchSetup(*Def));
    const int32 Placed = RA4::SpawnMissionEntities(*SimWorld, *Def);
    const int32 Declared = int32(Def->Setup.Spawns.size());

    bWasLocalPowerShortage = false;
    ActiveMissionId = MissionId;
    bMissionResultReported = false;

    if (Mission == nullptr)
    {
        Mission = new RA4::MissionRuntime();
    }
    Mission->Begin(*Def, /*LocalPlayer*/ 0, SimWorld->GetTick());

    AttachAICommanders(Difficulty, RA4::MakeContentId(Def->MissionId.c_str()).Value, /*LocalPlayer*/ 0);

    // A mission whose content this build does not have places fewer entities than it
    // declares and is still brought up, because a half-populated map is diagnosable
    // and a refusal to start is not. Eastern Coalition and Chrono Legion missions are
    // in exactly this state until those factions exist in the content database.
    if (Placed < Declared)
    {
        UE_LOG(LogTemp, Warning,
               TEXT("RA4 campaign mission '%s': placed %d of %d declared spawns; the rest name content this build does not have."),
               *MissionId, Placed, Declared);
    }

    UE_LOG(LogTemp, Display, TEXT("RA4 campaign mission '%s' started with %d entities and %llu objectives."),
           *MissionId, Placed, static_cast<uint64>(Def->Objectives.size()));
    return true;
}

// The map ships one ground plate, and it was sized for a smaller world than the one
// the bootstrap builds: a 5000-unit slab in the middle of a 12800-unit map, which
// left both starting bases standing over open space. Sizing it from MapDescription
// here means the floor always matches the match, whatever map is loaded.
void URA4SimWorldSubsystem::FitGroundPlaneToMap()
{
    UWorld* World = GetWorld();
    if (World == nullptr || SimWorld == nullptr)
    {
        return;
    }

    // A real sculpted Landscape (see RA4LandscapeCommandlet) takes over as the map's
    // ground once one has been baked into the level; the flat cube plane is only a
    // stand-in for maps that have neither, and must not be added on top of a
    // landscape that already covers the same area.
    TActorIterator<ALandscapeProxy> LandscapeIt(World);
    if (LandscapeIt)
    {
        return;
    }

    const RA4::MapDescription& Map = SimWorld->GetMap();
    const double SpanX = double(Map.Width) * double(RA4::kTileSizeUnits);
    const double SpanY = double(Map.Height) * double(RA4::kTileSizeUnits);
    if (SpanX <= 0.0 || SpanY <= 0.0)
    {
        return;
    }

    // The engine's basic cube is 100 units across and centred on its origin, so a
    // scale of Span/100 covers the map exactly and a thin Z keeps it a floor rather
    // than a block units would have to climb.


    constexpr double CubeSize = 100.0;
    constexpr double Thickness = 0.5;
    const FVector Scale(SpanX / CubeSize, SpanY / CubeSize, Thickness);
    const FVector Location(SpanX * 0.5, SpanY * 0.5, RA4Coords::GroundZ - CubeSize * Thickness * 0.5);

    // Matched by mesh, not by name: the level's floor is labelled RA4_Ground in the
    // editor but its object name at runtime is StaticMeshActor_1, so a name match
    // silently missed it and spawned a second floor on top of the first.
    AStaticMeshActor* ExistingGround = nullptr;
    for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
    {
        AStaticMeshActor* Actor = *It;
        const UStaticMeshComponent* MeshComp = Actor != nullptr ? Actor->GetStaticMeshComponent() : nullptr;
        const UStaticMesh* Asset = MeshComp != nullptr ? MeshComp->GetStaticMesh() : nullptr;
        if (Asset == nullptr)
        {
            continue;
        }
        if (ExistingGround == nullptr && Asset->GetPathName().Contains(TEXT("BasicShapes/Cube")))
        {
            ExistingGround = Actor;
        }
    }

    if (ExistingGround != nullptr)
    {
        ExistingGround->SetMobility(EComponentMobility::Movable);
        ExistingGround->SetActorLocation(Location);
        ExistingGround->SetActorScale3D(Scale);
        UE_LOG(LogTemp, Display, TEXT("RA4 ground plane resized to %.0f x %.0f units"), SpanX, SpanY);
        return;
    }

    // No floor in the map at all: spawn one, so a bare map is still playable.
    FActorSpawnParameters Params;
    Params.Name = TEXT("RA4_GroundGenerated");
    AStaticMeshActor* Ground = World->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, Params);
    if (Ground == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RA4 ground plane: spawn failed, the map will render as empty space"));
        return;
    }
    Ground->SetMobility(EComponentMobility::Movable);
    Ground->SetActorScale3D(Scale);
    if (UStaticMeshComponent* MeshComp = Ground->GetStaticMeshComponent())
    {
        if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
        {
            MeshComp->SetStaticMesh(Cube);
        }
        
        // Phase 0: Dark industrial ground
        if (UMaterialInterface* ShapeMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
        {
            MeshComp->SetMaterial(0, ShapeMat);
            if (UMaterialInstanceDynamic* DynMat = MeshComp->CreateAndSetMaterialInstanceDynamic(0))
            {
                DynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.08f, 0.08f, 0.09f, 1.0f));
            }
        }
    }
    UE_LOG(LogTemp, Display, TEXT("RA4 ground plane spawned at %.0f x %.0f units"), SpanX, SpanY);
}

void URA4SimWorldSubsystem::Deinitialize()
{
    // Commanders hold no ownership over the world, but they must stop before it goes.
    for (RA4::AI::AICommander* Commander : AICommanders)
    {
        delete Commander;
    }
    AICommanders.clear();

    // Simulation first: it references Content.
    if (SimWorld)
    {
        delete SimWorld;
        SimWorld = nullptr;
    }
    if (Mission)
    {
        delete Mission;
        Mission = nullptr;
    }
    if (Campaign)
    {
        delete Campaign;
        Campaign = nullptr;
    }
    if (Content)
    {
        delete Content;
        Content = nullptr;
    }
    if (LatestSnapshot)
    {
        delete LatestSnapshot;
        LatestSnapshot = nullptr;
    }
    if (SnapshotBuilder)
    {
        delete SnapshotBuilder;
        SnapshotBuilder = nullptr;
    }
    PendingCommands.clear();

    Super::Deinitialize();
}

void URA4SimWorldSubsystem::Tick(float DeltaTime)
{
    if (!SimWorld) return;

    if (!bGroundPlaneFitted)
    {
        bGroundPlaneFitted = true;
        FitGroundPlaneToMap();
    }

    TimeSinceLastSimTick += DeltaTime;

    // Fixed-step loop
    while (TimeSinceLastSimTick >= SimTickDelta)
    {
        // In a lockstep match this peer may not run a tick whose frame has not
        // arrived. Stalling here is the correct behaviour: running ahead locally would
        // execute a different command stream than everyone else and end the match.
        // The accumulator is not drained while stalled, so the peer catches up once
        // the frame lands.
        if (const URA4NetworkManager* Network = GetActiveNetwork())
        {
            if (!Network->CanAdvanceToTick(SimWorld->GetTick()))
            {
                break;
            }
        }

        TickSimulation();
        TimeSinceLastSimTick -= SimTickDelta;
    }

    // Sync presentation once per render frame
    SyncPresentation();
}

TStatId URA4SimWorldSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(URA4SimWorldSubsystem, STATGROUP_Tickables);
}

void URA4SimWorldSubsystem::EnqueueCommand(const RA4::Command& Command)
{
    // In a lockstep match a command is not queued locally -- it is scheduled with the
    // server and comes back in the authoritative frame alongside every other player's,
    // input-delay ticks later. Applying it here as well would execute it twice on this
    // peer and once everywhere else, which is a desync.
    if (URA4NetworkManager* Network = GetActiveNetwork())
    {
        Network->SendCommandToServer(Command, SimWorld ? SimWorld->GetTick() : 0);
        return;
    }

    PendingCommands.push_back(Command);
}

void URA4SimWorldSubsystem::SetSelectedEntitiesForHUD(const std::vector<RA4::EntityId>& Selection)
{
    LocalSelection = Selection;
}

void URA4SimWorldSubsystem::TickSimulation()
{
    const uint32 CurrentTick = SimWorld->GetTick();
    URA4NetworkManager* Network = GetActiveNetwork();

    RA4::CommandFrame Frame;

    if (Network != nullptr)
    {
        // Send this peer's slice first. Commands issued since the previous tick were
        // scheduled onto CurrentTick + InputDelay, so that is the frame to post, and
        // posting it before the tick rather than after keeps one tick of latency out
        // of the round trip.
        Network->FlushLocalFrame(CurrentTick + Network->GetInputDelay());

        // The frame that actually runs is whatever the server broadcast. Tick() only
        // calls in here once CanAdvanceToTick says it has landed, so a miss means the
        // frame was consumed already; running an empty one in its place would drop
        // every order in it.
        if (!Network->ConsumeFrameForTick(CurrentTick, Frame))
        {
            return;
        }

        // The AI plans on the authority only, and its orders travel in the same frame
        // as the players'. Ticking it on every peer instead would make the match
        // depend on the AI being deterministic as well as the simulation -- a strictly
        // stronger assumption, and one nothing currently tests.
        if (Network->IsServer())
        {
            std::vector<RA4::Command> AICommands;
            for (RA4::AI::AICommander* Commander : AICommanders)
            {
                Commander->Tick(*SimWorld, AICommands);
            }
            for (const RA4::Command& AICommand : AICommands)
            {
                Network->SendCommandToServer(AICommand, CurrentTick);
            }
        }
    }
    else
    {
        // Single player: everything queued since the previous tick executes in one
        // frame, in the order it was issued.
        Frame.Tick = CurrentTick;
        Frame.Commands.swap(PendingCommands);
        PendingCommands.clear();

        // The AI issues commands through exactly the same frame the player does, so it
        // is bound by the same validation and stays replay-compatible.
        for (RA4::AI::AICommander* Commander : AICommanders)
        {
            Commander->Tick(*SimWorld, Frame.Commands);
        }
    }

    SimWorld->Tick(&Frame);
    EvaluateMission();
    ProcessPresentationEvents();

    if (SnapshotBuilder && LatestSnapshot && SimWorld)
    {
        SnapshotBuilder->Build(*SimWorld, LocalSelection, *LatestSnapshot);

        // Hand the snapshot to the UI once per simulation tick rather than per
        // frame. This is the only path game data takes into the HUD: widgets bind
        // to view models, so nothing in the interface polls the world.
        if (UWorld* OwningWorld = GetWorld())
        {
            if (URA4UIDataProviderSubsystem* UIData =
                    OwningWorld->GetSubsystem<URA4UIDataProviderSubsystem>())
            {
                UIData->ApplySnapshot(*LatestSnapshot);
            }
        }
    }

    SimWorld->ClearEvents();

    if (Network != nullptr)
    {
        // The checksum is what turns a desync from "the match slowly stopped making
        // sense" into a report naming the tick and the player. It is taken after the
        // tick has fully settled, so it covers the state the next frame will act on.
        Network->SubmitStateChecksum(CurrentTick, SimWorld->ComputeStateChecksum());

        // Retire bookkeeping well behind the playhead rather than at it. A client's
        // checksum for a tick can arrive after the server has already ticked past it,
        // and pruning the reference out from under it would let the late report become
        // the new reference -- silently retiring the desync it was meant to catch.
        constexpr uint32 PruneLagTicks = 200;
        if (CurrentTick > PruneLagTicks)
        {
            Network->PruneUpToTick(CurrentTick - PruneLagTicks);
        }
    }
}

namespace
{
const TCHAR* ObjectiveStateName(RA4::ObjectiveState State)
{
    switch (State)
    {
    case RA4::ObjectiveState::Hidden:    return TEXT("hidden");
    case RA4::ObjectiveState::Active:    return TEXT("active");
    case RA4::ObjectiveState::Completed: return TEXT("completed");
    case RA4::ObjectiveState::Failed:    return TEXT("failed");
    }
    return TEXT("unknown");
}
} // namespace

void URA4SimWorldSubsystem::EvaluateMission()
{
    if (Mission == nullptr || SimWorld == nullptr)
    {
        return;
    }

    const RA4::MissionStatus Status = Mission->Evaluate(*SimWorld);

    // Transitions are drained rather than read: each one is an announcement that
    // happens once, and leaving them in the list would replay every completed
    // objective on every tick for the rest of the mission.
    for (const RA4::ObjectiveTransition& Transition : Mission->GetTransitions())
    {
        UE_LOG(LogTemp, Display, TEXT("RA4 objective '%s' -> %s at tick %u"),
               UTF8_TO_TCHAR(Transition.ObjectiveId.c_str()), ObjectiveStateName(Transition.NewState),
               Transition.Tick);
    }
    Mission->ClearTransitions();

    if (bMissionResultReported || !Mission->IsFinished())
    {
        return;
    }
    bMissionResultReported = true;

    if (Status == RA4::MissionStatus::Won)
    {
        UE_LOG(LogTemp, Display, TEXT("RA4 campaign mission '%s' won at tick %u."),
               *ActiveMissionId, SimWorld->GetTick());
        return;
    }

    // Which clause ended it, not just that it ended: "your construction yard was
    // destroyed" and "the convoy was lost" are different missions to replay.
    UE_LOG(LogTemp, Display, TEXT("RA4 campaign mission '%s' lost at tick %u (failure condition %d)."),
           *ActiveMissionId, SimWorld->GetTick(), Mission->GetTriggeredFailure());
}

URA4NetworkManager* URA4SimWorldSubsystem::GetActiveNetwork() const
{
    if (UWorld* OwningWorld = GetWorld())
    {
        if (URA4NetworkManager* Network = OwningWorld->GetSubsystem<URA4NetworkManager>())
        {
            // A manager exists in every world; it only means anything once BeginMatch
            // has run. Before that this is a single-player match.
            if (Network->IsMatchActive())
            {
                return Network;
            }
        }
    }
    return nullptr;
}

void URA4SimWorldSubsystem::ProcessPresentationEvents()
{
    UWorld* UnrealWorld = GetWorld();
    if (UnrealWorld == nullptr || SimWorld == nullptr || Content == nullptr)
    {
        return;
    }

    // Audio is optional: it is absent in PIE sessions that start without the subsystem
    // and in any headless-ish configuration. Bailing out here would also take every
    // muzzle line and impact marker below with it, which is why combat used to render
    // as nothing at all rather than as a silent fight. Each playback site checks
    // instead, so sound can be missing without the fight becoming invisible.
    URA4AudioSubsystem* Audio = UnrealWorld->GetSubsystem<URA4AudioSubsystem>();

    constexpr RA4::PlayerId LocalPlayer = 0;
    const RA4::PlayerState& Player = SimWorld->GetPlayer(LocalPlayer);
    const uint8 Faction = static_cast<uint8>(Player.Faction);

    const auto PlayEVA =
        [Audio, Faction](ERA4EVAEvent EVAEvent, bool bBypassCooldown = false)
    {
        if (Audio != nullptr)
        {
            Audio->PlayEVA(Faction, EVAEvent, bBypassCooldown);
        }
    };

    const auto PlayVoiceForContent =
        [this, Audio](RA4::ContentId ContentId, ERA4VoiceEvent VoiceEvent, bool bBypassCooldown)
    {
        if (Audio == nullptr)
        {
            return;
        }
        const RA4::VoiceSetDef* VoiceSet = Content->FindVoiceSet(ContentId);
        if (VoiceSet != nullptr && !VoiceSet->VoiceId.empty())
        {
            Audio->PlayUnitVoice(FString(VoiceSet->VoiceId.c_str()), VoiceEvent, bBypassCooldown);
        }
    };

    for (const RA4::SimEvent& Event : SimWorld->GetEvents())
    {
        switch (Event.Type)
        {
        case RA4::SimEventType::DamageApplied:
            // A lethal hit also emits EntityDestroyed. Skip its damage bark so the
            // death line is not played on top of it.
            if (SimWorld->IsAlive(Event.Entity))
            {
                const RA4::EntityCore* Core = SimWorld->GetCore(Event.Entity);
                if (Core != nullptr && Core->Owner == LocalPlayer)
                {
                    PlayVoiceForContent(Core->Def, ERA4VoiceEvent::Damaged, false);
                    if (Core->Kind == RA4::EntityKind::Building)
                    {
                        PlayEVA(ERA4EVAEvent::BaseUnderAttack);
                    }
                }
            }
            break;

        case RA4::SimEventType::EntityDestroyed:
            if (Event.Player == LocalPlayer)
            {
                const RA4::EntityDef* Def = Content->FindEntity(Event.Content);
                if (Def != nullptr && Def->Kind == RA4::EntityKind::Building)
                {
                    PlayEVA(ERA4EVAEvent::BuildingLost);
                }
                else
                {
                    PlayVoiceForContent(Event.Content, ERA4VoiceEvent::Death, true);
                }
            }
            if (UnrealWorld)
            {
                FVector ImpactPoint = RA4Coords::ToUnreal(Event.Location);
                ImpactPoint.Z = SampleGroundHeight(ImpactPoint.X, ImpactPoint.Y) + 40.0f;
                DrawDebugPoint(UnrealWorld, ImpactPoint, 35.0f, FColor::Red, false, 0.4f);
            }
            break;

        case RA4::SimEventType::EntityVeterancyPromoted:
            if (Event.Player == LocalPlayer)
            {
                PlayVoiceForContent(Event.Content, ERA4VoiceEvent::Elite, true);
            }
            break;

        case RA4::SimEventType::BuildingCompleted:
            if (Event.Player == LocalPlayer)
            {
                PlayEVA(ERA4EVAEvent::ConstructionComplete);
            }
            break;

        case RA4::SimEventType::ProductionCompleted:
            if (Event.Player == LocalPlayer)
            {
                PlayEVA(ERA4EVAEvent::UnitReady);
            }
            break;

        case RA4::SimEventType::CommandRejected:
            if (Event.Player == LocalPlayer &&
                Event.Value == int32(RA4::CommandReject::InsufficientCredits))
            {
                PlayEVA(ERA4EVAEvent::InsufficientFunds);
            }
            break;

        // ADR-0012: an unaffordable order is now accepted and funded gradually, so
        // "you are broke" surfaces here rather than as a command rejection. The
        // simulation emits this only on the transition into Starved, so it is already
        // edge-triggered and does not need extra throttling.
        case RA4::SimEventType::ProductionStarved:
            if (Event.Player == LocalPlayer)
            {
                PlayEVA(ERA4EVAEvent::InsufficientFunds);
            }
            break;

        case RA4::SimEventType::MatchEnded:
            PlayEVA(
                Event.Player == LocalPlayer ? ERA4EVAEvent::Victory : ERA4EVAEvent::Defeat,
                true);
            break;

        case RA4::SimEventType::WeaponFired:
            if (UnrealWorld)
            {
                FVector Start = RA4Coords::ToUnreal(Event.Location);
                Start.Z = SampleGroundHeight(Start.X, Start.Y) + 20.0f;
                FVector End = Start;
                const auto& Transforms = SimWorld->GetAllTransforms();
                if (Event.Other.IsValid() && SimWorld->IsAlive(Event.Other) && Event.Other.Index < Transforms.size())
                {
                    End = RA4Coords::ToUnreal(Transforms[Event.Other.Index].Position);
                    End.Z = SampleGroundHeight(End.X, End.Y) + 20.0f;
                    DrawDebugLine(UnrealWorld, Start, End, FColor::Yellow, false, 0.15f, 0, 2.5f);
                }
                else
                {
                    DrawDebugPoint(UnrealWorld, Start, 15.0f, FColor::Yellow, false, 0.15f);
                }
            }
            break;

        case RA4::SimEventType::ProjectileImpact:
            if (UnrealWorld)
            {
                FVector ImpactPoint = RA4Coords::ToUnreal(Event.Location);
                ImpactPoint.Z = SampleGroundHeight(ImpactPoint.X, ImpactPoint.Y) + 30.0f;
                DrawDebugPoint(UnrealWorld, ImpactPoint, 25.0f, FColor::Orange, false, 0.25f);
            }
            break;

        default:
            break;
        }
    }

    const bool bPowerShortage =
        Player.PowerConsumed > 0 && Player.PowerProduced < Player.PowerConsumed;
    if (bPowerShortage && !bWasLocalPowerShortage)
    {
        PlayEVA(ERA4EVAEvent::PowerLow);
    }
    bWasLocalPowerShortage = bPowerShortage;
}

float URA4SimWorldSubsystem::SampleGroundHeight(double WorldX, double WorldY)
{
    if (!bLandscapeSearched)
    {
        bLandscapeSearched = true;
        if (UWorld* World = GetWorld())
        {
            TActorIterator<ALandscapeProxy> It(World);
            if (It)
            {
                CachedLandscape = *It;
            }
        }
    }

    ALandscapeProxy* Landscape = CachedLandscape.Get();
    if (Landscape == nullptr)
    {
        return float(RA4Coords::GroundZ);
    }

    const TOptional<float> Height = Landscape->GetHeightAtLocation(FVector(WorldX, WorldY, 0.0));
    return Height.IsSet() ? Height.GetValue() : float(RA4Coords::GroundZ);
}

void URA4SimWorldSubsystem::SyncPresentation()
{
    if (!SimWorld) return;

    // Iterate all cores and update their bound actors
    const auto& Cores = SimWorld->GetAllCores();
    const auto& Transforms = SimWorld->GetAllTransforms();
    UWorld* World = GetWorld();
    int32 AliveEntities = 0;

    for (uint32 Index = 0; Index < Cores.size(); ++Index)
    {
        if (Cores[Index].bAlive)
        {
            ++AliveEntities;
            ARA4EntityActor* Actor = nullptr;
            bool bNewActor = false;
            if (ARA4EntityActor** ActorPtr = EntityActors.Find(Index))
            {
                // The slot may have been recycled since this actor was bound. SimWorld
                // frees a slot in the same breath as bumping its generation, so an
                // entity that died and one that took its place between two syncs are
                // indistinguishable by bAlive -- both read as alive at this index. Left
                // unchecked, the dead unit's actor is silently handed to the new entity
                // and keeps its mesh, scale and team colour. Retire it instead and let
                // the spawn path below build the right one.
                //
                // IsValid comes first because this map holds raw pointers outside any
                // UPROPERTY, so an entry can outlive the actor it names.
                if (!IsValid(*ActorPtr))
                {
                    EntityActors.Remove(Index);
                }
                else if ((*ActorPtr)->GetEntityGeneration() == Cores[Index].Generation)
                {
                    Actor = *ActorPtr;
                }
                else
                {
                    (*ActorPtr)->Destroy();
                    EntityActors.Remove(Index);
                }
            }

            if (Actor == nullptr && World)
            {
                // Auto-spawn presentation actor if registered class is set
                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                
                UClass* ClassToSpawn = EntityActorClass.Get();
                if (ClassToSpawn == nullptr)
                {
                    ClassToSpawn = ARA4EntityActor::StaticClass();
                }
                if (ClassToSpawn->HasAnyClassFlags(CLASS_Abstract))
                {
                    if (!bReportedPresentationState)
                    {
                        UE_LOG(LogTemp, Error, TEXT("RA4 presentation class %s is abstract"),
                               *GetNameSafe(ClassToSpawn));
                    }
                    continue;
                }
                Actor = World->SpawnActor<ARA4EntityActor>(ClassToSpawn, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
                
                if (Actor)
                {
                    bNewActor = true;
                    Actor->BindToEntity(Index, Cores[Index].Generation);
                    RegisterEntityActor(Index, Actor);

                    // Assign 3D mesh if ContentId exists in registry. When it does
                    // not, the actor keeps its placeholder primitive, and we log the miss
                    // once per content id so a bad asset path is not invisible in PIE.
                    uint32 ContentIdValue = Cores[Index].Def.Value;
                    if (UStaticMesh** MeshPtr = ContentMeshRegistry.Find(ContentIdValue))
                    {
                        Actor->SetEntityMesh(*MeshPtr);
                    }
                    else if (Cores[Index].Kind == RA4::EntityKind::ResourceNode)
                    {
                        // Ore fields have no bespoke mesh in the blockout set (it only
                        // covers buildings and units) -- a cone reads as a heap of ore
                        // at a glance, which a flat cube never will.
                        if (UStaticMesh* OreMesh =
                                LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")))
                        {
                            Actor->SetEntityMesh(OreMesh);
                        }
                    }
                    else if (!MissingMeshContentIds.Contains(ContentIdValue))
                    {
                        FString MissingName = TEXT("<no-def>");
                        if (Content != nullptr)
                        {
                            if (const RA4::EntityDef* Def = Content->FindEntity(Cores[Index].Def))
                            {
                                MissingName = FString(Def->Name.c_str());
                            }
                        }
                        UE_LOG(LogTemp, Warning,
                               TEXT("RA4 presentation has no mesh mapping for content id %u (%s)"),
                               ContentIdValue, *MissingName);
                        MissingMeshContentIds.Add(ContentIdValue);
                    }

                    // Size and colour the placeholder from real definition data, so a
                    // 3x3 war factory reads as a building and a rifleman does not.
                    FVector Scale(1.0, 1.0, 1.0);
                    if (Content != nullptr)
                    {
                        if (const RA4::EntityDef* Def = Content->FindEntity(Cores[Index].Def))
                        {
                            if (Def->Kind == RA4::EntityKind::Building)
                            {
                                Scale = FVector(double(Def->Building.FootprintX) * 2.0,
                                                double(Def->Building.FootprintY) * 2.0, 2.0);
                            }
                            else
                            {
                                const double Diameter = Def->Unit.CollisionRadius.ToDoubleUnsafe() * 2.0;
                                const double Footprint = FMath::Max(Diameter / 100.0, 0.5);
                                Scale = FVector(Footprint, Footprint, Footprint * 0.8);
                            }
                        }
                        else
                        {
                            Scale = FVector(1.5, 1.5, 0.3);   // resource node
                        }
                    }
                    Actor->SetVisualScale(Scale);
                    FString EntityIdString = TEXT("unknown");
                    if (Content != nullptr)
                    {
                        if (const RA4::EntityDef* Def = Content->FindEntity(Cores[Index].Def))
                        {
                            EntityIdString = FString(Def->Name.c_str());
                        }
                    }
                    if (EntityIdString == TEXT("unknown"))
                    {
                        if (Cores[Index].Kind == RA4::EntityKind::ResourceNode)
                        {
                            EntityIdString = TEXT("ore_resource_node");
                        }
                        else if (Cores[Index].Kind == RA4::EntityKind::Building)
                        {
                            EntityIdString = TEXT("building_structure");
                        }
                        else if (Cores[Index].Kind == RA4::EntityKind::Unit)
                        {
                            EntityIdString = TEXT("unit_rifleman_infantry");
                        }
                    }
                    Actor->ApplyPrimitiveComposition(EntityIdString);

                    // Player colours until faction themes drive this.
                    static const FLinearColor PlayerColours[3] = {
                        FLinearColor(0.85f, 0.12f, 0.10f),   // player 0
                        FLinearColor(0.15f, 0.40f, 0.90f),   // player 1
                        FLinearColor(0.75f, 0.70f, 0.20f)};  // neutral / resources
                    const uint8 Owner = Cores[Index].Owner;
                    Actor->SetTeamColor(PlayerColours[Owner < 2 ? Owner : 2]);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("RA4 failed to spawn presentation actor %s in world %s"),
                           *GetNameSafe(ClassToSpawn), *GetNameSafe(World));
                }
            }

            if (Actor)
            {
                const RA4::TransformComp& SimTransform = Transforms[Index];
                
                FVector UnrealPos = RA4Coords::ToUnreal(SimTransform.Position);
                // The simulation itself is flat -- this only lifts the visual actor to
                // sit on whatever terrain relief the level happens to have.
                UnrealPos.Z = SampleGroundHeight(UnrealPos.X, UnrealPos.Y);

                // The simulation stores angles in 4096 units per full turn. Using
                // 255 here rotated every unit by roughly a factor of sixteen.
                const float UnrealRotZ = static_cast<float>(RA4Coords::FacingToYawDegrees(SimTransform.Facing));
                Actor->UpdateFromSimulation(UnrealPos, UnrealRotZ, bNewActor);

                // One-off dump of the first few actors. "The match runs but the
                // screen is empty" is only answerable with the actual mesh, scale
                // and position the renderer sees.
                if (bNewActor && DiagnosticDumpsRemaining > 0)
                {
                    --DiagnosticDumpsRemaining;
                    UE_LOG(LogTemp, Warning, TEXT("RA4 visual[%u] owner=%d kind=%d %s"), Index,
                           int32(Cores[Index].Owner), int32(Cores[Index].Kind),
                           *Actor->DescribeVisualState());
                }
            }
        }
        else
        {
            // Clean up destroyed entity actors
            if (ARA4EntityActor** ActorPtr = EntityActors.Find(Index))
            {
                if (ARA4EntityActor* Actor = *ActorPtr)
                {
                    Actor->Destroy();
                }
                EntityActors.Remove(Index);
            }
        }
    }

    if (!bReportedPresentationState)
    {
        bReportedPresentationState = true;
        UE_LOG(LogTemp, Display,
               TEXT("RA4 presentation synchronized %d actors from %d alive / %llu total entities using %s"),
               EntityActors.Num(), AliveEntities, static_cast<uint64>(Cores.size()),
               *GetNameSafe(EntityActorClass.Get()));
    }
}

void URA4SimWorldSubsystem::RegisterEntityActor(int32 EntityIndex, ARA4EntityActor* Actor)
{
    if (Actor)
    {
        EntityActors.Add(static_cast<uint32>(EntityIndex), Actor);
    }
}

void URA4SimWorldSubsystem::UnregisterEntityActor(int32 EntityIndex)
{
    EntityActors.Remove(static_cast<uint32>(EntityIndex));
}

ARA4EntityActor* URA4SimWorldSubsystem::GetEntityActor(int32 EntityIndex) const
{
    if (EntityIndex < 0) return nullptr;
    if (ARA4EntityActor* const* ActorPtr = const_cast<TMap<uint32, ARA4EntityActor*>&>(EntityActors).Find(static_cast<uint32>(EntityIndex)))
    {
        return *ActorPtr;
    }
    return nullptr;
}

ARA4EntityActor* URA4SimWorldSubsystem::GetEntityActor(RA4::EntityId Id) const
{
    if (!Id.IsValid()) return nullptr;
    return GetEntityActor(static_cast<int32>(Id.Index));
}

void URA4SimWorldSubsystem::LoadBlockoutMesh(uint32 ContentIdValue, const TCHAR* AssetPath)
{
    if (UStaticMesh* Mesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, AssetPath)))
    {
        ContentMeshRegistry.Add(ContentIdValue, Mesh);
    }
}

void URA4SimWorldSubsystem::RegisterDefaultBlockoutMeshes()
{
// Automatically generated mapping for all 142 blockout assets across 4 factions
    LoadBlockoutMesh(RA4::MakeContentId("building.sov.construction_yard").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ConYard_Blockout.SM_Soviet_SU_ConYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.sov.tesla_reactor").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_PowerPlant_Blockout.SM_Soviet_SU_PowerPlant_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.sov.ore_refinery").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Refinery_Blockout.SM_Soviet_SU_Refinery_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.sov.barracks").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Barracks_Blockout.SM_Soviet_SU_Barracks_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.sov.war_factory").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_WarFactory_Blockout.SM_Soviet_SU_WarFactory_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.sov.gun_turret").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GunTurret_Blockout.SM_Soviet_SU_GunTurret_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.sov.mcv").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_MCV_MobileYard_Blockout.SM_Soviet_SU_MCV_MobileYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.sov.ore_harvester").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_BogatyrOreCarrier_Blockout.SM_Soviet_SU_BogatyrOreCarrier_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.sov.conscript").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_RubezhRifleman_Blockout.SM_Soviet_SU_RubezhRifleman_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.sov.rocket_trooper").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ZaslonAATeam_Blockout.SM_Soviet_SU_ZaslonAATeam_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.sov.heavy_tank").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GranitMBT_Blockout.SM_Soviet_SU_GranitMBT_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.all.construction_yard").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_ConYard_Blockout.SM_Alliance_AL_ConYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.all.power_plant").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_PowerPlant_Blockout.SM_Alliance_AL_PowerPlant_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.all.ore_refinery").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Refinery_Blockout.SM_Alliance_AL_Refinery_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.all.barracks").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Barracks_Blockout.SM_Alliance_AL_Barracks_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.all.war_factory").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_WarFactory_Blockout.SM_Alliance_AL_WarFactory_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.all.pillbox").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_GunTurret_Blockout.SM_Alliance_AL_GunTurret_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.all.mcv").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_MCV_MobileNode_Blockout.SM_Alliance_AL_MCV_MobileNode_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.all.ore_harvester").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_PioneerHarvester_Blockout.SM_Alliance_AL_PioneerHarvester_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.all.rifleman").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_SentinelRifleman_Blockout.SM_Alliance_AL_SentinelRifleman_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.all.missile_infantry").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_LancerTeam_Blockout.SM_Alliance_AL_LancerTeam_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.all.light_tank").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_CitadelTank_Blockout.SM_Alliance_AL_CitadelTank_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_MCV_MobileYard").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_MCV_MobileYard_Blockout.SM_Soviet_SU_MCV_MobileYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_ConYard").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ConYard_Blockout.SM_Soviet_SU_ConYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_PowerPlant").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_PowerPlant_Blockout.SM_Soviet_SU_PowerPlant_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_Refinery").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Refinery_Blockout.SM_Soviet_SU_Refinery_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_Barracks").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Barracks_Blockout.SM_Soviet_SU_Barracks_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_WarFactory").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_WarFactory_Blockout.SM_Soviet_SU_WarFactory_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_Airfield").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Airfield_Blockout.SM_Soviet_SU_Airfield_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_NavalYard").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_NavalYard_Blockout.SM_Soviet_SU_NavalYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_Radar").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Radar_Blockout.SM_Soviet_SU_Radar_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_TechCenter").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_TechCenter_Blockout.SM_Soviet_SU_TechCenter_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_GunTurret").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GunTurret_Blockout.SM_Soviet_SU_GunTurret_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_AATurret").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_AATurret_Blockout.SM_Soviet_SU_AATurret_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_TeslaTower").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_TeslaTower_Blockout.SM_Soviet_SU_TeslaTower_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_Bunker").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Bunker_Blockout.SM_Soviet_SU_Bunker_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_SuperweaponDome").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_SuperweaponDome_Blockout.SM_Soviet_SU_SuperweaponDome_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_SuperweaponSilo").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_SuperweaponSilo_Blockout.SM_Soviet_SU_SuperweaponSilo_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_RubezhRifleman").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_RubezhRifleman_Blockout.SM_Soviet_SU_RubezhRifleman_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_ZapalGrenadier").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ZapalGrenadier_Blockout.SM_Soviet_SU_ZapalGrenadier_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_ZaslonAATeam").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ZaslonAATeam_Blockout.SM_Soviet_SU_ZaslonAATeam_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_MasterEngineer").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_MasterEngineer_Blockout.SM_Soviet_SU_MasterEngineer_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_RazryadTrooper").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_RazryadTrooper_Blockout.SM_Soviet_SU_RazryadTrooper_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_VektorOfficer").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_VektorOfficer_Blockout.SM_Soviet_SU_VektorOfficer_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_BogatyrOreCarrier").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_BogatyrOreCarrier_Blockout.SM_Soviet_SU_BogatyrOreCarrier_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_RysScout").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_RysScout_Blockout.SM_Soviet_SU_RysScout_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_GranitMBT").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GranitMBT_Blockout.SM_Soviet_SU_GranitMBT_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_ZarevoMLRS").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ZarevoMLRS_Blockout.SM_Soviet_SU_ZarevoMLRS_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_GromoboyRam").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GromoboyRam_Blockout.SM_Soviet_SU_GromoboyRam_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_VoevodaHeavyTank").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_VoevodaHeavyTank_Blockout.SM_Soviet_SU_VoevodaHeavyTank_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_KrechetInterceptor").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_KrechetInterceptor_Blockout.SM_Soviet_SU_KrechetInterceptor_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_KorshunGunship").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_KorshunGunship_Blockout.SM_Soviet_SU_KorshunGunship_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_GromadaAirship").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GromadaAirship_Blockout.SM_Soviet_SU_GromadaAirship_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_BuranPatrolBoat").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_BuranPatrolBoat_Blockout.SM_Soviet_SU_BuranPatrolBoat_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_MorokSubmarine").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_MorokSubmarine_Blockout.SM_Soviet_SU_MorokSubmarine_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_SvyatogorCruiser").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_SvyatogorCruiser_Blockout.SM_Soviet_SU_SvyatogorCruiser_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("SU_Hero_Morozova").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Hero_Morozova_Blockout.SM_Soviet_SU_Hero_Morozova_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_MCV_MobileNode").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_MCV_MobileNode_Blockout.SM_Alliance_AL_MCV_MobileNode_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_ConYard").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_ConYard_Blockout.SM_Alliance_AL_ConYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_PowerPlant").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_PowerPlant_Blockout.SM_Alliance_AL_PowerPlant_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_Refinery").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Refinery_Blockout.SM_Alliance_AL_Refinery_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_Barracks").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Barracks_Blockout.SM_Alliance_AL_Barracks_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_WarFactory").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_WarFactory_Blockout.SM_Alliance_AL_WarFactory_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_Airfield").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Airfield_Blockout.SM_Alliance_AL_Airfield_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_NavalYard").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_NavalYard_Blockout.SM_Alliance_AL_NavalYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_Radar").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Radar_Blockout.SM_Alliance_AL_Radar_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_TechCenter").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_TechCenter_Blockout.SM_Alliance_AL_TechCenter_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_GunTurret").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_GunTurret_Blockout.SM_Alliance_AL_GunTurret_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_AATurret").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_AATurret_Blockout.SM_Alliance_AL_AATurret_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_PrismTower").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_PrismTower_Blockout.SM_Alliance_AL_PrismTower_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_ShieldProjectorBuilding").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_ShieldProjectorBuilding_Blockout.SM_Alliance_AL_ShieldProjectorBuilding_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_SuperweaponChrono").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_SuperweaponChrono_Blockout.SM_Alliance_AL_SuperweaponChrono_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_SuperweaponOrbital").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_SuperweaponOrbital_Blockout.SM_Alliance_AL_SuperweaponOrbital_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_SentinelRifleman").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_SentinelRifleman_Blockout.SM_Alliance_AL_SentinelRifleman_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_LancerTeam").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_LancerTeam_Blockout.SM_Alliance_AL_LancerTeam_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_FieldEngineer").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_FieldEngineer_Blockout.SM_Alliance_AL_FieldEngineer_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_LongwatchSniper").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_LongwatchSniper_Blockout.SM_Alliance_AL_LongwatchSniper_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_LifelineMedic").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_LifelineMedic_Blockout.SM_Alliance_AL_LifelineMedic_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_FrostlineSpecialist").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_FrostlineSpecialist_Blockout.SM_Alliance_AL_FrostlineSpecialist_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_PioneerHarvester").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_PioneerHarvester_Blockout.SM_Alliance_AL_PioneerHarvester_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_KestrelScout").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_KestrelScout_Blockout.SM_Alliance_AL_KestrelScout_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_BulwarkMBT").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_BulwarkMBT_Blockout.SM_Alliance_AL_BulwarkMBT_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_OracleArtillery").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_OracleArtillery_Blockout.SM_Alliance_AL_OracleArtillery_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_RefractionTank").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_RefractionTank_Blockout.SM_Alliance_AL_RefractionTank_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_WardShieldCarrier").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_WardShieldCarrier_Blockout.SM_Alliance_AL_WardShieldCarrier_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_CitadelTank").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_CitadelTank_Blockout.SM_Alliance_AL_CitadelTank_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_ShrikeInterceptor").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_ShrikeInterceptor_Blockout.SM_Alliance_AL_ShrikeInterceptor_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_VectorVTOL").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_VectorVTOL_Blockout.SM_Alliance_AL_VectorVTOL_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_NightveilBomber").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_NightveilBomber_Blockout.SM_Alliance_AL_NightveilBomber_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_MantaPatrolCraft").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_MantaPatrolCraft_Blockout.SM_Alliance_AL_MantaPatrolCraft_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_ResoluteDestroyer").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_ResoluteDestroyer_Blockout.SM_Alliance_AL_ResoluteDestroyer_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_HorizonCarrier").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_HorizonCarrier_Blockout.SM_Alliance_AL_HorizonCarrier_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("AL_Hero_Hart").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Hero_Hart_Blockout.SM_Alliance_AL_Hero_Hart_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_MCV_MobileNode").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_MCV_MobileNode_Blockout.SM_Coalition_CO_MCV_MobileNode_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_ConYard").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_ConYard_Blockout.SM_Coalition_CO_ConYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_PowerPlant").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_PowerPlant_Blockout.SM_Coalition_CO_PowerPlant_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_Refinery").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_Refinery_Blockout.SM_Coalition_CO_Refinery_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_Barracks").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_Barracks_Blockout.SM_Coalition_CO_Barracks_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_WarFactory").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_WarFactory_Blockout.SM_Coalition_CO_WarFactory_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_Airfield").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_Airfield_Blockout.SM_Coalition_CO_Airfield_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_NavalYard").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_NavalYard_Blockout.SM_Coalition_CO_NavalYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_Radar").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_Radar_Blockout.SM_Coalition_CO_Radar_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_TechCenter").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_TechCenter_Blockout.SM_Coalition_CO_TechCenter_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_GunTurret").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_GunTurret_Blockout.SM_Coalition_CO_GunTurret_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_AATurret").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_AATurret_Blockout.SM_Coalition_CO_AATurret_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_RailTower").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_RailTower_Blockout.SM_Coalition_CO_RailTower_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_ShieldHub").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_ShieldHub_Blockout.SM_Coalition_CO_ShieldHub_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_SuperweaponMatrix").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_SuperweaponMatrix_Blockout.SM_Coalition_CO_SuperweaponMatrix_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_SuperweaponSeismic").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_SuperweaponSeismic_Blockout.SM_Coalition_CO_SuperweaponSeismic_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_QianweiRifleman").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_QianweiRifleman_Blockout.SM_Coalition_CO_QianweiRifleman_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_VajraLancer").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_VajraLancer_Blockout.SM_Coalition_CO_VajraLancer_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_JieTechnician").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_JieTechnician_Blockout.SM_Coalition_CO_JieTechnician_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_ShengongMarksman").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_ShengongMarksman_Blockout.SM_Coalition_CO_ShengongMarksman_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_SanjivaniMedic").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_SanjivaniMedic_Blockout.SM_Coalition_CO_SanjivaniMedic_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_RakshaGuard").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_RakshaGuard_Blockout.SM_Coalition_CO_RakshaGuard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_YuanCollector").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_YuanCollector_Blockout.SM_Coalition_CO_YuanCollector_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_KamakiriWalker").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_KamakiriWalker_Blockout.SM_Coalition_CO_KamakiriWalker_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_QinglongMBT").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_QinglongMBT_Blockout.SM_Coalition_CO_QinglongMBT_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_MonsoonArtillery").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_MonsoonArtillery_Blockout.SM_Coalition_CO_MonsoonArtillery_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_SeimonShieldCarrier").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_SeimonShieldCarrier_Blockout.SM_Coalition_CO_SeimonShieldCarrier_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_AiravataWalker").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_AiravataWalker_Blockout.SM_Coalition_CO_AiravataWalker_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_TianmenFortress").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_TianmenFortress_Blockout.SM_Coalition_CO_TianmenFortress_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_KawasemiDrone").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_KawasemiDrone_Blockout.SM_Coalition_CO_KawasemiDrone_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_LeiheGunship").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_LeiheGunship_Blockout.SM_Coalition_CO_LeiheGunship_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_AgnipakshaBomber").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_AgnipakshaBomber_Blockout.SM_Coalition_CO_AgnipakshaBomber_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_KazekiriCorvette").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_KazekiriCorvette_Blockout.SM_Coalition_CO_KazekiriCorvette_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_XuanwuCruiser").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_XuanwuCruiser_Blockout.SM_Coalition_CO_XuanwuCruiser_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_SamudraCarrier").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_SamudraCarrier_Blockout.SM_Coalition_CO_SamudraCarrier_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CO_Hero_Mei").Value, TEXT("/Game/RA4/Art/Blockout/Coalition/SM_Coalition_CO_Hero_Mei_Blockout.SM_Coalition_CO_Hero_Mei_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_MCV_MobileArk").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_MCV_MobileArk_Blockout.SM_Chronolegion_CH_MCV_MobileArk_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_ConYard").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_ConYard_Blockout.SM_Chronolegion_CH_ConYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_PowerPlant").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_PowerPlant_Blockout.SM_Chronolegion_CH_PowerPlant_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_Refinery").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_Refinery_Blockout.SM_Chronolegion_CH_Refinery_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_Barracks").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_Barracks_Blockout.SM_Chronolegion_CH_Barracks_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_WarFactory").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_WarFactory_Blockout.SM_Chronolegion_CH_WarFactory_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_Airfield").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_Airfield_Blockout.SM_Chronolegion_CH_Airfield_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_NavalYard").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_NavalYard_Blockout.SM_Chronolegion_CH_NavalYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_Radar").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_Radar_Blockout.SM_Chronolegion_CH_Radar_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_TechCenter").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_TechCenter_Blockout.SM_Chronolegion_CH_TechCenter_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_EchoTurret").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_EchoTurret_Blockout.SM_Chronolegion_CH_EchoTurret_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_AATurret").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_AATurret_Blockout.SM_Chronolegion_CH_AATurret_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_StasisProjectorBuilding").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_StasisProjectorBuilding_Blockout.SM_Chronolegion_CH_StasisProjectorBuilding_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_CausalityAnchor").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_CausalityAnchor_Blockout.SM_Chronolegion_CH_CausalityAnchor_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_SuperweaponRewind").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_SuperweaponRewind_Blockout.SM_Chronolegion_CH_SuperweaponRewind_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_SuperweaponSingularity").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_SuperweaponSingularity_Blockout.SM_Chronolegion_CH_SuperweaponSingularity_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_ResonanceRifleman").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_ResonanceRifleman_Blockout.SM_Chronolegion_CH_ResonanceRifleman_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_PunctureLancer").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_PunctureLancer_Blockout.SM_Chronolegion_CH_PunctureLancer_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_CausalityEngineer").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_CausalityEngineer_Blockout.SM_Chronolegion_CH_CausalityEngineer_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_ReversalMedic").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_ReversalMedic_Blockout.SM_Chronolegion_CH_ReversalMedic_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_AporiaSniper").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_AporiaSniper_Blockout.SM_Chronolegion_CH_AporiaSniper_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_CensorOperative").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_CensorOperative_Blockout.SM_Chronolegion_CH_CensorOperative_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_ProbabilistHarvester").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_ProbabilistHarvester_Blockout.SM_Chronolegion_CH_ProbabilistHarvester_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_ParallaxScout").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_ParallaxScout_Blockout.SM_Chronolegion_CH_ParallaxScout_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_TimelineTank").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_TimelineTank_Blockout.SM_Chronolegion_CH_TimelineTank_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_DeltaDelayArtillery").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_DeltaDelayArtillery_Blockout.SM_Chronolegion_CH_DeltaDelayArtillery_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_PauseProjector").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_PauseProjector_Blockout.SM_Chronolegion_CH_PauseProjector_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_EraEngine").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_EraEngine_Blockout.SM_Chronolegion_CH_EraEngine_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_GapInterceptor").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_GapInterceptor_Blockout.SM_Chronolegion_CH_GapInterceptor_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_TrailGunship").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_TrailGunship_Blockout.SM_Chronolegion_CH_TrailGunship_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_CriticalPointBomber").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_CriticalPointBomber_Blockout.SM_Chronolegion_CH_CriticalPointBomber_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_IsobathFrigate").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_IsobathFrigate_Blockout.SM_Chronolegion_CH_IsobathFrigate_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_BathysSubmarine").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_BathysSubmarine_Blockout.SM_Chronolegion_CH_BathysSubmarine_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_AttractorArk").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_AttractorArk_Blockout.SM_Chronolegion_CH_AttractorArk_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("CH_Hero_Voss").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_Hero_Voss_Blockout.SM_Chronolegion_CH_Hero_Voss_Blockout"));

    // The legacy blockout registry remains a safe fallback for the rest of the
    // roster. These verified vertical-slice assets deliberately overwrite the
    // same ContentIds after all fallback entries have been registered.
    struct FProductionMeshEntry
    {
        const char* ContentId;
        const TCHAR* AssetPath;
    };
    static const FProductionMeshEntry ProductionMeshes[] = {
        {"building.sov.construction_yard", TEXT("/Game/RA4/Art/Buildings/Soviet/SM_Soviet_SU_ConYard.SM_Soviet_SU_ConYard")},
        {"building.sov.tesla_reactor", TEXT("/Game/RA4/Art/Buildings/Soviet/SM_Soviet_SU_PowerPlant.SM_Soviet_SU_PowerPlant")},
        {"building.sov.ore_refinery", TEXT("/Game/RA4/Art/Buildings/Soviet/SM_Soviet_SU_Refinery.SM_Soviet_SU_Refinery")},
        {"building.sov.barracks", TEXT("/Game/RA4/Art/Buildings/Soviet/SM_Soviet_SU_Barracks.SM_Soviet_SU_Barracks")},
        {"building.sov.war_factory", TEXT("/Game/RA4/Art/Buildings/Soviet/SM_Soviet_SU_WarFactory.SM_Soviet_SU_WarFactory")},
        {"unit.sov.ore_harvester", TEXT("/Game/RA4/Art/Units/Soviet/SM_Soviet_SU_BogatyrOreCarrier.SM_Soviet_SU_BogatyrOreCarrier")},
        {"unit.sov.heavy_tank", TEXT("/Game/RA4/Art/Units/Soviet/SM_Soviet_SU_GranitMBT.SM_Soviet_SU_GranitMBT")},
        {"building.all.construction_yard", TEXT("/Game/RA4/Art/Buildings/Alliance/SM_Alliance_AL_ConYard.SM_Alliance_AL_ConYard")},
        {"building.all.power_plant", TEXT("/Game/RA4/Art/Buildings/Alliance/SM_Alliance_AL_PowerPlant.SM_Alliance_AL_PowerPlant")},
        {"building.all.ore_refinery", TEXT("/Game/RA4/Art/Buildings/Alliance/SM_Alliance_AL_Refinery.SM_Alliance_AL_Refinery")},
        {"building.all.barracks", TEXT("/Game/RA4/Art/Buildings/Alliance/SM_Alliance_AL_Barracks.SM_Alliance_AL_Barracks")},
        {"building.all.war_factory", TEXT("/Game/RA4/Art/Buildings/Alliance/SM_Alliance_AL_WarFactory.SM_Alliance_AL_WarFactory")},
        {"unit.all.ore_harvester", TEXT("/Game/RA4/Art/Units/Alliance/SM_Alliance_AL_PioneerHarvester.SM_Alliance_AL_PioneerHarvester")},
        {"unit.all.light_tank", TEXT("/Game/RA4/Art/Units/Alliance/SM_Alliance_AL_BulwarkMBT.SM_Alliance_AL_BulwarkMBT")},
        {"SU_ConYard", TEXT("/Game/RA4/Art/Buildings/Soviet/SM_Soviet_SU_ConYard.SM_Soviet_SU_ConYard")},
        {"SU_PowerPlant", TEXT("/Game/RA4/Art/Buildings/Soviet/SM_Soviet_SU_PowerPlant.SM_Soviet_SU_PowerPlant")},
        {"SU_Refinery", TEXT("/Game/RA4/Art/Buildings/Soviet/SM_Soviet_SU_Refinery.SM_Soviet_SU_Refinery")},
        {"SU_Barracks", TEXT("/Game/RA4/Art/Buildings/Soviet/SM_Soviet_SU_Barracks.SM_Soviet_SU_Barracks")},
        {"SU_WarFactory", TEXT("/Game/RA4/Art/Buildings/Soviet/SM_Soviet_SU_WarFactory.SM_Soviet_SU_WarFactory")},
        {"SU_BogatyrOreCarrier", TEXT("/Game/RA4/Art/Units/Soviet/SM_Soviet_SU_BogatyrOreCarrier.SM_Soviet_SU_BogatyrOreCarrier")},
        {"SU_RysScout", TEXT("/Game/RA4/Art/Units/Soviet/SM_Soviet_SU_RysScout.SM_Soviet_SU_RysScout")},
        {"SU_GranitMBT", TEXT("/Game/RA4/Art/Units/Soviet/SM_Soviet_SU_GranitMBT.SM_Soviet_SU_GranitMBT")},
        {"SU_ZarevoMLRS", TEXT("/Game/RA4/Art/Units/Soviet/SM_Soviet_SU_ZarevoMLRS.SM_Soviet_SU_ZarevoMLRS")},
        {"AL_ConYard", TEXT("/Game/RA4/Art/Buildings/Alliance/SM_Alliance_AL_ConYard.SM_Alliance_AL_ConYard")},
        {"AL_PowerPlant", TEXT("/Game/RA4/Art/Buildings/Alliance/SM_Alliance_AL_PowerPlant.SM_Alliance_AL_PowerPlant")},
        {"AL_Refinery", TEXT("/Game/RA4/Art/Buildings/Alliance/SM_Alliance_AL_Refinery.SM_Alliance_AL_Refinery")},
        {"AL_Barracks", TEXT("/Game/RA4/Art/Buildings/Alliance/SM_Alliance_AL_Barracks.SM_Alliance_AL_Barracks")},
        {"AL_WarFactory", TEXT("/Game/RA4/Art/Buildings/Alliance/SM_Alliance_AL_WarFactory.SM_Alliance_AL_WarFactory")},
        {"AL_PioneerHarvester", TEXT("/Game/RA4/Art/Units/Alliance/SM_Alliance_AL_PioneerHarvester.SM_Alliance_AL_PioneerHarvester")},
        {"AL_KestrelScout", TEXT("/Game/RA4/Art/Units/Alliance/SM_Alliance_AL_KestrelScout.SM_Alliance_AL_KestrelScout")},
        {"AL_BulwarkMBT", TEXT("/Game/RA4/Art/Units/Alliance/SM_Alliance_AL_BulwarkMBT.SM_Alliance_AL_BulwarkMBT")},
        {"AL_OracleArtillery", TEXT("/Game/RA4/Art/Units/Alliance/SM_Alliance_AL_OracleArtillery.SM_Alliance_AL_OracleArtillery")},
        {"CO_ConYard", TEXT("/Game/RA4/Art/Buildings/Coalition/SM_Coalition_CO_ConYard.SM_Coalition_CO_ConYard")},
        {"CO_PowerPlant", TEXT("/Game/RA4/Art/Buildings/Coalition/SM_Coalition_CO_PowerPlant.SM_Coalition_CO_PowerPlant")},
        {"CO_Refinery", TEXT("/Game/RA4/Art/Buildings/Coalition/SM_Coalition_CO_Refinery.SM_Coalition_CO_Refinery")},
        {"CO_Barracks", TEXT("/Game/RA4/Art/Buildings/Coalition/SM_Coalition_CO_Barracks.SM_Coalition_CO_Barracks")},
        {"CO_WarFactory", TEXT("/Game/RA4/Art/Buildings/Coalition/SM_Coalition_CO_WarFactory.SM_Coalition_CO_WarFactory")},
        {"CO_YuanCollector", TEXT("/Game/RA4/Art/Units/Coalition/SM_Coalition_CO_YuanCollector.SM_Coalition_CO_YuanCollector")},
        {"CO_KamakiriWalker", TEXT("/Game/RA4/Art/Units/Coalition/SM_Coalition_CO_KamakiriWalker.SM_Coalition_CO_KamakiriWalker")},
        {"CO_QinglongMBT", TEXT("/Game/RA4/Art/Units/Coalition/SM_Coalition_CO_QinglongMBT.SM_Coalition_CO_QinglongMBT")},
        {"CO_MonsoonArtillery", TEXT("/Game/RA4/Art/Units/Coalition/SM_Coalition_CO_MonsoonArtillery.SM_Coalition_CO_MonsoonArtillery")},
        {"CH_ConYard", TEXT("/Game/RA4/Art/Buildings/Chronolegion/SM_Chronolegion_CH_ConYard.SM_Chronolegion_CH_ConYard")},
        {"CH_PowerPlant", TEXT("/Game/RA4/Art/Buildings/Chronolegion/SM_Chronolegion_CH_PowerPlant.SM_Chronolegion_CH_PowerPlant")},
        {"CH_Refinery", TEXT("/Game/RA4/Art/Buildings/Chronolegion/SM_Chronolegion_CH_Refinery.SM_Chronolegion_CH_Refinery")},
        {"CH_Barracks", TEXT("/Game/RA4/Art/Buildings/Chronolegion/SM_Chronolegion_CH_Barracks.SM_Chronolegion_CH_Barracks")},
        {"CH_WarFactory", TEXT("/Game/RA4/Art/Buildings/Chronolegion/SM_Chronolegion_CH_WarFactory.SM_Chronolegion_CH_WarFactory")},
        {"CH_ProbabilistHarvester", TEXT("/Game/RA4/Art/Units/Chronolegion/SM_Chronolegion_CH_ProbabilistHarvester.SM_Chronolegion_CH_ProbabilistHarvester")},
        {"CH_ParallaxScout", TEXT("/Game/RA4/Art/Units/Chronolegion/SM_Chronolegion_CH_ParallaxScout.SM_Chronolegion_CH_ParallaxScout")},
        {"CH_TimelineTank", TEXT("/Game/RA4/Art/Units/Chronolegion/SM_Chronolegion_CH_TimelineTank.SM_Chronolegion_CH_TimelineTank")},
        {"CH_DeltaDelayArtillery", TEXT("/Game/RA4/Art/Units/Chronolegion/SM_Chronolegion_CH_DeltaDelayArtillery.SM_Chronolegion_CH_DeltaDelayArtillery")},
    };
    for (const FProductionMeshEntry& Entry : ProductionMeshes)
    {
        LoadBlockoutMesh(RA4::MakeContentId(Entry.ContentId).Value, Entry.AssetPath);
    }
}
