// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RA4UITheme.generated.h"

UENUM(BlueprintType)
enum class ERA4FactionTheme : uint8
{
    USSR = 0,
    Allies,
    EasternCoalition,
    Chronolegion
};

/**
 * Data asset holding colors and style parameters for a specific faction theme.
 */
UCLASS(BlueprintType, Const)
class RA4UI_API URA4UITheme : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme")
    ERA4FactionTheme Faction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Colors")
    FLinearColor PrimaryColor;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Colors")
    FLinearColor SecondaryColor;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Colors")
    FLinearColor BackgroundColor;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Colors")
    FLinearColor TextColor;
};
