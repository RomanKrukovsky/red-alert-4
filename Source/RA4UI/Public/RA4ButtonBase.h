// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "RA4ButtonBase.generated.h"

/**
 * Base button class for all interactive Red Alert 4 UI buttons.
 * Supports styling through CommonUI.
 */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4ButtonBase : public UCommonButtonBase
{
    GENERATED_BODY()

public:
    URA4ButtonBase(const FObjectInitializer& ObjectInitializer);

    /** Scale multiplier applied on hover. 1.0 = no change. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RA4|Animation", meta = (ClampMin = "0.8", ClampMax = "1.5"))
    float HoverScale = 1.06f;

    /** Scale multiplier applied on press. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RA4|Animation", meta = (ClampMin = "0.5", ClampMax = "1.2"))
    float PressScale = 0.95f;

    /** Duration of the scale transition in seconds. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RA4|Animation", meta = (ClampMin = "0.01", ClampMax = "0.5"))
    float AnimDuration = 0.12f;

protected:
    virtual void NativePreConstruct() override;
    virtual void NativeOnHovered() override;
    virtual void NativeOnUnhovered() override;
    virtual void NativeOnPressed() override;
    virtual void NativeOnReleased() override;

private:
    /** Applies a target render transform scale with an interpolation curve. */
    void AnimateToScale(float TargetScale);

    /** Tick callback for smooth interpolation. */
    FTimerHandle AnimTimerHandle;
    float CurrentScale = 1.0f;
    float ScaleStart = 1.0f;
    float ScaleTarget = 1.0f;
    float AnimElapsed = 0.0f;
    bool bAnimating = false;

    void TickAnimation();
};
