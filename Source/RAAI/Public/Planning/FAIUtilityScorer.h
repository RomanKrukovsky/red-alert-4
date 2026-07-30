#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "AI/Planning/FAIUtilityScorer.generated.h"

class AAIDirector;

USTRUCT(BlueprintType)
struct FUtilityFactor
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag FactorName;

    UPROPERTY()
    float Weight = 1.0f;

    UPROPERTY()
    float MinValue = 0.0f;

    UPROPERTY()
    float MaxValue = 1.0f;

    UPROPERTY()
    bool bInvert = false;
};

USTRUCT(BlueprintType)
struct FUtilityOption
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag OptionTag;

    UPROPERTY()
    TArray<FUtilityFactor> Factors;

    UPROPERTY()
    float BaseScore = 0.5f;
};

USTRUCT(BlueprintType)
struct FScoredOption
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag OptionTag;

    UPROPERTY()
    float FinalScore = 0.0f;

    UPROPERTY()
    TMap<FGameplayTag, float> FactorScores;
};

class RAAI_API FAIUtilityScorer
{
public:
    FAIUtilityScorer();

    void Initialize(AAIDirector* InDirector);
    void Shutdown();

    float ScoreGoal(const FStrategicGoal& Goal, AAIDirector* Director) const;
    float ScoreAction(const FGameplayTag& ActionTag, AAIDirector* Director) const;

    TArray<FScoredOption> ScoreOptions(const TArray<FUtilityOption>& Options, AAIDirector* Director) const;
    FGameplayTag SelectBestOption(const TArray<FUtilityOption>& Options, AAIDirector* Director) const;

    void RegisterOption(const FUtilityOption& Option);
    void SetFactorWeight(const FGameplayTag& OptionTag, const FGameplayTag& FactorName, float Weight);
    float EvaluateFactor(const FUtilityFactor& Factor, AAIDirector* Director) const;

    float ScoreBuildOrder(const FGameplayTag& UnitTag, AAIDirector* Director) const;
    float ScoreAttackTarget(const FVector& TargetLocation, AAIDirector* Director) const;
    float ScoreDefensePosition(const FVector& Location, AAIDirector* Director) const;
    float ScoreExpansion(const FVector& Location, AAIDirector* Director) const;

private:
    TArray<FUtilityOption> RegisteredOptions;
    AAIDirector* Director = nullptr;

    float NormalizeValue(float Value, float Min, float Max) const;
    float ApplyResponseCurve(float NormalizedValue, const FGameplayTag& FactorName) const;
};