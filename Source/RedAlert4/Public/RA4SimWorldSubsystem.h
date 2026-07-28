// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/TickableWorldSubsystem.h"
#include "RA4EntityActor.h"

// Forward declare the C++ simulation class without including the header here
// to minimize compile-time dependencies.
namespace RA4 { class SimWorld; }

#include "RA4SimWorldSubsystem.generated.h"

UCLASS()
class REDALERT4_API URA4SimWorldSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    URA4SimWorldSubsystem();
    virtual ~URA4SimWorldSubsystem();

    // UWorldSubsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime);
    virtual TStatId GetStatId() const override;

    // Registers a presentation actor to a simulation entity ID
    UFUNCTION(BlueprintCallable, Category = "Simulation")
    void RegisterEntityActor(int32 EntityIndex, ARA4EntityActor* Actor);

    // Unregisters a presentation actor
    UFUNCTION(BlueprintCallable, Category = "Simulation")
    void UnregisterEntityActor(int32 EntityIndex);

    // Map of ContentId / UnitType ID to Unreal StaticMesh asset
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
    TMap<int32, UStaticMesh*> ContentMeshRegistry;

    // Base Actor class to spawn for visual representations
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
    TSubclassOf<ARA4EntityActor> EntityActorClass;

private:
    void TickSimulation();
    void SyncPresentation();

    // Pointer to the deterministic C++ simulation core
    RA4::SimWorld* SimWorld;

    // Accumulator for fixed-step simulation ticking
    float TimeSinceLastSimTick;
    static constexpr float SimTickRate = 20.0f;
    static constexpr float SimTickDelta = 1.0f / SimTickRate;

    // Mapping from simulation EntityIndex to presentation AActor
    TMap<uint32, ARA4EntityActor*> EntityActors;
};
