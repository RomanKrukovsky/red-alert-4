#include "AI/AAIDirector.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "AI/Planning/FAIHTNPlanner.h"
#include "AI/Planning/FAIUtilityScorer.h"
#include "Managers/FAIEconomyManager.h"
#include "Managers/FAIBasePlanner.h"
#include "Managers/FAIProductionManager.h"
#include "Intelligence/FAIIntelManager.h"

AAIDirector::AAIDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;
}

void AAIDirector::BeginPlay()
{
    Super::BeginPlay();

    InitializeArchetypeData();

    EconomyManager.Initialize(this);
    BasePlanner.Initialize(this);
    ProductionManager.Initialize(this);
    IntelManager.Initialize(this);
    HTNPlanner.Initialize(this);
    UtilityScorer.Initialize(this);

    CreateDefaultArmyGroups();

    UE_LOG(LogTemp, Log, TEXT("RAAI Director initialized for Player %d with Archetype %d"), PlayerIndex, (int32)Archetype);
}

void AAIDirector::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    StrategicTimer += DeltaTime;
    TacticalTimer += DeltaTime;

    if (TacticalTimer >= TacticalUpdateInterval)
    {
        TacticalTimer = 0.0f;
        UpdateArmyGroups(DeltaTime);
        DistributeOrders(DeltaTime);
        ProductionManager.Update(DeltaTime);
        BasePlanner.Update(DeltaTime);
    }

    if (StrategicTimer >= StrategicUpdateInterval)
    {
        StrategicTimer = 0.0f;
        UpdateStrategicAI(DeltaTime);
    }

    EconomyManager.Update(DeltaTime);
    IntelManager.Update(DeltaTime);
}

void AAIDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

AAIDirector* AAIDirector::GetAIDirector(const UObject* WorldContextObject)
{
    if (!WorldContextObject) return nullptr;

    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return nullptr;

    for (TActorIterator<AAIDirector> It(World); It; ++It)
    {
        if (It->PlayerIndex == 1)
        {
            return *It;
        }
    }
    return nullptr;
}

void AAIDirector::InitializeDirector(EAIArchetype InArchetype, int32 InPlayerIndex)
{
    Archetype = InArchetype;
    PlayerIndex = InPlayerIndex;
    InitializeArchetypeData();
}

void AAIDirector::InitializeArchetypeData()
{
    GoalQueue.Empty();
    ActiveGoals.Empty();

    switch (Archetype)
    {
    case EAIArchetype::USSR_HeavyAssault:
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.BuildHeavyArmor"), 10.0f);
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.ControlOreFields"), 8.0f);
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.DestroyEnemyPower"), 7.0f);
        break;

    case EAIArchetype::Allied_ReconAir:
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.BuildAirForce"), 10.0f);
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.ScoutEnemyBase"), 9.0f);
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.HarassEnemyEconomy"), 8.0f);
        break;

    case EAIArchetype::Empire_TechTransform:
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.TechUpFast"), 10.0f);
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.BuildTransformUnits"), 9.0f);
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.DefendTechStructures"), 8.0f);
        break;

    case EAIArchetype::Cautious:
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.BuildDefenses"), 10.0f);
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.AccumulateResources"), 9.0f);
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.WaitForMistake"), 7.0f);
        break;

    case EAIArchetype::Aggressive:
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.EarlyRush"), 10.0f);
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.ConstantPressure"), 9.0f);
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.DenyExpansion"), 8.0f);
        break;

    case EAIArchetype::Guerrilla:
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.Harassment"), 10.0f);
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.Ambushes"), 9.0f);
        SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.HitAndRun"), 8.0f);
        break;
    }
}

void AAIDirector::UpdateStrategicAI(float DeltaTime)
{
    UpdatePhase();
    EvaluateGoals();
    ExecuteHTNPlanning();
}

void AAIDirector::UpdatePhase()
{
    int32 TotalUnits = 0;
    for (auto& Pair : UnitCounts)
    {
        TotalUnits += Pair.Value;
    }

    int32 TotalStructures = 0;
    for (auto& Pair : StructureCounts)
    {
        TotalStructures += Pair.Value;
    }

    EStrategicPhase NewPhase = CurrentPhase;

    if (TotalStructures < 3 && TotalUnits < 5)
    {
        NewPhase = EStrategicPhase::EarlyGame;
    }
    else if (TotalStructures < 8 || TotalUnits < 15)
    {
        NewPhase = EStrategicPhase::MidGame;
    }
    else if (TotalStructures < 15 || TotalUnits < 40)
    {
        NewPhase = EStrategicPhase::LateGame;
    }
    else
    {
        NewPhase = EStrategicPhase::EndGame;
    }

    if (NewPhase != CurrentPhase)
    {
        CurrentPhase = NewPhase;
        OnPhaseChanged.Broadcast(CurrentPhase);
        UE_LOG(LogTemp, Log, TEXT("RAAI Phase changed to: %d"), (int32)CurrentPhase);
    }
}

void AAIDirector::EvaluateGoals()
{
    TArray<FStrategicGoal> GoalsToRemove;

    for (auto& Pair : ActiveGoals)
    {
        FStrategicGoal& Goal = Pair.Value;

        bool bPrereqsMet = true;
        for (const FGameplayTag& Prereq : Goal.Prerequisites)
        {
            if (!ActiveGoals.Contains(Prereq) || ActiveGoals[Prereq].Progress < 1.0f)
            {
                bPrereqsMet = false;
                break;
            }
        }

        if (bPrereqsMet && Goal.Progress < 1.0f)
        {
            float UtilityScore = UtilityScorer.ScoreGoal(Goal, this);
            Goal.Progress = FMath::Min(1.0f, Goal.Progress + UtilityScore * StrategicUpdateInterval * 0.1f);

            if (Goal.Progress >= 1.0f)
            {
                UE_LOG(LogTemp, Log, TEXT("RAAI Goal completed: %s"), *Goal.GoalTag.ToString());
            }
        }
    }

    for (const FGameplayTag& Tag : GoalsToRemove)
    {
        ActiveGoals.Remove(Tag);
    }
}

void AAIDirector::ExecuteHTNPlanning()
{
    FHTNPlanResult PlanResult = HTNPlanner.GeneratePlan(this);

    if (PlanResult.bSuccess)
    {
        for (const FHTNTask& Task : PlanResult.Tasks)
        {
            ProcessHTNTask(Task);
        }
    }
}

void AAIDirector::ProcessHTNTask(const FHTNTask& Task)
{
    switch (Task.TaskType)
    {
    case EHTNTaskType::BuildStructure:
        BasePlanner.RequestStructure(Task.TargetTag, Task.Priority, Task.TargetLocation);
        break;

    case EHTNTaskType::TrainUnits:
        ProductionManager.RequestUnits(Task.TargetTag, Task.Count, Task.Priority);
        break;

    case EHTNTaskType::AttackTarget:
        CreateAttackGroup(Task.TargetLocation, Task.Priority);
        break;

    case EHTNTaskType::DefendPosition:
        CreateDefenseGroup(Task.TargetLocation, Task.Priority);
        break;

    case EHTNTaskType::ScoutArea:
        CreateScoutGroup(Task.TargetLocation, Task.Priority);
        break;

    case EHTNTaskType::ExpandEconomy:
        EconomyManager.RequestExpansion(Task.TargetLocation, Task.Priority);
        break;
    }
}

void AAIDirector::UpdateArmyGroups(float DeltaTime)
{
    for (FArmyGroup& Group : ArmyGroups)
    {
        if (!Group.bIsActive) continue;

        UpdateGroupStrength(Group);

        TArray<AActor*> DeadUnits;
        for (AActor* Unit : Group.Units)
        {
            if (!Unit || Unit->IsPendingKill())
            {
                DeadUnits.Add(Unit);
            }
        }

        for (AActor* DeadUnit : DeadUnits)
        {
            Group.Units.Remove(DeadUnit);
        }

        if (Group.Units.Num() == 0)
        {
            Group.bIsActive = false;
        }
    }

    AssignUnitsToGroups();
}

void AAIDirector::DistributeOrders(float DeltaTime)
{
    for (FArmyGroup& Group : ArmyGroups)
    {
        if (!Group.bIsActive || Group.Units.Num() == 0) continue;

        for (AActor* Unit : Group.Units)
        {
            if (!Unit) continue;

            UBlackboardComponent* UnitBB = Unit->FindComponentByClass<UBlackboardComponent>();
            if (UnitBB)
            {
                if (Group.CurrentOrder.IsValid())
                {
                    UnitBB->SetValueAsEnum("CurrentOrder", (uint8)Group.CurrentOrder);
                }

                if (!Group.OrderTargetLocation.IsZero())
                {
                    UnitBB->SetValueAsVector("OrderTargetLocation", Group.OrderTargetLocation);
                }

                if (Group.OrderTargetActor)
                {
                    UnitBB->SetValueAsObject("OrderTargetActor", Group.OrderTargetActor);
                }

                UnitBB->SetValueAsVector("RallyPoint", Group.RallyPoint);
            }
        }
    }
}

void AAIDirector::CreateDefaultArmyGroups()
{
    ArmyGroups.Empty();
    NextGroupID = 0;

    FVector BaseLocation = GetActorLocation();

    ArmyGroups.Add(CreateArmyGroup(FGameplayTag::RequestGameplayTag("ArmyGroup.MainAttack"), BaseLocation));
    ArmyGroups.Add(CreateArmyGroup(FGameplayTag::RequestGameplayTag("ArmyGroup.Defense"), BaseLocation));
    ArmyGroups.Add(CreateArmyGroup(FGameplayTag::RequestGameplayTag("ArmyGroup.Harassment"), BaseLocation));
    ArmyGroups.Add(CreateArmyGroup(FGameplayTag::RequestGameplayTag("ArmyGroup.Scout"), BaseLocation));
    ArmyGroups.Add(CreateArmyGroup(FGameplayTag::RequestGameplayTag("ArmyGroup.AntiAir"), BaseLocation));
}

FArmyGroup AAIDirector::CreateArmyGroup(const FGameplayTag& GroupType, const FVector& RallyPoint)
{
    FArmyGroup Group;
    Group.GroupID = NextGroupID++;
    Group.GroupType = GroupType;
    Group.RallyPoint = RallyPoint;
    Group.CurrentOrder = FGameplayTag::RequestGameplayTag("Order.Idle");
    Group.bIsActive = true;

    OnArmyGroupCreated.Broadcast(Group);

    return Group;
}

void AAIDirector::AssignUnitsToGroups()
{
    for (auto& Pair : UnitsByType)
    {
        const FGameplayTag& UnitType = Pair.Key;
        TArray<AActor*>& Units = Pair.Value;

        for (AActor* Unit : Units)
        {
            if (!Unit) continue;

            bool bAssigned = false;
            for (FArmyGroup& Group : ArmyGroups)
            {
                if (!Group.bIsActive) continue;

                bool bMatches = false;
                if (Group.GroupType.MatchesTag(FGameplayTag::RequestGameplayTag("ArmyGroup.MainAttack")) &&
                    (UnitType.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Tank")) ||
                     UnitType.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.HeavyArmor"))))
                {
                    bMatches = true;
                }
                else if (Group.GroupType.MatchesTag(FGameplayTag::RequestGameplayTag("ArmyGroup.AntiAir")) &&
                         UnitType.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.AntiAir")))
                {
                    bMatches = true;
                }
                else if (Group.GroupType.MatchesTag(FGameplayTag::RequestGameplayTag("ArmyGroup.Scout")) &&
                         UnitType.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.Fast")))
                {
                    bMatches = true;
                }

                if (bMatches && Group.Units.Num() < 12)
                {
                    Group.Units.Add(Unit);
                    bAssigned = true;
                    break;
                }
            }

            if (!bAssigned)
            {
                for (FArmyGroup& Group : ArmyGroups)
                {
                    if (Group.bIsActive && Group.Units.Num() < 12)
                    {
                        Group.Units.Add(Unit);
                        break;
                    }
                }
            }
        }
    }
}

void AAIDirector::UpdateGroupStrength(FArmyGroup& Group)
{
    Group.Strength = 0.0f;
    for (AActor* Unit : Group.Units)
    {
        if (Unit)
        {
            Group.Strength += 1.0f;
        }
    }
}

void AAIDirector::SetStrategicGoal(const FGameplayTag& GoalTag, float Priority, const FVector& TargetLocation)
{
    if (ActiveGoals.Contains(GoalTag)) return;

    FStrategicGoal Goal;
    Goal.GoalTag = GoalTag;
    Goal.Priority = Priority;
    Goal.WorldTargetLocation = TargetLocation;
    Goal.Progress = 0.0f;

    ActiveGoals.Add(GoalTag, Goal);
    OnStrategicGoalChanged.Broadcast(Goal);

    UE_LOG(LogTemp, Log, TEXT("RAAI New Strategic Goal: %s (Priority: %.1f)"), *GoalTag.ToString(), Priority);
}

void AAIDirector::ClearStrategicGoal(const FGameplayTag& GoalTag)
{
    if (ActiveGoals.Remove(GoalTag) > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("RAAI Goal cleared: %s"), *GoalTag.ToString());
    }
}

void AAIDirector::RequestBuildOrder(const FGameplayTag& UnitTag, int32 Count, float Priority)
{
    ProductionManager.RequestUnits(UnitTag, Count, Priority);
}

void AAIDirector::RegisterUnit(AActor* Unit, const FGameplayTag& UnitType)
{
    if (!Unit) return;

    UnitsByType.FindOrAdd(UnitType).AddUnique(Unit);
    UnitCounts.FindOrAdd(UnitType)++;

    UBlackboardComponent* BB = Unit->FindComponentByClass<UBlackboardComponent>();
    if (BB)
    {
        BB->SetValueAsObject("AIDirector", this);
        BB->SetValueAsInt("PlayerIndex", PlayerIndex);
    }
}

void AAIDirector::UnregisterUnit(AActor* Unit)
{
    if (!Unit) return;

    for (auto& Pair : UnitsByType)
    {
        Pair.Value.Remove(Unit);
        if (Pair.Value.Num() == 0)
        {
            UnitCounts.FindOrAdd(Pair.Key)--;
        }
    }
}

void AAIDirector::RegisterStructure(AActor* Structure, const FGameplayTag& StructureType)
{
    if (!Structure) return;

    StructuresByType.FindOrAdd(StructureType).AddUnique(Structure);
    StructureCounts.FindOrAdd(StructureType)++;

    BasePlanner.OnStructureBuilt(Structure, StructureType);
}

void AAIDirector::OnUnitDestroyed(AActor* Unit, const FGameplayTag& UnitType)
{
    UnregisterUnit(Unit);
    UnitCounts.FindOrAdd(UnitType)--;

    for (FArmyGroup& Group : ArmyGroups)
    {
        Group.Units.Remove(Unit);
    }
}

void AAIDirector::OnStructureDestroyed(AActor* Structure, const FGameplayTag& StructureType)
{
    StructuresByType.FindOrAdd(StructureType).Remove(Structure);
    StructureCounts.FindOrAdd(StructureType)--;

    BasePlanner.OnStructureDestroyed(Structure, StructureType);
}

void AAIDirector::ReportEnemySighting(const FVector& Location, const FGameplayTag& UnitType, int32 Count, float Confidence)
{
    IntelManager.ReportEnemySighting(Location, UnitType, Count, Confidence);
}

void AAIDirector::ReportEnemyStructure(const FVector& Location, const FGameplayTag& StructureType, float Confidence)
{
    IntelManager.ReportEnemyStructure(Location, StructureType, Confidence);
}

float AAIDirector::GetResourceAmount(const FGameplayTag& ResourceType) const
{
    return EconomyManager.GetResourceAmount(ResourceType);
}

int32 AAIDirector::GetUnitCount(const FGameplayTag& UnitType) const
{
    return UnitCounts.FindRef(UnitType);
}

TArray<FArmyGroup> AAIDirector::GetActiveArmyGroups() const
{
    TArray<FArmyGroup> ActiveGroups;
    for (const FArmyGroup& Group : ArmyGroups)
    {
        if (Group.bIsActive && Group.Units.Num() > 0)
        {
            ActiveGroups.Add(Group);
        }
    }
    return ActiveGroups;
}

void AAIDirector::CreateAttackGroup(const FVector& TargetLocation, float Priority)
{
    for (FArmyGroup& Group : ArmyGroups)
    {
        if (Group.GroupType.MatchesTag(FGameplayTag::RequestGameplayTag("ArmyGroup.MainAttack")) &&
            Group.bIsActive && Group.Strength > 5.0f)
        {
            Group.CurrentOrder = FGameplayTag::RequestGameplayTag("Order.AttackMove");
            Group.OrderTargetLocation = TargetLocation;
            Group.RallyPoint = TargetLocation;
            break;
        }
    }
}

void AAIDirector::CreateDefenseGroup(const FVector& TargetLocation, float Priority)
{
    for (FArmyGroup& Group : ArmyGroups)
    {
        if (Group.GroupType.MatchesTag(FGameplayTag::RequestGameplayTag("ArmyGroup.Defense")) &&
            Group.bIsActive)
        {
            Group.CurrentOrder = FGameplayTag::RequestGameplayTag("Order.Defend");
            Group.OrderTargetLocation = TargetLocation;
            Group.RallyPoint = TargetLocation;
            break;
        }
    }
}

void AAIDirector::CreateScoutGroup(const FVector& TargetLocation, float Priority)
{
    for (FArmyGroup& Group : ArmyGroups)
    {
        if (Group.GroupType.MatchesTag(FGameplayTag::RequestGameplayTag("ArmyGroup.Scout")) &&
            Group.bIsActive && Group.Units.Num() > 0)
        {
            Group.CurrentOrder = FGameplayTag::RequestGameplayTag("Order.Scout");
            Group.OrderTargetLocation = TargetLocation;
            break;
        }
    }
}