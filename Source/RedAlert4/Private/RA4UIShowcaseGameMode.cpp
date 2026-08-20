// Copyright (c) Red Alert 4 project.

#include "RA4UIShowcaseGameMode.h"

#include "RA4CampaignSelectWidget.h"
#include "RA4MainMenuScreenWidget.h"
#include "RA4ShowcaseWidget.h"
#include "RA4SplashScreenWidget.h"
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
    const FString ScreenshotPath = FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("Screenshots/MacEditor"),
        FString::Printf(TEXT("RA4_UI_Reference_%02d.png"), ActiveReference));
    FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
    UE_LOG(LogTemp, Display, TEXT("RA4 UI QA screenshot requested: %s"), *ScreenshotPath);
}
