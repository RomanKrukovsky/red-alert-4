// Copyright (c) Red Alert 4 project.

#include "RA4HUDViewModel.h"

namespace
{
bool SelectionGroupsEqual(const TArray<FRA4SelectionGroup>& A, const TArray<FRA4SelectionGroup>& B)
{
    if (A.Num() != B.Num())
    {
        return false;
    }
    for (int32 Index = 0; Index < A.Num(); ++Index)
    {
        if (A[Index].ContentId != B[Index].ContentId || A[Index].Count != B[Index].Count ||
            !FMath::IsNearlyEqual(A[Index].HealthRatio, B[Index].HealthRatio) ||
            !A[Index].DisplayName.EqualTo(B[Index].DisplayName))
        {
            return false;
        }
    }
    return true;
}

bool ProductionEntriesEqual(const TArray<FRA4ProductionEntry>& A, const TArray<FRA4ProductionEntry>& B)
{
    if (A.Num() != B.Num())
    {
        return false;
    }
    for (int32 Index = 0; Index < A.Num(); ++Index)
    {
        const FRA4ProductionEntry& Left = A[Index];
        const FRA4ProductionEntry& Right = B[Index];
        if (Left.ContentId != Right.ContentId || Left.ProgressPercent != Right.ProgressPercent ||
            Left.bPaused != Right.bPaused || Left.bAwaitingPlacement != Right.bAwaitingPlacement ||
            Left.SlotIndex != Right.SlotIndex || !Left.DisplayName.EqualTo(Right.DisplayName))
        {
            return false;
        }
    }
    return true;
}

bool BuildOptionsEqual(const TArray<FRA4BuildOption>& A, const TArray<FRA4BuildOption>& B)
{
    if (A.Num() != B.Num())
    {
        return false;
    }
    for (int32 Index = 0; Index < A.Num(); ++Index)
    {
        const FRA4BuildOption& Left = A[Index];
        const FRA4BuildOption& Right = B[Index];
        if (Left.ContentId != Right.ContentId || Left.Cost != Right.Cost ||
            Left.Category != Right.Category || Left.bAvailable != Right.bAvailable ||
            Left.BlockReason != Right.BlockReason || !Left.DisplayName.EqualTo(Right.DisplayName))
        {
            return false;
        }
    }
    return true;
}

bool ObjectivesEqual(const TArray<FRA4HUDObjective>& A, const TArray<FRA4HUDObjective>& B)
{
    if (A.Num() != B.Num())
    {
        return false;
    }
    for (int32 Index = 0; Index < A.Num(); ++Index)
    {
        const FRA4HUDObjective& Left = A[Index];
        const FRA4HUDObjective& Right = B[Index];
        if (!Left.Label.EqualTo(Right.Label) || Left.bCompleted != Right.bCompleted ||
            Left.bOptional != Right.bOptional || Left.Current != Right.Current || Left.Target != Right.Target)
        {
            return false;
        }
    }
    return true;
}

bool AlertsEqual(const TArray<FRA4Alert>& A, const TArray<FRA4Alert>& B)
{
    if (A.Num() != B.Num())
    {
        return false;
    }
    for (int32 Index = 0; Index < A.Num(); ++Index)
    {
        const FRA4Alert& Left = A[Index];
        const FRA4Alert& Right = B[Index];
        if (!Left.Message.EqualTo(Right.Message) || Left.Severity != Right.Severity ||
            Left.RepeatCount != Right.RepeatCount || Left.bHasLocation != Right.bHasLocation ||
            Left.WorldLocation != Right.WorldLocation)
        {
            return false;
        }
    }
    return true;
}
} // namespace

URA4HUDViewModel::URA4HUDViewModel()
{
    Credits = 0;
    CreditsDelta = 0;
    MatchElapsedSeconds = 0;
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
    HarvesterCargo = 0;
    HarvesterCapacity = 0;
}

ERA4HUDChangeFlags URA4HUDViewModel::ApplySnapshot(const FRA4HUDSnapshotView& Snapshot)
{
    ERA4HUDChangeFlags Changes = ERA4HUDChangeFlags::None;

    if (Credits != Snapshot.Credits || CreditsDelta != Snapshot.CreditsDelta ||
        PowerProduced != Snapshot.PowerProduced || PowerConsumed != Snapshot.PowerConsumed ||
        bPowerShortage != Snapshot.bPowerShortage || CommandLimitUsed != Snapshot.SupplyUsed ||
        CommandLimitMax != Snapshot.SupplyCap || MatchElapsedSeconds != Snapshot.MatchElapsedSeconds)
    {
        Changes |= ERA4HUDChangeFlags::Resources;
    }

    if (SelectionKind != Snapshot.SelectionKind || SelectionCount != Snapshot.SelectionCount ||
        !FMath::IsNearlyEqual(SelectionHealthRatio, Snapshot.SelectionHealthRatio) ||
        PrimaryEntityName != Snapshot.PrimaryEntityName || bPrimaryOwned != Snapshot.bPrimaryOwned ||
        HarvesterCargo != Snapshot.HarvesterCargo || HarvesterCapacity != Snapshot.HarvesterCapacity ||
        !SelectionGroupsEqual(SelectionGroups, Snapshot.SelectionGroups))
    {
        Changes |= ERA4HUDChangeFlags::Selection;
    }

    if (!ProductionEntriesEqual(DetailedProductionQueue, Snapshot.ProductionQueue) ||
        !BuildOptionsEqual(BuildOptions, Snapshot.BuildOptions))
    {
        Changes |= ERA4HUDChangeFlags::Production;
    }
    if (!ObjectivesEqual(Objectives, Snapshot.Objectives))
    {
        Changes |= ERA4HUDChangeFlags::Objectives;
    }
    if (!AlertsEqual(Alerts, Snapshot.Alerts))
    {
        Changes |= ERA4HUDChangeFlags::Alerts;
    }

    if (EnumHasAnyFlags(Changes, ERA4HUDChangeFlags::Resources))
    {
        SetCredits(Snapshot.Credits);
        if (CreditsDelta != Snapshot.CreditsDelta)
        {
            CreditsDelta = Snapshot.CreditsDelta;
            UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CreditsDelta);
        }
        SetPower(Snapshot.PowerProduced, Snapshot.PowerConsumed);
        SetPowerShortage(Snapshot.bPowerShortage);
        SetCommandLimit(Snapshot.SupplyUsed, Snapshot.SupplyCap);
        if (MatchElapsedSeconds != Snapshot.MatchElapsedSeconds)
        {
            MatchElapsedSeconds = Snapshot.MatchElapsedSeconds;
            UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(MatchElapsedSeconds);
        }
    }

    if (EnumHasAnyFlags(Changes, ERA4HUDChangeFlags::Selection))
    {
        SetSelectionKind(Snapshot.SelectionKind);
        SetSelectionState(
            Snapshot.SelectionCount, Snapshot.SelectionHealthRatio,
            Snapshot.PrimaryEntityName, Snapshot.bPrimaryOwned);
        if (HarvesterCargo != Snapshot.HarvesterCargo)
        {
            HarvesterCargo = Snapshot.HarvesterCargo;
            UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HarvesterCargo);
        }
        if (HarvesterCapacity != Snapshot.HarvesterCapacity)
        {
            HarvesterCapacity = Snapshot.HarvesterCapacity;
            UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HarvesterCapacity);
        }
        SelectionGroups = Snapshot.SelectionGroups;
    }

    if (EnumHasAnyFlags(Changes, ERA4HUDChangeFlags::Production))
    {
        DetailedProductionQueue = Snapshot.ProductionQueue;
        BuildOptions = Snapshot.BuildOptions;
        TArray<FRA4ProductionQueueItem> CompactQueue;
        CompactQueue.Reserve(DetailedProductionQueue.Num());
        for (const FRA4ProductionEntry& Entry : DetailedProductionQueue)
        {
            FRA4ProductionQueueItem Item;
            Item.DisplayName = Entry.DisplayName;
            Item.Progress = FMath::Clamp(float(Entry.ProgressPercent) / 100.0f, 0.0f, 1.0f);
            Item.Quantity = 1;
            CompactQueue.Add(Item);
        }
        SetProductionQueue(CompactQueue);
    }

    if (EnumHasAnyFlags(Changes, ERA4HUDChangeFlags::Objectives))
    {
        Objectives = Snapshot.Objectives;
    }
    if (EnumHasAnyFlags(Changes, ERA4HUDChangeFlags::Alerts))
    {
        Alerts = Snapshot.Alerts;
    }

    if (Changes != ERA4HUDChangeFlags::None)
    {
        OnHUDChanged.Broadcast(Changes);
    }
    return Changes;
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
    if (ProductionQueue.Num() == InQueue.Num())
    {
        bool bEqual = true;
        for (int32 Index = 0; Index < InQueue.Num(); ++Index)
        {
            if (!ProductionQueue[Index].DisplayName.EqualTo(InQueue[Index].DisplayName) ||
                ProductionQueue[Index].Cost != InQueue[Index].Cost ||
                !FMath::IsNearlyEqual(ProductionQueue[Index].Progress, InQueue[Index].Progress) ||
                ProductionQueue[Index].Quantity != InQueue[Index].Quantity)
            {
                bEqual = false;
                break;
            }
        }
        if (bEqual)
        {
            return;
        }
    }
    ProductionQueue = InQueue;
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ProductionQueue);
}
