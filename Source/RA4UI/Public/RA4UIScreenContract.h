// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4UIInputRouter.h"
#include "RA4UIScreenViewModel.h"
#include "RA4UIScreenContract.generated.h"

UENUM(BlueprintType)
enum class ERA4UIScreenFamily : uint8
{
    Splash,
    MainMenu,
    CampaignSelect,
    FactionCampaign,
    MissionMap,
    Briefing,
    VideoComms,
    Loading,
    MultiplayerLobby,
    InGameHud,
    PauseMenu,
    Victory
};

UENUM(BlueprintType)
enum class ERA4UIScreenVariant : uint8
{
    Default,
    AlliesAlternate,
    LoadingBriefing,
    EasternDetail,
    SovietBattle,
    SovietAlert,
    AlliesNaval,
    AlliesAir,
    ChronoSuperweapon
};

USTRUCT(BlueprintType)
struct RA4UI_API FRA4UIScreenContract
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Screen")
    int32 ReferenceNumber = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Screen")
    ERA4UIScreenId ScreenId = ERA4UIScreenId::Splash;

    UPROPERTY(BlueprintReadOnly, Category = "Screen")
    ERA4UIScreenVariant Variant = ERA4UIScreenVariant::Default;

    UPROPERTY(BlueprintReadOnly, Category = "Screen")
    ERA4FactionTheme Theme = ERA4FactionTheme::USSR;

    UPROPERTY(BlueprintReadOnly, Category = "Screen")
    ERA4UIScreenFamily Family = ERA4UIScreenFamily::Splash;

    UPROPERTY(BlueprintReadOnly, Category = "Screen")
    ERA4UIInputMode InputMode = ERA4UIInputMode::UIOnly;

    UPROPERTY(BlueprintReadOnly, Category = "Screen")
    bool bIsHud = false;
};

RA4UI_API FRA4UIScreenContract ResolveScreenContract(
    ERA4UIScreenId Screen,
    ERA4UIScreenVariant Variant = ERA4UIScreenVariant::Default);
