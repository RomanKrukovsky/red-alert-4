// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "RA4ViewModelBase.generated.h"

/**
 * Base View Model for Red Alert 4 UI.
 * Acts as the bridge between the headless simulation core/Unreal presentation layer and the CommonUI widgets.
 */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4ViewModelBase : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    URA4ViewModelBase();
};
