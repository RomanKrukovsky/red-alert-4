// Copyright (c) Red Alert 4 project.

#include "RA4EntityActor.h"
#include "RA4Presentation/RA4ArtMapping.h"

#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
// Fallback framing for an entity with no mesh yet (the placeholder cube). Behind,
// to the right and above, so even the placeholder is viewed from outside rather
// than from within. Real entities override this from their own bounds in
// ApplyPossessionCameraFraming().
const FVector kDefaultPossessionCameraOffset(-420.0, 220.0, 180.0);
}

ARA4EntityActor::ARA4EntityActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    SetRootComponent(MeshComponent);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Collision is handled by simulation core

    TurretComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretComponent"));
    TurretComponent->SetupAttachment(MeshComponent);
    TurretComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TurretComponent->SetVisibility(false);   // shown only when a turret mesh is set
    // Absolute rotation: the turret aims in WORLD space, so it must not inherit the
    // hull's yaw -- otherwise a turning tank drags its aim off target, which is the
    // bug this component exists to avoid. Scale still follows the hull.
    TurretComponent->SetAbsolute(false, true, false);

    ProgressTrackComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProgressTrackComponent"));
    ProgressTrackComponent->SetupAttachment(MeshComponent);
    ProgressTrackComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProgressTrackComponent->SetVisibility(false);
    // Absolute rotation and scale: the bar must keep its size and face the camera
    // regardless of how the hull is scaled or turned, otherwise a 3x3 factory gets
    // a bar three times the size of a tank's and a turning unit spins its own UI.
    ProgressTrackComponent->SetAbsolute(false, true, true);
    ProgressTrackComponent->SetCastShadow(false);

    ProgressFillComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProgressFillComponent"));
    ProgressFillComponent->SetupAttachment(MeshComponent);
    ProgressFillComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProgressFillComponent->SetVisibility(false);
    ProgressFillComponent->SetAbsolute(false, true, true);
    ProgressFillComponent->SetCastShadow(false);

    FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCameraComponent"));
    FirstPersonCameraComponent->SetupAttachment(MeshComponent);
    // Over-the-shoulder, NOT inside the hull. The old (0,0,80) put the camera
    // within the mesh, so the model's own geometry -- the barrel most of all --
    // filled the frame: the "gun barrel effect". The camera is now pulled back and
    // offset to the RIGHT, which places the controlled tank or infantryman in the
    // LEFT of the frame where the player can actually see what they are driving.
    // Values are recomputed per entity in ApplyPossessionCameraFraming() from the
    // real mesh size; these are the fallback for a mesh-less placeholder.
    FirstPersonCameraComponent->SetRelativeLocation(kDefaultPossessionCameraOffset);
    FirstPersonCameraComponent->bUsePawnControlRotation = false;

    SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
    SkeletalMeshComponent->SetupAttachment(MeshComponent);
    SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SkeletalMeshComponent->SetVisibility(false);
    SkeletalMeshComponent->SetAbsolute(false, false, true);

    SelectionDecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("SelectionDecalComponent"));
    SelectionDecalComponent->SetupAttachment(MeshComponent);
    SelectionDecalComponent->SetVisibility(false);
    SelectionDecalComponent->DecalSize = FVector(64.0f, 64.0f, 64.0f);

    // Fall back to an engine primitive so an entity is always visible. Without this
    // an unregistered content id spawns an actor with no mesh, and the match looks
    // empty even though the simulation is running correctly underneath.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        MeshComponent->SetStaticMesh(CubeMesh.Object);
    }
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaceholderMaterial(
        TEXT("/Game/RA4/Materials/M_RA4EntityPlaceholder_Lit.M_RA4EntityPlaceholder_Lit"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    // BasicShapeMaterial exposes the well-tested "Color" parameter on every
    // supported renderer. Prefer it for procedural placeholder geometry; the
    // authored RA4 material remains available as a fallback.
    if (ShapeMaterial.Succeeded())
    {
        MeshComponent->SetMaterial(0, ShapeMaterial.Object);
    }
    else if (PlaceholderMaterial.Succeeded())
    {
        MeshComponent->SetMaterial(0, PlaceholderMaterial.Object);
    }

    TargetPosition = FVector::ZeroVector;
    TargetRotationZ = 0.0f;
}

void ARA4EntityActor::SetSelected(bool bSelected)
{
    if (SelectionDecalComponent)
    {
        SelectionDecalComponent->SetVisibility(bSelected);
    }
}

void ARA4EntityActor::ApplyPossessionCameraFraming()
{
    if (FirstPersonCameraComponent == nullptr)
    {
        return;
    }

    // Frame the object from its actual size rather than a fixed distance: a
    // rifleman and a war factory need very different standoff, and a constant
    // offset either buries the camera in the tank or leaves infantry a speck.
    const double Height = GetMeshHeightUU();
    double Radius = 100.0;
    if (SkeletalMeshComponent != nullptr && SkeletalMeshComponent->IsVisible())
    {
        const FVector E = SkeletalMeshComponent->Bounds.BoxExtent;
        Radius = FMath::Max(E.X, E.Y);
    }
    else if (MeshComponent != nullptr)
    {
        const FVector E = MeshComponent->Bounds.BoxExtent;
        Radius = FMath::Max(E.X, E.Y);
    }
    Radius = FMath::Max(Radius, 40.0);

    // Behind by ~3 radii clears the longest barrel in the blockout set; the
    // sideways offset is what pushes the subject off-centre to the left. Positive
    // Y is right in Unreal, and moving the CAMERA right moves the SUBJECT left.
    const double Back = -(Radius * 3.0 + 120.0);
    const double Right = Radius * 1.6 + 60.0;
    const double Up = Height * 0.85 + 60.0;

    FirstPersonCameraComponent->SetRelativeLocation(FVector(Back, Right, Up));
    // Aim slightly left and down so the subject sits in the left third with the
    // ground and the horizon both readable -- looking straight ahead from an
    // offset camera would frame empty space instead.
    FirstPersonCameraComponent->SetRelativeRotation(FRotator(-8.0, -12.0, 0.0));
}

FVector ARA4EntityActor::GetPossessionCameraLocation() const
{
    if (FirstPersonCameraComponent)
    {
        return FirstPersonCameraComponent->GetComponentLocation();
    }
    // Fallback must also frame from OUTSIDE. The old value put the camera 80 uu
    // above the actor origin, i.e. inside the hull, which is the in-model view
    // this change exists to remove -- leaving it here would restore the barrel in
    // frame the moment the camera component is missing.
    return GetActorLocation() + GetActorRotation().RotateVector(kDefaultPossessionCameraOffset);
}

FRotator ARA4EntityActor::GetPossessionCameraRotation() const
{
    if (FirstPersonCameraComponent)
    {
        return FirstPersonCameraComponent->GetComponentRotation();
    }
    // Match the offset camera's aim, not the hull's: returning the raw hull
    // rotation from an offset position frames empty ground beside the subject.
    return GetActorRotation() + FRotator(-8.0, -12.0, 0.0);
}

void ARA4EntityActor::SetEntityMesh(UStaticMesh* InMesh)
{
    if (MeshComponent)
    {
        // The constructor puts BasicShapeMaterial on the emergency cube. Component
        // overrides survive SetStaticMesh(), so without clearing them every real
        // PBR slot is silently replaced by that placeholder material at runtime.
        MeshComponent->EmptyOverrideMaterials();
        MeshComponent->SetStaticMesh(InMesh);
        if (InMesh)
        {
            // EmptyOverrideMaterials alone is not enough for this native default
            // subobject: the constructor override can be restored from the actor
            // archetype after the mesh swap. Copy the authored asset slots back to
            // the component explicitly so GetMaterial() and rendering agree.
            const int32 MaterialCount = InMesh->GetStaticMaterials().Num();
            for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
            {
                MeshComponent->SetMaterial(MaterialIndex, InMesh->GetMaterial(MaterialIndex));
            }
        }
        // The scale correction is relative to the mesh's own bounds, so a swap
        // invalidates it. Recompute against the new geometry rather than relying on
        // callers to happen to set the mesh before the scale.
        if (bHasRequestedVisualScale)
        {
            SetVisualScale(RequestedVisualScale);
        }
    }
}

void ARA4EntityActor::SetTeamColor(const FLinearColor& TeamColor)
{
    if (MeshComponent == nullptr)
    {
        return;
    }

    // Production meshes already carry faction PBR instances per authored slot
    // (paint, rubber, glass, concrete and emissive). Replacing every slot with
    // the old blockout material destroys that work and makes the four factions
    // visually identical. Player ownership remains readable through selection
    // decals/UI until a dedicated mask channel is authored.
    if (const UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh())
    {
        const FString MeshPath = StaticMesh->GetPathName();
        if (MeshPath.Contains(TEXT("/RA4/Art/Units/")) ||
            MeshPath.Contains(TEXT("/RA4/Art/Buildings/")))
        {
            return;
        }
    }

    // Select the faction's textured metal material. Imported FBX blockouts commonly
    // have several material slots; changing only slot zero left most of every model
    // on its original white material even though the log reported the correct MID.
    UMaterialInterface* FactionMat = nullptr;
    if (TeamColor.R > 0.5f && TeamColor.B < 0.2f)
    {
        FactionMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RA4/Presentation/Materials/Blockout/MI_RA4_Blockout_SU.MI_RA4_Blockout_SU"));
    }
    else if (TeamColor.B > 0.5f && TeamColor.R < 0.2f)
    {
        FactionMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RA4/Presentation/Materials/Blockout/MI_RA4_Blockout_AL.MI_RA4_Blockout_AL"));
    }
    else
    {
        FactionMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RA4/Presentation/Materials/Blockout/MI_RA4_Blockout_Neutral.MI_RA4_Blockout_Neutral"));
    }

    const auto ApplyFactionMaterial = [FactionMat, &TeamColor](UMeshComponent* Component)
    {
        if (Component == nullptr)
        {
            return;
        }

        const int32 SlotCount = FMath::Max(1, Component->GetNumMaterials());
        for (int32 Slot = 0; Slot < SlotCount; ++Slot)
        {
            UMaterialInstanceDynamic* DynamicMaterial =
                Component->CreateDynamicMaterialInstance(Slot, FactionMat);
            if (DynamicMaterial != nullptr)
            {
                DynamicMaterial->SetVectorParameterValue(TEXT("TeamColor"), TeamColor);
                DynamicMaterial->SetVectorParameterValue(TEXT("Color"), TeamColor);
                DynamicMaterial->SetScalarParameterValue(TEXT("Metallic"), 0.85f);
                DynamicMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.35f);
                DynamicMaterial->SetScalarParameterValue(TEXT("Specular"), 0.6f);
            }
        }
    };

    ApplyFactionMaterial(MeshComponent);
    ApplyFactionMaterial(SkeletalMeshComponent);
}

void ARA4EntityActor::SetVisualScale(const FVector& Scale)
{
    if (MeshComponent == nullptr)
    {
        return;
    }

    RequestedVisualScale = Scale;
    bHasRequestedVisualScale = true;

    // Scale expresses the wanted world footprint in units of the 100 cm engine
    // placeholder cube. Authored blockout meshes do not share that source size:
    // several are tens of thousands of units wide. Multiplying those by the
    // placeholder scale made a single building engulf the entire camera.
    const UStaticMesh* Mesh = MeshComponent->GetStaticMesh();
    const FVector SourceSize = Mesh != nullptr
                                   ? Mesh->GetBounds().BoxExtent * 2.0
                                   : FVector(100.0);
    const FVector DesiredSize = Scale * 100.0;
    const FVector NormalizedScale(
        SourceSize.X > UE_SMALL_NUMBER ? DesiredSize.X / SourceSize.X : 1.0,
        SourceSize.Y > UE_SMALL_NUMBER ? DesiredSize.Y / SourceSize.Y : 1.0,
        SourceSize.Z > UE_SMALL_NUMBER ? DesiredSize.Z / SourceSize.Z : 1.0);
    MeshComponent->SetWorldScale3D(NormalizedScale);

    // Camera framing depends on the same bounds, so refresh it here rather than
    // leaving a stale standoff from a previous mesh.
    ApplyPossessionCameraFraming();

    // Ground-hugging offset from the mesh's ACTUAL bounds, not from an assumed
    // centre pivot. The old code used DesiredSize.Z * 0.5, which is only correct
    // for a centre-pivot mesh: every base-pivot blockout floated by half its
    // height, and anything with an off-centre pivot was wrong by an arbitrary
    // amount. GetBounds() gives the origin-to-centre offset, so the distance from
    // the actor origin down to the mesh's lowest point is (centre - extent) on Z,
    // scaled the same way the mesh is.
    if (Mesh != nullptr)
    {
        const FBoxSphereBounds LocalBounds = Mesh->GetBounds();
        const double LowestPointLocal = LocalBounds.Origin.Z - LocalBounds.BoxExtent.Z;
        // Negative when the mesh hangs below its origin (centre pivot), ~0 when the
        // pivot already sits at the base. Lifting by -lowest puts the base at Z=0.
        VisualZOffset = float(-LowestPointLocal * NormalizedScale.Z);
    }
    else
    {
        // No mesh yet: the placeholder cube is centre-pivoted, so half its height.
        VisualZOffset = float(DesiredSize.Z * 0.5);
    }
}



double ARA4EntityActor::GetMeshHeightUU() const
{
    // Whichever body is actually visible: a skeletal unit and a static building
    // must both answer this correctly, and asking the hidden one would return a
    // stale or zero extent.
    if (SkeletalMeshComponent != nullptr && SkeletalMeshComponent->IsVisible())
    {
        return SkeletalMeshComponent->Bounds.BoxExtent.Z * 2.0;
    }
    if (MeshComponent != nullptr)
    {
        return MeshComponent->Bounds.BoxExtent.Z * 2.0;
    }
    return 100.0;   // placeholder cube height
}


void ARA4EntityActor::UpdateConstructionIndicator()
{
    if (ProgressTrackComponent == nullptr || ProgressFillComponent == nullptr)
    {
        return;
    }

    const bool bShow = ConstructionPerMille < 1000;
    if (!bShow)
    {
        if (ProgressTrackComponent->IsVisible())
        {
            ProgressTrackComponent->SetVisibility(false);
            ProgressFillComponent->SetVisibility(false);
        }
        return;
    }

    // Lazily give the bars a mesh the first time one is needed: an engine cube,
    // squashed. Loading it in the constructor would pay for every entity in the
    // match when only buildings under construction ever show a bar.
    if (ProgressTrackComponent->GetStaticMesh() == nullptr)
    {
        UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        if (Cube == nullptr)
        {
            return;   // no primitive available; no bar rather than a crash
        }
        ProgressTrackComponent->SetStaticMesh(Cube);
        ProgressFillComponent->SetStaticMesh(Cube);
    }

    // Sit above the FULL height of the finished building, not the sunken visual,
    // so the bar holds still while the building rises underneath it.
    const double FullHeight = GetMeshHeightUU();
    const double BarZ = VisualZOffset + FullHeight + 120.0;
    const double BarWidth = FMath::Clamp(FullHeight * 1.2, 300.0, 1200.0);
    constexpr double BarThickness = 18.0;
    constexpr double CubeSize = 100.0;   // engine cube is 100 uu per side

    ProgressTrackComponent->SetVisibility(true);
    ProgressFillComponent->SetVisibility(true);

    ProgressTrackComponent->SetRelativeLocation(FVector(0.0, 0.0, BarZ));
    ProgressTrackComponent->SetWorldScale3D(
        FVector(BarWidth / CubeSize, BarThickness / CubeSize, BarThickness / CubeSize));

    const double Fraction = double(ConstructionPerMille) / 1000.0;
    // The fill grows from the left edge rather than from the centre, which is what
    // reads as a progress bar; centring it would look like a shrinking object.
    const double FillWidth = FMath::Max(BarWidth * Fraction, 1.0);
    ProgressFillComponent->SetWorldScale3D(
        FVector(FillWidth / CubeSize, BarThickness / CubeSize * 1.25, BarThickness / CubeSize * 1.25));

    // Billboard both parts at the local camera each update.
    FRotator BarRotation = FRotator::ZeroRotator;
    if (const UWorld* World = GetWorld())
    {
        if (const APlayerController* PC = World->GetFirstPlayerController())
        {
            FVector CamLoc;
            FRotator CamRot;
            PC->GetPlayerViewPoint(CamLoc, CamRot);
            BarRotation = FRotator(0.0f, CamRot.Yaw, 0.0f);
        }
    }
    ProgressTrackComponent->SetWorldRotation(BarRotation);
    ProgressFillComponent->SetWorldRotation(BarRotation);

    // Offset the fill so its left edge lines up with the track's left edge.
    const FVector Right = FRotationMatrix(BarRotation).GetUnitAxis(EAxis::X);
    const FVector TrackCentre = ProgressTrackComponent->GetComponentLocation();
    ProgressFillComponent->SetWorldLocation(
        TrackCentre - Right * (BarWidth * 0.5) + Right * (FillWidth * 0.5));
}

void ARA4EntityActor::SetConstructionProgress(int32 ProgressPerMille)
{
    ConstructionPerMille = FMath::Clamp(ProgressPerMille, 0, 1000);
}

void ARA4EntityActor::SetAirborne(bool bInAirborne, float InAltitude)
{
    bIsAirborne = bInAirborne;
    AirborneAltitude = InAltitude;
}


void ARA4EntityActor::SetTurretMesh(UStaticMesh* TurretMesh, float MountHeight)
{
    if (TurretComponent == nullptr)
    {
        return;
    }
    bHasTurret = TurretMesh != nullptr;
    TurretComponent->SetStaticMesh(TurretMesh);
    TurretComponent->SetVisibility(bHasTurret);
    if (bHasTurret)
    {
        // Sits on top of the hull. Relative Z only -- X/Y stay centred, so the
        // turret rotates about the hull's own axis rather than orbiting it.
        TurretComponent->SetRelativeLocation(FVector(0.0, 0.0, double(MountHeight)));
    }
}

void ARA4EntityActor::SetTurretYaw(float NewYawDegrees, bool bTeleport)
{
    TargetTurretYaw = NewYawDegrees;
    if (bTeleport)
    {
        CurrentTurretYaw = NewYawDegrees;
        if (TurretComponent != nullptr && bHasTurret)
        {
            TurretComponent->SetWorldRotation(FRotator(0.0f, CurrentTurretYaw, 0.0f));
        }
    }
}

void ARA4EntityActor::BeginPlay()
{
    Super::BeginPlay();
    TargetPosition = GetActorLocation();
    TargetRotationZ = GetActorRotation().Yaw;
}

void ARA4EntityActor::UpdateFromSimulation(const FVector& NewPosition, float NewRotationZ, bool bTeleport)
{
    // Ground units are lifted so their mesh rests on the terrain; aircraft are
    // lifted to a fixed altitude above it and deliberately do NOT hug the ground
    // -- a helicopter sitting on a hillside is the same physics violation as a
    // tank floating over flat land, just in the other direction.
    double ZLift = bIsAirborne ? double(AirborneAltitude) : double(VisualZOffset);

    // Under construction: sink the mesh so only the built fraction shows above
    // ground. There is no construction animation, so this IS the animation --
    // and it doubles as an unmistakable readable state, because a half-sunk
    // building cannot be confused with a finished one at any zoom level.
    // MeshHeight comes from the same bounds that produced VisualZOffset, so the
    // two cannot disagree about where the ground is.
    if (ConstructionPerMille < 1000)
    {
        const double MeshHeight = GetMeshHeightUU();
        const double HiddenFraction = double(1000 - ConstructionPerMille) / 1000.0;
        ZLift -= MeshHeight * HiddenFraction;
    }

    TargetPosition = NewPosition + FVector(0.0, 0.0, ZLift);
    TargetRotationZ = NewRotationZ;
    
    if (bTeleport)
    {
        SetActorLocationAndRotation(TargetPosition, FRotator(0.0f, TargetRotationZ, 0.0f));
    }
}

void ARA4EntityActor::BindToEntity(uint32 InEntityIndex, uint32 InEntityGeneration)
{
    EntityIndex = InEntityIndex;
    EntityGeneration = InEntityGeneration;
}

void ARA4EntityActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Simple interpolation for smooth presentation (lerp rate ~ 10.0f to smooth 20Hz tick)
    FVector CurrentLocation = GetActorLocation();
    FVector InterpolatedLocation = FMath::VInterpTo(CurrentLocation, TargetPosition, DeltaTime, 15.0f);
    
    FRotator CurrentRotation = GetActorRotation();
    // A unit that aims but has no separate turret mesh turns its whole hull to the
    // gun's angle. Not ideal-looking, but it is the truth: the simulation fires
    // along TurretFacing, and a model pointing elsewhere would be lying about
    // where the shot goes. Replaced automatically once art ships a turret mesh.
    const float HullTargetYaw = (bAimsWithHull && !bHasTurret) ? TargetTurretYaw : TargetRotationZ;
    FRotator TargetRotator(0.0f, HullTargetYaw, 0.0f);
    FRotator InterpolatedRotation = FMath::RInterpTo(CurrentRotation, TargetRotator, DeltaTime, 15.0f);

    UpdateConstructionIndicator();

    // Turret: interpolated like the hull, because the simulation only produces a
    // new angle 20 times a second and a hard set reads as a stutter at 60+ FPS.
    // FInterpTo would take the long way round from 359 to 1 degree, so the delta
    // is normalised to [-180, 180] first -- that is the difference between a
    // turret tracking a target and one spinning a full circle to reach it.
    if (bHasTurret && TurretComponent != nullptr)
    {
        const float YawDelta = FMath::UnwindDegrees(TargetTurretYaw - CurrentTurretYaw);
        // Same rate as the hull so the two read as one machine. The simulation
        // already enforces the real traverse-speed limit; this is only smoothing
        // between ticks and must never be slower than the sim, or the visual
        // would lag behind where the gun is actually pointing.
        const float Step = YawDelta * FMath::Min(1.0f, DeltaTime * 15.0f);
        CurrentTurretYaw = FMath::UnwindDegrees(CurrentTurretYaw + Step);
        TurretComponent->SetWorldRotation(FRotator(0.0f, CurrentTurretYaw, 0.0f));
    }

    // AAA RTS Vehicle Dynamics: pitch dip on acceleration/brake, roll lean on turn, engine vibration
    const FVector Velocity = (InterpolatedLocation - CurrentLocation) / FMath::Max(DeltaTime, 0.001f);
    const float Speed = Velocity.Size();

    if (MeshComponent != nullptr && Speed > 10.0f && GetWorld() != nullptr)
    {
        const FVector Forward = InterpolatedRotation.Vector();
        const FVector Right = FRotationMatrix(InterpolatedRotation).GetUnitAxis(EAxis::Y);

        const float ForwardSpeed = FVector::DotProduct(Velocity, Forward);
        const float LateralSpeed = FVector::DotProduct(Velocity, Right);

        const float PitchTilt = FMath::Clamp(ForwardSpeed * 0.004f, -6.0f, 6.0f);
        const float RollTilt = FMath::Clamp(LateralSpeed * 0.006f, -5.0f, 5.0f);
        const float EngineVibration = FMath::Sin(GetWorld()->GetTimeSeconds() * 22.0f) * 0.8f;

        FRotator DynamicRotator = InterpolatedRotation;
        DynamicRotator.Pitch += PitchTilt;
        DynamicRotator.Roll += RollTilt;

        FVector DynamicLocation = InterpolatedLocation;
        DynamicLocation.Z += EngineVibration;

        SetActorLocationAndRotation(DynamicLocation, DynamicRotator);
    }
    else
    {
        SetActorLocationAndRotation(InterpolatedLocation, InterpolatedRotation);
    }

    // Phase 2: Infantry Locomotion Animation
    if (SkeletalMeshComponent && SkeletalMeshComponent->IsVisible())
    {
        bool bIsMoving = Speed > 20.0f;
        
        static UAnimSequence* DefaultIdleAnim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/ThirdParty/QuantumCharacter/Demo/Animations/A_MM_Idle.A_MM_Idle"));
        static UAnimSequence* DefaultRunAnim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/ThirdParty/QuantumCharacter/Demo/Animations/A_MM_Run_Fwd.A_MM_Run_Fwd"));
        
        UAnimSequence* IdleAnim = CachedIdleAnim.Get() != nullptr ? CachedIdleAnim.Get() : DefaultIdleAnim;
        UAnimSequence* RunAnim = CachedRunAnim.Get() != nullptr ? CachedRunAnim.Get() : DefaultRunAnim;
        UAnimSequence* TargetAnim = bIsMoving ? RunAnim : IdleAnim;

        if (TargetAnim && SkeletalMeshComponent->GetAnimationMode() != EAnimationMode::AnimationSingleNode)
        {
            SkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        }
        
        if (TargetAnim && SkeletalMeshComponent->GetSingleNodeInstance() && SkeletalMeshComponent->AnimationData.AnimToPlay != TargetAnim)
        {
            SkeletalMeshComponent->PlayAnimation(TargetAnim, true);
        }
    }
}

FString ARA4EntityActor::DescribeVisualState() const
{
    if (MeshComponent == nullptr)
    {
        return TEXT("no mesh component");
    }
    const UStaticMesh* Mesh = MeshComponent->GetStaticMesh();
    const FVector Loc = GetActorLocation();
    const FVector Scale = MeshComponent->GetComponentScale();
    const FBoxSphereBounds Bounds = MeshComponent->Bounds;
    return FString::Printf(
        TEXT("mesh=%s mat=%s pos=(%.0f,%.0f,%.0f) scale=(%.1f,%.1f,%.1f) radius=%.0f visible=%d hidden=%d"),
        Mesh != nullptr ? *Mesh->GetName() : TEXT("NONE"),
        MeshComponent->GetMaterial(0) != nullptr ? *MeshComponent->GetMaterial(0)->GetName() : TEXT("NONE"),
        Loc.X, Loc.Y, Loc.Z, Scale.X, Scale.Y, Scale.Z,
        Bounds.SphereRadius, MeshComponent->IsVisible() ? 1 : 0, IsHidden() ? 1 : 0);
}

void ARA4EntityActor::SetArtMappingAsset(URA4ArtMappingDataAsset* InArtMapping)
{
    ArtMapping = InArtMapping;
}

void ARA4EntityActor::SetEntityId(const FString& InEntityId)
{
    EntityId = InEntityId;
}

void ARA4EntityActor::ApplyPrimitiveComposition(const FString& InEntityId)
{
    SetEntityId(InEntityId);
    // If a real 3D static mesh is assigned, skip Phase 0 procedural shape composition
    if (MeshComponent != nullptr && MeshComponent->GetStaticMesh() != nullptr &&
        !MeshComponent->GetStaticMesh()->GetPathName().Contains(TEXT("BasicShapes/Cube")))
    {
        return;
    }

    // Phase 0: Visually distinguish buildings without external assets.
    UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* ConeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));

    if (EntityId.Contains(TEXT("Headquarters")))
    {
        // Two tall sections and an antenna
        UStaticMeshComponent* Tower1 = NewObject<UStaticMeshComponent>(this);
        Tower1->SetStaticMesh(CubeMesh);
        Tower1->SetupAttachment(MeshComponent);
        Tower1->SetRelativeLocation(FVector(0, -30, 50));
        Tower1->SetRelativeScale3D(FVector(0.5f, 0.5f, 2.0f));
        Tower1->RegisterComponent();

        UStaticMeshComponent* Tower2 = NewObject<UStaticMeshComponent>(this);
        Tower2->SetStaticMesh(CubeMesh);
        Tower2->SetupAttachment(MeshComponent);
        Tower2->SetRelativeLocation(FVector(0, 30, 70));
        Tower2->SetRelativeScale3D(FVector(0.5f, 0.5f, 2.5f));
        Tower2->RegisterComponent();

        UStaticMeshComponent* Antenna = NewObject<UStaticMeshComponent>(this);
        Antenna->SetStaticMesh(CylinderMesh);
        Antenna->SetupAttachment(Tower2);
        Antenna->SetRelativeLocation(FVector(0, 0, 80));
        Antenna->SetRelativeScale3D(FVector(0.1f, 0.1f, 3.0f));
        Antenna->RegisterComponent();
    }
    else if (EntityId.Contains(TEXT("PowerPlant")))
    {
        // Cylindrical elements
        UStaticMeshComponent* Cyl1 = NewObject<UStaticMeshComponent>(this);
        Cyl1->SetStaticMesh(CylinderMesh);
        Cyl1->SetupAttachment(MeshComponent);
        Cyl1->SetRelativeLocation(FVector(-20, -20, 30));
        Cyl1->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.5f));
        Cyl1->RegisterComponent();

        UStaticMeshComponent* Cyl2 = NewObject<UStaticMeshComponent>(this);
        Cyl2->SetStaticMesh(CylinderMesh);
        Cyl2->SetupAttachment(MeshComponent);
        Cyl2->SetRelativeLocation(FVector(20, 20, 30));
        Cyl2->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.5f));
        Cyl2->RegisterComponent();
    }
    else if (EntityId.Contains(TEXT("Radar")))
    {
        UStaticMeshComponent* Dish = NewObject<UStaticMeshComponent>(this);
        Dish->SetStaticMesh(ConeMesh);
        Dish->SetupAttachment(MeshComponent);
        Dish->SetRelativeLocation(FVector(0, 0, 60));
        Dish->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.2f));
        Dish->RegisterComponent();
    }
    else if (EntityId.Contains(TEXT("WarFactory")) || EntityId.Contains(TEXT("NavalYard")))
    {
        // Large industrial silhouette
        UStaticMeshComponent* Hangar = NewObject<UStaticMeshComponent>(this);
        Hangar->SetStaticMesh(CylinderMesh); // horizontal cylinder for hangar roof
        Hangar->SetupAttachment(MeshComponent);
        Hangar->SetRelativeLocation(FVector(0, 0, 50));
        Hangar->SetRelativeRotation(FRotator(90, 0, 0));
        Hangar->SetRelativeScale3D(FVector(1.0f, 1.5f, 1.0f));
        Hangar->RegisterComponent();
    }
    else if (EntityId.Contains(TEXT("Refinery")))
    {
        // Container for ore
        UStaticMeshComponent* Container = NewObject<UStaticMeshComponent>(this);
        Container->SetStaticMesh(CubeMesh);
        Container->SetupAttachment(MeshComponent);
        Container->SetRelativeLocation(FVector(-30, 0, 30));
        Container->SetRelativeScale3D(FVector(0.8f, 1.2f, 0.6f));
        Container->RegisterComponent();
    }
    FString LowerId = EntityId.ToLower();
    // Substring matching on the content name is a trap here: "ore" is inside
    // building.sov.ore_refinery and unit.all.ore_harvester, so matching it swapped the
    // refinery's and the harvester's authored mesh for a cylinder -- and because the
    // scale had already been normalised against the blockout's bounds, the tiny scale
    // applied to a 100-unit cylinder collapsed both to radius 0 and they vanished.
    // Only an actual resource node is a heap of ore; it is identified by the caller.
    if (LowerId == TEXT("ore_resource_node"))
    {
        if (CylinderMesh)
        {
            MeshComponent->SetStaticMesh(CylinderMesh);
        }
        if (SkeletalMeshComponent)
        {
            SkeletalMeshComponent->SetVisibility(false);
        }
        return;
    }

    bool bIsInfantry = (LowerId.Contains(TEXT("rifleman")) || 
                        LowerId.Contains(TEXT("engineer")) || 
                        LowerId.Contains(TEXT("conscript")) || 
                        LowerId.Contains(TEXT("rubezh")) || 
                        LowerId.Contains(TEXT("zaslon")) || 
                        LowerId.Contains(TEXT("sentinel")) || 
                        LowerId.Contains(TEXT("lancer")) || 
                        LowerId.Contains(TEXT("soldier"))) &&
                        !LowerId.Contains(TEXT("ore")) &&
                        !LowerId.Contains(TEXT("resource")) &&
                        !LowerId.Contains(TEXT("building")) &&
                        !LowerId.Contains(TEXT("tank")) &&
                        !LowerId.Contains(TEXT("harvester"));

    // Query DataAsset ArtMapping for custom SkeletalMesh or StaticMesh overrides.
    // Art data keys are checked in two name spaces: the raw content id
    // (`unit.sov.conscript`) and the short bible key (`SU_Conscript`) so that either
    // authoring convention resolves without renaming every blockout asset.
    // ToRawPtr on the first arm: TObjectPtr and a raw pointer convert both ways, so
    // the ternary is ambiguous without pinning one side to a raw pointer.
    const URA4ArtMappingDataAsset* ArtData = ArtMapping
        ? ToRawPtr(ArtMapping)
        : LoadObject<URA4ArtMappingDataAsset>(nullptr, TEXT("/Game/RA4/Art/Generated/DA_RA4_ArtMappings.DA_RA4_ArtMappings"));

    auto ApplyUnitArt = [this](const FRA4UnitArtDefinition& UnitArt)
    {
        // Turret first: it is independent of whether the body ends up static or
        // skeletal, and an early return below would otherwise skip it.
        if (!UnitArt.TurretMesh.IsNull())
        {
            SetTurretMesh(UnitArt.TurretMesh.LoadSynchronous(), UnitArt.TurretMountHeight);
        }

        if (!UnitArt.IdleAnim.IsNull()) CachedIdleAnim = UnitArt.IdleAnim.LoadSynchronous();
        if (!UnitArt.RunAnim.IsNull()) CachedRunAnim = UnitArt.RunAnim.LoadSynchronous();
        if (!UnitArt.AttackAnim.IsNull()) CachedAttackAnim = UnitArt.AttackAnim.LoadSynchronous();

        if (!UnitArt.SkeletalMesh.IsNull() && SkeletalMeshComponent)
        {
            USkeletalMesh* SkelMesh = UnitArt.SkeletalMesh.LoadSynchronous();
            if (SkelMesh)
            {
                MeshComponent->SetVisibility(false, true);
                SkeletalMeshComponent->SetSkeletalMesh(SkelMesh);
                SkeletalMeshComponent->SetVisibility(true);
                SkeletalMeshComponent->SetRelativeLocation(UnitArt.MeshOffset);
                SkeletalMeshComponent->SetRelativeRotation(UnitArt.MeshRotation);
                SkeletalMeshComponent->SetWorldScale3D(UnitArt.MeshScale);
                return true;
            }
        }

        if (!UnitArt.StaticMesh.IsNull() && MeshComponent)
        {
            UStaticMesh* NewMesh = UnitArt.StaticMesh.LoadSynchronous();
            if (NewMesh)
            {
                MeshComponent->SetStaticMesh(NewMesh);
                MeshComponent->SetVisibility(true);
                if (bHasRequestedVisualScale)
                {
                    SetVisualScale(RequestedVisualScale);
                }
                return true;
            }
        }
        return false;
    };

    if (ArtData)
    {
        FRA4UnitArtDefinition UnitArt;
        if (ArtData->FindUnitArt(FName(*EntityId), UnitArt) && ApplyUnitArt(UnitArt))
        {
            return;
        }

        // unit.sov.conscript -> SU_Conscript style content id
        FString BibleId;
        if (EntityId.StartsWith(TEXT("unit.")) || EntityId.StartsWith(TEXT("building.")))
        {
            int32 DotCount = 0;
            for (int32 Index = 0; Index < EntityId.Len(); ++Index)
            {
                if (EntityId[Index] == TEXT('.')) { ++DotCount; }
            }
            if (DotCount == 2)
            {
                int32 FirstDot = EntityId.Find(TEXT("."));
                int32 SecondDot = EntityId.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromStart, FirstDot + 1);
                if (FirstDot != INDEX_NONE && SecondDot != INDEX_NONE && SecondDot > FirstDot)
                {
                    const FString Faction = EntityId.Mid(FirstDot + 1, SecondDot - FirstDot - 1);
                    const FString Name = EntityId.Mid(SecondDot + 1);
                    const FString Prefix = Faction.ToUpper();
                    // e.g. sov -> SU, all -> AL, coa -> CO, chr -> CH
                    const TCHAR* Short = TEXT("");
                    if (Prefix == TEXT("SOV")) Short = TEXT("SU");
                    else if (Prefix == TEXT("ALL")) Short = TEXT("AL");
                    else if (Prefix == TEXT("COA")) Short = TEXT("CO");
                    else if (Prefix == TEXT("CHR")) Short = TEXT("CH");
                    if (Short[0] != TEXT('\0'))
                    {
                        // CamelCase the snake_case name: heavy_tank -> HeavyTank
                        FString TitleName;
                        bool bUpperNext = true;
                        for (const TCHAR Ch : Name)
                        {
                            if (Ch == TEXT('_'))
                            {
                                bUpperNext = true;
                                continue;
                            }
                            TitleName += bUpperNext ? static_cast<TCHAR>(FChar::ToUpper(Ch)) : Ch;
                            bUpperNext = false;
                        }
                        BibleId = FString::Printf(TEXT("%s_%s"), Short, *TitleName);
                        if (ArtData->FindUnitArt(FName(*BibleId), UnitArt) && ApplyUnitArt(UnitArt))
                        {
                            return;
                        }

                        // The DataAsset was authored against older Bible keys, not the
                        // current content names. Map them explicitly so the lookup works
                        // without renaming every row in the DA.
                        static const TMap<FString, FString> KnownAliases = {
                            { TEXT("unit.sov.conscript"),       TEXT("SU_Conscript") },
                            { TEXT("unit.sov.ore_harvester"),   TEXT("SU_Harvester") },
                            { TEXT("unit.sov.heavy_tank"),      TEXT("SU_HammerTank") },
                            { TEXT("unit.sov.rocket_trooper"),  TEXT("SU_ShockTrooper") },
                            { TEXT("unit.sov.mcv"),             TEXT("SU_MCV") },
                            { TEXT("unit.all.rifleman"),        TEXT("AL_Peacekeeper") },
                            { TEXT("unit.all.ore_harvester"),   TEXT("AL_Prospector") },
                            { TEXT("unit.all.light_tank"),      TEXT("AL_GuardianTank") },
                            { TEXT("unit.all.missile_infantry"), TEXT("AL_Javelin") },
                            { TEXT("unit.all.mcv"),             TEXT("AL_MCV") },
                            { TEXT("building.sov.construction_yard"), TEXT("SU_ConYard") },
                            { TEXT("building.sov.tesla_reactor"),     TEXT("SU_PowerPlant") },
                            { TEXT("building.sov.ore_refinery"),      TEXT("SU_Refinery") },
                            { TEXT("building.sov.barracks"),          TEXT("SU_Barracks") },
                            { TEXT("building.sov.war_factory"),       TEXT("SU_WarFactory") },
                            { TEXT("building.sov.gun_turret"),        TEXT("SU_SentryTurret") },
                            { TEXT("building.all.construction_yard"), TEXT("AL_ConYard") },
                            { TEXT("building.all.power_plant"),       TEXT("AL_PowerPlant") },
                            { TEXT("building.all.ore_refinery"),      TEXT("AL_Refinery") },
                            { TEXT("building.all.barracks"),          TEXT("AL_Barracks") },
                            { TEXT("building.all.war_factory"),       TEXT("AL_WarFactory") },
                            { TEXT("building.all.pillbox"),           TEXT("AL_MultigunTurret") },
                        };
                        if (const FString* Alias = KnownAliases.Find(EntityId))
                        {
                            BibleId = *Alias;
                            if (ArtData->FindUnitArt(FName(*BibleId), UnitArt) && ApplyUnitArt(UnitArt))
                            {
                                return;
                            }
                        }
                    }
                }
            }
        }

        FRA4BuildingArtDefinition BuildingArt;
        if (ArtData->FindBuildingArt(FName(*EntityId), BuildingArt) || (!BibleId.IsEmpty() && ArtData->FindBuildingArt(FName(*BibleId), BuildingArt)))
        {
            TSoftObjectPtr<UStaticMesh> StageMesh = BuildingArt.Stage4_ActiveMesh;
            if (StageMesh.IsNull()) StageMesh = BuildingArt.Stage2_StructureMesh;
            if (StageMesh.IsNull()) StageMesh = BuildingArt.Stage1_FoundationMesh;
            if (StageMesh.IsNull()) StageMesh = BuildingArt.Stage0_DeliveryMesh;
            if (!StageMesh.IsNull() && MeshComponent)
            {
                UStaticMesh* NewMesh = StageMesh.LoadSynchronous();
                if (NewMesh)
                {
                    MeshComponent->SetStaticMesh(NewMesh);
                    MeshComponent->SetVisibility(true);
                    if (bHasRequestedVisualScale)
                    {
                        SetVisualScale(RequestedVisualScale);
                    }
                    return;
                }
            }
        }
    }

    if (bIsInfantry)
    {
        USkeletalMesh* QuantumMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/ThirdParty/QuantumCharacter/Mesh/SKM_QuantumCharacter.SKM_QuantumCharacter"));
        if (QuantumMesh && SkeletalMeshComponent)
        {
            MeshComponent->SetVisibility(false, true); // Hide cube and all child components
            SkeletalMeshComponent->SetSkeletalMesh(QuantumMesh);
            SkeletalMeshComponent->SetVisibility(true);
            SkeletalMeshComponent->SetRelativeLocation(FVector(0, 0, -90));
            SkeletalMeshComponent->SetRelativeRotation(FRotator(0, -90, 0));
            SkeletalMeshComponent->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
        }
    }
    else
    {
        if (SkeletalMeshComponent)
        {
            SkeletalMeshComponent->SetVisibility(false);
        }
    }
}
