// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RA4UIScreenViewModel.h"
#include "RA4UIRouterSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRA4ScreenChanged, ERA4UIScreenId, Screen);

/**
 * Owns menu history and presentation state. It deliberately has no dependency
 * on RA4Simulation: presentation adapters push data into the ViewModel.
 */
UCLASS()
class RA4UI_API URA4UIRouterSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintPure, Category = "RA4 UI|Navigation")
    URA4UIScreenViewModel* GetScreenViewModel() const;

    UFUNCTION(BlueprintCallable, Category = "RA4 UI|Navigation")
    void NavigateTo(ERA4UIScreenId TargetScreen, bool bAddToHistory = true);

    UFUNCTION(BlueprintCallable, Category = "RA4 UI|Navigation")
    bool NavigateBack();

    UFUNCTION(BlueprintCallable, Category = "RA4 UI|Modal")
    void ShowModal(const FText& Title, const FText& Body);

    UFUNCTION(BlueprintCallable, Category = "RA4 UI|Modal")
    void CloseModal();

    UPROPERTY(BlueprintAssignable, Category = "RA4 UI|Navigation")
    FRA4ScreenChanged OnScreenChanged;

private:
    UPROPERTY(Transient)
    TObjectPtr<URA4UIScreenViewModel> ScreenViewModel;

    TArray<ERA4UIScreenId> NavigationHistory;
};
