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

protected:
    virtual void NativePreConstruct() override;
};
