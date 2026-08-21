// Copyright (c) Red Alert 4 project.

#include "RA4EntityActor.h"
#include "RA4Presentation/RA4ArtMapping.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ARA4EntityActor::ARA4EntityActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    bFindCameraComponentWhenViewTarget = true;
    
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    SetRootComponent(MeshComponent);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Collision is handled by simulation core

    DirectControlSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("DirectControlSpringArm"));
    DirectControlSpringArm->SetupAttachment(MeshComponent);
    DirectControlSpringArm->TargetArmLength = 360.0f;
    DirectControlSpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
    DirectControlSpringArm->SetRelativeRotation(FRotator(-10.0f, 0.0f, 0.0f));
    DirectControlSpringArm->bDoCollisionTest = false;
    DirectControlSpringArm->bEnableCameraLag = false;
    DirectControlSpringArm->bEnableCameraRotationLag = false;
    DirectControlSpringArm->bInheritPitch = true;
    DirectControlSpringArm->bInheritYaw = true;
    DirectControlSpringArm->bInheritRoll = false;
    // Over-the-shoulder offset: +Y is right in UE -> shifts camera to the right and puts object on LEFT of screen
    DirectControlSpringArm->SocketOffset = FVector(0.0f, 90.0f, 45.0f);
    DirectControlSpringArm->TargetOffset = FVector(0.0f, 0.0f, 35.0f);

    FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCameraComponent"));
    FirstPersonCameraComponent->SetupAttachment(DirectControlSpringArm, USpringArmComponent::SocketName);
    FirstPersonCameraComponent->bUsePawnControlRotation = false;

    SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
    SkeletalMeshComponent->SetupAttachment(MeshComponent);
    SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SkeletalMeshComponent->SetVisibility(false);
    SkeletalMeshComponent->SetAbsolute(false, false, true);

    FoundationBibComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FoundationBibComponent"));
    FoundationBibComponent->SetupAttachment(MeshComponent);
    FoundationBibComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FoundationBibComponent->SetCastShadow(true);
    FoundationBibComponent->SetVisibility(false);

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
        FoundationBibComponent->SetStaticMesh(CubeMesh.Object);
    }
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ConcreteFloorMat(
        TEXT("/Game/ThirdParty/CityPark/Materials/Buildings/MI_Floor01.MI_Floor01"));
    if (ConcreteFloorMat.Succeeded())
    {
        FoundationBibComponent->SetMaterial(0, ConcreteFloorMat.Object);
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

FVector ARA4EntityActor::GetPossessionCameraLocation() const
{
    if (FirstPersonCameraComponent)
    {
        return FirstPersonCameraComponent->GetComponentLocation();
    }
    return GetActorLocation() + FVector(-250.0f, 90.0f, 150.0f);
}

FRotator ARA4EntityActor::GetPossessionCameraRotation() const
{
    if (FirstPersonCameraComponent)
    {
        return FirstPersonCameraComponent->GetComponentRotation();
    }
    return GetActorRotation();
}

void ARA4EntityActor::SetupDirectControlView(bool bEnable, float InFov)
{
    if (FirstPersonCameraComponent != nullptr)
    {
        FirstPersonCameraComponent->SetFieldOfView(InFov > 0.0f ? InFov : 90.0f);
    }
    if (DirectControlSpringArm != nullptr)
    {
        const float ScaleMult = FMath::Clamp(float(RequestedVisualScale.GetMax()), 0.8f, 3.0f);
        DirectControlSpringArm->TargetArmLength = 340.0f * ScaleMult;
        DirectControlSpringArm->SocketOffset = FVector(0.0f, 90.0f * ScaleMult, 45.0f * ScaleMult);
    }
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

    // Authored production meshes carry their own multi-slot PBR materials.
    // Do not overwrite them with prototype blockout textures.
    if (const UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh())
    {
        const FString MeshPath = StaticMesh->GetPathName();
        if (MeshPath.Contains(TEXT("/RA4/Art/Units/")) ||
            MeshPath.Contains(TEXT("/RA4/Art/Buildings/")) ||
            MeshPath.Contains(TEXT("/ThirdParty/")))
        {
            return;
        }
    }

    // Clean, high-grade military painted metal PBR (No rust, no grunge)
    UMaterialInterface* FactionMat = nullptr;
    if (TeamColor.R > 0.5f && TeamColor.B < 0.2f)
    {
        FactionMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RA4/Art/Materials/MI_RA4_Surface_Soviet.MI_RA4_Surface_Soviet"));
    }
    else if (TeamColor.B > 0.5f && TeamColor.R < 0.2f)
    {
        FactionMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RA4/Art/Materials/MI_RA4_Surface_Alliance.MI_RA4_Surface_Alliance"));
    }
    else if (TeamColor.G > 0.5f && TeamColor.R < 0.4f)
    {
        FactionMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RA4/Art/Materials/MI_RA4_Surface_Coalition.MI_RA4_Surface_Coalition"));
    }
    else
    {
        FactionMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RA4/Art/Materials/MI_RA4_Surface_Dark.MI_RA4_Surface_Dark"));
    }

    if (FactionMat == nullptr)
    {
        FactionMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
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
                DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), TeamColor);
                DynamicMaterial->SetScalarParameterValue(TEXT("Metallic"), 0.15f);
                DynamicMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.40f);
                DynamicMaterial->SetScalarParameterValue(TEXT("Specular"), 0.50f);
                DynamicMaterial->SetScalarParameterValue(TEXT("RustAmount"), 0.0f);
                DynamicMaterial->SetScalarParameterValue(TEXT("Grunge"), 0.0f);
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

    // Ensure all unit and building meshes sit flush on the ground surface without dipping below or floating
    if (Mesh != nullptr)
    {
        const FBoxSphereBounds Bounds = Mesh->GetBounds();
        const float MeshMinZ = (Bounds.Origin.Z - Bounds.BoxExtent.Z) * NormalizedScale.Z;
        VisualZOffset = -MeshMinZ;
        if (VisualZOffset < 0.0f)
        {
            VisualZOffset = 0.0f;
        }
    }
    else
    {
        VisualZOffset = 0.0f;
    }

    // Dynamically place first-person camera above the vehicle turret/hull to prevent mesh clipping
    if (FirstPersonCameraComponent != nullptr)
    {
        FirstPersonCameraComponent->SetRelativeLocation(
            FVector(DesiredSize.X * 0.2f, 0.0f, DesiredSize.Z * 0.85f + 15.0f));
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
    TargetPosition = NewPosition + FVector(0.0, 0.0, VisualZOffset);
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
    
    // Simple interpolation for smooth presentation (lerp rate ~ 15.0f to smooth 20Hz tick)
    FVector CurrentLocation = GetActorLocation();
    FVector InterpolatedLocation = FMath::VInterpTo(CurrentLocation, TargetPosition, DeltaTime, 15.0f);
    
    FRotator CurrentRotation = GetActorRotation();
    FRotator TargetRotator(0.0f, TargetRotationZ, 0.0f);
    FRotator InterpolatedRotation = FMath::RInterpTo(CurrentRotation, TargetRotator, DeltaTime, 15.0f);

    // Vehicle Dynamics: pitch dip on acceleration/brake, roll lean on turn
    const FVector Velocity = (InterpolatedLocation - CurrentLocation) / FMath::Max(DeltaTime, 0.001f);
    const float Speed = Velocity.Size();

    if (MeshComponent != nullptr && Speed > 10.0f && GetWorld() != nullptr)
    {
        const FVector Forward = InterpolatedRotation.Vector();
        const FVector Right = FRotationMatrix(InterpolatedRotation).GetUnitAxis(EAxis::Y);

        const float ForwardSpeed = FVector::DotProduct(Velocity, Forward);
        const float LateralSpeed = FVector::DotProduct(Velocity, Right);

        const float PitchTilt = FMath::Clamp(ForwardSpeed * 0.004f, -5.0f, 5.0f);
        const float RollTilt = FMath::Clamp(LateralSpeed * 0.005f, -4.0f, 4.0f);

        FRotator DynamicRotator = InterpolatedRotation;
        DynamicRotator.Pitch += PitchTilt;
        DynamicRotator.Roll += RollTilt;

        SetActorLocationAndRotation(InterpolatedLocation, DynamicRotator);
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

    const bool bIsBuilding = InEntityId.Contains(TEXT("building"), ESearchCase::IgnoreCase) ||
                             InEntityId.Contains(TEXT("structure"), ESearchCase::IgnoreCase) ||
                             InEntityId.Contains(TEXT("conyard"), ESearchCase::IgnoreCase) ||
                             InEntityId.Contains(TEXT("headquarters"), ESearchCase::IgnoreCase) ||
                             InEntityId.Contains(TEXT("barracks"), ESearchCase::IgnoreCase) ||
                             InEntityId.Contains(TEXT("factory"), ESearchCase::IgnoreCase) ||
                             InEntityId.Contains(TEXT("power"), ESearchCase::IgnoreCase) ||
                             InEntityId.Contains(TEXT("refinery"), ESearchCase::IgnoreCase) ||
                             InEntityId.Contains(TEXT("radar"), ESearchCase::IgnoreCase) ||
                             InEntityId.Contains(TEXT("turret"), ESearchCase::IgnoreCase) ||
                             InEntityId.Contains(TEXT("techcenter"), ESearchCase::IgnoreCase) ||
                             InEntityId.Contains(TEXT("silo"), ESearchCase::IgnoreCase) ||
                             InEntityId.Contains(TEXT("superweapon"), ESearchCase::IgnoreCase);

    if (IsValid(FoundationBibComponent))
    {
        if (bIsBuilding)
        {
            FoundationBibComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -0.45f));
            FoundationBibComponent->SetRelativeScale3D(FVector(1.25f, 1.25f, 0.08f));
            FoundationBibComponent->SetVisibility(true);
        }
        else
        {
            FoundationBibComponent->SetVisibility(false);
        }
    }
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

                        // Explicit Bible ID alias mappings for clean-room asset registry resolution.
                        static const TMap<FString, FString> KnownAliases = {
                            { TEXT("unit.sov.conscript"),             TEXT("SU_RubezhRifleman") },
                            { TEXT("unit.sov.ore_harvester"),         TEXT("SU_BogatyrOreCarrier") },
                            { TEXT("unit.sov.heavy_tank"),            TEXT("SU_GranitMBT") },
                            { TEXT("unit.sov.rocket_trooper"),        TEXT("SU_GrozaRocketeer") },
                            { TEXT("unit.sov.mcv"),                   TEXT("SU_MCV") },
                            { TEXT("unit.all.rifleman"),              TEXT("AL_VanguardRifleman") },
                            { TEXT("unit.all.ore_harvester"),         TEXT("AL_MinerCarrier") },
                            { TEXT("unit.all.light_tank"),            TEXT("AL_PaladinTank") },
                            { TEXT("unit.all.missile_infantry"),      TEXT("AL_StrikerInfantry") },
                            { TEXT("unit.all.mcv"),                   TEXT("AL_MCV") },
                            { TEXT("building.sov.construction_yard"), TEXT("SU_ConYard") },
                            { TEXT("building.sov.tesla_reactor"),     TEXT("SU_PowerPlant") },
                            { TEXT("building.sov.ore_refinery"),      TEXT("SU_Refinery") },
                            { TEXT("building.sov.barracks"),          TEXT("SU_Barracks") },
                            { TEXT("building.sov.war_factory"),       TEXT("SU_HeavyFactory") },
                            { TEXT("building.sov.gun_turret"),        TEXT("SU_Pillbox") },
                            { TEXT("building.all.construction_yard"), TEXT("AL_ConYard") },
                            { TEXT("building.all.power_plant"),       TEXT("AL_PowerPlant") },
                            { TEXT("building.all.ore_refinery"),      TEXT("AL_Refinery") },
                            { TEXT("building.all.barracks"),          TEXT("AL_Barracks") },
                            { TEXT("building.all.war_factory"),       TEXT("AL_WarFactory") },
                            { TEXT("building.all.pillbox"),           TEXT("AL_Pillbox") },
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
            SkeletalMeshComponent->SetRelativeLocation(FVector(0, 0, 0));
            SkeletalMeshComponent->SetRelativeRotation(FRotator(0, -90, 0));
            SkeletalMeshComponent->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f));
            VisualZOffset = 0.0f;
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
