// Copyright (c) Red Alert 4 project.

#include "RA4UIScreenHost.h"

#include "RA4UIRouterSubsystem.h"
#include "RA4SplashScreenWidget.h"
#include "RA4MainMenuScreenWidget.h"
#include "RA4CampaignSelectWidget.h"
#include "RA4CampaignScreenWidget.h"
#include "RA4MissionFlowWidgets.h"
#include "RA4LobbyScreenWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

void URA4UIScreenHost::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (URA4UIRouterSubsystem* Router = GetGameInstance()->GetSubsystem<URA4UIRouterSubsystem>())
    {
        Router->OnScreenChanged.AddDynamic(this, &URA4UIScreenHost::HandleScreenChanged);
    }
}

void URA4UIScreenHost::Deinitialize()
{
    if (URA4UIRouterSubsystem* Router = GetGameInstance()->GetSubsystem<URA4UIRouterSubsystem>())
    {
        Router->OnScreenChanged.RemoveDynamic(this, &URA4UIScreenHost::HandleScreenChanged);
    }
    Super::Deinitialize();
}

void URA4UIScreenHost::ShowInitialScreen(APlayerController* PlayerController)
{
    if (!PlayerController || ActiveMenuWidget)
    {
        return;
    }

    URA4UIRouterSubsystem* Router = GetGameInstance()->GetSubsystem<URA4UIRouterSubsystem>();
    const ERA4UIScreenId Current = Router && Router->GetScreenViewModel()
        ? Router->GetScreenViewModel()->GetActiveScreen()
        : ERA4UIScreenId::Splash;

    SwapToScreen(PlayerController, Current);
}

void URA4UIScreenHost::SwapToScreen(
    APlayerController* PlayerController,
    ERA4UIScreenId ScreenId)
{
    if (!PlayerController)
    {
        return;
    }

    const TSubclassOf<UUserWidget> WidgetClass = ResolveWidgetClassForScreen(ScreenId);
    if (!WidgetClass)
    {
        // Not a host-owned screen (e.g. in-game HUD): leave any existing menu in
        // place so the match controller keeps its chrome.
        return;
    }

    if (ActiveMenuWidget)
    {
        ActiveMenuWidget->RemoveFromParent();
        ActiveMenuWidget = nullptr;
    }

    UUserWidget* NewWidget = CreateWidget(PlayerController, WidgetClass);
    if (!NewWidget)
    {
        return;
    }

    // Faction campaign screens need the faction configured before they build.
    if (URA4CampaignScreenWidget* Campaign = Cast<URA4CampaignScreenWidget>(NewWidget))
    {
        ERA4FactionTheme Faction = ERA4FactionTheme::EurasianPact;
        switch (ScreenId)
        {
        case ERA4UIScreenId::EurasianCampaign:    Faction = ERA4FactionTheme::EurasianPact; break;
        case ERA4UIScreenId::AtlanticCampaign:    Faction = ERA4FactionTheme::AtlanticAlliance; break;
        case ERA4UIScreenId::EasternCampaign:     Faction = ERA4FactionTheme::EasternCoalition; break;
        case ERA4UIScreenId::PacificCampaign:     Faction = ERA4FactionTheme::PacificPact; break;
        case ERA4UIScreenId::IndependentCampaign: Faction = ERA4FactionTheme::Independent; break;
        default: break;
        }
        Campaign->ConfigureCampaign(Faction);
    }

    NewWidget->AddToViewport(0);
    ActiveMenuWidget = NewWidget;

    PlayerController->bShowMouseCursor = true;
    PlayerController->SetInputMode(FInputModeGameAndUI());
}

TSubclassOf<UUserWidget> URA4UIScreenHost::ResolveWidgetClassForScreen(
    const ERA4UIScreenId ScreenId) const
{
    switch (ScreenId)
    {
    case ERA4UIScreenId::Splash:           return URA4SplashScreenWidget::StaticClass();
    case ERA4UIScreenId::MainMenu:         return URA4MainMenuScreenWidget::StaticClass();
    case ERA4UIScreenId::CampaignSelect:   return URA4CampaignSelectWidget::StaticClass();
    case ERA4UIScreenId::EurasianCampaign:
    case ERA4UIScreenId::AtlanticCampaign:
    case ERA4UIScreenId::EasternCampaign:
    case ERA4UIScreenId::PacificCampaign:
    case ERA4UIScreenId::IndependentCampaign:
        return URA4CampaignScreenWidget::StaticClass();
    case ERA4UIScreenId::MissionMap:      return URA4MissionMapScreenWidget::StaticClass();
    case ERA4UIScreenId::Briefing:        return URA4BriefingScreenWidget::StaticClass();
    case ERA4UIScreenId::VideoComms:      return URA4VideoCommsScreenWidget::StaticClass();
    case ERA4UIScreenId::Loading:         return URA4LoadingScreenWidget::StaticClass();
    case ERA4UIScreenId::MultiplayerLobby: return URA4LobbyScreenWidget::StaticClass();
    default:
        // Encyclopedia/TechTree/Mods/Settings/HUDs are not host-owned.
        return nullptr;
    }
}

void URA4UIScreenHost::HandleScreenChanged(const ERA4UIScreenId NewScreen)
{
    if (const UWorld* World = GetGameInstance()->GetWorld())
    {
        if (APlayerController* PlayerController = World->GetFirstPlayerController())
        {
            SwapToScreen(PlayerController, NewScreen);
        }
    }
}