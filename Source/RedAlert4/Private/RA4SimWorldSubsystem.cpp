// Copyright (c) Red Alert 4 project.

#include "RA4SimWorldSubsystem.h"
#include "RA4AI/AICommander.h"
#include "RA4Simulation/SimWorld.h"
#include "RA4Content/ContentDatabase.h"
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

void URA4SimWorldSubsystem::StartSkirmishMatch(uint8 PlayerFaction, uint8 EnemyFaction, int32 Difficulty)
{
    // The lobby or GameMode supplies the match setup.
    FRA4MatchBootstrap::BuildSkirmish(*Content, *SimWorld, /*Seed*/ 20260728, static_cast<RA4::FactionId>(PlayerFaction), static_cast<RA4::FactionId>(EnemyFaction));
    bWasLocalPowerShortage = false;

    // Every active player other than the local one gets a commander.
    constexpr RA4::PlayerId kLocalPlayer = 0;
    for (RA4::PlayerId Player = 0; Player < RA4::kMaxPlayers; ++Player)
    {
        if (Player == kLocalPlayer || !SimWorld->GetPlayer(Player).bActive)
        {
            continue;
        }
        RA4::AI::AICommander* Commander = new RA4::AI::AICommander();
        Commander->Initialize(Player, RA4::AI::AIProfile::Balanced, 20260728ull ^ (uint64(Player) * 0x9E3779B9ull));
        AICommanders.push_back(Commander);
        UE_LOG(LogTemp, Display, TEXT("RA4 AI commander attached to player %d"), int32(Player));
    }

    UE_LOG(LogTemp, Display, TEXT("RA4 skirmish initialized with %llu simulation entities"),
           static_cast<uint64>(SimWorld->GetAllCores().size()));
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
    PendingCommands.push_back(Command);
}

void URA4SimWorldSubsystem::SetSelectedEntitiesForHUD(const std::vector<RA4::EntityId>& Selection)
{
    LocalSelection = Selection;
}

void URA4SimWorldSubsystem::TickSimulation()
{
    // Everything queued since the previous tick executes in one frame, in the order
    // it was issued. The network layer replaces this local drain with the frame the
    // server broadcasts, and nothing else about the tick changes.
    RA4::CommandFrame Frame;
    Frame.Tick = SimWorld->GetTick();
    Frame.Commands.swap(PendingCommands);
    PendingCommands.clear();

    // The AI issues commands through exactly the same frame the player does, so it is
    // bound by the same server-side validation and stays replay-compatible.
    for (RA4::AI::AICommander* Commander : AICommanders)
    {
        Commander->Tick(*SimWorld, Frame.Commands);
    }

    SimWorld->Tick(&Frame);
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
}

void URA4SimWorldSubsystem::ProcessPresentationEvents()
{
    UWorld* UnrealWorld = GetWorld();
    if (UnrealWorld == nullptr || SimWorld == nullptr || Content == nullptr)
    {
        return;
    }

    URA4AudioSubsystem* Audio = UnrealWorld->GetSubsystem<URA4AudioSubsystem>();
    if (Audio == nullptr)
    {
        return;
    }

    constexpr RA4::PlayerId LocalPlayer = 0;
    const RA4::PlayerState& Player = SimWorld->GetPlayer(LocalPlayer);
    const uint8 Faction = static_cast<uint8>(Player.Faction);

    const auto PlayVoiceForContent =
        [this, Audio](RA4::ContentId ContentId, ERA4VoiceEvent VoiceEvent, bool bBypassCooldown)
    {
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
                        Audio->PlayEVA(Faction, ERA4EVAEvent::BaseUnderAttack);
                    }
                }
            }
            break;

        case RA4::SimEventType::EntityDestroyed:
            if (Event.Player == LocalPlayer)
            {
                PlayVoiceForContent(Event.Content, ERA4VoiceEvent::Death, true);
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
                Audio->PlayEVA(Faction, ERA4EVAEvent::ConstructionComplete);
            }
            break;

        case RA4::SimEventType::ProductionCompleted:
            if (Event.Player == LocalPlayer)
            {
                Audio->PlayEVA(Faction, ERA4EVAEvent::UnitReady);
            }
            break;

        case RA4::SimEventType::CommandRejected:
            if (Event.Player == LocalPlayer &&
                Event.Value == int32(RA4::CommandReject::InsufficientCredits))
            {
                Audio->PlayEVA(Faction, ERA4EVAEvent::InsufficientFunds);
            }
            break;

        case RA4::SimEventType::MatchEnded:
            Audio->PlayEVA(
                Faction,
                Event.Player == LocalPlayer ? ERA4EVAEvent::Victory : ERA4EVAEvent::Defeat,
                true);
            break;

        case RA4::SimEventType::WeaponFired:
            if (UnrealWorld && CVarRA4DebugCombatDraw.GetValueOnGameThread() != 0)
            {
                FVector Start = RA4Coords::ToUnreal(Event.Location);
                Start.Z = SampleGroundHeight(Start.X, Start.Y) + 20.0f;
                FVector End = Start;
                const auto& Transforms = SimWorld->GetAllTransforms();
                if (Event.Other.IsValid() && SimWorld->IsAlive(Event.Other) && Event.Other.Index < Transforms.size())
                {
                    End = RA4Coords::ToUnreal(Transforms[Event.Other.Index].Position);
                    End.Z = SampleGroundHeight(End.X, End.Y) + 20.0f;
                    DrawDebugLine(UnrealWorld, Start, End, FColor::Yellow, false, 0.2f, 0, 3.0f);
                }
                else
                {
                    DrawDebugPoint(UnrealWorld, Start, 15.0f, FColor::Yellow, false, 0.2f);
                }
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
        Audio->PlayEVA(Faction, ERA4EVAEvent::PowerLow);
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
                Actor = *ActorPtr;
            }
            else if (World)
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
                    // not, the actor keeps its placeholder primitive rather than
                    // rendering nothing.
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
}
