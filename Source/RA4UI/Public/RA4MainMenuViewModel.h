// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4ViewModelBase.h"
#include "RA4MainMenuViewModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRA4MainMenuActionDelegate);

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

private:
    UPROPERTY(FieldNotify, Setter, Getter)
    FText PlayerProfileName;
};
