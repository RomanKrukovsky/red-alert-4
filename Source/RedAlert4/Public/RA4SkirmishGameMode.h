// Copyright (c) Red Alert 4 project. Playable skirmish sandbox.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "RA4SkirmishGameMode.generated.h"

/** Wires the RTS controller, camera pawn and code-drawn HUD together. */
UCLASS()
class REDALERT4_API ARA4SkirmishGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ARA4SkirmishGameMode();

protected:
    virtual void BeginPlay() override;

    /** Attaches a network channel to each joining player and, once every expected
        player is present, starts the lockstep match on all of them. */
    virtual void PostLogin(APlayerController* NewPlayer) override;

private:
    /** How many players the match waits for before it starts. Taken from the
        ?NumPlayers URL option; 1 means single player and no networking at all. */
    int32 ExpectedPlayers = 1;

    /** Assigned in join order, which is also the lockstep slot order. */
    int32 NextPlayerSlot = 0;

    bool bMatchStarted = false;

    void TryStartNetworkedMatch();
};
