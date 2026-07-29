// Copyright (c) Red Alert 4 project.

#include "RA4UIRouterSubsystem.h"

void URA4UIRouterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ScreenViewModel = NewObject<URA4UIScreenViewModel>(this);
}

void URA4UIRouterSubsystem::Deinitialize()
{
    NavigationHistory.Reset();
    ScreenViewModel = nullptr;
    Super::Deinitialize();
}

URA4UIScreenViewModel* URA4UIRouterSubsystem::GetScreenViewModel() const
{
    return ScreenViewModel;
}

void URA4UIRouterSubsystem::NavigateTo(const ERA4UIScreenId TargetScreen, const bool bAddToHistory)
{
    if (!ScreenViewModel || ScreenViewModel->GetActiveScreen() == TargetScreen)
    {
        return;
    }

    if (bAddToHistory)
    {
        NavigationHistory.Add(ScreenViewModel->GetActiveScreen());
    }

    ScreenViewModel->CloseModal();
    ScreenViewModel->SetActiveScreen(TargetScreen);
    OnScreenChanged.Broadcast(TargetScreen);
}

bool URA4UIRouterSubsystem::NavigateBack()
{
    if (!ScreenViewModel)
    {
        return false;
    }

    if (ScreenViewModel->IsModalVisible())
    {
        ScreenViewModel->CloseModal();
        return true;
    }

    if (NavigationHistory.IsEmpty())
    {
        return false;
    }

    const ERA4UIScreenId PreviousScreen = NavigationHistory.Pop();
    NavigateTo(PreviousScreen, false);
    return true;
}

void URA4UIRouterSubsystem::ShowModal(const FText& Title, const FText& Body)
{
    if (ScreenViewModel)
    {
        ScreenViewModel->ShowModal(Title, Body);
    }
}

void URA4UIRouterSubsystem::CloseModal()
{
    if (ScreenViewModel)
    {
        ScreenViewModel->CloseModal();
    }
}
