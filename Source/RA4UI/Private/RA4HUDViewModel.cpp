// Copyright (c) Red Alert 4 project.

#include "RA4HUDViewModel.h"

URA4HUDViewModel::URA4HUDViewModel()
{
    Credits = 0;
    PowerProduced = 0;
    PowerConsumed = 0;
    PowerRatio = 1.0f;
    bPowerShortage = false;
    CommandLimitUsed = 0;
    CommandLimitMax = 0;
    SelectionCount = 0;
    SelectionHealthRatio = 0.0f;
    bPrimaryOwned = false;
    SelectionKind = ERA4SelectionKind::Empty;
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
    SetPowerProduced(InPowerProvided);
    SetPowerConsumed(InPowerDrained);

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

void URA4HUDViewModel::SetPowerProduced(int32 InPowerProduced)
{
    if (PowerProduced != InPowerProduced)
    {
        PowerProduced = InPowerProduced;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(PowerProduced);
    }
}

void URA4HUDViewModel::SetPowerConsumed(int32 InPowerConsumed)
{
    if (PowerConsumed != InPowerConsumed)
    {
        PowerConsumed = InPowerConsumed;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(PowerConsumed);
    }
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

void URA4HUDViewModel::SetCommandLimit(int32 InUsed, int32 InMax)
{
    SetCommandLimitUsed(InUsed);
    SetCommandLimitMax(InMax);
}

void URA4HUDViewModel::SetCommandLimitUsed(int32 InUsed)
{
    if (CommandLimitUsed != InUsed)
    {
        CommandLimitUsed = InUsed;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CommandLimitUsed);
    }
}

void URA4HUDViewModel::SetCommandLimitMax(int32 InMax)
{
    if (CommandLimitMax != InMax)
    {
        CommandLimitMax = InMax;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CommandLimitMax);
    }
}

void URA4HUDViewModel::SetSelectionState(int32 Count, float HealthRatio, const FString& EntityName, bool bOwned)
{
    if (SelectionCount != Count)
    {
        SelectionCount = Count;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectionCount);
    }
    if (!FMath::IsNearlyEqual(SelectionHealthRatio, HealthRatio))
    {
        SelectionHealthRatio = HealthRatio;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectionHealthRatio);
    }
    if (PrimaryEntityName != EntityName)
    {
        PrimaryEntityName = EntityName;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(PrimaryEntityName);
    }
    if (bPrimaryOwned != bOwned)
    {
        bPrimaryOwned = bOwned;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bPrimaryOwned);
    }
}

void URA4HUDViewModel::SetSelectionKind(ERA4SelectionKind InKind)
{
    if (SelectionKind != InKind)
    {
        SelectionKind = InKind;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectionKind);
    }
}

void URA4HUDViewModel::SetProductionQueue(const TArray<FRA4ProductionQueueItem>& InQueue)
{
    ProductionQueue = InQueue;
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ProductionQueue);
}

