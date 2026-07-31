#include "Planning/FAIUtilityScorer.h"
#include "AI/AAIDirector.h"
#include "Managers/FAIEconomyManager.h"
#include "Managers/FAIBasePlanner.h"
#include "Intelligence/FAIIntelManager.h"

FAIUtilityScorer::FAIUtilityScorer()
{
}

void FAIUtilityScorer::Initialize(AAIDirector* InDirector)
{
    Director = InDirector;

    RegisterOption(FUtilityOption{
        FGameplayTag::RequestGameplayTag("Action.BuildPowerPlant"),
        {
            {FGameplayTag::RequestGameplayTag("Factor.PowerDeficit"), 2.0f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.CreditAvailability"), 1.0f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.EarlyGame"), 1.5f, 0.0f, 1.0f}
        },
        0.8f
    });

    RegisterOption(FUtilityOption{
        FGameplayTag::RequestGameplayTag("Action.BuildRefinery"),
        {
            {FGameplayTag::RequestGameplayTag("Factor.OreNeed"), 2.0f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.CreditAvailability"), 1.5f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.HarvesterCount"), 1.0f, 0.0f, 1.0f}
        },
        0.7f
    });

    RegisterOption(FUtilityOption{
        FGameplayTag::RequestGameplayTag("Action.BuildBarracks"),
        {
            {FGameplayTag::RequestGameplayTag("Factor.InfantryNeed"), 1.5f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.CreditAvailability"), 1.0f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.EarlyGame"), 1.0f, 0.0f, 1.0f}
        },
        0.6f
    });

    RegisterOption(FUtilityOption{
        FGameplayTag::RequestGameplayTag("Action.BuildWarFactory"),
        {
            {FGameplayTag::RequestGameplayTag("Factor.VehicleNeed"), 2.0f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.CreditAvailability"), 1.0f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.MidGame"), 1.5f, 0.0f, 1.0f}
        },
        0.5f
    });

    RegisterOption(FUtilityOption{
        FGameplayTag::RequestGameplayTag("Action.TrainHarvester"),
        {
            {FGameplayTag::RequestGameplayTag("Factor.OreNeed"), 2.0f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.HarvesterRatio"), 1.5f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.CreditAvailability"), 1.0f, 0.0f, 1.0f}
        },
        0.8f
    });

    RegisterOption(FUtilityOption{
        FGameplayTag::RequestGameplayTag("Action.TrainTank"),
        {
            {FGameplayTag::RequestGameplayTag("Factor.EnemyVehicleCount"), 1.5f, 0.0f, 1.0f, true},
            {FGameplayTag::RequestGameplayTag("Factor.CreditAvailability"), 1.0f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.WarFactoryReady"), 2.0f, 0.0f, 1.0f}
        },
        0.7f
    });

    RegisterOption(FUtilityOption{
        FGameplayTag::RequestGameplayTag("Action.TrainAntiAir"),
        {
            {FGameplayTag::RequestGameplayTag("Factor.EnemyAirThreat"), 2.0f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.CreditAvailability"), 1.0f, 0.0f, 1.0f}
        },
        0.5f
    });

    RegisterOption(FUtilityOption{
        FGameplayTag::RequestGameplayTag("Action.AttackEnemyBase"),
        {
            {FGameplayTag::RequestGameplayTag("Factor.ArmyStrength"), 2.0f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.EnemyBaseWeakness"), 1.5f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.PowerDown"), 2.0f, 0.0f, 1.0f}
        },
        0.4f
    });

    RegisterOption(FUtilityOption{
        FGameplayTag::RequestGameplayTag("Action.DefendBase"),
        {
            {FGameplayTag::RequestGameplayTag("Factor.EnemyNearBase"), 2.0f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.BaseDefenseStrength"), 1.0f, 0.0f, 1.0f, true},
            {FGameplayTag::RequestGameplayTag("Factor.HarvesterUnderAttack"), 3.0f, 0.0f, 1.0f}
        },
        0.6f
    });

    RegisterOption(FUtilityOption{
        FGameplayTag::RequestGameplayTag("Action.ExpandEconomy"),
        {
            {FGameplayTag::RequestGameplayTag("Factor.OreFieldAvailable"), 2.0f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.CreditSurplus"), 1.5f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.ExpansionSafe"), 1.5f, 0.0f, 1.0f}
        },
        0.5f
    });

    RegisterOption(FUtilityOption{
        FGameplayTag::RequestGameplayTag("Action.BuildDefense"),
        {
            {FGameplayTag::RequestGameplayTag("Factor.EnemyPressure"), 1.5f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.CreditAvailability"), 1.0f, 0.0f, 1.0f},
            {FGameplayTag::RequestGameplayTag("Factor.KeyLocationUndefended"), 2.0f, 0.0f, 1.0f}
        },
        0.4f
    });
}

void FAIUtilityScorer::Shutdown()
{
    RegisteredOptions.Empty();
    Director = nullptr;
}

float FAIUtilityScorer::ScoreGoal(const FStrategicGoal& Goal, AAIDirector* InDirector) const
{
    float Score = Goal.Priority * 0.1f;

    if (InDirector)
    {
        if (Goal.GoalTag.MatchesTag(FGameplayTag::RequestGameplayTag("Strategic.Goal.BuildHeavyArmor")))
        {
            Score += InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Structure.WarFactory")) > 0 ? 0.5f : 0.0f;
            Score += InDirector->GetResourceAmount(FGameplayTag::RequestGameplayTag("Resource.Credits")) > 2000 ? 0.3f : 0.0f;
        }
        else if (Goal.GoalTag.MatchesTag(FGameplayTag::RequestGameplayTag("Strategic.Goal.ControlOreFields")))
        {
            Score += InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Unit.Harvester")) < 3 ? 0.5f : 0.0f;
        }
        else if (Goal.GoalTag.MatchesTag(FGameplayTag::RequestGameplayTag("Strategic.Goal.DestroyEnemyPower")))
        {
            Score += InDirector->IntelManager.GetKnownEnemyStructures().Num() > 0 ? 0.5f : 0.0f;
        }
    }

    return FMath::Clamp(Score, 0.0f, 1.0f);
}

float FAIUtilityScorer::ScoreAction(const FGameplayTag& ActionTag, AAIDirector* InDirector) const
{
    for (const FUtilityOption& Option : RegisteredOptions)
    {
        if (Option.OptionTag.MatchesTag(ActionTag))
        {
            float Score = Option.BaseScore;

            for (const FUtilityFactor& Factor : Option.Factors)
            {
                float FactorValue = EvaluateFactor(Factor, InDirector);
                float Normalized = NormalizeValue(FactorValue, Factor.MinValue, Factor.MaxValue);
                Normalized = ApplyResponseCurve(Normalized, Factor.FactorName);

                if (Factor.bInvert) Normalized = 1.0f - Normalized;

                Score += Normalized * Factor.Weight;
            }

            return FMath::Clamp(Score, 0.0f, 1.0f);
        }
    }

    return 0.0f;
}

TArray<FScoredOption> FAIUtilityScorer::ScoreOptions(const TArray<FUtilityOption>& Options, AAIDirector* InDirector) const
{
    TArray<FScoredOption> Results;

    for (const FUtilityOption& Option : Options)
    {
        FScoredOption Scored;
        Scored.OptionTag = Option.OptionTag;
        Scored.FinalScore = Option.BaseScore;

        for (const FUtilityFactor& Factor : Option.Factors)
        {
            float FactorValue = EvaluateFactor(Factor, InDirector);
            float Normalized = NormalizeValue(FactorValue, Factor.MinValue, Factor.MaxValue);
            Normalized = ApplyResponseCurve(Normalized, Factor.FactorName);

            if (Factor.bInvert) Normalized = 1.0f - Normalized;

            Scored.FactorScores.Add(Factor.FactorName, Normalized);
            Scored.FinalScore += Normalized * Factor.Weight;
        }

        Scored.FinalScore = FMath::Clamp(Scored.FinalScore, 0.0f, 1.0f);
        Results.Add(Scored);
    }

    Results.Sort([](const FScoredOption& A, const FScoredOption& B) {
        return A.FinalScore > B.FinalScore;
    });

    return Results;
}

FGameplayTag FAIUtilityScorer::SelectBestOption(const TArray<FUtilityOption>& Options, AAIDirector* InDirector) const
{
    TArray<FScoredOption> Scored = ScoreOptions(Options, InDirector);

    if (Scored.Num() > 0 && Scored[0].FinalScore > 0.1f)
    {
        return Scored[0].OptionTag;
    }

    return FGameplayTag();
}

void FAIUtilityScorer::RegisterOption(const FUtilityOption& Option)
{
    RegisteredOptions.Add(Option);
}

void FAIUtilityScorer::SetFactorWeight(const FGameplayTag& OptionTag, const FGameplayTag& FactorName, float Weight)
{
    for (FUtilityOption& Option : RegisteredOptions)
    {
        if (Option.OptionTag.MatchesTag(OptionTag))
        {
            for (FUtilityFactor& Factor : Option.Factors)
            {
                if (Factor.FactorName.MatchesTag(FactorName))
                {
                    Factor.Weight = Weight;
                    break;
                }
            }
            break;
        }
    }
}

float FAIUtilityScorer::EvaluateFactor(const FUtilityFactor& Factor, AAIDirector* InDirector) const
{
    if (!InDirector) return 0.0f;

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.PowerDeficit")))
    {
        int32 PowerPlants = InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Structure.PowerPlant"));
        int32 PowerConsumers = InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Structure.Radar")) +
                               InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Structure.TechCenter"));
        return PowerConsumers > PowerPlants ? 1.0f : 0.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.CreditAvailability")))
    {
        float Credits = InDirector->GetResourceAmount(FGameplayTag::RequestGameplayTag("Resource.Credits"));
        return FMath::Clamp(Credits / 5000.0f, 0.0f, 1.0f);
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.OreNeed")))
    {
        int32 Harvesters = InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Unit.Harvester"));
        int32 Refineries = InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Structure.Refinery"));
        return Harvesters < Refineries * 2 ? 1.0f : 0.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.EarlyGame")))
    {
        return InDirector->GetCurrentPhase() == EStrategicPhase::EarlyGame ? 1.0f : 0.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.MidGame")))
    {
        return InDirector->GetCurrentPhase() == EStrategicPhase::MidGame ? 1.0f : 0.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.HarvesterCount")))
    {
        int32 Count = InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Unit.Harvester"));
        return FMath::Clamp(Count / 4.0f, 0.0f, 1.0f);
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.InfantryNeed")))
    {
        int32 Infantry = InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Unit.Infantry"));
        return Infantry < 10 ? 1.0f : 0.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.VehicleNeed")))
    {
        int32 Vehicles = InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Unit.Tank.Light")) +
                         InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Unit.Tank.Medium")) +
                         InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Unit.Tank.Heavy"));
        return Vehicles < 8 ? 1.0f : 0.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.EnemyVehicleCount")))
    {
        return InDirector->IntelManager.GetKnownEnemyUnitCount(FGameplayTag::RequestGameplayTag("Unit.Tank")) / 10.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.WarFactoryReady")))
    {
        return InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Structure.WarFactory")) > 0 ? 1.0f : 0.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.HarvesterRatio")))
    {
        int32 Harvesters = InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Unit.Harvester"));
        int32 Refineries = InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Structure.Refinery"));
        return Refineries > 0 ? FMath::Clamp(float(Harvesters) / float(Refineries * 2), 0.0f, 1.0f) : 1.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.EnemyAirThreat")))
    {
        return InDirector->IntelManager.GetKnownEnemyUnitCount(FGameplayTag::RequestGameplayTag("Unit.Aircraft")) / 5.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.ArmyStrength")))
    {
        int32 TotalUnits = 0;
        for (auto& Pair : InDirector->UnitCounts)
        {
            if (Pair.Key.MatchesTag(FGameplayTag::RequestGameplayTag("Unit.")))
                TotalUnits += Pair.Value;
        }
        return FMath::Clamp(TotalUnits / 30.0f, 0.0f, 1.0f);
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.EnemyBaseWeakness")))
    {
        return InDirector->IntelManager.GetBaseWeaknessScore();
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.PowerDown")))
    {
        return InDirector->IntelManager.IsEnemyPowerDown() ? 1.0f : 0.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.EnemyNearBase")))
    {
        return InDirector->IntelManager.GetEnemyProximityToBase() > 0.0f ? 1.0f : 0.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.BaseDefenseStrength")))
    {
        int32 Turrets = InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Structure.Defense.Turret"));
        int32 AA = InDirector->GetUnitCount(FGameplayTag::RequestGameplayTag("Structure.Defense.AA"));
        return FMath::Clamp((Turrets + AA) / 5.0f, 0.0f, 1.0f);
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.HarvesterUnderAttack")))
    {
        return InDirector->IntelManager.AreHarvestersUnderAttack() ? 1.0f : 0.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.OreFieldAvailable")))
    {
        return InDirector->BasePlanner.HasAvailableOreField() ? 1.0f : 0.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.CreditSurplus")))
    {
        float Credits = InDirector->GetResourceAmount(FGameplayTag::RequestGameplayTag("Resource.Credits"));
        return Credits > 3000 ? 1.0f : 0.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.ExpansionSafe")))
    {
        return InDirector->IntelManager.IsExpansionSafe() ? 1.0f : 0.0f;
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.EnemyPressure")))
    {
        return InDirector->IntelManager.GetEnemyPressureLevel();
    }

    if (Factor.FactorName.MatchesTag(FGameplayTag::RequestGameplayTag("Factor.KeyLocationUndefended")))
    {
        return InDirector->BasePlanner.HasUndefendedKeyLocation() ? 1.0f : 0.0f;
    }

    return 0.0f;
}

float FAIUtilityScorer::NormalizeValue(float Value, float Min, float Max) const
{
    if (FMath::IsNearlyEqual(Max, Min)) return 0.0f;
    return FMath::Clamp((Value - Min) / (Max - Min), 0.0f, 1.0f);
}

float FAIUtilityScorer::ApplyResponseCurve(float NormalizedValue, const FGameplayTag& FactorName) const
{
    return NormalizedValue * NormalizedValue;
}

float FAIUtilityScorer::ScoreBuildOrder(const FGameplayTag& UnitTag, AAIDirector* InDirector) const
{
    return ScoreAction(FGameplayTag::RequestGameplayTag("Action.Train*"), InDirector);
}

float FAIUtilityScorer::ScoreAttackTarget(const FVector& TargetLocation, AAIDirector* InDirector) const
{
    return ScoreAction(FGameplayTag::RequestGameplayTag("Action.AttackEnemyBase"), InDirector);
}

float FAIUtilityScorer::ScoreDefensePosition(const FVector& Location, AAIDirector* InDirector) const
{
    return ScoreAction(FGameplayTag::RequestGameplayTag("Action.DefendBase"), InDirector);
}

float FAIUtilityScorer::ScoreExpansion(const FVector& Location, AAIDirector* InDirector) const
{
    return ScoreAction(FGameplayTag::RequestGameplayTag("Action.ExpandEconomy"), InDirector);
}