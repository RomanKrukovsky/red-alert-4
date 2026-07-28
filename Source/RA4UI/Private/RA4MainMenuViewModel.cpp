// Copyright (c) Red Alert 4 project.

#include "RA4MainMenuViewModel.h"

URA4MainMenuViewModel::URA4MainMenuViewModel()
{
    PlayerProfileName = TEXT("Commander");
}

void URA4MainMenuViewModel::SetPlayerProfileName(const FString& InName)
{
    if (PlayerProfileName != InName)
    {
        PlayerProfileName = InName;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetPlayerProfileName);
    }
}

FString URA4MainMenuViewModel::GetPlayerProfileName() const
{
    return PlayerProfileName;
}
