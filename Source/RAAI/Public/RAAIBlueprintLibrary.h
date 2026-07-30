#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RAAIBlueprintLibrary.generated.h"

class AAIDirector;
class AActor;

UCLASS()
class RAAI_API URAAIBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Economy
    UFUNCTION(BlueprintPure, Category = "RAAI|Economy", meta = (WorldContext = "WorldContextObject"))
    static float GetResourceAmount(UObject* WorldContextObject, FGameplayTag ResourceType);

    UFUNCTION(BlueprintPure, Category = "RAAI|Economy", meta = (WorldContext = "WorldContextObject"))
    static bool CanAfford(UObject* WorldContextObject, FGameplayTag ResourceType, float Amount);

    UFUNCTION(BlueprintPure, Category = "RAAI|Economy", meta = (WorldContext = "WorldContextObject"))
    static float GetNetIncome(UObject* WorldContextObject, FGameplayTag ResourceType);

    UFUNCTION(BlueprintPure, Category = "RAAI|Economy", meta = (WorldContext = "WorldContextObject"))
    static bool IsPowerPositive(UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "RAAI|Economy", meta = (WorldContext = "WorldContextObject"))
    static float GetPowerRatio(UObject* WorldContextObject);

    // Intelligence
    UFUNCTION(BlueprintPure, Category = "RAAI|Intelligence", meta = (WorldContext = "WorldContextObject"))
    static int32 GetKnownEnemyUnitCount(UObject* WorldContextObject, FGameplayTag UnitType);

    UFUNCTION(BlueprintPure, Category = "RAAI|Intelligence", meta = (WorldContext = "WorldContextObject"))
    static int32 GetKnownEnemyStructureCount(UObject* WorldContextObject, FGameplayTag StructureType);

    UFUNCTION(BlueprintPure, Category = "RAAI|Intelligence", meta = (WorldContext = "WorldContextObject"))
    static bool IsEnemyPowerDown(UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "RAAI|Intelligence", meta = (WorldContext = "WorldContextObject"))
    static float GetEnemyPressureLevel(UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "RAAI|Intelligence", meta = (WorldContext = "WorldContextObject"))
    static float GetBaseWeaknessScore(UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "RAAI|Intelligence", meta = (WorldContext = "WorldContextObject"))
    static bool AreHarvestersUnderAttack(UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "RAAI|Intelligence", meta = (WorldContext = "WorldContextObject"))
    static bool IsExpansionSafe(UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "RAAI|Intelligence", meta = (WorldContext = "WorldContextObject"))
    static float GetEnemyProximityToBase(UObject* WorldContextObject);

    // Production
    UFUNCTION(BlueprintPure, Category = "RAAI|Production", meta = (WorldContext = "WorldContextObject"))
    static int32 GetQueuedUnitCount(UObject* WorldContextObject, FGameplayTag UnitTag);

    UFUNCTION(BlueprintPure, Category = "RAAI|Production", meta = (WorldContext = "WorldContextObject"))
    static float GetQueueProgress(UObject* WorldContextObject, FGameplayTag UnitTag);

    UFUNCTION(BlueprintPure, Category = "RAAI|Production", meta = (WorldContext = "WorldContextObject"))
    static bool CanProduce(UObject* WorldContextObject, FGameplayTag UnitTag);

    // Base
    UFUNCTION(BlueprintPure, Category = "RAAI|Base", meta = (WorldContext = "WorldContextObject"))
    static bool HasStructure(UObject* WorldContextObject, FGameplayTag StructureTag);

    UFUNCTION(BlueprintPure, Category = "RAAI|Base", meta = (WorldContext = "WorldContextObject"))
    static bool HasAvailableOreField(UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "RAAI|Base", meta = (WorldContext = "WorldContextObject"))
    static FVector FindBestBuildLocation(UObject* WorldContextObject, FGameplayTag StructureTag);

    // Director Access
    UFUNCTION(BlueprintPure, Category = "RAAI|Director", meta = (WorldContext = "WorldContextObject"))
    static AAIDirector* GetAIDirector(UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "RAAI|Director", meta = (WorldContext = "WorldContextObject"))
    static TArray<FGameplayTag> GetActiveStrategicGoals(UObject* WorldContextObject);
};