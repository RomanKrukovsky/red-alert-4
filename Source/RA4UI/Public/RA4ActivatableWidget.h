// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "RA4ActivatableWidget.generated.h"

/**
 * Base class for all full-screen or modal UI menus in Red Alert 4.
 * Integrates natively with CommonUI routing and input handling.
 */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4ActivatableWidget : public UCommonActivatableWidget
{
    GENERATED_BODY()

public:
    URA4ActivatableWidget(const FObjectInitializer& ObjectInitializer);

    // Determines if this widget consumes input entirely or allows pass-through
    virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
};
