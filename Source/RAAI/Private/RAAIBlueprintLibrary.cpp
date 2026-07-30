#include "RAAIBlueprintLibrary.h"
#include "AI/AAIDirector.h"

AAIDirector* URAAIBlueprintLibrary::GetAIDirector(UObject* WorldContextObject)
{
    return AAIDirector::GetAIDirector(WorldContextObject);
}

float URAAIBlueprintLibrary::GetResourceAmount(UObject* WorldContextObject, FGameplayTag ResourceType)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->GetResourceAmount(ResourceType) : 0.0f;
}

bool URAAIBlueprintLibrary::CanAfford(UObject* WorldContextObject, FGameplayTag ResourceType, float Amount)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->EconomyManager.CanAfford(ResourceType, Amount) : false;
}

float URAAIBlueprintLibrary::GetNetIncome(UObject* WorldContextObject, FGameplayTag ResourceType)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->EconomyManager.GetNetIncome(ResourceType) : 0.0f;
}

bool URAAIBlueprintLibrary::IsPowerPositive(UObject* WorldContextObject)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->EconomyManager.IsPowerPositive() : true;
}

float URAAIBlueprintLibrary::GetPowerRatio(UObject* WorldContextObject)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->EconomyManager.GetPowerRatio() : 1.0f;
}

int32 URAAIBlueprintLibrary::GetKnownEnemyUnitCount(UObject* WorldContextObject, FGameplayTag UnitType)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->IntelManager.GetKnownEnemyUnitCount(UnitType) : 0;
}

int32 URAAIBlueprintLibrary::GetKnownEnemyStructureCount(UObject* WorldContextObject, FGameplayTag StructureType)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->IntelManager.GetKnownEnemyStructureCount(StructureType) : 0;
}

bool URAAIBlueprintLibrary::IsEnemyPowerDown(UObject* WorldContextObject)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->IntelManager.IsEnemyPowerDown() : false;
}

float URAAIBlueprintLibrary::GetEnemyPressureLevel(UObject* WorldContextObject)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->IntelManager.GetEnemyPressureLevel() : 0.0f;
}

float URAAIBlueprintLibrary::GetBaseWeaknessScore(UObject* WorldContextObject)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->IntelManager.GetBaseWeaknessScore() : 0.0f;
}

bool URAAIBlueprintLibrary::AreHarvestersUnderAttack(UObject* WorldContextObject)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->IntelManager.AreHarvestersUnderAttack() : false;
}

bool URAAIBlueprintLibrary::IsExpansionSafe(UObject* WorldContextObject)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->IntelManager.IsExpansionSafe() : true;
}

float URAAIBlueprintLibrary::GetEnemyProximityToBase(UObject* WorldContextObject)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->IntelManager.GetEnemyProximityToBase() : 0.0f;
}

int32 URAAIBlueprintLibrary::GetQueuedUnitCount(UObject* WorldContextObject, FGameplayTag UnitTag)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->ProductionManager.GetQueuedCount(UnitTag) : 0;
}

float URAAIBlueprintLibrary::GetQueueProgress(UObject* WorldContextObject, FGameplayTag UnitTag)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->ProductionManager.GetQueueProgress(UnitTag) : 0.0f;
}

bool URAAIBlueprintLibrary::CanProduce(UObject* WorldContextObject, FGameplayTag UnitTag)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->ProductionManager.CanProduce(UnitTag) : false;
}

bool URAAIBlueprintLibrary::HasStructure(UObject* WorldContextObject, FGameplayTag StructureTag)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->BasePlanner.HasStructure(StructureTag) : false;
}

bool URAAIBlueprintLibrary::HasAvailableOreField(UObject* WorldContextObject)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->BasePlanner.HasAvailableOreField() : false;
}

FVector URAAIBlueprintLibrary::FindBestBuildLocation(UObject* WorldContextObject, FGameplayTag StructureTag)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    return Director ? Director->BasePlanner.FindBestBuildLocation(StructureTag) : FVector::ZeroVector;
}

TArray<FGameplayTag> URAAIBlueprintLibrary::GetActiveStrategicGoals(UObject* WorldContextObject)
{
    AAIDirector* Director = GetAIDirector(WorldContextObject);
    if (!Director) return TArray<FGameplayTag>();

    TArray<FGameplayTag> Goals;
    for (auto& Pair : Director->ActiveGoals)
    {
        if (Pair.Value.Progress < 1.0f)
        {
            Goals.Add(Pair.Key);
        }
    }
    return Goals;
}