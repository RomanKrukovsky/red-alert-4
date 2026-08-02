// Copyright (c) Red Alert 4 project.

#include "RA4UINavigationService.h"

#include "RA4UIInputRouter.h"
#include "RA4UIRouterSubsystem.h"

void URA4UINavigationService::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void URA4UINavigationService::Deinitialize()
{
    Super::Deinitialize();
}

URA4UIRouterSubsystem* URA4UINavigationService::GetRouterSubsystem() const
{
    if (UGameInstance* GI = GetGameInstance())
    {
        return GI->GetSubsystem<URA4UIRouterSubsystem>();
    }
    return nullptr;
}

URA4UIInputRouter* URA4UINavigationService::GetInputRouter() const
{
    if (UGameInstance* GI = GetGameInstance())
    {
        return GI->GetSubsystem<URA4UIInputRouter>();
    }
    return nullptr;
}

void URA4UINavigationService::NavigateToScreen(ERA4UIScreenId TargetScreen, bool bAddToHistory)
{
    if (URA4UIRouterSubsystem* Router = GetRouterSubsystem())
    {
        Router->NavigateTo(TargetScreen, bAddToHistory);
        ApplyInputModeForScreen(TargetScreen);
    }
}

bool URA4UINavigationService::NavigateBack()
{
    if (URA4UIRouterSubsystem* Router = GetRouterSubsystem())
    {
        bool bNavigated = Router->NavigateBack();
        if (bNavigated)
        {
            ApplyInputModeForScreen(GetActiveScreen());
        }
        return bNavigated;
    }
    return false;
}

ERA4UIScreenId URA4UINavigationService::GetActiveScreen() const
{
    if (URA4UIRouterSubsystem* Router = GetRouterSubsystem())
    {
        if (URA4UIScreenViewModel* VM = Router->GetScreenViewModel())
        {
            return VM->GetActiveScreen();
        }
    }
    return ERA4UIScreenId::Splash;
}

void URA4UINavigationService::ShowModal(const FText& Title, const FText& Body)
{
    if (URA4UIRouterSubsystem* Router = GetRouterSubsystem())
    {
        Router->ShowModal(Title, Body);
    }
}

void URA4UINavigationService::CloseModal()
{
    if (URA4UIRouterSubsystem* Router = GetRouterSubsystem())
    {
        Router->CloseModal();
    }
}

void URA4UINavigationService::ApplyInputModeForScreen(ERA4UIScreenId Screen)
{
    URA4UIInputRouter* InputRouter = GetInputRouter();
    if (!InputRouter)
    {
        return;
    }

    ERA4UIInputMode TargetInputMode = ERA4UIInputMode::UIOnly;

    switch (Screen)
    {
    case ERA4UIScreenId::SovietHud:
    case ERA4UIScreenId::AlliesHud:
    case ERA4UIScreenId::EasternHud:
    case ERA4UIScreenId::ChronoHud:
        TargetInputMode = ERA4UIInputMode::GameAndUI;
        break;

    case ERA4UIScreenId::Splash:
    case ERA4UIScreenId::MainMenu:
    case ERA4UIScreenId::CampaignSelect:
    case ERA4UIScreenId::SovietCampaign:
    case ERA4UIScreenId::AlliesCampaign:
    case ERA4UIScreenId::EasternCampaign:
    case ERA4UIScreenId::ChronoCampaign:
    case ERA4UIScreenId::MissionMap:
    case ERA4UIScreenId::Briefing:
    case ERA4UIScreenId::VideoComms:
    case ERA4UIScreenId::Loading:
    case ERA4UIScreenId::Pause:
    case ERA4UIScreenId::Victory:
    case ERA4UIScreenId::MultiplayerLobby:
    case ERA4UIScreenId::Encyclopedia:
    case ERA4UIScreenId::TechTree:
    case ERA4UIScreenId::Mods:
    case ERA4UIScreenId::Settings:
    default:
        TargetInputMode = ERA4UIInputMode::UIOnly;
        break;
    }

    InputRouter->SetInputMode(TargetInputMode);
    OnNavigationStateChanged.Broadcast(Screen, TargetInputMode);
}
