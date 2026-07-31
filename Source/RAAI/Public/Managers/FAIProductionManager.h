#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "FAIProductionManager.generated.h"

class AAIDirector;

USTRUCT(BlueprintType)
struct FProductionQueueItem
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag UnitTag;

    UPROPERTY()
    int32 Count = 1;

    UPROPERTY()
    float Cost = 0.0f;

    UPROPERTY()
    float BuildTime = 0.0f;

    UPROPERTY()
    float Progress = 0.0f;

    UPROPERTY()
    float Priority = 1.0f;

    bool operator==(const FProductionQueueItem& Other) const
    {
        return UnitTag == Other.UnitTag && Count == Other.Count;
    }

    UPROPERTY()
    FGameplayTag ProducerTag;
};

USTRUCT(BlueprintType)
struct FProducerInfo
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag ProducerTag;

    UPROPERTY()
    TArray<FGameplayTag> CanProduce;

    UPROPERTY()
    float BuildSpeedMultiplier = 1.0f;

    UPROPERTY()
    bool bIsActive = true;

    bool operator==(const FProducerInfo& Other) const
    {
        return ProducerTag == Other.ProducerTag;
    }
};

class RAAI_API FAIProductionManager
{
public:
    FAIProductionManager();

    void Initialize(AAIDirector* InDirector);
    void Shutdown();
    void Update(float DeltaTime);

    void RequestUnits(const FGameplayTag& UnitTag, int32 Count, float Priority);
    void CancelUnitRequest(const FGameplayTag& UnitTag, int32 Count);
    void OnUnitCompleted(const FGameplayTag& UnitTag, const FGameplayTag& ProducerTag);

    int32 GetQueuedCount(const FGameplayTag& UnitTag) const;
    float GetQueueProgress(const FGameplayTag& UnitTag) const;
    TArray<FProductionQueueItem> GetProductionQueue() const;

    void RegisterProducer(const FProducerInfo& Producer);
    void RegisterProducer(const FGameplayTag& ProducerTag, const TArray<FGameplayTag>& CanProduce, float BuildSpeedMultiplier = 1.0f);
    void UnregisterProducer(const FGameplayTag& ProducerTag);
    FGameplayTag GetProducerForUnit(const FGameplayTag& UnitTag) const;
    TArray<FProducerInfo> GetAvailableProducers(const FGameplayTag& UnitTag) const;

    void SetRallyPoint(const FVector& Location);
    FVector GetRallyPoint() const;

    bool CanProduce(const FGameplayTag& UnitTag) const;
    float GetEstimatedCompletionTime(const FGameplayTag& UnitTag) const;

    void PrioritizeUnit(const FGameplayTag& UnitTag);
    void DeprioritizeUnit(const FGameplayTag& UnitTag);

private:
    AAIDirector* Director = nullptr;

    TArray<FProductionQueueItem> ProductionQueue;
    TArray<FProducerInfo> Producers;

    FVector RallyPoint = FVector::ZeroVector;

    float UpdateTimer = 0.0f;
    float UpdateInterval = 0.5f;

    void ProcessQueue(float DeltaTime);
    void SortQueue();
    FProducerInfo* FindBestProducer(const FGameplayTag& UnitTag);
    float GetUnitCost(const FGameplayTag& UnitTag) const;
    float GetUnitBuildTime(const FGameplayTag& UnitTag) const;
    TArray<FGameplayTag> GetUnitPrerequisites(const FGameplayTag& UnitTag) const;
};