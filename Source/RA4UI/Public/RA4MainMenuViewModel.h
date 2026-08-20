// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4UIScreenViewModel.h"
#include "RA4ViewModelBase.h"
#include "RA4MainMenuViewModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRA4MainMenuActionDelegate);

UENUM(BlueprintType)
enum class ERA4MainMenuAction : uint8
{
    Campaign,
    Multiplayer,
    Skirmish,
    Editor,
    Encyclopedia,
    Modifications,
    Settings,
    Exit
};

USTRUCT(BlueprintType)
struct FRA4MainMenuEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
    FText Label;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
    ERA4UIScreenId TargetScreen = ERA4UIScreenId::MainMenu;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
    ERA4MainMenuAction Action = ERA4MainMenuAction::Campaign;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
    bool bSelected = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FRA4MainMenuActionRequested,
    ERA4MainMenuAction,
    Action);

UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4MainMenuViewModel : public URA4ViewModelBase
{
    GENERATED_BODY()

public:
    URA4MainMenuViewModel();

    // Field-notifying properties for MVVM data binding
    UFUNCTION(BlueprintCallable, Category = "ViewModel|Profile")
    void SetPlayerProfileName(const FText& InName);
    
    UFUNCTION(BlueprintPure, Category = "ViewModel|Profile")
    FText GetPlayerProfileName() const;

    UFUNCTION(BlueprintPure, Category = "ViewModel|Menu")
    const TArray<FRA4MainMenuEntry>& GetMenuEntries() const;

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Menu")
    void SetSelectedMenuIndex(int32 InIndex);

    UFUNCTION(BlueprintPure, Category = "ViewModel|Menu")
    int32 GetSelectedMenuIndex() const;

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Commands")
    void ExecuteAction(ERA4MainMenuAction Action);

    // --- Main Menu Commands / Actions ---
    UFUNCTION(BlueprintCallable, Category = "ViewModel|Commands")
    void ExecuteNewSkirmishCommand();

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Commands")
    void ExecuteSettingsCommand();

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Commands")
    void ExecuteExitCommand();

    UPROPERTY(BlueprintAssignable, Category = "ViewModel|Events")
    FRA4MainMenuActionDelegate OnNewSkirmishClicked;

    UPROPERTY(BlueprintAssignable, Category = "ViewModel|Events")
    FRA4MainMenuActionDelegate OnSettingsClicked;

    UPROPERTY(BlueprintAssignable, Category = "ViewModel|Events")
    FRA4MainMenuActionDelegate OnExitClicked;

    UPROPERTY(BlueprintAssignable, Category = "ViewModel|Events")
    FRA4MainMenuActionRequested OnActionRequested;

private:
    UPROPERTY(FieldNotify, Setter, Getter)
    FText PlayerProfileName;

    UPROPERTY(FieldNotify, Getter)
    TArray<FRA4MainMenuEntry> MenuEntries;

    UPROPERTY(FieldNotify, Setter, Getter)
    int32 SelectedMenuIndex = 0;
};
