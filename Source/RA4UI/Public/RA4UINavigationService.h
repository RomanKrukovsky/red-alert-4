// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RA4UIInputRouter.h"
#include "RA4UIScreenViewModel.h"
#include "RA4UINavigationService.generated.h"

class URA4UIRouterSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRA4OnNavigationStateChanged, ERA4UIScreenId, CurrentScreen, ERA4UIInputMode, InputMode);

/**
 * Service managing higher-level UI navigation state, modal popups, and automatic
 * switching of UI/Game input modes via URA4UIInputRouter.
 */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4UINavigationService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Navigates to a specific screen ID and automatically updates input routing modes. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Navigation")
    void NavigateToScreen(ERA4UIScreenId TargetScreen, bool bAddToHistory = true);

    /** Pops back one screen in history. Returns true if navigation occurred. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Navigation")
    bool NavigateBack();

    /** Returns currently active screen ID. */
    UFUNCTION(BlueprintPure, Category = "RA4|UI|Navigation")
    ERA4UIScreenId GetActiveScreen() const;

    /** Displays a modal notification/confirmation dialog over current UI. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Navigation")
    void ShowModal(const FText& Title, const FText& Body);

    /** Closes active modal dialog. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Navigation")
    void CloseModal();

    /** Derives and applies appropriate ERA4UIInputMode for the specified screen. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Navigation")
    void ApplyInputModeForScreen(ERA4UIScreenId Screen);

    UPROPERTY(BlueprintAssignable, Category = "RA4|UI|Navigation")
    FRA4OnNavigationStateChanged OnNavigationStateChanged;

private:
    URA4UIRouterSubsystem* GetRouterSubsystem() const;
    URA4UIInputRouter* GetInputRouter() const;
};
