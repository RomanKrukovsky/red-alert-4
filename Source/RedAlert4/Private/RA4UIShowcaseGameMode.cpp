// Copyright (c) Red Alert 4 project.

#include "RA4UIShowcaseGameMode.h"

#include "RA4CampaignSelectWidget.h"
#include "RA4CampaignScreenWidget.h"
#include "RA4FactionHUDWidget.h"
#include "RA4MainMenuScreenWidget.h"
#include "RA4LobbyScreenWidget.h"
#include "RA4MissionFlowWidgets.h"
#include "RA4ShowcaseWidget.h"
#include "RA4SplashScreenWidget.h"
#include "RA4SkirmishSetupWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UnrealClient.h"

void ARA4UIShowcaseGameMode::BeginPlay()
{
    Super::BeginPlay();

    ShowInterface(GetWorld()->GetFirstPlayerController());
}

void ARA4UIShowcaseGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    ShowInterface(NewPlayer);
}

void ARA4UIShowcaseGameMode::ShowInterface(APlayerController* PlayerController)
{
    if (!PlayerController || !PlayerController->IsLocalController() || ActiveRootWidget)
    {
        return;
    }

    int32 RequestedScreen = 0;
    FParse::Value(FCommandLine::Get(), TEXT("RA4Screen="), RequestedScreen);
    ActiveReference = RequestedScreen;

    UUserWidget* RootWidget = nullptr;
    if (RequestedScreen == 1)
    {
        RootWidget = CreateWidget<URA4SplashScreenWidget>(
            PlayerController, URA4SplashScreenWidget::StaticClass());
    }
    else if (RequestedScreen == 0 || RequestedScreen == 2)
    {
        RootWidget = CreateWidget<URA4MainMenuScreenWidget>(
            PlayerController, URA4MainMenuScreenWidget::StaticClass());
    }
    else if (RequestedScreen == 3)
    {
        RootWidget = CreateWidget<URA4CampaignSelectWidget>(
            PlayerController, URA4CampaignSelectWidget::StaticClass());
    }
<<<<<<< HEAD
    else if (RequestedScreen >= 4 && RequestedScreen <= 7)
    {
        if (URA4CampaignScreenWidget* Campaign = CreateWidget<URA4CampaignScreenWidget>(
            PlayerController, URA4CampaignScreenWidget::StaticClass()))
        {
            const ERA4FactionTheme Factions[] = {
                ERA4FactionTheme::USSR,
                ERA4FactionTheme::Allies,
                ERA4FactionTheme::EasternCoalition,
                ERA4FactionTheme::Chronolegion
            };
            Campaign->ConfigureCampaign(Factions[RequestedScreen - 4]);
            RootWidget = Campaign;
        }
    }
    else if (RequestedScreen == 8)
    {
        RootWidget = CreateWidget<URA4MissionMapScreenWidget>(
            PlayerController, URA4MissionMapScreenWidget::StaticClass());
    }
    else if (RequestedScreen == 9)
    {
        RootWidget = CreateWidget<URA4BriefingScreenWidget>(
            PlayerController, URA4BriefingScreenWidget::StaticClass());
    }
    else if (RequestedScreen == 10)
    {
        RootWidget = CreateWidget<URA4VideoCommsScreenWidget>(
            PlayerController, URA4VideoCommsScreenWidget::StaticClass());
    }
    else if (RequestedScreen == 11)
    {
        if (URA4CampaignScreenWidget* Campaign = CreateWidget<URA4CampaignScreenWidget>(
            PlayerController, URA4CampaignScreenWidget::StaticClass()))
        {
            Campaign->ConfigureCampaign(
                ERA4FactionTheme::Allies, ERA4UIScreenVariant::AlliesAlternate);
            RootWidget = Campaign;
        }
    }
    else if (RequestedScreen == 12 || RequestedScreen == 19)
    {
        if (URA4LoadingScreenWidget* Loading = CreateWidget<URA4LoadingScreenWidget>(
            PlayerController, URA4LoadingScreenWidget::StaticClass()))
        {
            Loading->SetLoadingVariant(RequestedScreen == 19
                ? ERA4UIScreenVariant::LoadingBriefing
                : ERA4UIScreenVariant::Default);
            Loading->SetLoadingProgress(0.72f);
            RootWidget = Loading;
        }
    }
    else if (RequestedScreen == 18)
    {
        if (URA4CampaignScreenWidget* Campaign = CreateWidget<URA4CampaignScreenWidget>(
            PlayerController, URA4CampaignScreenWidget::StaticClass()))
        {
            Campaign->ConfigureCampaign(
                ERA4FactionTheme::EasternCoalition, ERA4UIScreenVariant::EasternDetail);
            RootWidget = Campaign;
        }
    }
    else if (RequestedScreen == 17)
    {
        RootWidget = CreateWidget<URA4LobbyScreenWidget>(
            PlayerController, URA4LobbyScreenWidget::StaticClass());
    }
    else if ((RequestedScreen >= 13 && RequestedScreen <= 16) ||
             (RequestedScreen >= 20 && RequestedScreen <= 24))
    {
        if (URA4FactionHUDWidget* HUD = CreateWidget<URA4FactionHUDWidget>(
            PlayerController, URA4FactionHUDWidget::StaticClass()))
        {
            HUD->ConfigureReference(RequestedScreen);
            RootWidget = HUD;
        }
    }
    else if (RequestedScreen == 100)
    {
        // Real production widget rather than a showcase mock: QA screenshots must
        // show the screen the player actually gets.
        RootWidget = CreateWidget<URA4SkirmishSetupWidget>(
            PlayerController, URA4SkirmishSetupWidget::StaticClass());
    }
    else
    {
        if (URA4ShowcaseWidget* ScreenWidget = CreateWidget<URA4ShowcaseWidget>(
            PlayerController, URA4ShowcaseWidget::StaticClass()))
        {
            ScreenWidget->SetInitialScreenForPresentation(RequestedScreen);
            RootWidget = ScreenWidget;
        }
    }

    if (RootWidget)
    {
        ActiveRootWidget = RootWidget;
        RootWidget->AddToViewport(0);
        PlayerController->bShowMouseCursor = true;
        PlayerController->SetInputMode(FInputModeGameAndUI());
        UE_LOG(LogTemp, Display, TEXT("RA4 UI showcase added to the viewport."));

        if (!bCaptureScheduled && FParse::Param(FCommandLine::Get(), TEXT("RA4CaptureUI")))
        {
            bCaptureScheduled = true;
            GetWorldTimerManager().SetTimer(
                CaptureTimer, this, &ARA4UIShowcaseGameMode::CaptureInterfaceForQA, 4.0f, false);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RA4 UI showcase could not be created."));
    }
}

void ARA4UIShowcaseGameMode::CaptureInterfaceForQA()
{
    int32 RequestedScreen = 0;
    FParse::Value(FCommandLine::Get(), TEXT("RA4Screen="), RequestedScreen);

    // One file per screen so a batch capture over all screens does not overwrite
    // itself; QA diffs need every screen side by side.
    const FString ScreenshotPath = FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("Screenshots/MacEditor"),
        FString::Printf(TEXT("RA4_UI_Reference_%02d.png"), ActiveReference));
    FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
    UE_LOG(LogTemp, Display, TEXT("RA4 UI QA screenshot requested: %s"), *ScreenshotPath);
}
