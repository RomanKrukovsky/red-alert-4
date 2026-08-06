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

    // Called once when the actor is bound: tells the visual whether it should rest
    // on the terrain or fly above it, and how high. Content-driven (MovementLayer),
    // never guessed from the mesh.
    void SetAirborne(bool bInAirborne, float InAltitude);

    // Assigns the turret mesh and where it sits on the hull. Without a mesh the
    // component stays hidden and turret yaw is ignored, which is the correct
    // behaviour for anything that has no turret.
    void SetTurretMesh(UStaticMesh* TurretMesh, float MountHeight);
    // Absolute world yaw for the turret, in degrees, straight from the
    // simulation's TurretFacing. Interpolated in Tick like position is, so a 20 Hz
    // sim tick does not read as a 20 Hz stutter at 60+ FPS.
    void SetTurretYaw(float NewYawDegrees, bool bTeleport);
    bool HasTurret() const { return bHasTurret; }
    // True when the unit aims but has no separate turret mesh, so the hull itself
    // must rotate to the turret angle. Honest fallback: the alternative is a tank
    // that fires sideways while its model points forward.
    void SetAimsWithHull(bool bInAimsWithHull) { bAimsWithHull = bInAimsWithHull; }

    // Construction progress, 0..1000 per mille, straight from the simulation.
    // 1000 means finished, and anything below sinks the mesh proportionally so the
    // building rises out of the ground as it is built -- the classic RTS read, and
    // the one visual that needs no animation asset. Also drives the progress bar.
    void SetConstructionProgress(int32 ProgressPerMille);

    // World-space height of the visible mesh in Unreal units. Used by the
    // construction sink and by anything that needs to place a widget above the
    // object; derived from live component bounds so it follows scaling.
    double GetMeshHeightUU() const;

    // Places the possession camera behind and to the right of the object, so the
    // controlled tank or infantryman is framed in the LEFT of the screen and no
    // part of its own geometry (barrel included) blocks the view. Recomputed from
    // live mesh bounds whenever art changes, because a fixed offset that suits a
    // tank buries the camera inside a war factory.
    void ApplyPossessionCameraFraming();

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

    UFUNCTION(BlueprintPure, Category = "DirectControl")
    class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

    // A separate turret mesh, so a tank's gun can face its target while the hull
    // faces its heading. The simulation has tracked TurretFacing all along and
    // nothing rendered it, because there was no component to rotate -- the whole
    // vehicle was one static mesh. Hidden unless a turret mesh is assigned, so
    // infantry and buildings are unaffected.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> TurretComponent;

    // Construction progress bar: two thin boxes, a dark track and a bright fill,
    // billboarded at the camera. Built from engine primitives on purpose -- a UMG
    // widget component would need an authored widget asset, and there is none; this
    // works today and can be replaced by real UI later without touching the
    // simulation-facing code.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> ProgressTrackComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> ProgressFillComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<class UCameraComponent> FirstPersonCameraComponent;

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
    // Distance from the actor origin up to the mesh's lowest point, so
    // SetActorLocation(groundHeight) lands the mesh ON the ground instead of
    // burying or floating it. Derived from real mesh bounds rather than assumed
    // to be half the height: authored blockouts do not agree on pivot placement
    // -- some sit at the base, some at the centre -- and assuming a centre pivot
    // is what made every base-pivot mesh hover by half its own height.
    float VisualZOffset = 0.0f;

    // Airborne units are deliberately exempt from ground-hugging: their altitude
    // is presentation state (the simulation is 2D), so they hold a fixed height
    // above the terrain instead of resting on it.
    bool bIsAirborne = false;
    float AirborneAltitude = 0.0f;

    bool bHasTurret = false;
    bool bAimsWithHull = false;
    // 1000 = complete. Kept as an int to match the simulation exactly rather than
    // converting to float and back.
    int32 ConstructionPerMille = 1000;
    void UpdateConstructionIndicator();
    float TargetTurretYaw = 0.0f;
    float CurrentTurretYaw = 0.0f;

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
