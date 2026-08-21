// Copyright (c) Red Alert 4 project.

#include "RA4UIScreenViewModel.h"

#define LOCTEXT_NAMESPACE "RA4UIScreenViewModel"

URA4UIScreenViewModel::URA4UIScreenViewModel()
    : ModalTitle(LOCTEXT("DefaultModalTitle", "СИСТЕМНОЕ СООБЩЕНИЕ"))
    , ModalBody(LOCTEXT("DefaultModalBody", "Ожидание приказа командующего."))
{
    ProductionQueue = {
        {LOCTEXT("QueueGranitMBT", "Танк «Гранит»"), 1400, 0.62f, 1},
        {LOCTEXT("QueueRubezhRifleman", "Стрелок «Рубеж»"), 100, 0.0f, 5},
    };

    LobbySlots = {
        {LOCTEXT("LobbySokolov", "ГЕНЕРАЛ СОКОЛОВ"), ERA4FactionTheme::EurasianPact, true},
        {LOCTEXT("LobbyWard", "АДМИРАЛ УОРД"), ERA4FactionTheme::AtlanticAlliance, true},
        {LOCTEXT("LobbyZhang", "ГЕНЕРАЛ ЧЖАН"), ERA4FactionTheme::EasternCoalition, false},
        {LOCTEXT("LobbyTanaka", "КОМАНДУЮЩИЙ ТАНАКА"), ERA4FactionTheme::PacificPact, false},
        {LOCTEXT("LobbyRezai", "КОМАНДИР РЕЗАИ"), ERA4FactionTheme::Independent, false},
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
