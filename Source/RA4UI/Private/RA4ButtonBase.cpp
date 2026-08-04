// Copyright (c) Red Alert 4 project.

#include "RA4ButtonBase.h"

URA4ButtonBase::URA4ButtonBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URA4ButtonBase::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void URA4ButtonBase::NativeOnHovered()
{
    Super::NativeOnHovered();
    AnimateToScale(HoverScale);
}

void URA4ButtonBase::NativeOnUnhovered()
{
    Super::NativeOnUnhovered();
    AnimateToScale(1.0f);
}

void URA4ButtonBase::NativeOnPressed()
{
    Super::NativeOnPressed();
    AnimateToScale(PressScale);
}

void URA4ButtonBase::NativeOnReleased()
{
    Super::NativeOnReleased();
    AnimateToScale(IsHovered() ? HoverScale : 1.0f);
}

void URA4ButtonBase::AnimateToScale(float TargetScale)
{
    ScaleStart = CurrentScale;
    ScaleTarget = TargetScale;
    AnimElapsed = 0.0f;
    bAnimating = true;

    if (!AnimTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().SetTimer(
            AnimTimerHandle, this, &URA4ButtonBase::TickAnimation,
            0.016f, true);
    }
}

void URA4ButtonBase::TickAnimation()
{
    if (!bAnimating)
    {
        GetWorld()->GetTimerManager().ClearTimer(AnimTimerHandle);
        AnimTimerHandle.Invalidate();
        return;
    }

    AnimElapsed += 0.016f;
    const float Alpha = FMath::Clamp(AnimElapsed / FMath::Max(AnimDuration, 0.01f), 0.0f, 1.0f);

    // Ease-out cubic for snappy feel
    const float Eased = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);
    CurrentScale = FMath::Lerp(ScaleStart, ScaleTarget, Eased);

    // Apply via Render Transform on the underlying Slate widget
    if (GetCachedWidget().IsValid())
    {
        GetCachedWidget()->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
        GetCachedWidget()->SetRenderTransform(FSlateRenderTransform(FScale2D(CurrentScale)));
    }

    if (Alpha >= 1.0f)
    {
        CurrentScale = ScaleTarget;
        bAnimating = false;
        GetWorld()->GetTimerManager().ClearTimer(AnimTimerHandle);
        AnimTimerHandle.Invalidate();
    }
}
