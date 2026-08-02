// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4HUDViewModel.h"
#include "RA4NoesisHUDViewModel.generated.h"

/**
 * Specialized ViewModel for NoesisGUI HUD bindings.
 * Derives from URA4HUDViewModel to share identical state, FieldNotifies,
 * and presentation event hooks with native Unreal MVVM / Slate / Noesis components.
 */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4NoesisHUDViewModel : public URA4HUDViewModel
{
    GENERATED_BODY()

public:
    URA4NoesisHUDViewModel();
};
