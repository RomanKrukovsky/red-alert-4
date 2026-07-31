#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "FAIIntelManager.generated.h"

class AAIDirector;

USTRUCT(BlueprintType)
struct FEnemySighting
{
    GENERATED_BODY()

    UPROPERTY()
    FVector Location;

    UPROPERTY()
    FGameplayTag UnitType;

    UPROPERTY()
    int32 Count = 0;

    UPROPERTY()
    float Confidence = 0.0f;

    UPROPERTY()
    float Timestamp = 0.0f;

    UPROPERTY()
    float LastSeenTime = 0.0f;
};

USTRUCT(BlueprintType)
struct FEnemyStructure
{
    GENERATED_BODY()

    UPROPERTY()
    FVector Location;

    UPROPERTY()
    FGameplayTag StructureType;

    UPROPERTY()
    float Confidence = 0.0f;

    UPROPERTY()
    float DiscoveryTime = 0.0f;

    UPROPERTY()
    bool bIsDestroyed = false;

    UPROPERTY()
    float DestructionTime = 0.0f;
};

USTRUCT(BlueprintType)
struct FProbabilisticEstimate
{
    GENERATED_BODY()

    UPROPERTY()
    FVector EstimatedLocation;

    UPROPERTY()
    float Probability = 0.0f;

    UPROPERTY()
    FGameplayTag UnitType;

    UPROPERTY()
    int32 EstimatedCount = 0;

    UPROPERTY()
    float UncertaintyRadius = 500.0f;
};

USTRUCT(BlueprintType)
struct FThreatAssessment
{
    GENERATED_BODY()

    UPROPERTY()
    FVector Location;

    UPROPERTY()
    float ThreatLevel = 0.0f;

    UPROPERTY()
    TArray<FGameplayTag> ThreatTypes;

    UPROPERTY()
    float TimeSinceUpdate = 0.0f;
};

class RAAI_API FAIIntelManager
{
public:
    FAIIntelManager();

    void Initialize(AAIDirector* InDirector);
    void Shutdown();
    void Update(float DeltaTime);

    void ReportEnemySighting(const FVector& Location, const FGameplayTag& UnitType, int32 Count, float Confidence);
    void ReportEnemyStructure(const FVector& Location, const FGameplayTag& StructureType, float Confidence);
    void ReportEnemyUnitDestroyed(const FVector& Location, const FGameplayTag& UnitType);
    void ReportEnemyStructureDestroyed(const FVector& Location, const FGameplayTag& StructureType);

    TArray<FEnemySighting> GetRecentSightings(float TimeWindow = 60.0f) const;
    TArray<FEnemyStructure> GetKnownEnemyStructures() const;
    TArray<FProbabilisticEstimate> GetProbabilisticEstimates() const;
    TArray<FThreatAssessment> GetThreatMap() const;

    int32 GetKnownEnemyUnitCount(const FGameplayTag& UnitType) const;
    int32 GetKnownEnemyStructureCount(const FGameplayTag& StructureType) const;

    bool IsEnemyPowerDown() const;
    float GetEnemyPressureLevel() const;
    float GetBaseWeaknessScore() const;
    bool AreHarvestersUnderAttack() const;
    bool IsExpansionSafe() const;
    float GetEnemyProximityToBase() const;

    FVector GetLastKnownEnemyBaseLocation() const;
    TArray<FVector> GetPredictedEnemyExpansionLocations() const;

    void ForgetOldIntel(float MaxAge = 120.0f);

private:
    AAIDirector* Director = nullptr;

    TArray<FEnemySighting> Sightings;
    TArray<FEnemyStructure> KnownStructures;
    TArray<FProbabilisticEstimate> ProbabilisticEstimates;
    TArray<FThreatAssessment> ThreatMap;

    float UpdateTimer = 0.0f;
    float UpdateInterval = 5.0f;

    void UpdateProbabilisticEstimates(float DeltaTime);
    void UpdateThreatMap();
    void DecayConfidence(float DeltaTime);
    void MergeSightings();
    void GenerateProbabilisticEstimate(const FEnemySighting& Sighting);
    void UpdateProbabilisticEstimate(const FEnemySighting& Sighting);
    float CalculateThreatLevel(const FVector& Location) const;
    void PruneOldSightings(float MaxAge);
};