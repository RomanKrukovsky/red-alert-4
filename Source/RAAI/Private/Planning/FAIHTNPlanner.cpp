#include "Planning/FAIHTNPlanner.h"
#include "AI/AAIDirector.h"

FAIHTNPlanner::FAIHTNPlanner()
{
}

void FAIHTNPlanner::Initialize(AAIDirector* InDirector)
{
    Director = InDirector;

    RegisterPrimitiveTask(EHTNTaskType::BuildStructure, FGameplayTag::RequestGameplayTag("Structure.PowerPlant"), 500.0f, 30.0f);
    RegisterPrimitiveTask(EHTNTaskType::BuildStructure, FGameplayTag::RequestGameplayTag("Structure.Refinery"), 1000.0f, 45.0f);
    RegisterPrimitiveTask(EHTNTaskType::BuildStructure, FGameplayTag::RequestGameplayTag("Structure.Barracks"), 800.0f, 40.0f);
    RegisterPrimitiveTask(EHTNTaskType::BuildStructure, FGameplayTag::RequestGameplayTag("Structure.WarFactory"), 1200.0f, 50.0f);
    RegisterPrimitiveTask(EHTNTaskType::BuildStructure, FGameplayTag::RequestGameplayTag("Structure.TechCenter"), 2000.0f, 60.0f);
    RegisterPrimitiveTask(EHTNTaskType::BuildStructure, FGameplayTag::RequestGameplayTag("Structure.Defense.Turret"), 400.0f, 20.0f);
    RegisterPrimitiveTask(EHTNTaskType::BuildStructure, FGameplayTag::RequestGameplayTag("Structure.Defense.AA"), 500.0f, 25.0f);

    RegisterPrimitiveTask(EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Infantry"), 100.0f, 10.0f);
    RegisterPrimitiveTask(EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Tank.Light"), 400.0f, 20.0f);
    RegisterPrimitiveTask(EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Tank.Medium"), 700.0f, 30.0f);
    RegisterPrimitiveTask(EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Tank.Heavy"), 1200.0f, 45.0f);
    RegisterPrimitiveTask(EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Artillery"), 800.0f, 35.0f);
    RegisterPrimitiveTask(EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.AntiAir"), 500.0f, 25.0f);
    RegisterPrimitiveTask(EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Harvester"), 1400.0f, 40.0f);
    RegisterPrimitiveTask(EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Aircraft"), 1500.0f, 50.0f);

    RegisterPrimitiveTask(EHTNTaskType::AttackTarget, FGameplayTag(), 0.0f, 5.0f);
    RegisterPrimitiveTask(EHTNTaskType::DefendPosition, FGameplayTag(), 0.0f, 3.0f);
    RegisterPrimitiveTask(EHTNTaskType::ScoutArea, FGameplayTag(), 0.0f, 2.0f);
    RegisterPrimitiveTask(EHTNTaskType::ExpandEconomy, FGameplayTag(), 0.0f, 10.0f);

    RegisterMethod(FHTNMethod{
        FGameplayTag::RequestGameplayTag("Strategic.Goal.BuildHeavyArmor"),
        {
            FHTNTask{EHTNTaskType::BuildStructure, FGameplayTag::RequestGameplayTag("Structure.WarFactory")},
            FHTNTask{EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Tank.Heavy"), FVector::ZeroVector, nullptr, 4, 8.0f},
            FHTNTask{EHTNTaskType::AttackTarget, FGameplayTag(), FVector::ZeroVector, nullptr, 0, 9.0f}
        },
        2500.0f,
        {FGameplayTag::RequestGameplayTag("Structure.WarFactory")}
    });

    RegisterMethod(FHTNMethod{
        FGameplayTag::RequestGameplayTag("Strategic.Goal.ControlOreFields"),
        {
            FHTNTask{EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Harvester"), FVector::ZeroVector, nullptr, 2, 7.0f},
            FHTNTask{EHTNTaskType::BuildStructure, FGameplayTag::RequestGameplayTag("Structure.Refinery")},
            FHTNTask{EHTNTaskType::DefendPosition, FGameplayTag(), FVector::ZeroVector, nullptr, 0, 6.0f}
        },
        3000.0f,
        {FGameplayTag::RequestGameplayTag("Structure.Refinery")}
    });

    RegisterMethod(FHTNMethod{
        FGameplayTag::RequestGameplayTag("Strategic.Goal.DestroyEnemyPower"),
        {
            FHTNTask{EHTNTaskType::ScoutArea, FGameplayTag(), FVector::ZeroVector, nullptr, 0, 5.0f},
            FHTNTask{EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Aircraft"), FVector::ZeroVector, nullptr, 3, 8.0f},
            FHTNTask{EHTNTaskType::AttackTarget, FGameplayTag::RequestGameplayTag("Structure.PowerPlant"), FVector::ZeroVector, nullptr, 0, 9.0f}
        },
        4000.0f,
        {FGameplayTag::RequestGameplayTag("Unit.Aircraft")}
    });

    RegisterMethod(FHTNMethod{
        FGameplayTag::RequestGameplayTag("Strategic.Goal.BuildAirForce"),
        {
            FHTNTask{EHTNTaskType::BuildStructure, FGameplayTag::RequestGameplayTag("Structure.Airfield")},
            FHTNTask{EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Aircraft"), FVector::ZeroVector, nullptr, 4, 8.0f},
            FHTNTask{EHTNTaskType::AttackTarget, FGameplayTag(), FVector::ZeroVector, nullptr, 0, 7.0f}
        },
        3500.0f,
        {FGameplayTag::RequestGameplayTag("Structure.Airfield")}
    });

    RegisterMethod(FHTNMethod{
        FGameplayTag::RequestGameplayTag("Strategic.Goal.TechUpFast"),
        {
            FHTNTask{EHTNTaskType::BuildStructure, FGameplayTag::RequestGameplayTag("Structure.TechCenter")},
            FHTNTask{EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Transform"), FVector::ZeroVector, nullptr, 3, 9.0f}
        },
        3000.0f,
        {FGameplayTag::RequestGameplayTag("Structure.TechCenter")}
    });

    RegisterMethod(FHTNMethod{
        FGameplayTag::RequestGameplayTag("Strategic.Goal.EarlyRush"),
        {
            FHTNTask{EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Tank.Light"), FVector::ZeroVector, nullptr, 4, 9.0f},
            FHTNTask{EHTNTaskType::AttackTarget, FGameplayTag(), FVector::ZeroVector, nullptr, 0, 10.0f}
        },
        2000.0f,
        {}
    });

    RegisterMethod(FHTNMethod{
        FGameplayTag::RequestGameplayTag("Strategic.Goal.Harassment"),
        {
            FHTNTask{EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Fast"), FVector::ZeroVector, nullptr, 3, 7.0f},
            FHTNTask{EHTNTaskType::ScoutArea, FGameplayTag(), FVector::ZeroVector, nullptr, 0, 5.0f},
            FHTNTask{EHTNTaskType::AttackTarget, FGameplayTag::RequestGameplayTag("Unit.Harvester"), FVector::ZeroVector, nullptr, 0, 8.0f}
        },
        1500.0f,
        {}
    });
}

void FAIHTNPlanner::Shutdown()
{
    Methods.Empty();
    PrimitiveTasks.Empty();
    Director = nullptr;
}

void FAIHTNPlanner::RegisterMethod(const FHTNMethod& Method)
{
    Methods.Add(Method);
}

void FAIHTNPlanner::RegisterPrimitiveTask(EHTNTaskType TaskType, const FGameplayTag& Tag, float BaseCost, float BaseTime)
{
    FPrimitiveTaskInfo Info;
    Info.BaseCost = BaseCost;
    Info.BaseTime = BaseTime;
    PrimitiveTasks.FindOrAdd(TaskType).Add(Tag, Info);
}

FHTNPlanResult FAIHTNPlanner::GeneratePlan(AAIDirector* InDirector)
{
    Director = InDirector;
    FHTNPlanResult Result;
    Result.bSuccess = false;
    Result.TotalCost = 0.0f;
    Result.TotalTime = 0.0f;

    if (!Director) return Result;

    TArray<FStrategicGoal> SortedGoals;
    for (auto& Pair : Director->ActiveGoals)
    {
        SortedGoals.Add(Pair.Value);
    }

    SortedGoals.Sort([](const FStrategicGoal& A, const FStrategicGoal& B) {
        return A.Priority > B.Priority;
    });

    for (const FStrategicGoal& Goal : SortedGoals)
    {
        if (Goal.Progress >= 1.0f) continue;

        float RemainingBudget = 5000.0f - Result.TotalCost;
        float RemainingTime = 300.0f - Result.TotalTime;

        if (RemainingBudget <= 0 || RemainingTime <= 0) break;

        DecomposeGoal(Goal.GoalTag, Result.Tasks, RemainingBudget, RemainingTime);

        if (Result.Tasks.Num() > 0)
        {
            Result.bSuccess = true;
            for (const FHTNTask& Task : Result.Tasks)
            {
                Result.TotalCost += EstimateTaskCost(Task, Director);
                Result.TotalTime += EstimateTaskTime(Task, Director);
            }
            break;
        }
    }

    return Result;
}

FHTNPlanResult FAIHTNPlanner::GeneratePlanForGoal(AAIDirector* InDirector, const FGameplayTag& GoalTag)
{
    Director = InDirector;
    FHTNPlanResult Result;
    Result.bSuccess = false;

    if (!Director) return Result;

    DecomposeGoal(GoalTag, Result.Tasks, 5000.0f, 300.0f);

    if (Result.Tasks.Num() > 0)
    {
        Result.bSuccess = true;
        for (const FHTNTask& Task : Result.Tasks)
        {
            Result.TotalCost += EstimateTaskCost(Task, Director);
            Result.TotalTime += EstimateTaskTime(Task, Director);
        }
    }

    return Result;
}

bool FAIHTNPlanner::CanExecuteTask(const FHTNTask& Task, AAIDirector* InDirector) const
{
    if (!InDirector) return false;

    for (const FGameplayTag& Prereq : Task.Prerequisites)
    {
        if (!InDirector->ActiveGoals.Contains(Prereq) || InDirector->ActiveGoals[Prereq].Progress < 1.0f)
        {
            return false;
        }
    }

    float Cost = EstimateTaskCost(Task, InDirector);
    if (InDirector->GetResourceAmount(FGameplayTag::RequestGameplayTag("Resource.Credits")) < Cost)
    {
        return false;
    }

    return true;
}

float FAIHTNPlanner::EstimateTaskCost(const FHTNTask& Task, AAIDirector* InDirector) const
{
    float Cost = Task.EstimatedCost;
    if (Cost <= 0.0f)
    {
        if (const TMap<FGameplayTag, FPrimitiveTaskInfo>* TaskMap = PrimitiveTasks.Find(Task.TaskType))
        {
            if (const FPrimitiveTaskInfo* Info = TaskMap->Find(Task.TargetTag))
            {
                Cost = Info->BaseCost * Task.Count;
            }
        }
    }
    return Cost;
}

float FAIHTNPlanner::EstimateTaskTime(const FHTNTask& Task, AAIDirector* InDirector) const
{
    float Time = Task.EstimatedTime;
    if (Time <= 0.0f)
    {
        if (const TMap<FGameplayTag, FPrimitiveTaskInfo>* TaskMap = PrimitiveTasks.Find(Task.TaskType))
        {
            if (const FPrimitiveTaskInfo* Info = TaskMap->Find(Task.TargetTag))
            {
                Time = Info->BaseTime * Task.Count;
            }
        }
    }
    return Time;
}

void FAIHTNPlanner::DecomposeGoal(const FGameplayTag& GoalTag, TArray<FHTNTask>& OutTasks, float MaxCost, float MaxTime, int32 Depth) const
{
    if (Depth > 10) return;

    TArray<FHTNMethod> AvailableMethods = FindMethodsForGoal(GoalTag);
    if (AvailableMethods.Num() == 0) return;

    FHTNMethod BestMethod = SelectBestMethod(AvailableMethods, Director);
    if (!CheckConditions(BestMethod.Conditions, Director)) return;

    float CurrentCost = 0.0f;
    float CurrentTime = 0.0f;

    for (const FHTNTask& Task : BestMethod.Tasks)
    {
        float TaskCost = EstimateTaskCost(Task, Director);
        float TaskTime = EstimateTaskTime(Task, Director);

        if (CurrentCost + TaskCost > MaxCost || CurrentTime + TaskTime > MaxTime)
        {
            break;
        }

        if (CanExecuteTask(Task, Director))
        {
            OutTasks.Add(Task);
            CurrentCost += TaskCost;
            CurrentTime += TaskTime;
        }
        else if (Task.TaskType != EHTNTaskType::BuildStructure && Task.TaskType != EHTNTaskType::TrainUnits)
        {
            DecomposeGoal(Task.TargetTag, OutTasks, MaxCost - CurrentCost, MaxTime - CurrentTime, Depth + 1);
        }
    }
}

bool FAIHTNPlanner::CheckConditions(const TArray<FGameplayTag>& Conditions, AAIDirector* InDirector) const
{
    for (const FGameplayTag& Condition : Conditions)
    {
        if (Condition.MatchesTag(FGameplayTag::RequestGameplayTag("Structure.")))
        {
            if (InDirector->GetUnitCount(Condition) <= 0)
                return false;
        }
        else if (Condition.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.")))
        {
            if (InDirector->GetUnitCount(Condition) <= 0)
                return false;
        }
    }
    return true;
}

TArray<FHTNMethod> FAIHTNPlanner::FindMethodsForGoal(const FGameplayTag& GoalTag) const
{
    TArray<FHTNMethod> FoundMethods;
    for (const FHTNMethod& Method : Methods)
    {
        if (Method.GoalTag.MatchesTag(GoalTag))
        {
            FoundMethods.Add(Method);
        }
    }
    return FoundMethods;
}

FHTNMethod FAIHTNPlanner::SelectBestMethod(const TArray<FHTNMethod>& InMethods, AAIDirector* InDirector) const
{
    if (InMethods.Num() == 0) return FHTNMethod{};

    float BestScore = -1.0f;
    FHTNMethod BestMethod = InMethods[0];

    for (const FHTNMethod& Method : InMethods)
    {
        float Score = 1.0f / FMath::Max(1.0f, Method.Cost);

        if (InDirector)
        {
            int32 CompletedPrereqs = 0;
            for (const FGameplayTag& Cond : Method.Conditions)
            {
                if (InDirector->GetUnitCount(Cond) > 0 || InDirector->StructureCounts.FindRef(Cond) > 0)
                {
                    CompletedPrereqs++;
                }
            }
            Score += CompletedPrereqs * 0.5f;
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            BestMethod = Method;
        }
    }

    return BestMethod;
}