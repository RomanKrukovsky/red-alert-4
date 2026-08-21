// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RA4UIScreenViewModel.h"
#include "RA4UIScreenData.generated.h"

class UCommonActivatableWidget;
class UTexture2D;

/**
 * One visual screen entry. Instances are authored in UE as DA_RA4_* assets and
 * contain only presentation references; no actor or simulation state belongs here.
 */
UCLASS(BlueprintType, Const)
class RA4UI_API URA4UIScreenData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen")
    ERA4UIScreenId ScreenId = ERA4UIScreenId::Splash;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen")
    ERA4FactionTheme Theme = ERA4FactionTheme::EurasianPact;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen")
    TSoftClassPtr<UCommonActivatableWidget> WidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen")
    TSoftObjectPtr<UTexture2D> BackgroundArt;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen")
    FText AccessibleTitle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen")
    bool bUsesSafeZone = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Screen", meta = (ClampMin = "0.1", ClampMax = "4.0"))
    float ReferenceAspectRatio = 1.777778f;
};
