// Copyright (c) Red Alert 4 project. C++ AnimInstance implementation.

#include "RA4InfantryAnimInstance.h"
#include "GameFramework/Actor.h"

URA4InfantryAnimInstance::URA4InfantryAnimInstance()
    : GroundSpeed(0.0f)
    , Direction(0.0f)
    , bIsMoving(false)
    , bIsAttacking(false)
    , bIsDead(false)
{
}

void URA4InfantryAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    AActor* OwningActor = GetOwningActor();
    if (OwningActor)
    {
        const FVector Velocity = OwningActor->GetVelocity();
        GroundSpeed = Velocity.Size2D();
        bIsMoving = GroundSpeed > 10.0f;
    }
}
