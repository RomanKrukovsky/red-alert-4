// Copyright (c) Red Alert 4 project.

#include "RA4SimWorldSubsystem.h"
#include "RA4Simulation/SimWorld.h"
#include "RA4Content/ContentDatabase.h"
#include "RA4Presentation/HudSnapshot.h"
#include "RA4Core/Command.h"
#include "RA4MatchBootstrap.h"
#include "RA4SimCoords.h"
#include "RA4UIDataProviderSubsystem.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

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
    return !FParse::Param(FCommandLine::Get(), TEXT("RA4UIOnly"))
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

    // Temporary: the lobby will supply the match setup. Until then a fixed skirmish
    // is seeded so the map is actually playable rather than an empty world.
    FRA4MatchBootstrap::BuildSkirmish(*Content, *SimWorld, /*Seed*/ 20260728);

    // Every active player other than the local one gets a commander. Without this the
    // opponent's base sat inert: the AI existed and was tested, but nothing in the
    // engine module ever ticked it.
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

    RegisterDefaultBlockoutMeshes();
    UE_LOG(LogTemp, Display, TEXT("RA4 skirmish initialized with %llu simulation entities"),
           static_cast<uint64>(SimWorld->GetAllCores().size()));
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
                
                const FVector UnrealPos = RA4Coords::ToUnreal(SimTransform.Position);
                
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
    // Soviet
    LoadBlockoutMesh(RA4::MakeContentId("building.sov.construction_yard").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ConYard_Blockout.SM_Soviet_SU_ConYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.sov.tesla_reactor").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_PowerPlant_Blockout.SM_Soviet_SU_PowerPlant_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.sov.ore_refinery").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Refinery_Blockout.SM_Soviet_SU_Refinery_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.sov.barracks").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Barracks_Blockout.SM_Soviet_SU_Barracks_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.sov.war_factory").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_WarFactory_Blockout.SM_Soviet_SU_WarFactory_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.sov.gun_turret").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GunTurret_Blockout.SM_Soviet_SU_GunTurret_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.sov.mcv").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_MCV_MobileYard_Blockout.SM_Soviet_SU_MCV_MobileYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.sov.ore_harvester").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_BogatyrOreCarrier_Blockout.SM_Soviet_SU_BogatyrOreCarrier_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.sov.conscript").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_RubezhRifleman_Blockout.SM_Soviet_SU_RubezhRifleman_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.sov.rocketeer").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ZaslonAATeam_Blockout.SM_Soviet_SU_ZaslonAATeam_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.sov.heavy_tank").Value, TEXT("/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GranitMBT_Blockout.SM_Soviet_SU_GranitMBT_Blockout"));

    // Alliance
    LoadBlockoutMesh(RA4::MakeContentId("building.all.construction_yard").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_ConYard_Blockout.SM_Alliance_AL_ConYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.all.power_plant").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_PowerPlant_Blockout.SM_Alliance_AL_PowerPlant_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.all.ore_refinery").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Refinery_Blockout.SM_Alliance_AL_Refinery_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.all.barracks").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Barracks_Blockout.SM_Alliance_AL_Barracks_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.all.war_factory").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_WarFactory_Blockout.SM_Alliance_AL_WarFactory_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.all.gun_turret").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_GunTurret_Blockout.SM_Alliance_AL_GunTurret_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.all.mcv").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_MCV_MobileNode_Blockout.SM_Alliance_AL_MCV_MobileNode_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.all.ore_harvester").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_PioneerHarvester_Blockout.SM_Alliance_AL_PioneerHarvester_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.all.rifleman").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_SentinelRifleman_Blockout.SM_Alliance_AL_SentinelRifleman_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.all.lancer").Value, TEXT("/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_LancerTeam_Blockout.SM_Alliance_AL_LancerTeam_Blockout"));
    // Eastern Coalition
    LoadBlockoutMesh(RA4::MakeContentId("building.eco.construction_yard").Value, TEXT("/Game/RA4/Art/Blockout/EasternCoalition/SM_EasternCoalition_CO_ConYard_Blockout.SM_EasternCoalition_CO_ConYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.eco.power_plant").Value, TEXT("/Game/RA4/Art/Blockout/EasternCoalition/SM_EasternCoalition_CO_PowerPlant_Blockout.SM_EasternCoalition_CO_PowerPlant_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.eco.ore_refinery").Value, TEXT("/Game/RA4/Art/Blockout/EasternCoalition/SM_EasternCoalition_CO_Refinery_Blockout.SM_EasternCoalition_CO_Refinery_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.eco.barracks").Value, TEXT("/Game/RA4/Art/Blockout/EasternCoalition/SM_EasternCoalition_CO_Barracks_Blockout.SM_EasternCoalition_CO_Barracks_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.eco.war_factory").Value, TEXT("/Game/RA4/Art/Blockout/EasternCoalition/SM_EasternCoalition_CO_WarFactory_Blockout.SM_EasternCoalition_CO_WarFactory_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.eco.gun_turret").Value, TEXT("/Game/RA4/Art/Blockout/EasternCoalition/SM_EasternCoalition_CO_GunTurret_Blockout.SM_EasternCoalition_CO_GunTurret_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.eco.mcv").Value, TEXT("/Game/RA4/Art/Blockout/EasternCoalition/SM_EasternCoalition_CO_MCV_MobileDepot_Blockout.SM_EasternCoalition_CO_MCV_MobileDepot_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.eco.ore_harvester").Value, TEXT("/Game/RA4/Art/Blockout/EasternCoalition/SM_EasternCoalition_CO_BishanHarvester_Blockout.SM_EasternCoalition_CO_BishanHarvester_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.eco.rifleman").Value, TEXT("/Game/RA4/Art/Blockout/EasternCoalition/SM_EasternCoalition_CO_VolunteerRifleman_Blockout.SM_EasternCoalition_CO_VolunteerRifleman_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.eco.heavy_tank").Value, TEXT("/Game/RA4/Art/Blockout/EasternCoalition/SM_EasternCoalition_CO_Type99Zheng_Blockout.SM_EasternCoalition_CO_Type99Zheng_Blockout"));

    // Chrono Legion
    LoadBlockoutMesh(RA4::MakeContentId("building.chr.construction_yard").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_ConYard_Blockout.SM_Chronolegion_CH_ConYard_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.chr.power_plant").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_EraEngine_Blockout.SM_Chronolegion_CH_EraEngine_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.chr.ore_refinery").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_CausalityAnchor_Blockout.SM_Chronolegion_CH_CausalityAnchor_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.chr.barracks").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_Barracks_Blockout.SM_Chronolegion_CH_Barracks_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.chr.war_factory").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_Airfield_Blockout.SM_Chronolegion_CH_Airfield_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("building.chr.gun_turret").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_EchoTurret_Blockout.SM_Chronolegion_CH_EchoTurret_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.chr.ore_harvester").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_ParadoxSiphon_Blockout.SM_Chronolegion_CH_ParadoxSiphon_Blockout"));
    LoadBlockoutMesh(RA4::MakeContentId("unit.chr.rifleman").Value, TEXT("/Game/RA4/Art/Blockout/Chronolegion/SM_Chronolegion_CH_TemporalInfantry_Blockout.SM_Chronolegion_CH_TemporalInfantry_Blockout"));
}
