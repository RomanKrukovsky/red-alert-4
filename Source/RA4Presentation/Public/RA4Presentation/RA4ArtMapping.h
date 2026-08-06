// Copyright (c) Red Alert 4 project. Data-driven mapping from Bible ID to Unreal Engine presentation assets.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RA4ArtMapping.generated.h"

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSoftObjectPtr<UAnimSequence> IdleAnim;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSoftObjectPtr<UAnimSequence> RunAnim;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSoftObjectPtr<UAnimSequence> AttackAnim;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Transform")
    FVector MeshOffset = FVector(0.0, 0.0, 0.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Transform")
    FRotator MeshRotation = FRotator(0.0, 0.0, 0.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Transform")
    FVector MeshScale = FVector(1.0, 1.0, 1.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets")
    FName TurretSocketName = FName("Socket_Turret");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets")
    FName MuzzleSocketName = FName("Socket_Muzzle");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets")
    FName EngineSocketName = FName("Socket_Engine");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets")
    FName CargoSocketName = FName("Socket_Cargo");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    TSoftObjectPtr<UMaterialInterface> CustomMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    FLinearColor TeamColorOverride = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

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
    TMap<FString, FRA4UnitArtDefinition> Units;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art Mappings")
    TMap<FString, FRA4BuildingArtDefinition> Buildings;

    bool FindUnitArt(FName UnitId, FRA4UnitArtDefinition& OutDefinition) const;
    bool FindBuildingArt(FName BuildingId, FRA4BuildingArtDefinition& OutDefinition) const;
};
