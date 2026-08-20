// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4HUDTypes.h"
#include "RA4UIScreenViewModel.h"
#include "RA4ViewModelBase.h"
#include "RA4HUDViewModel.generated.h"

UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4HUDViewModel : public URA4ViewModelBase
{
    GENERATED_BODY()

public:
    URA4HUDViewModel();

    /** Applies an immutable presentation snapshot and returns only changed sections. */
    UFUNCTION(BlueprintCallable, Category = "ViewModel|HUD")
    ERA4HUDChangeFlags ApplySnapshot(const FRA4HUDSnapshotView& Snapshot);

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnHUDChanged, ERA4HUDChangeFlags);
    FOnHUDChanged OnHUDChanged;

    // --- Economy & Power ---
    UFUNCTION(BlueprintCallable, Category = "ViewModel|Economy")
    void SetCredits(int32 InCredits);
    
    UFUNCTION(BlueprintPure, Category = "ViewModel|Economy")
    int32 GetCredits() const;

    UFUNCTION(BlueprintPure, Category = "ViewModel|Economy")
    int32 GetCreditsDelta() const { return CreditsDelta; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Economy")
    int32 GetMatchElapsedSeconds() const { return MatchElapsedSeconds; }

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Economy")
    void SetPower(int32 InPowerProvided, int32 InPowerDrained);

    UFUNCTION(BlueprintPure, Category = "ViewModel|Economy")
    float GetPowerRatio() const;

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Economy")
    void SetPowerProduced(int32 InPowerProduced);

    UFUNCTION(BlueprintPure, Category = "ViewModel|Economy")
    int32 GetPowerProduced() const { return PowerProduced; }

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Economy")
    void SetPowerConsumed(int32 InPowerConsumed);

    UFUNCTION(BlueprintPure, Category = "ViewModel|Economy")
    int32 GetPowerConsumed() const { return PowerConsumed; }

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Economy")
    void SetPowerShortage(bool bShortage);

    UFUNCTION(BlueprintPure, Category = "ViewModel|Economy")
    bool IsPowerShortage() const;

    // --- Command Limit / Supply ---
    UFUNCTION(BlueprintCallable, Category = "ViewModel|Supply")
    void SetCommandLimit(int32 InUsed, int32 InMax);

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Supply")
    void SetCommandLimitUsed(int32 InUsed);

    UFUNCTION(BlueprintPure, Category = "ViewModel|Supply")
    int32 GetCommandLimitUsed() const { return CommandLimitUsed; }

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Supply")
    void SetCommandLimitMax(int32 InMax);

    UFUNCTION(BlueprintPure, Category = "ViewModel|Supply")
    int32 GetCommandLimitMax() const { return CommandLimitMax; }

    // --- Selection ---
    UFUNCTION(BlueprintCallable, Category = "ViewModel|Selection")
    void SetSelectionState(int32 Count, float HealthRatio, const FString& EntityName, bool bOwned);

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Selection")
    void SetSelectionKind(ERA4SelectionKind InKind);

    UFUNCTION(BlueprintPure, Category = "ViewModel|Selection")
    int32 GetSelectionCount() const { return SelectionCount; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Selection")
    float GetSelectionHealthRatio() const { return SelectionHealthRatio; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Selection")
    FString GetPrimaryEntityName() const { return PrimaryEntityName; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Selection")
    bool IsPrimaryOwned() const { return bPrimaryOwned; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Selection")
    ERA4SelectionKind GetSelectionKind() const { return SelectionKind; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Selection")
    int32 GetHarvesterCargo() const { return HarvesterCargo; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Selection")
    int32 GetHarvesterCapacity() const { return HarvesterCapacity; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Selection")
    const TArray<FRA4SelectionGroup>& GetSelectionGroups() const { return SelectionGroups; }

    // --- Production Queue ---
    UFUNCTION(BlueprintCallable, Category = "ViewModel|Production")
    void SetProductionQueue(const TArray<FRA4ProductionQueueItem>& InQueue);

    UFUNCTION(BlueprintPure, Category = "ViewModel|Production")
    const TArray<FRA4ProductionQueueItem>& GetProductionQueue() const { return ProductionQueue; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Production")
    const TArray<FRA4ProductionEntry>& GetDetailedProductionQueue() const { return DetailedProductionQueue; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Production")
    const TArray<FRA4BuildOption>& GetBuildOptions() const { return BuildOptions; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Objectives")
    const TArray<FRA4HUDObjective>& GetObjectives() const { return Objectives; }

    UFUNCTION(BlueprintPure, Category = "ViewModel|Alerts")
    const TArray<FRA4Alert>& GetAlerts() const { return Alerts; }

private:
    UPROPERTY(FieldNotify, Setter, Getter)
    int32 Credits;

    UPROPERTY(FieldNotify, Getter = GetCreditsDelta)
    int32 CreditsDelta;

    UPROPERTY(FieldNotify, Getter = GetMatchElapsedSeconds)
    int32 MatchElapsedSeconds;

    UPROPERTY(FieldNotify, Setter = SetPowerProduced, Getter = GetPowerProduced)
    int32 PowerProduced;

    UPROPERTY(FieldNotify, Setter = SetPowerConsumed, Getter = GetPowerConsumed)
    int32 PowerConsumed;

    UPROPERTY(FieldNotify, Getter = GetPowerRatio)
    float PowerRatio;

    UPROPERTY(FieldNotify, Getter = IsPowerShortage)
    bool bPowerShortage;

    UPROPERTY(FieldNotify, Setter = SetCommandLimitUsed, Getter = GetCommandLimitUsed)
    int32 CommandLimitUsed;

    UPROPERTY(FieldNotify, Setter = SetCommandLimitMax, Getter = GetCommandLimitMax)
    int32 CommandLimitMax;

    UPROPERTY(FieldNotify, Getter = GetSelectionCount)
    int32 SelectionCount;

    UPROPERTY(FieldNotify, Getter = GetSelectionHealthRatio)
    float SelectionHealthRatio;

    UPROPERTY(FieldNotify, Getter = GetPrimaryEntityName)
    FString PrimaryEntityName;

    UPROPERTY(FieldNotify, Getter = IsPrimaryOwned)
    bool bPrimaryOwned;

    UPROPERTY(FieldNotify, Setter = SetSelectionKind, Getter = GetSelectionKind)
    ERA4SelectionKind SelectionKind;

    UPROPERTY(FieldNotify, Getter = GetHarvesterCargo)
    int32 HarvesterCargo;

    UPROPERTY(FieldNotify, Getter = GetHarvesterCapacity)
    int32 HarvesterCapacity;

    UPROPERTY()
    TArray<FRA4SelectionGroup> SelectionGroups;

    UPROPERTY(FieldNotify, Setter = SetProductionQueue, Getter = GetProductionQueue)
    TArray<FRA4ProductionQueueItem> ProductionQueue;

    UPROPERTY()
    TArray<FRA4ProductionEntry> DetailedProductionQueue;

    UPROPERTY()
    TArray<FRA4BuildOption> BuildOptions;

    UPROPERTY()
    TArray<FRA4HUDObjective> Objectives;

    UPROPERTY()
    TArray<FRA4Alert> Alerts;
};
