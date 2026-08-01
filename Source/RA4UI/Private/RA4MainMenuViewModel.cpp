// Copyright (c) Red Alert 4 project.

#include "RA4MainMenuViewModel.h"

#define LOCTEXT_NAMESPACE "RA4MainMenuViewModel"

URA4MainMenuViewModel::URA4MainMenuViewModel()
{
    PlayerProfileName = LOCTEXT("DefaultCommander", "COMMANDER");
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

#undef LOCTEXT_NAMESPACE
