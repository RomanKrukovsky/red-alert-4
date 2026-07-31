class UObject;
class UStaticMesh;
class USkeletalMesh;
class UAnimInstance;
class UAnimSequence;
class UMaterialInterface;
class USoundBase;

#if __has_include("CoreMinimal.h")
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RA4ArtMapping.generated.h"
#else
#include <string>
#include <map>

struct FName
{
    std::string Name;
    FName() = default;
    FName(const char* s) : Name(s ? s : "") {}
    bool operator==(const FName& Other) const { return Name == Other.Name; }
};

struct FLinearColor
{
    float R = 0.0f, G = 0.0f, B = 0.0f, A = 1.0f;
    FLinearColor() = default;
    FLinearColor(float r, float g, float b, float a = 1.0f) : R(r), G(g), B(b), A(a) {}
};

struct FVector
{
    double X = 0.0, Y = 0.0, Z = 0.0;
    FVector() = default;
    FVector(double x, double y, double z) : X(x), Y(y), Z(z) {}
};

struct FRotator
{
    double Pitch = 0.0, Yaw = 0.0, Roll = 0.0;
    FRotator() = default;
    FRotator(double p, double y, double r) : Pitch(p), Yaw(y), Roll(r) {}
};

template<typename T>
struct TSoftObjectPtr
{
    std::string Path;
    bool IsNull() const { return Path.empty(); }
};

template<typename T>
struct TSoftClassPtr
{
    std::string Path;
    bool IsNull() const { return Path.empty(); }
};

class UDataAsset {};

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

#endif

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
    FVector MeshOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Transform")
    FRotator MeshRotation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Transform")
    FVector MeshScale;

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
    std::map<std::string, FRA4UnitArtDefinition> Units;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Art Mappings")
    std::map<std::string, FRA4BuildingArtDefinition> Buildings;

    bool FindUnitArt(const std::string& UnitId, FRA4UnitArtDefinition& OutDefinition) const;
    bool FindBuildingArt(const std::string& BuildingId, FRA4BuildingArtDefinition& OutDefinition) const;
};
