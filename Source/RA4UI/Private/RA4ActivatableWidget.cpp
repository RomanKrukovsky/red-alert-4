// Copyright (c) Red Alert 4 project.

#include "RA4ActivatableWidget.h"

#include "RA4UIRouterSubsystem.h"
#include "Engine/GameInstance.h"

URA4ActivatableWidget::URA4ActivatableWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // By default, UI doesn't pause the game and allows routing
}

TOptional<FUIInputConfig> URA4ActivatableWidget::GetDesiredInputConfig() const
{
    // Default config for UI elements: Game and UI, Mouse visible
    return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

URA4UIScreenViewModel* URA4ActivatableWidget::GetScreenViewModel() const
{
    const UGameInstance* GameInstance = GetGameInstance();
    const URA4UIRouterSubsystem* Router = GameInstance ? GameInstance->GetSubsystem<URA4UIRouterSubsystem>() : nullptr;
    return Router ? Router->GetScreenViewModel() : nullptr;
}

void URA4ActivatableWidget::NavigateToScreen(const ERA4UIScreenId TargetScreen)
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (URA4UIRouterSubsystem* Router = GameInstance->GetSubsystem<URA4UIRouterSubsystem>())
        {
            Router->NavigateTo(TargetScreen);
        }
    }
}

bool URA4ActivatableWidget::NavigateBack()
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (URA4UIRouterSubsystem* Router = GameInstance->GetSubsystem<URA4UIRouterSubsystem>())
        {
            return Router->NavigateBack();
        }
    }

    return false;
}

void URA4ActivatableWidget::CloseActiveModal()
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (URA4UIRouterSubsystem* Router = GameInstance->GetSubsystem<URA4UIRouterSubsystem>())
        {
            Router->CloseModal();
        }
    }
}
