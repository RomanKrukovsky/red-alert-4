// Copyright (c) Red Alert 4 project.

#include "RA4SimWorldSubsystem.h"
#include "RA4Simulation/SimWorld.h"
#include "RA4Core/Command.h"
#include "Engine/World.h"

URA4SimWorldSubsystem::URA4SimWorldSubsystem()
    : SimWorld(nullptr)
    , TimeSinceLastSimTick(0.0f)
{
}

URA4SimWorldSubsystem::~URA4SimWorldSubsystem()
{
}

void URA4SimWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Instantiate the simulation core
    SimWorld = new RA4::SimWorld();
    
    // In a real flow, we would load MatchSetup from GameInstance/Lobby.
    // For now, we initialize an empty setup.
    RA4::MatchSetup Setup;
    Setup.Seed = 12345;
    Setup.Map.Width = 64;
    Setup.Map.Height = 64;
    
    // SimWorld->Initialize(nullptr, Setup); // Need ContentDatabase passed in properly later
    
    // Register for world tick
    if (UWorld* World = GetWorld())
    {
        // UTickableWorldSubsystem automatically registers for ticking
    }
}

void URA4SimWorldSubsystem::Deinitialize()
{
    if (SimWorld)
    {
        delete SimWorld;
        SimWorld = nullptr;
    }

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

void URA4SimWorldSubsystem::TickSimulation()
{
    // Pass empty frame for now (Networking will inject real commands here)
    RA4::CommandFrame Frame;
    SimWorld->Tick(&Frame);
}

void URA4SimWorldSubsystem::SyncPresentation()
{
    if (!SimWorld) return;

    // Iterate all cores and update their bound actors
    const auto& Cores = SimWorld->GetAllCores();
    const auto& Transforms = SimWorld->GetAllTransforms();
    UWorld* World = GetWorld();

    for (uint32 Index = 0; Index < Cores.size(); ++Index)
    {
        if (Cores[Index].bAlive)
        {
            ARA4EntityActor* Actor = nullptr;
            if (ARA4EntityActor** ActorPtr = EntityActors.Find(Index))
            {
                Actor = *ActorPtr;
            }
            else if (World)
            {
                // Auto-spawn presentation actor if registered class is set
                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                
                UClass* ClassToSpawn = EntityActorClass ? EntityActorClass.Get() : ARA4EntityActor::StaticClass();
                Actor = World->SpawnActor<ARA4EntityActor>(ClassToSpawn, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
                
                if (Actor)
                {
                    Actor->BindToEntity(Index, Cores[Index].Id.Generation);
                    RegisterEntityActor(Index, Actor);

                    // Assign 3D mesh if ContentId exists in registry
                    uint32 ContentIdValue = Cores[Index].Content.Value;
                    if (UStaticMesh** MeshPtr = ContentMeshRegistry.Find(ContentIdValue))
                    {
                        Actor->SetEntityMesh(*MeshPtr);
                    }
                }
            }

            if (Actor)
            {
                const RA4::TransformComp& SimTransform = Transforms[Index];
                
                FVector UnrealPos(
                    static_cast<float>(SimTransform.Position.X.ToDoubleUnsafe()),
                    static_cast<float>(SimTransform.Position.Y.ToDoubleUnsafe()),
                    0.0f
                );
                
                float UnrealRotZ = static_cast<float>(SimTransform.Facing) * (360.0f / 255.0f);
                Actor->UpdateFromSimulation(UnrealPos, UnrealRotZ, false);
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
