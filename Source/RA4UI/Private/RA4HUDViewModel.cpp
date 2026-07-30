// Copyright (c) Red Alert 4 project.

#include "RA4HUDViewModel.h"

URA4HUDViewModel::URA4HUDViewModel()
{
    Credits = 0;
    PowerRatio = 1.0f;
}

void URA4HUDViewModel::SetCredits(int32 InCredits)
{
    if (Credits != InCredits)
    {
        Credits = InCredits;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Credits);
    }
}

int32 URA4HUDViewModel::GetCredits() const
{
    return Credits;
}

void URA4HUDViewModel::SetPower(int32 InPowerProvided, int32 InPowerDrained)
{
    float NewRatio = 0.0f;
    if (InPowerProvided > 0)
    {
        NewRatio = FMath::Clamp((float)InPowerDrained / (float)InPowerProvided, 0.0f, 2.0f);
    }
    
    if (PowerRatio != NewRatio)
    {
        PowerRatio = NewRatio;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(PowerRatio);
    }
}

float URA4HUDViewModel::GetPowerRatio() const
{
    return PowerRatio;
}

void URA4HUDViewModel::SetPowerShortage(bool bShortage)
{
    if (bPowerShortage != bShortage)
    {
        bPowerShortage = bShortage;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bPowerShortage);
    }
}

bool URA4HUDViewModel::IsPowerShortage() const
{
    return bPowerShortage;
}

void URA4HUDViewModel::SetSelectionState(int32 Count, float HealthRatio, const FString& EntityName, bool bOwned)
{
    bool bChanged = false;
    if (SelectionCount != Count)
    {
        SelectionCount = Count;
        bChanged = true;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectionCount);
    }
    if (!FMath::IsNearlyEqual(SelectionHealthRatio, HealthRatio))
    {
        SelectionHealthRatio = HealthRatio;
        bChanged = true;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectionHealthRatio);
    }
    if (PrimaryEntityName != EntityName)
    {
        PrimaryEntityName = EntityName;
        bChanged = true;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(PrimaryEntityName);
    }
    if (bPrimaryOwned != bOwned)
    {
        bPrimaryOwned = bOwned;
        bChanged = true;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bPrimaryOwned);
    }
}
