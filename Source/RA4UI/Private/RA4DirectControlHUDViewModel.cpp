// Copyright (c) Red Alert 4 project. Direct-control combat HUD ViewModel.
#include "RA4DirectControlHUDViewModel.h"

#define LOCTEXT_NAMESPACE "RA4DirectControlHUD"

URA4DirectControlHUDViewModel::URA4DirectControlHUDViewModel()
    : URA4ViewModelBase()
{
    ExitHintText = LOCTEXT("ExitHint", "F — Exit Direct Control");
}

void URA4DirectControlHUDViewModel::Refresh(
    int32 InVehicleHealth, int32 InVehicleMaxHealth,
    float InSpeedKph,
    int32 InTurretYawDegrees, int32 InHullYawDegrees,
    int32 InTargetRangeMetres,
    bool bInOpticsZoomed,
    int32 InDetectedTargetCount,
    const FText& InPrimaryWeaponName,
    int32 InPrimaryCooldownPercent, bool bInPrimaryReloading,
    const FText& InSecondaryWeaponName,
    bool bInEngineDamaged, bool bInTracksDamaged, bool bInTurretDamaged,
    const FText& InCurrentTask, const FText& InEvaMessage)
{
    SetVehicleHealth(InVehicleHealth);
    SetVehicleMaxHealth(InVehicleMaxHealth);
    SetSpeedKph(InSpeedKph);
    SetTurretYawDegrees(InTurretYawDegrees);
    SetHullYawDegrees(InHullYawDegrees);
    SetTargetRangeMetres(InTargetRangeMetres);
    SetOpticsZoomed(bInOpticsZoomed);
    SetDetectedTargetCount(InDetectedTargetCount);
    SetHasDetectedTarget(InDetectedTargetCount > 0);
    SetPrimaryWeaponName(InPrimaryWeaponName);
    SetPrimaryCooldownPercent(InPrimaryCooldownPercent);
    SetPrimaryReloading(bInPrimaryReloading);
    SetSecondaryWeaponName(InSecondaryWeaponName);
    SetEngineDamaged(bInEngineDamaged);
    SetTracksDamaged(bInTracksDamaged);
    SetTurretDamaged(bInTurretDamaged);
    SetCurrentTaskText(InCurrentTask);
    SetEvaMessageText(InEvaMessage);
}

#define RA4_DCVM_SETTER(Name, Type, Field) \
    void URA4DirectControlHUDViewModel::Set##Name(Type V) \
    { \
        if (Field == V) return; \
        Field = V; \
        UE_MVVM_SET_PROPERTY_VALUE(Field, V); \
    }

RA4_DCVM_SETTER(VehicleHealth, int32, VehicleHealth)
RA4_DCVM_SETTER(VehicleMaxHealth, int32, VehicleMaxHealth)
RA4_DCVM_SETTER(VehicleArmorPercent, int32, VehicleArmorPercent)
RA4_DCVM_SETTER(SpeedKph, float, SpeedKph)
RA4_DCVM_SETTER(TurretYawDegrees, int32, TurretYawDegrees)
RA4_DCVM_SETTER(HullYawDegrees, int32, HullYawDegrees)
RA4_DCVM_SETTER(TargetRangeMetres, int32, TargetRangeMetres)
RA4_DCVM_SETTER(OpticsZoomed, bool, bOpticsZoomed)
RA4_DCVM_SETTER(HasDetectedTarget, bool, bHasDetectedTarget)
RA4_DCVM_SETTER(DetectedTargetCount, int32, DetectedTargetCount)
RA4_DCVM_SETTER(PrimaryCooldownPercent, int32, PrimaryCooldownPercent)
RA4_DCVM_SETTER(PrimaryReloading, bool, bPrimaryReloading)
RA4_DCVM_SETTER(SecondaryCooldownPercent, int32, SecondaryCooldownPercent)
RA4_DCVM_SETTER(EngineDamaged, bool, bEngineDamaged)
RA4_DCVM_SETTER(TracksDamaged, bool, bTracksDamaged)
RA4_DCVM_SETTER(TurretDamaged, bool, bTurretDamaged)

#undef RA4_DCVM_SETTER

#define RA4_DCVM_SETTER_TEXT(Name, Field) \
    void URA4DirectControlHUDViewModel::Set##Name(const FText& V) \
    { \
        if (Field.EqualTo(V)) return; \
        Field = V; \
        UE_MVVM_SET_PROPERTY_VALUE(Field, V); \
    }

RA4_DCVM_SETTER_TEXT(PrimaryWeaponName, PrimaryWeaponName)
RA4_DCVM_SETTER_TEXT(SecondaryWeaponName, SecondaryWeaponName)
RA4_DCVM_SETTER_TEXT(CurrentTaskText, CurrentTaskText)
RA4_DCVM_SETTER_TEXT(EvaMessageText, EvaMessageText)
RA4_DCVM_SETTER_TEXT(ExitHintText, ExitHintText)

#undef RA4_DCVM_SETTER_TEXT

#undef LOCTEXT_NAMESPACE