// Copyright (c) Red Alert 4 project.

#include "RA4EntityActor.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ARA4EntityActor::ARA4EntityActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    SetRootComponent(MeshComponent);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Collision is handled by simulation core

    SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
    SkeletalMeshComponent->SetupAttachment(MeshComponent);
    SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SkeletalMeshComponent->SetVisibility(false);

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

void ARA4EntityActor::SetEntityMesh(UStaticMesh* InMesh)
{
    if (MeshComponent)
    {
        MeshComponent->SetStaticMesh(InMesh);
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

    // Phase 0: Dynamically select faction material instances if they exist
    UMaterialInterface* FactionMat = nullptr;
    if (TeamColor.R > 0.5f && TeamColor.B < 0.2f)
    {
        FactionMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RA4/Presentation/Materials/Blockout/MI_RA4_Blockout_SU.MI_RA4_Blockout_SU"));
    }
    else if (TeamColor.B > 0.5f && TeamColor.R < 0.2f)
    {
        FactionMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RA4/Presentation/Materials/Blockout/MI_RA4_Blockout_AL.MI_RA4_Blockout_AL"));
    }

    if (FactionMat)
    {
        MeshComponent->SetMaterial(0, FactionMat);
    }

    UMaterialInstanceDynamic* DynMat = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
    if (DynMat == nullptr)
    {
        return;
    }
    
    // Set the TeamColor for the blockout structure
    DynMat->SetVectorParameterValue(TEXT("TeamColor"), TeamColor);
    DynMat->SetVectorParameterValue(TEXT("Color"), TeamColor);
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

    // The visual rests on the ground even when the imported source dimensions
    // differ from the engine cube.
    VisualZOffset = DesiredSize.Z * 0.5f;
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
    
    // Simple interpolation for smooth presentation (lerp rate ~ 10.0f to smooth 20Hz tick)
    FVector CurrentLocation = GetActorLocation();
    FVector InterpolatedLocation = FMath::VInterpTo(CurrentLocation, TargetPosition, DeltaTime, 15.0f);
    
    FRotator CurrentRotation = GetActorRotation();
    FRotator TargetRotator(0.0f, TargetRotationZ, 0.0f);
    FRotator InterpolatedRotation = FMath::RInterpTo(CurrentRotation, TargetRotator, DeltaTime, 15.0f);
    
    SetActorLocationAndRotation(InterpolatedLocation, InterpolatedRotation);

    // Phase 2: Infantry Locomotion Animation
    if (SkeletalMeshComponent && SkeletalMeshComponent->IsVisible())
    {
        float Speed = (InterpolatedLocation - CurrentLocation).Size() / FMath::Max(DeltaTime, 0.001f);
        bool bIsMoving = Speed > 20.0f;
        
        static UAnimSequence* IdleAnim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/ThirdParty/QuantumCharacter/Demo/Animations/A_MM_Idle.A_MM_Idle"));
        static UAnimSequence* RunAnim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/ThirdParty/QuantumCharacter/Demo/Animations/A_MM_Run_Fwd.A_MM_Run_Fwd"));
        
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

void ARA4EntityActor::ApplyPrimitiveComposition(const FString& EntityId)
{
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
