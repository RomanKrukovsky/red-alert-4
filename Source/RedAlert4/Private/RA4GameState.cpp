// Copyright (c) Red Alert 4 project. Canonical GameState implementation.
#include "RA4GameState.h"
#include "Net/UnrealNetwork.h"

ARA4GameState::ARA4GameState()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ARA4GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARA4GameState, CurrentSimTick);
    DOREPLIFETIME(ARA4GameState, MatchPhase);
    DOREPLIFETIME(ARA4GameState, WinnerPlayerId);
}
