// Copyright (c) Red Alert 4 project. Direct-control combat HUD ViewModel.
//
// MVVM ViewModel for the in-vehicle first-person HUD. Every property here is
// populated from the authoritative SimWorld state via the DirectControlSubsystem
// -- never from the visual Actor, never from client-only state. The Blueprint
// widget binds to these and renders the reticle, compass, vehicle card, weapon
// readouts and minimap.
//
// This uses the native CommonUI + UMG + Slate stack used by the rest of the HUD.
#pragma once

#include "CoreMinimal.h"
#include "RA4ViewModelBase.h"
#include "RA4DirectControlHUDViewModel.generated.h"

class URA4DirectControlSubsystem;
class URA4SimWorldSubsystem;

UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4DirectControlHUDViewModel : public URA4ViewModelBase
{
    GENERATED_BODY()

public:
    URA4DirectControlHUDViewModel();

    // Called by the player controller every frame while in direct control.
    // The controller reads the authoritative SimWorld and pushes the values
    // into these setters. The ViewModel does no simulation reads itself --
    // that keeps RA4UI free of any dependency on RedAlert4 and its
    // presentation subsystems.
    UFUNCTION(BlueprintCallable, Category = "ViewModel|DirectControl")
    void Refresh(int32 InVehicleHealth, int32 InVehicleMaxHealth,
                 float InSpeedKph,
                 int32 InTurretYawDegrees, int32 InHullYawDegrees,
                 int32 InTargetRangeMetres,
                 bool bInOpticsZoomed,
                 int32 InDetectedTargetCount,
                 const FText& InPrimaryWeaponName,
                 int32 InPrimaryCooldownPercent, bool bInPrimaryReloading,
                 const FText& InSecondaryWeaponName,
                 bool bInEngineDamaged, bool bInTracksDamaged, bool bInTurretDamaged,
                 const FText& InCurrentTask, const FText& InEvaMessage);

    // --- Vehicle card (bottom-left) ---
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Vehicle")
    int32 GetVehicleHealth() const { return VehicleHealth; }
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Vehicle")
    int32 GetVehicleMaxHealth() const { return VehicleMaxHealth; }
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Vehicle")
    int32 GetVehicleArmorPercent() const { return VehicleArmorPercent; }
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Vehicle")
    bool IsEngineDamaged() const { return bEngineDamaged; }
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Vehicle")
    bool IsTracksDamaged() const { return bTracksDamaged; }
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Vehicle")
    bool IsTurretDamaged() const { return bTurretDamaged; }

    // --- Speed (bottom-left, beside the card) ---
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Vehicle")
    float GetSpeedKph() const { return SpeedKph; }

    // --- Compass (top-center) ---
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Compass")
    int32 GetTurretYawDegrees() const { return TurretYawDegrees; }
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Compass")
    int32 GetHullYawDegrees() const { return HullYawDegrees; }

    // --- Reticle (center) ---
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Reticle")
    int32 GetTargetRangeMetres() const { return TargetRangeMetres; }
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Reticle")
    bool IsOpticsZoomed() const { return bOpticsZoomed; }
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Reticle")
    bool HasDetectedTarget() const { return bHasDetectedTarget; }
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Reticle")
    int32 GetDetectedTargetCount() const { return DetectedTargetCount; }

    // --- Weapons (bottom-right) ---
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Weapons")
    FText GetPrimaryWeaponName() const { return PrimaryWeaponName; }
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Weapons")
    int32 GetPrimaryCooldownPercent() const { return PrimaryCooldownPercent; }
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Weapons")
    bool IsPrimaryReloading() const { return bPrimaryReloading; }
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Weapons")
    FText GetSecondaryWeaponName() const { return SecondaryWeaponName; }
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Weapons")
    int32 GetSecondaryCooldownPercent() const { return SecondaryCooldownPercent; }

    // --- Task/EVA (top-left/top-right) ---
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Task")
    FText GetCurrentTaskText() const { return CurrentTaskText; }
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Task")
    FText GetEvaMessageText() const { return EvaMessageText; }

    // --- Exit hint (bottom-center) ---
    UFUNCTION(BlueprintPure, Category = "ViewModel|DirectControl|Hint")
    FText GetExitHintText() const { return ExitHintText; }

private:
    // Mirrored authoritative state. Marked SetProperty so the MVVM
    // broadcast fires only when a value actually changes.
    void SetVehicleHealth(int32 V);
    void SetVehicleMaxHealth(int32 V);
    void SetVehicleArmorPercent(int32 V);
    void SetSpeedKph(float V);
    void SetTurretYawDegrees(int32 V);
    void SetHullYawDegrees(int32 V);
    void SetTargetRangeMetres(int32 V);
    void SetOpticsZoomed(bool V);
    void SetHasDetectedTarget(bool V);
    void SetDetectedTargetCount(int32 V);
    void SetPrimaryWeaponName(const FText& V);
    void SetPrimaryCooldownPercent(int32 V);
    void SetPrimaryReloading(bool V);
    void SetSecondaryWeaponName(const FText& V);
    void SetSecondaryCooldownPercent(int32 V);
    void SetEngineDamaged(bool V);
    void SetTracksDamaged(bool V);
    void SetTurretDamaged(bool V);
    void SetCurrentTaskText(const FText& V);
    void SetEvaMessageText(const FText& V);
    void SetExitHintText(const FText& V);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    int32 VehicleHealth = 0;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    int32 VehicleMaxHealth = 0;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    int32 VehicleArmorPercent = 100;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    float SpeedKph = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    int32 TurretYawDegrees = 0;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    int32 HullYawDegrees = 0;

    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    int32 TargetRangeMetres = 0;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    bool bOpticsZoomed = false;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    bool bHasDetectedTarget = false;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    int32 DetectedTargetCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    FText PrimaryWeaponName;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    int32 PrimaryCooldownPercent = 0;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    bool bPrimaryReloading = false;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    FText SecondaryWeaponName;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    int32 SecondaryCooldownPercent = 0;

    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    bool bEngineDamaged = false;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    bool bTracksDamaged = false;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    bool bTurretDamaged = false;

    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    FText CurrentTaskText;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    FText EvaMessageText;
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel|DirectControl", FieldNotify)
    FText ExitHintText;
};
