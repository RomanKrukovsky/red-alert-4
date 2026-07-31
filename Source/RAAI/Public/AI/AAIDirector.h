#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "FAIHTNPlanner.h"
#include "FAIUtilityScorer.h"
#include "FAIEconomyManager.h"
#include "FAIBasePlanner.h"
#include "FAIProductionManager.h"
#include "FAIIntelManager.h"
#include "GameFramework/Actor.h"
#include "AAIDirector.generated.h"

class ARAIArmyCommander;
class UBlackboardComponent;
class UBehaviorTreeComponent;

UENUM(BlueprintType)
enum class EAIArchetype : uint8
{
    USSR_HeavyAssault,
    Allied_ReconAir,
    Empire_TechTransform,
    Cautious,
    Aggressive,
    Guerrilla
};

UENUM(BlueprintType)
enum class EStrategicPhase : uint8
{
    EarlyGame,
    MidGame,
    LateGame,
    EndGame
};

USTRUCT(BlueprintType)
struct FStrategicGoal
{
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag GoalTag;

    UPROPERTY()
    float Priority = 1.0f;

    UPROPERTY()
    float Progress = 0.0f;

    UPROPERTY()
    TArray<FGameplayTag> Prerequisites;

    UPROPERTY()
    FVector WorldTargetLocation;

    UPROPERTY()
    TArray<FGameplayTag> RequiredUnits;

    UPROPERTY()
    float EstimatedCost = 0.0f;

    UPROPERTY()
    float EstimatedTime = 0.0f;
};

USTRUCT(BlueprintType)
struct FArmyGroup
{
    GENERATED_BODY()

    UPROPERTY()
    int32 GroupID = -1;

    UPROPERTY()
    FGameplayTag GroupType;

    UPROPERTY()
    TArray<AActor*> Units;

    UPROPERTY()
    FVector RallyPoint;

    UPROPERTY()
    FGameplayTag CurrentOrder;

    UPROPERTY()
    FVector OrderTargetLocation;

    UPROPERTY()
    AActor* OrderTargetActor = nullptr;

    UPROPERTY()
    float Strength = 0.0f;

    UPROPERTY()
    bool bIsActive = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStrategicGoalChanged, const FStrategicGoal&, NewGoal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChanged, EStrategicPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArmyGroupCreated, const FArmyGroup&, NewGroup);

UCLASS(Blueprintable, BlueprintType)
class RAAI_API AAIDirector : public AActor
{
    GENERATED_BODY()

public:
    AAIDirector();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "RAAI|Director")
    static AAIDirector* GetAIDirector(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "RAAI|Director")
    void InitializeDirector(EAIArchetype InArchetype, int32 InPlayerIndex);

    UFUNCTION(BlueprintCallable, Category = "RAAI|Director")
    void SetStrategicGoal(const FGameplayTag& GoalTag, float Priority, const FVector& TargetLocation = FVector::ZeroVector);

    UFUNCTION(BlueprintCallable, Category = "RAAI|Director")
    void ClearStrategicGoal(const FGameplayTag& GoalTag);

    UFUNCTION(BlueprintCallable, Category = "RAAI|Director")
    void RequestBuildOrder(const FGameplayTag& UnitTag, int32 Count, float Priority);

    UFUNCTION(BlueprintCallable, Category = "RAAI|Director")
    void RegisterUnit(AActor* Unit, const FGameplayTag& UnitType);

    UFUNCTION(BlueprintCallable, Category = "RAAI|Director")
    void UnregisterUnit(AActor* Unit);

    UFUNCTION(BlueprintCallable, Category = "RAAI|Director")
    void RegisterStructure(AActor* Structure, const FGameplayTag& StructureType);

    UFUNCTION(BlueprintCallable, Category = "RAAI|Director")
    void OnUnitDestroyed(AActor* Unit, const FGameplayTag& UnitType);

    UFUNCTION(BlueprintCallable, Category = "RAAI|Director")
    void OnStructureDestroyed(AActor* Structure, const FGameplayTag& StructureType);

    UFUNCTION(BlueprintCallable, Category = "RAAI|Director")
    void ReportEnemySighting(const FVector& Location, const FGameplayTag& UnitType, int32 Count, float Confidence);

    UFUNCTION(BlueprintCallable, Category = "RAAI|Director")
    void ReportEnemyStructure(const FVector& Location, const FGameplayTag& StructureType, float Confidence);

    UFUNCTION(BlueprintPure, Category = "RAAI|Director")
    EAIArchetype GetArchetype() const { return Archetype; }

    UFUNCTION(BlueprintPure, Category = "RAAI|Director")
    EStrategicPhase GetCurrentPhase() const { return CurrentPhase; }

    UFUNCTION(BlueprintPure, Category = "RAAI|Director")
    float GetResourceAmount(const FGameplayTag& ResourceType) const;

    UFUNCTION(BlueprintPure, Category = "RAAI|Director")
    int32 GetUnitCount(const FGameplayTag& UnitType) const;

    UFUNCTION(BlueprintPure, Category = "RAAI|Director")
    TArray<FArmyGroup> GetActiveArmyGroups() const;

    UPROPERTY(BlueprintAssignable, Category = "RAAI|Events")
    FOnStrategicGoalChanged OnStrategicGoalChanged;

    UPROPERTY(BlueprintAssignable, Category = "RAAI|Events")
    FOnPhaseChanged OnPhaseChanged;

    UPROPERTY(BlueprintAssignable, Category = "RAAI|Events")
    FOnArmyGroupCreated OnArmyGroupCreated;

public:
    void ProcessHTNTask(const FHTNTask& Task);

    virtual void UpdateStrategicAI(float DeltaTime);
    virtual void UpdatePhase();
    virtual void EvaluateGoals();
    virtual void ExecuteHTNPlanning();
    virtual void UpdateArmyGroups(float DeltaTime);
    virtual void DistributeOrders(float DeltaTime);

    void InitializeArchetypeData();
    void CreateDefaultArmyGroups();
    FArmyGroup CreateArmyGroup(const FGameplayTag& GroupType, const FVector& RallyPoint);
    void CreateAttackGroup(const FVector& TargetLocation, float Priority);
    void CreateDefenseGroup(const FVector& TargetLocation, float Priority);
    void CreateScoutGroup(const FVector& TargetLocation, float Priority);
    void AssignUnitsToGroups();
    void UpdateGroupStrength(FArmyGroup& Group);

    UPROPERTY(EditDefaultsOnly, Category = "RAAI|Configuration")
    EAIArchetype Archetype = EAIArchetype::USSR_HeavyAssault;

    UPROPERTY(EditDefaultsOnly, Category = "RAAI|Configuration")
    int32 PlayerIndex = 1;

    UPROPERTY(EditDefaultsOnly, Category = "RAAI|Configuration")
    float StrategicUpdateInterval = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "RAAI|Configuration")
    float TacticalUpdateInterval = 0.5f;

    UPROPERTY()
    EStrategicPhase CurrentPhase = EStrategicPhase::EarlyGame;

    UPROPERTY()
    TMap<FGameplayTag, FStrategicGoal> ActiveGoals;

    UPROPERTY()
    TArray<FStrategicGoal> GoalQueue;

    UPROPERTY()
    TMap<FGameplayTag, int32> UnitCounts;

    UPROPERTY()
    TMap<FGameplayTag, int32> StructureCounts;

    TMap<FGameplayTag, TArray<AActor*>> UnitsByType;

    TMap<FGameplayTag, TArray<AActor*>> StructuresByType;

    UPROPERTY()
    TArray<FArmyGroup> ArmyGroups;

    UPROPERTY()
    int32 NextGroupID = 0;

    UPROPERTY()
    float StrategicTimer = 0.0f;

    UPROPERTY()
    float TacticalTimer = 0.0f;

    UPROPERTY()
    TObjectPtr<UBlackboardComponent> SharedBlackboard;

    FAIHTNPlanner HTNPlanner;

    FAIUtilityScorer UtilityScorer;

    FAIEconomyManager EconomyManager;

    FAIBasePlanner BasePlanner;

    FAIProductionManager ProductionManager;

    FAIIntelManager IntelManager;
};