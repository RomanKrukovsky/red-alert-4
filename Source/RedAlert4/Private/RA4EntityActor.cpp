// Copyright (c) Red Alert 4 project.

#include "RA4EntityActor.h"

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
    }
}

void ARA4EntityActor::SetTeamColor(const FLinearColor& TeamColor)
{
    if (MeshComponent == nullptr)
    {
        return;
    }
    UMaterialInstanceDynamic* DynMat = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
    if (DynMat == nullptr)
    {
        return;
    }
    // The project material exposes TeamColor; the engine placeholder exposes Color.
    // Setting an absent parameter is a no-op, so writing both keeps the placeholder
    // legible now without breaking the real material later.
    DynMat->SetVectorParameterValue(TEXT("TeamColor"), TeamColor);
    DynMat->SetVectorParameterValue(TEXT("Color"), TeamColor);
}

void ARA4EntityActor::SetVisualScale(const FVector& Scale)
{
    if (MeshComponent != nullptr)
    {
        // The placeholder cube is 100 units across, so scale is the desired footprint
        // in metres. Replaced wholesale once real meshes exist.
        MeshComponent->SetWorldScale3D(Scale);
        // The cube's pivot is its centre, so it needs lifting by half its height or
        // it sits waist-deep in the ground. The mesh component is the *root*, so a
        // relative offset here would move the whole actor and then be overwritten by
        // the next position update. The lift is carried separately and applied when
        // the simulation position is consumed.
        VisualZOffset = Scale.Z * 50.0f;
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
    
    // Simple interpolation for smooth presentation (lerp rate ~ 10.0f to smooth 20Hz tick)
    FVector CurrentLocation = GetActorLocation();
    FVector InterpolatedLocation = FMath::VInterpTo(CurrentLocation, TargetPosition, DeltaTime, 15.0f);
    
    FRotator CurrentRotation = GetActorRotation();
    FRotator TargetRotator(0.0f, TargetRotationZ, 0.0f);
    FRotator InterpolatedRotation = FMath::RInterpTo(CurrentRotation, TargetRotator, DeltaTime, 15.0f);
    
    SetActorLocationAndRotation(InterpolatedLocation, InterpolatedRotation);
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
