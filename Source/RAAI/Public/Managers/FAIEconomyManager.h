#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "Managers/FAIEconomyManager.generated.h"

class AAIDirector;

USTRUCT(BlueprintType)
struct FResourceInfo
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag ResourceTag;

    UPROPERTY()
    float CurrentAmount = 0.0f;

    UPROPERTY()
    float MaxStorage = 10000.0f;

    UPROPERTY()
    float IncomeRate = 0.0f;

    UPROPERTY()
    float ExpenseRate = 0.0f;
};

USTRUCT(BlueprintType)
struct FProductionQueueItem
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag ItemTag;

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

    UPROPERTY()
    bool bIsStructure = false;
};

class RAAI_API FAIEconomyManager
{
public:
    FAIEconomyManager();

    void Initialize(AAIDirector* InDirector);
    void Shutdown();
    void Update(float DeltaTime);

    float GetResourceAmount(const FGameplayTag& ResourceType) const;
    void AddResource(const FGameplayTag& ResourceType, float Amount);
    void SpendResource(const FGameplayTag& ResourceType, float Amount);
    bool CanAfford(const FGameplayTag& ResourceType, float Amount) const;

    float GetIncomeRate(const FGameplayTag& ResourceType) const;
    float GetExpenseRate(const FGameplayTag& ResourceType) const;

    void RegisterProducer(const FGameplayTag& ProducerTag, float ProductionRate);
    void UnregisterProducer(const FGameplayTag& ProducerTag);
    void RegisterConsumer(const FGameplayTag& ConsumerTag, float ConsumptionRate);
    void UnregisterConsumer(const FGameplayTag& ConsumerTag);

    void RequestExpansion(const FVector& PreferredLocation, float Priority);
    bool ShouldExpand() const;

    void SetResourceTarget(const FGameplayTag& ResourceType, float TargetAmount);
    float GetResourceTarget(const FGameplayTag& ResourceType) const;

    float GetTotalIncome() const;
    float GetTotalExpenses() const;
    float GetNetIncome() const;

    TArray<FResourceInfo> GetAllResources() const;

    void EmergencyEconomyMode(bool bEnable);

private:
    AAIDirector* Director = nullptr;

    TMap<FGameplayTag, FResourceInfo> Resources;
    TMap<FGameplayTag, float> ResourceTargets;
    TMap<FGameplayTag, float> ProducerRates;
    TMap<FGameplayTag, float> ConsumerRates;

    TArray<FVector> ExpansionLocations;
    TArray<float> ExpansionPriorities;

    float UpdateTimer = 0.0f;
    float UpdateInterval = 1.0f;

    bool bEmergencyMode = false;
    float EmergencyThreshold = 200.0f;

    void UpdateResourceFlow(float DeltaTime);
    void UpdateExpansions();
    void CalculateRates();
    FVector FindBestExpansionLocation() const;
    float EvaluateExpansionLocation(const FVector& Location) const;
};