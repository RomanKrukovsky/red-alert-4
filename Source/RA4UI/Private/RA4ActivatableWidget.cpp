// Copyright (c) Red Alert 4 project.

#include "RA4ActivatableWidget.h"

URA4ActivatableWidget::URA4ActivatableWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // By default, UI doesn't pause the game and allows routing
}

TOptional<FUIInputConfig> URA4ActivatableWidget::GetDesiredInputConfig() const
{
    // Default config for UI elements: Game and UI, Mouse visible
    return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}
