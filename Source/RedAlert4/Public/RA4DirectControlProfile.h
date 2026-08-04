// Copyright (c) Red Alert 4 project. DirectControl profile DataAsset.
//
// Per-vehicle presentation settings for first-person direct control. This
// asset is *content*: it does not change simulation behaviour. The simulation
// only enforces ownership, liveness and "armed or turreted"; everything
// below -- camera placement, sensitivity, HUD layout, socket names, sound
// mix -- is presentation and lives here so a designer can tune it without a
// recompile.
//
// See ADR-0011 for the simulation/presentation split.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RA4DirectControlProfile.generated.h"

class UCameraComponent;
class UMetaSoundSource;

UENUM(BlueprintType)
enum class ERA4DirectControlOpticsMode : uint8
{
    Wide = 0,
    Zoomed,
};

USTRUCT(BlueprintType)
struct FRA4DirectControlCameraSettings
{
    GENERATED_BODY()

    // Socket on the vehicle mesh the first-person camera attaches to. Must
    // exist on the skeletal/static mesh or the camera falls back to the
    // hard-coded FirstPersonCameraComponent.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    FName CameraSocket = TEXT("DirectControlCamera");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    FName GunnerSightSocket = TEXT("GunnerSight");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    FName CommanderSightSocket = TEXT("CommanderSight");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    FName MuzzlePrimarySocket = TEXT("MuzzlePrimary");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    FName MuzzleSecondarySocket = TEXT("MuzzleSecondary");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float MouseSensitivityYaw = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float MouseSensitivityPitch = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    bool bInvertPitch = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float InputDeadZone = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AimSmoothing = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "-180.0", ClampMax = "180.0"))
    float TurretYawLimit = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "-80.0", ClampMax = "80.0"))
    float TurretPitchMin = -8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", meta = (ClampMin = "-80.0", ClampMax = "80.0"))
    float TurretPitchMax = 30.0f;

    // Shake amplitude multipliers for accessibility. 0 disables shake entirely.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Accessibility", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float RecoilShakeScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Accessibility", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float MotionShakeScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Accessibility")
    bool bReduceMotion = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Optics", meta = (ClampMin = "30.0", ClampMax = "170.0"))
    float WideFOV = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Optics", meta = (ClampMin = "5.0", ClampMax = "60.0"))
    float ZoomedFOV = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Optics", meta = (ClampMin = "0.05", ClampMax = "2.0"))
    float OpticsBlendTime = 0.18f;
};

USTRUCT(BlueprintType)
struct FRA4DirectControlWeaponSlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    FGameplayTag SlotTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    FName WeaponContentId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    FName AmmoCounterTextKey;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    bool bShowReloadIndicator = true;
};

/**
 * Per-vehicle direct-control profile. Bound by content id (e.g. unit.sov.heavy_tank)
 * via the DirectControlSubsystem. A vehicle without a profile is rejected by
 * the simulation with DirectIneligibleUnit only if it is also unarmed and
 * has no turret; a vehicle with a profile but no simulation eligibility is
 * rejected by the subsystem before a command is ever submitted.
 */
UCLASS(BlueprintType)
class REDALERT4_API URA4DirectControlProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FPrimaryAssetId BoundContentId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText VehicleDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText VehicleClassLabel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    FRA4DirectControlCameraSettings Camera;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
    TArray<FRA4DirectControlWeaponSlot> WeaponSlots;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
    FSoftObjectPath HudWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
    FName HudTag = TEXT("DirectControl.Granit");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    FSoftObjectPath InteriorMix;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    FSoftObjectPath ExteriorMix;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transitions", meta = (ClampMin = "0.05", ClampMax = "2.0"))
    float EnterBlendTime = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transitions", meta = (ClampMin = "0.05", ClampMax = "2.0"))
    float ExitBlendTime = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transitions", meta = (ClampMin = "0.05", ClampMax = "2.0"))
    float DestroyedBlendTime = 0.6f;
};