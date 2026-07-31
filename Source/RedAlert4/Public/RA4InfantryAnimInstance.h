// Copyright (c) Red Alert 4 project. C++ AnimInstance for RTS infantry locomotion.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "RA4InfantryAnimInstance.generated.h"

UCLASS(BlueprintType, Blueprintable)
class REDALERT4_API URA4InfantryAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    URA4InfantryAnimInstance();

    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
    float GroundSpeed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
    float Direction = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion")
    bool bIsMoving = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    bool bIsAttacking = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    bool bIsDead = false;
};
