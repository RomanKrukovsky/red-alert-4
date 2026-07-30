// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4ViewModelBase.h"
#include "RA4HUDViewModel.generated.h"

UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4HUDViewModel : public URA4ViewModelBase
{
    GENERATED_BODY()

public:
    URA4HUDViewModel();

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Economy")
    void SetCredits(int32 InCredits);
    
    UFUNCTION(BlueprintPure, Category = "ViewModel|Economy")
    int32 GetCredits() const;

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Economy")
    void SetPower(int32 InPowerProvided, int32 InPowerDrained);

    UFUNCTION(BlueprintPure, Category = "ViewModel|Economy")
    float GetPowerRatio() const;

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Economy")
    void SetPowerShortage(bool bShortage);

    UFUNCTION(BlueprintPure, Category = "ViewModel|Economy")
    bool IsPowerShortage() const;

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Selection")
    void SetSelectionState(int32 Count, float HealthRatio, const FString& EntityName, bool bOwned);

    UFUNCTION(BlueprintPure, Category = "ViewModel|Selection")
    int32 GetSelectionCount() const { return SelectionCount; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Selection")
    float GetSelectionHealthRatio() const { return SelectionHealthRatio; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Selection")
    FString GetPrimaryEntityName() const { return PrimaryEntityName; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Selection")
    bool IsPrimaryOwned() const { return bPrimaryOwned; }

private:
    UPROPERTY(FieldNotify, Setter, Getter)
    int32 Credits;

    UPROPERTY(FieldNotify, Getter = GetPowerRatio)
    float PowerRatio;

    UPROPERTY(FieldNotify, Getter = IsPowerShortage)
    bool bPowerShortage;

    UPROPERTY(FieldNotify, Getter = GetSelectionCount)
    int32 SelectionCount;

    UPROPERTY(FieldNotify, Getter = GetSelectionHealthRatio)
    float SelectionHealthRatio;

    UPROPERTY(FieldNotify, Getter = GetPrimaryEntityName)
    FString PrimaryEntityName;

    UPROPERTY(FieldNotify, Getter = IsPrimaryOwned)
    bool bPrimaryOwned;
};
