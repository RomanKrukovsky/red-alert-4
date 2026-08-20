// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "RA4UIScreenContract.h"
#include "RA4UIScreenViewModel.h"
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

    UFUNCTION(BlueprintCallable, Category = "RA4 UI|State")
    void SetScreenIdentity(
        ERA4UIScreenId InScreenId,
        ERA4UIScreenVariant InVariant = ERA4UIScreenVariant::Default);

    UFUNCTION(BlueprintPure, Category = "RA4 UI|State")
    FRA4UIScreenContract GetScreenContract() const;

    UFUNCTION(BlueprintPure, Category = "RA4 UI|State")
    URA4UIScreenViewModel* GetScreenViewModel() const;

    UFUNCTION(BlueprintCallable, Category = "RA4 UI|Navigation")
    void NavigateToScreen(ERA4UIScreenId TargetScreen);

    UFUNCTION(BlueprintCallable, Category = "RA4 UI|Navigation")
    bool NavigateBack();

    UFUNCTION(BlueprintCallable, Category = "RA4 UI|Modal")
    void CloseActiveModal();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RA4 UI|State")
    ERA4UIScreenId ScreenId = ERA4UIScreenId::Splash;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RA4 UI|State")
    ERA4UIScreenVariant ScreenVariant = ERA4UIScreenVariant::Default;
};
