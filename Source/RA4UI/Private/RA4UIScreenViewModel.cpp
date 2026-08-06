// Copyright (c) Red Alert 4 project.

#include "RA4UIScreenViewModel.h"

#define LOCTEXT_NAMESPACE "RA4UIScreenViewModel"

URA4UIScreenViewModel::URA4UIScreenViewModel()
    : ModalTitle(LOCTEXT("DefaultModalTitle", "SYSTEM MESSAGE"))
    , ModalBody(LOCTEXT("DefaultModalBody", "Awaiting commander orders."))
{
    // Showcase filler, but it ships: RA4UI is in RedAlert4.Target.cs's ExtraModuleNames,
    // so these strings are in the Game build and get harvested into the shipped
    // localization catalogs. They therefore use the project's own unit names -- the same
    // ones RA4UIDataProviderSubsystem maps content ids to -- rather than trademarked
    // Red Alert 2 names, which the project is not licensed to use.
    ProductionQueue = {
        {LOCTEXT("QueueGranitMBT", "Tank «Granit»"), 1400, 0.62f, 1},
        {LOCTEXT("QueueRubezhRifleman", "Strelok «Rubezh»"), 100, 0.0f, 5},
    };

    LobbySlots = {
        {LOCTEXT("LobbyCommander", "COMMANDER"), ERA4FactionTheme::USSR, true},
        {LOCTEXT("LobbyWard", "ADMIRAL WARD"), ERA4FactionTheme::Allies, true},
        {LOCTEXT("LobbyGao", "GENERAL GAO"), ERA4FactionTheme::EasternCoalition, false},
        {LOCTEXT("LobbyChronos", "CHRONOS-07"), ERA4FactionTheme::Chronolegion, false},
    };
}

void URA4UIScreenViewModel::SetActiveScreen(const ERA4UIScreenId InScreen)
{
    if (ActiveScreen != InScreen)
    {
        ActiveScreen = InScreen;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveScreen);
    }
}

ERA4UIScreenId URA4UIScreenViewModel::GetActiveScreen() const
{
    return ActiveScreen;
}

void URA4UIScreenViewModel::SetSelectedFaction(const ERA4FactionTheme InFaction)
{
    if (SelectedFaction != InFaction)
    {
        SelectedFaction = InFaction;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedFaction);
    }
}

ERA4FactionTheme URA4UIScreenViewModel::GetSelectedFaction() const
{
    return SelectedFaction;
}

void URA4UIScreenViewModel::SetSelectedMission(const int32 InMissionIndex)
{
    const int32 ClampedMission = FMath::Max(0, InMissionIndex);
    if (SelectedMission != ClampedMission)
    {
        SelectedMission = ClampedMission;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedMission);
    }
}

int32 URA4UIScreenViewModel::GetSelectedMission() const
{
    return SelectedMission;
}

void URA4UIScreenViewModel::SetLoadingProgress(const float InProgress)
{
    const float ClampedProgress = FMath::Clamp(InProgress, 0.0f, 1.0f);
    if (!FMath::IsNearlyEqual(LoadingProgress, ClampedProgress))
    {
        LoadingProgress = ClampedProgress;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(LoadingProgress);
    }
}

float URA4UIScreenViewModel::GetLoadingProgress() const
{
    return LoadingProgress;
}

void URA4UIScreenViewModel::ShowModal(const FText& InTitle, const FText& InBody)
{
    ModalTitle = InTitle;
    ModalBody = InBody;
    bModalVisible = true;
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ModalTitle);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ModalBody);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bModalVisible);
}

void URA4UIScreenViewModel::CloseModal()
{
    if (bModalVisible)
    {
        bModalVisible = false;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bModalVisible);
    }
}

bool URA4UIScreenViewModel::IsModalVisible() const
{
    return bModalVisible;
}

FText URA4UIScreenViewModel::GetModalTitle() const
{
    return ModalTitle;
}

FText URA4UIScreenViewModel::GetModalBody() const
{
    return ModalBody;
}

void URA4UIScreenViewModel::SetProductionQueue(const TArray<FRA4ProductionQueueItem>& InQueue)
{
    ProductionQueue = InQueue;
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ProductionQueue);
}

const TArray<FRA4ProductionQueueItem>& URA4UIScreenViewModel::GetProductionQueue() const
{
    return ProductionQueue;
}

void URA4UIScreenViewModel::SetLobbySlots(const TArray<FRA4LobbyPlayerSlot>& InSlots)
{
    LobbySlots = InSlots;
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(LobbySlots);
}

const TArray<FRA4LobbyPlayerSlot>& URA4UIScreenViewModel::GetLobbySlots() const
{
    return LobbySlots;
}

#undef LOCTEXT_NAMESPACE
