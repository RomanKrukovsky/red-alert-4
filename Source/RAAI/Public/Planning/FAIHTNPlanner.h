#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "AI/Planning/FAIHTNPlanner.generated.h"

class AAIDirector;

UENUM(BlueprintType)
enum class EHTNTaskType : uint8
{
    None,
    BuildStructure,
    TrainUnits,
    AttackTarget,
    DefendPosition,
    ScoutArea,
    ExpandEconomy,
    ResearchTech,
    BuildDefense,
    RepairStructure,
    Retreat
};

USTRUCT(BlueprintType)
struct FHTNTask
{
    GENERATED_BODY()

    UPROPERTY()
    EHTNTaskType TaskType = EHTNTaskType::None;

    UPROPERTY()
    FGameplayTag TargetTag;

    UPROPERTY()
    FVector TargetLocation;

    UPROPERTY()
    AActor* TargetActor = nullptr;

    UPROPERTY()
    int32 Count = 1;

    UPROPERTY()
    float Priority = 1.0f;

    UPROPERTY()
    float EstimatedCost = 0.0f;

    UPROPERTY()
    float EstimatedTime = 0.0f;

    UPROPERTY()
    TArray<FGameplayTag> Prerequisites;

    UPROPERTY()
    TArray<FHTNTask> SubTasks;
};

USTRUCT(BlueprintType)
struct FHTNMethod
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag GoalTag;

    UPROPERTY()
    TArray<FHTNTask> Tasks;

    UPROPERTY()
    float Cost = 1.0f;

    UPROPERTY()
    TArray<FGameplayTag> Conditions;
};

USTRUCT(BlueprintType)
struct FHTNPlanResult
{
    GENERATED_BODY()

    UPROPERTY()
    bool bSuccess = false;

    UPROPERTY()
    TArray<FHTNTask> Tasks;

    UPROPERTY()
    float TotalCost = 0.0f;

    UPROPERTY()
    float TotalTime = 0.0f;
};

class RAAI_API FAIHTNPlanner
{
public:
    FAIHTNPlanner();

    void Initialize(AAIDirector* InDirector);
    void Shutdown();

    FHTNPlanResult GeneratePlan(AAIDirector* Director);
    FHTNPlanResult GeneratePlanForGoal(AAIDirector* Director, const FGameplayTag& GoalTag);

    void RegisterMethod(const FHTNMethod& Method);
    void RegisterPrimitiveTask(EHTNTaskType TaskType, const FGameplayTag& Tag, float BaseCost, float BaseTime);

    bool CanExecuteTask(const FHTNTask& Task, AAIDirector* Director) const;
    float EstimateTaskCost(const FHTNTask& Task, AAIDirector* Director) const;
    float EstimateTaskTime(const FHTNTask& Task, AAIDirector* Director) const;

private:
    TArray<FHTNMethod> Methods;
    TMap<EHTNTaskType, TMap<FGameplayTag, FPrimitiveTaskInfo>> PrimitiveTasks;

    struct FPrimitiveTaskInfo
    {
        float BaseCost = 1.0f;
        float BaseTime = 1.0f;
    };

    AAIDirector* Director = nullptr;

    void DecomposeGoal(const FGameplayTag& GoalTag, TArray<FHTNTask>& OutTasks, float MaxCost, float MaxTime, int32 Depth = 0) const;
    bool CheckConditions(const TArray<FGameplayTag>& Conditions, AAIDirector* Director) const;
    TArray<FHTNMethod> FindMethodsForGoal(const FGameplayTag& GoalTag) const;
    FHTNMethod SelectBestMethod(const TArray<FHTNMethod>& Methods, AAIDirector* Director) const;
};