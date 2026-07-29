// Copyright (c) Red Alert 4 project.

#include "RA4SimWorldSubsystem.h"
#include "RA4Simulation/SimWorld.h"
#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Command.h"
#include "RA4MatchBootstrap.h"
#include "RA4SimCoords.h"
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

    // Temporary: the lobby will supply the match setup. Until then a fixed skirmish
    // is seeded so the map is actually playable rather than an empty world.
    FRA4MatchBootstrap::BuildSkirmish(*Content, *SimWorld, /*Seed*/ 20260728);
    UE_LOG(LogTemp, Display, TEXT("RA4 skirmish initialized with %llu simulation entities"),
           static_cast<uint64>(SimWorld->GetAllCores().size()));
}

void URA4SimWorldSubsystem::Deinitialize()
{
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

void URA4SimWorldSubsystem::TickSimulation()
{
    // Everything queued since the previous tick executes in one frame, in the order
    // it was issued. The network layer replaces this local drain with the frame the
    // server broadcasts, and nothing else about the tick changes.
    RA4::CommandFrame Frame;
    Frame.Tick = SimWorld->GetTick();
    Frame.Commands.swap(PendingCommands);
    PendingCommands.clear();

    SimWorld->Tick(&Frame);
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
