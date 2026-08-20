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
    switch (GetScreenContract().InputMode)
    {
    case ERA4UIInputMode::GameOnly:
        return FUIInputConfig(
            ECommonInputMode::Game,
            EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
    case ERA4UIInputMode::UIOnly:
        return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
    case ERA4UIInputMode::GameAndUI:
        return FUIInputConfig(ECommonInputMode::All, EMouseCaptureMode::NoCapture);
    default:
        checkNoEntry();
        return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
    }
}

void URA4ActivatableWidget::SetScreenIdentity(
    const ERA4UIScreenId InScreenId,
    const ERA4UIScreenVariant InVariant)
{
    ScreenId = InScreenId;
    ScreenVariant = InVariant;
}

FRA4UIScreenContract URA4ActivatableWidget::GetScreenContract() const
{
    return ResolveScreenContract(ScreenId, ScreenVariant);
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
