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
    else if (RequestedScreen == 30)
    {
        if (URA4CampaignSelectWidget* Select = CreateWidget<URA4CampaignSelectWidget>(
            PlayerController, URA4CampaignSelectWidget::StaticClass()))
        {
            // Each step of the selection path has its own reference frame, so QA
            // capture must be able to open the screen directly on a given step.
            // The wizard lives outside the 1..19 plate range: references 3..6
            // belong to the faction campaign screens.
            int32 RequestedStep = 0;
            FParse::Value(FCommandLine::Get(), TEXT("RA4Step="), RequestedStep);
            Select->SetInitialStep(static_cast<ERA4CampaignSelectStep>(
                FMath::Clamp(RequestedStep, 0, 2)));

            int32 RequestedBloc = 0;
            int32 RequestedCountry = 0;
            FParse::Value(FCommandLine::Get(), TEXT("RA4Bloc="), RequestedBloc);
            FParse::Value(FCommandLine::Get(), TEXT("RA4Country="), RequestedCountry);
            Select->SetInitialSelection(RequestedBloc, RequestedCountry);
            RootWidget = Select;
        }
    }
    else if (RequestedScreen >= 3 && RequestedScreen <= 6)
    {
        if (URA4CampaignScreenWidget* Campaign = CreateWidget<URA4CampaignScreenWidget>(
            PlayerController, URA4CampaignScreenWidget::StaticClass()))
        {
            const ERA4FactionTheme Factions[] = {
                ERA4FactionTheme::EurasianPact,
                ERA4FactionTheme::AtlanticAlliance,
                ERA4FactionTheme::EasternCoalition,
                ERA4FactionTheme::PacificPact
            };
            Campaign->ConfigureCampaign(Factions[RequestedScreen - 3]);
            RootWidget = Campaign;
        }
    }
    else if (RequestedScreen == 19)
    {
        if (URA4CampaignScreenWidget* Campaign = CreateWidget<URA4CampaignScreenWidget>(
            PlayerController, URA4CampaignScreenWidget::StaticClass()))
        {
            Campaign->ConfigureCampaign(ERA4FactionTheme::Independent);
            RootWidget = Campaign;
        }
    }
    else if (RequestedScreen == 7)
    {
        RootWidget = CreateWidget<URA4MissionMapScreenWidget>(
            PlayerController, URA4MissionMapScreenWidget::StaticClass());
    }
    else if (RequestedScreen == 8)
    {
        RootWidget = CreateWidget<URA4BriefingScreenWidget>(
            PlayerController, URA4BriefingScreenWidget::StaticClass());
    }
    else if (RequestedScreen == 9)
    {
        RootWidget = CreateWidget<URA4VideoCommsScreenWidget>(
            PlayerController, URA4VideoCommsScreenWidget::StaticClass());
    }
    else if (RequestedScreen == 10)
    {
        if (URA4LoadingScreenWidget* Loading = CreateWidget<URA4LoadingScreenWidget>(
            PlayerController, URA4LoadingScreenWidget::StaticClass()))
        {
            Loading->SetLoadingProgress(0.72f);
            RootWidget = Loading;
        }
    }
    else if (RequestedScreen == 11)
    {
        RootWidget = CreateWidget<URA4LobbyScreenWidget>(
            PlayerController, URA4LobbyScreenWidget::StaticClass());
    }
    else if (RequestedScreen >= 12 && RequestedScreen <= 18)
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
    // The pointer starts at the viewport centre, which leaves whatever panel sits
    // there stuck in its hover state. A QA frame must show the resting look, so
    // the cursor is parked in the corner first.
    if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        PlayerController->bShowMouseCursor = false;
    }

    // Parking the cursor is not enough: whatever sits under it keeps its hover
    // brush, and a direction whose hover colour is bright reads as a solid fill.
    // Making the tree hit-test invisible for the frame forces every widget back
    // to its resting state, which is what a reference comparison needs.
    if (ActiveRootWidget)
    {
        ActiveRootWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
    }

    FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
    UE_LOG(LogTemp, Display, TEXT("RA4 UI QA screenshot requested: %s"), *ScreenshotPath);

    // A batch capture walks every reference in turn, so the process must close
    // itself once the frame is on disk. The delay lets the screenshot request
    // finish flushing before the exit is issued.
    if (FParse::Param(FCommandLine::Get(), TEXT("RA4ExitAfterCapture")))
    {
        GetWorldTimerManager().SetTimer(
            ExitTimer,
            FTimerDelegate::CreateLambda([]()
            {
                UE_LOG(LogTemp, Display, TEXT("RA4 UI QA capture complete, exiting."));
                FPlatformMisc::RequestExit(false);
            }),
            2.0f,
            false);
    }
}
