// Copyright (c) Red Alert 4 project.

#include "RA4EntityActor.h"

ARA4EntityActor::ARA4EntityActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    
    TargetPosition = FVector::ZeroVector;
    TargetRotationZ = 0.0f;
}

void ARA4EntityActor::BeginPlay()
{
    Super::BeginPlay();
    TargetPosition = GetActorLocation();
    TargetRotationZ = GetActorRotation().Yaw;
}

void ARA4EntityActor::UpdateFromSimulation(const FVector& NewPosition, float NewRotationZ, bool bTeleport)
{
    TargetPosition = NewPosition;
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
