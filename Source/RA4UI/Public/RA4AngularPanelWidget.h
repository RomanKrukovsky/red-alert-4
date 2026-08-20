// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Components/Border.h"
#include "RA4AngularPanelWidget.generated.h"

class URA4UITheme;

UENUM(BlueprintType)
enum class ERA4PanelRole : uint8
{
    Compact,
    Standard,
    DenseHUD,
    Hero
};

/** Single-child themed panel shared across menus and the in-game HUD. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4AngularPanelWidget : public UBorder
{
    GENERATED_BODY()

public:
    URA4AngularPanelWidget(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Theme")
    void SetTheme(const URA4UITheme* Theme);

    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Layout")
    void SetPanelRole(ERA4PanelRole Role);

    UFUNCTION(BlueprintPure, Category = "RA4|UI|Layout")
    ERA4PanelRole GetPanelRole() const { return PanelRole; }

private:
    UPROPERTY(EditAnywhere, Category = "RA4|UI|Layout")
    ERA4PanelRole PanelRole = ERA4PanelRole::Standard;
};
