// Copyright (c) Red Alert 4 project. Data-driven mapping from Bible ID to Unreal Engine presentation assets.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RA4ArtMapping.generated.h"

class UStaticMesh;
class USkeletalMesh;
class UAnimInstance;
class UMaterialInterface;
class USoundBase;

USTRUCT(BlueprintType)
struct RA4PRESENTATION_API FRA4UnitArtDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    FName UnitId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
    TSoftObjectPtr<UStaticMesh> StaticMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSoftClassPtr<UAnimInstance> AnimClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets")
    FName TurretSocketName = FName("Socket_Turret");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets")
    FName MuzzleSocketName = FName("Socket_Muzzle");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets")
    FName EngineSocketName = FName("Socket_Engine");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets")
    FName CargoSocketName = FName("Socket_Cargo");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    TSoftObjectPtr<UMaterialInterface> MaterialOverride;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    FLinearColor TeamColorOverride = FLinearColor::Red;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    TSoftObjectPtr<UObject> MuzzleFlashVFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    TSoftObjectPtr<UObject> ProjectileTracerVFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    TSoftObjectPtr<UObject> ImpactVFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    TSoftObjectPtr<UObject> ExplosionVFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destruction")
    TSoftObjectPtr<UStaticMesh> WreckMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> VoiceSelectedSFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> VoiceMoveSFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> VoiceAttackSFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> VoiceDeathSFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> VoiceCannotComplySFX;
};

USTRUCT(BlueprintType)
struct RA4PRESENTATION_API FRA4BuildingArtDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    FName BuildingId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction Stages")
    TSoftObjectPtr<UStaticMesh> Stage0_DeliveryMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction Stages")
    TSoftObjectPtr<UStaticMesh> Stage1_FoundationMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction Stages")
    TSoftObjectPtr<UStaticMesh> Stage2_StructureMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction Stages")
    TSoftObjectPtr<UStaticMesh> Stage3_WiringMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Construction Stages")
    TSoftObjectPtr<UStaticMesh> Stage4_ActiveMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    TSoftObjectPtr<UMaterialInterface> MaterialOverride;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    TSoftObjectPtr<UObject> ConstructionSparksVFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
    TSoftObjectPtr<UObject> DestructionExplosionVFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> SoundBuildStarted;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TSoftObjectPtr<USoundBase> SoundBuildCompleted;
};

UCLASS(BlueprintType)
class RA4PRESENTATION_API URA4ArtMappingDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art Mappings")
    TMap<FName, FRA4UnitArtDefinition> Units;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art Mappings")
    TMap<FName, FRA4BuildingArtDefinition> Buildings;

    UFUNCTION(BlueprintCallable, Category = "Art Mappings")
    bool FindUnitArt(FName UnitId, FRA4UnitArtDefinition& OutDefinition) const;

    UFUNCTION(BlueprintCallable, Category = "Art Mappings")
    bool FindBuildingArt(FName BuildingId, FRA4BuildingArtDefinition& OutDefinition) const;
};
