// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "RA4UITheme.generated.h"

class UCommonButtonStyle;

UENUM(BlueprintType)
enum class ERA4FactionTheme : uint8
{
    EurasianPact = 0 UMETA(DisplayName = "Евразийский пакт"),
    AtlanticAlliance = 1 UMETA(DisplayName = "Атлантический альянс"),
    EasternCoalition = 2 UMETA(DisplayName = "Восточная коалиция"),
    PacificPact = 3 UMETA(DisplayName = "Тихоокеанский пакт"),
    Independent = 4 UMETA(DisplayName = "Независимые державы"),
    Chronolegion = 5 UMETA(DisplayName = "Хронолегион (Legacy)"),

    // Legacy aliases
    USSR = EurasianPact,
    Allies = AtlanticAlliance
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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Visuals")
    TSoftObjectPtr<UTexture2D> MenuBackground;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Visuals")
    TSoftObjectPtr<UTexture2D> CommanderPortrait;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Visuals")
    TSoftObjectPtr<UTexture2D> FactionIcon;

    /** Stretchable 9-slice panel supplied by the active visual kit. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Styles")
    FSlateBrush PanelBrush;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Styles")
    TSoftClassPtr<UCommonButtonStyle> ButtonStyle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Styles")
    FTextBlockStyle TextStyle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Visuals")
    TSoftObjectPtr<UMaterialInterface> PanelMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Visuals")
    TSoftObjectPtr<UMaterialInterface> FrameMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Visuals")
    TSoftObjectPtr<UMaterialInterface> GlowMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Audio")
    TSoftObjectPtr<USoundBase> ClickSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Audio")
    TSoftObjectPtr<USoundBase> HoverSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Animation")
    TSoftObjectPtr<UCurveFloat> TransitionCurve;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Animation", meta = (ClampMin = "0.0"))
    float TransitionDuration = 0.28f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Layout", meta = (ClampMin = "1.0"))
    float FrameStroke = 2.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Layout", meta = (ClampMin = "0.0"))
    float GlowStrength = 1.0f;
};
