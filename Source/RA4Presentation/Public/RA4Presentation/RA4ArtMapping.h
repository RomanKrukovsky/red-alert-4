// Copyright (c) Red Alert 4 project. Data-driven mapping from Bible ID to Unreal Engine presentation assets.
#pragma once

#if __has_include("CoreMinimal.h")
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RA4ArtMapping.generated.h"
#else
#include <string>
#include <map>
#include <memory>

#ifndef USTRUCT
#define USTRUCT(...)
#endif
#ifndef UCLASS
#define UCLASS(...)
#endif
#ifndef UPROPERTY
#define UPROPERTY(...)
#endif
#ifndef UFUNCTION
#define UFUNCTION(...)
#endif
#ifndef GENERATED_BODY
#define GENERATED_BODY(...)
#endif
#ifndef RA4PRESENTATION_API
#define RA4PRESENTATION_API
#endif
#ifndef BlueprintType
#define BlueprintType
#endif

struct FName
{
    std::string Name;
    FName() = default;
    FName(const char* InName) : Name(InName ? InName : "") {}
    FName(const std::string& InName) : Name(InName) {}
    bool operator==(const FName& Other) const { return Name == Other.Name; }
};

struct FVector
{
    float X = 0.0f, Y = 0.0f, Z = 0.0f;
    FVector() = default;
    FVector(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}
};

struct FRotator
{
    float Pitch = 0.0f, Yaw = 0.0f, Roll = 0.0f;
    FRotator() = default;
    FRotator(float InP, float InY, float InR) : Pitch(InP), Yaw(InY), Roll(InR) {}
};

struct FLinearColor
{
    float R = 1.0f, G = 0.0f, B = 0.0f, A = 1.0f;
    FLinearColor() = default;
    FLinearColor(float InR, float InG, float InB, float InA = 1.0f) : R(InR), G(InG), B(InB), A(InA) {}
};

template<typename K, typename V>
using TMap = std::map<K, V>;

template<typename T>
struct TSoftObjectPtr
{
    std::string Path;
    bool IsNull() const { return Path.empty(); }
    T* LoadSynchronous() const { return nullptr; }
};

template<typename T>
struct TSoftClassPtr
{
    std::string Path;
    bool IsNull() const { return Path.empty(); }
};

class UObject {};
class UStaticMesh {};
class USkeletalMesh {};
class UAnimInstance {};
class UAnimSequence {};
class UMaterialInterface {};
class USoundBase {};
class UDataAsset {};

#endif

class UStaticMesh;
class USkeletalMesh;
class UAnimInstance;
class UAnimSequence;
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSoftObjectPtr<UAnimSequence> IdleAnim;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSoftObjectPtr<UAnimSequence> WalkAnim;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSoftObjectPtr<UAnimSequence> RunAnim;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSoftObjectPtr<UAnimSequence> AttackAnim;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSoftObjectPtr<UAnimSequence> DeathAnim;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Transform")
    FVector MeshScale = FVector(1.0f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Transform")
    FVector MeshOffset = FVector(0.0f, 0.0f, -90.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Transform")
    FRotator MeshRotation = FRotator(0.0f, -90.0f, 0.0f);

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
    TMap<FName, FRA4UnitArtDefinition> Units;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art Mappings")
    TMap<FName, FRA4BuildingArtDefinition> Buildings;

    UFUNCTION(BlueprintCallable, Category = "Art Mappings")
    bool FindUnitArt(FName UnitId, FRA4UnitArtDefinition& OutDefinition) const;

    UFUNCTION(BlueprintCallable, Category = "Art Mappings")
    bool FindBuildingArt(FName BuildingId, FRA4BuildingArtDefinition& OutDefinition) const;
};
