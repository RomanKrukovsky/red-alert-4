// Copyright (c) Red Alert 4 project.

#include "RA4MainMenuViewModel.h"

#define LOCTEXT_NAMESPACE "RA4MainMenuViewModel"

URA4MainMenuViewModel::URA4MainMenuViewModel()
{
    PlayerProfileName = LOCTEXT("DefaultCommander", "КОМАНДИР");
    MenuEntries = {
        {LOCTEXT("Campaign", "КАМПАНИЯ"), ERA4UIScreenId::CampaignSelect, ERA4MainMenuAction::Campaign, true},
        {LOCTEXT("Multiplayer", "СЕТЕВАЯ ИГРА"), ERA4UIScreenId::MultiplayerLobby, ERA4MainMenuAction::Multiplayer, false},
        {LOCTEXT("Skirmish", "СХВАТКА"), ERA4UIScreenId::MainMenu, ERA4MainMenuAction::Skirmish, false},
        {LOCTEXT("Editor", "РЕДАКТОР"), ERA4UIScreenId::TechTree, ERA4MainMenuAction::Editor, false},
        {LOCTEXT("Encyclopedia", "ЭНЦИКЛОПЕДИЯ"), ERA4UIScreenId::Encyclopedia, ERA4MainMenuAction::Encyclopedia, false},
        {LOCTEXT("Modifications", "МОДИФИКАЦИИ"), ERA4UIScreenId::Mods, ERA4MainMenuAction::Modifications, false},
        {LOCTEXT("Settings", "НАСТРОЙКИ"), ERA4UIScreenId::Settings, ERA4MainMenuAction::Settings, false},
        {LOCTEXT("Exit", "ВЫХОД"), ERA4UIScreenId::Splash, ERA4MainMenuAction::Exit, false},
    };
}

void URA4MainMenuViewModel::SetPlayerProfileName(const FText& InName)
{
    if (!PlayerProfileName.EqualTo(InName))
    {
        PlayerProfileName = InName;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(PlayerProfileName);
    }
}

FText URA4MainMenuViewModel::GetPlayerProfileName() const
{
    return PlayerProfileName;
}

const TArray<FRA4MainMenuEntry>& URA4MainMenuViewModel::GetMenuEntries() const
{
    return MenuEntries;
}

void URA4MainMenuViewModel::SetSelectedMenuIndex(const int32 InIndex)
{
    if (MenuEntries.IsEmpty())
    {
        SelectedMenuIndex = 0;
        return;
    }

    const int32 ClampedIndex = FMath::Clamp(InIndex, 0, MenuEntries.Num() - 1);
    if (SelectedMenuIndex == ClampedIndex)
    {
        return;
    }

    MenuEntries[SelectedMenuIndex].bSelected = false;
    SelectedMenuIndex = ClampedIndex;
    MenuEntries[SelectedMenuIndex].bSelected = true;
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedMenuIndex);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(MenuEntries);
}

int32 URA4MainMenuViewModel::GetSelectedMenuIndex() const
{
    return SelectedMenuIndex;
}

void URA4MainMenuViewModel::ExecuteAction(const ERA4MainMenuAction Action)
{
    OnActionRequested.Broadcast(Action);

    switch (Action)
    {
    case ERA4MainMenuAction::Campaign:
    case ERA4MainMenuAction::Multiplayer:
    case ERA4MainMenuAction::Editor:
    case ERA4MainMenuAction::Encyclopedia:
    case ERA4MainMenuAction::Modifications:
        break;
    case ERA4MainMenuAction::Skirmish:
        OnNewSkirmishClicked.Broadcast();
        break;
    case ERA4MainMenuAction::Settings:
        OnSettingsClicked.Broadcast();
        break;
    case ERA4MainMenuAction::Exit:
        OnExitClicked.Broadcast();
        break;
    default:
        checkNoEntry();
        break;
    }
}

void URA4MainMenuViewModel::ExecuteNewSkirmishCommand()
{
    ExecuteAction(ERA4MainMenuAction::Skirmish);
}

void URA4MainMenuViewModel::ExecuteSettingsCommand()
{
    ExecuteAction(ERA4MainMenuAction::Settings);
}

void URA4MainMenuViewModel::ExecuteExitCommand()
{
    ExecuteAction(ERA4MainMenuAction::Exit);
}

#undef LOCTEXT_NAMESPACE
