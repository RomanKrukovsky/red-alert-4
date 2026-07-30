// Copyright (c) Red Alert 4 project. Canonical PlayerState implementation.
#include "RA4PlayerState.h"
#include "Net/UnrealNetwork.h"

ARA4PlayerState::ARA4PlayerState()
{
}

void ARA4PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARA4PlayerState, FactionId);
    DOREPLIFETIME(ARA4PlayerState, Credits);
    DOREPLIFETIME(ARA4PlayerState, PowerProduced);
    DOREPLIFETIME(ARA4PlayerState, PowerConsumed);
    DOREPLIFETIME(ARA4PlayerState, bDefeated);
}
