// Copyright (c) Red Alert 4 project.

#include "RA4UINavigationService.h"

#include "RA4UIInputRouter.h"
#include "RA4UIRouterSubsystem.h"
#include "RA4UIScreenContract.h"

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

    const ERA4UIInputMode TargetInputMode = ResolveScreenContract(Screen).InputMode;

    InputRouter->SetInputMode(TargetInputMode);
    OnNavigationStateChanged.Broadcast(Screen, TargetInputMode);
}
