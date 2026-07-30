// Copyright (c) Red Alert 4 project. Canonical GameState for match state replication.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "RA4GameState.generated.h"

UCLASS()
class REDALERT4_API ARA4GameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ARA4GameState();

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    int32 CurrentSimTick = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    uint8 MatchPhase = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
    uint8 WinnerPlayerId = 255;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
