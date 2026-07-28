// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RA4EntityActor.generated.h"

UCLASS(Abstract, Blueprintable)
class REDALERT4_API ARA4EntityActor : public AActor
{
    GENERATED_BODY()

public:
    ARA4EntityActor();

    // Sets the visual transform and any other state extrapolated from the simulation.
    // The simulation runs at a fixed tick (e.g. 20 Hz), and this function is called
    // every frame by the WorldSubsystem to smoothly interpolate position and rotation.
    UFUNCTION(BlueprintCallable, Category = "Simulation")
    void UpdateFromSimulation(const FVector& NewPosition, float NewRotationZ, bool bTeleport);

    // Binds this Actor to a simulation entity.
    void BindToEntity(uint32 InEntityIndex, uint32 InEntityGeneration);

    // Returns the current Simulation Entity Index.
    UFUNCTION(BlueprintPure, Category = "Simulation")
    int32 GetEntityIndex() const { return static_cast<int32>(EntityIndex); }

    // Sets the static mesh representation for this entity actor.
    UFUNCTION(BlueprintCallable, Category = "Visuals")
    void SetEntityMesh(UStaticMesh* InMesh);

    // Sets the team/faction color for dynamic material instances.
    UFUNCTION(BlueprintCallable, Category = "Visuals")
    void SetTeamColor(const FLinearColor& TeamColor);

protected:
    virtual void BeginPlay() override;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    // Smooth interpolation targets
    FVector TargetPosition;
    float TargetRotationZ;

private:
    uint32 EntityIndex = 0xFFFFFFFFu;
    uint32 EntityGeneration = 0;
};
