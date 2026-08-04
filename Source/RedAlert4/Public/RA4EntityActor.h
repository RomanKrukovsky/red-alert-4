// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RA4EntityActor.generated.h"

UCLASS(Blueprintable)
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

    // Returns the generation this Actor was bound at. The simulation recycles entity
    // slots, so the index alone does not identify an entity: a slot freed on one tick
    // can be handed to a different unit before presentation next runs. The generation
    // is what tells the two apart.
    uint32 GetEntityGeneration() const { return EntityGeneration; }

    // Sets the static mesh representation for this entity actor.
    UFUNCTION(BlueprintCallable, Category = "Visuals")
    void SetEntityMesh(UStaticMesh* InMesh);

    // Sets the team/faction color for dynamic material instances.
    UFUNCTION(BlueprintCallable, Category = "Visuals")
    void SetTeamColor(const FLinearColor& TeamColor);

    // Sizes either the placeholder or an authored mesh to the entity's real world
    // footprint, independent of the source asset's import scale.
    UFUNCTION(BlueprintCallable, Category = "Visuals")
    void SetVisualScale(const FVector& Scale);

    // Procedurally compose primitives for phase 0 blockout presentation.
    UFUNCTION(BlueprintCallable, Category = "Visuals")
    void ApplyPrimitiveComposition(const FString& EntityId);

    // Sets the art mapping asset used to resolve presentation assets for this entity.
    void SetArtMappingAsset(class URA4ArtMappingDataAsset* InArtMapping);

    // Sets the faction-scoped content id (e.g. unit.sov.conscript) used by ArtMapping.
    void SetEntityId(const FString& InEntityId);

    // Diagnostic dump of what this actor is actually rendering. Used to answer
    // "the match runs but I see nothing" with data instead of guesses.
    FString DescribeVisualState() const;

    // Direct Unit Possession Camera View Location & Rotation
    UFUNCTION(BlueprintCallable, Category = "DirectControl")
    FVector GetPossessionCameraLocation() const;

    UFUNCTION(BlueprintCallable, Category = "DirectControl")
    FRotator GetPossessionCameraRotation() const;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class UNiagaraComponent> VFXComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class UDecalComponent> SelectionDecalComponent;

    UFUNCTION(BlueprintCallable, Category = "Visuals")
    void SetSelected(bool bSelected);

    UPROPERTY(Transient)
    TObjectPtr<class UAnimSequence> CachedIdleAnim;

    UPROPERTY(Transient)
    TObjectPtr<class UAnimSequence> CachedRunAnim;

    UPROPERTY(Transient)
    TObjectPtr<class UAnimSequence> CachedAttackAnim;

    UPROPERTY(Transient)
    TObjectPtr<class URA4ArtMappingDataAsset> ArtMapping;

    // Kind tag such as "unit.sov.conscript" (from the simulation) or
    // "building.sov.construction_yard"; drives ArtMapping lookup and primitive fallback.
    FString EntityId;

    // Smooth interpolation targets
    FVector TargetPosition;
    float TargetRotationZ;

    // Half the placeholder's height, so the cube rests on the ground plane instead
    // of being centred in it.
    float VisualZOffset = 0.0f;

    // The footprint last asked for, kept so the scale can be recomputed whenever the
    // root mesh is swapped. SetVisualScale normalises against the *current* mesh's
    // bounds, so a later mesh change would otherwise leave the old mesh's correction
    // applied to new geometry -- which is exactly how the refinery and the harvester
    // once collapsed to nothing.
    FVector RequestedVisualScale = FVector::OneVector;
    bool bHasRequestedVisualScale = false;

private:
    uint32 EntityIndex = 0xFFFFFFFFu;
    uint32 EntityGeneration = 0;
};
