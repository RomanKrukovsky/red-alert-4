// Copyright (c) Red Alert 4 project. Primary Data Assets for Unreal Engine 5 integration.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RA4Content/ContentTypes.h"
#include "RA4DataAssets.generated.h"

UCLASS(BlueprintType)
class REDALERT4_API URA4FactionDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
    FText FactionName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
    FGameplayTag FactionTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
    int32 StartingCredits = 10000;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
    int32 StartingCommandLimit = 50;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
    int32 MaxCommandLimit = 200;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
    FText UniqueResourceName;
};

UCLASS(BlueprintType)
class REDALERT4_API URA4WeaponDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    FText WeaponName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    int32 BaseDamage = 20;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    FGameplayTag DamageTypeTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    float MaxRange = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    float CooldownSeconds = 1.0f;
};

UCLASS(BlueprintType)
class REDALERT4_API URA4UnitDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    FText UnitName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    FGameplayTag UnitTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    FGameplayTag FactionTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    int32 Cost = 100;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    int32 BuildTimeSeconds = 4;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    int32 CommandLimit = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    int32 MaxHealth = 100;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    FGameplayTag ArmorTypeTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    float MaxSpeed = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    float TargetDPS = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
    TSoftClassPtr<AActor> UnitClassPtr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
    TSoftObjectPtr<UStaticMesh> MeshPtr;
};

UCLASS(BlueprintType)
class REDALERT4_API URA4BuildingDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    FText BuildingName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    FGameplayTag BuildingTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    int32 Cost = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    int32 BuildTimeSeconds = 20;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    int32 PowerOutput = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    int32 PowerConsumption = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    int32 CommandLimitProvided = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
    TSoftObjectPtr<UStaticMesh> MeshPtr;
};

UCLASS(BlueprintType)
class REDALERT4_API URA4AbilityDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
    FText AbilityName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
    FGameplayTag AbilityTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
    float CooldownSeconds = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
    int32 ResourceCost = 0;
};

UCLASS(BlueprintType)
class REDALERT4_API URA4VoiceSetDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice")
    FGameplayTag UnitTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice")
    TMap<FGameplayTag, FText> VoiceEvents;
};

UCLASS(BlueprintType)
class REDALERT4_API URA4EconomyRulesDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
    int32 StandardOreAmount = 45000;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
    int32 RichOreAmount = 75000;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
    int32 HarvesterCapacity = 1200;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
    int32 OilDerrickIncomePerSec = 8;
};

UCLASS(BlueprintType)
class REDALERT4_API URA4DamageMatrixDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class REDALERT4_API URA4VeterancyDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class REDALERT4_API URA4TechTreeDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class REDALERT4_API URA4FactionResourceDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class REDALERT4_API URA4AIBehaviorDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class REDALERT4_API URA4StartingArmyDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
};
