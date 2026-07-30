// Copyright (c) Red Alert 4 project. Canonical PlayerState for player replication.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "RA4PlayerState.generated.h"

UCLASS()
class REDALERT4_API ARA4PlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    ARA4PlayerState();

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
    uint8 FactionId = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
    int32 Credits = 10000;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
    int32 PowerProduced = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
    int32 PowerConsumed = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
    bool bDefeated = false;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
