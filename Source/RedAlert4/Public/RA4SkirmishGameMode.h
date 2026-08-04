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

    /** Reads the match options out of the URL and brings the simulation up. Split out
        of BeginPlay so the campaign mode can start a mission instead of a skirmish
        without also reimplementing the lighting and terrain setup below it. */
    virtual void StartSimulationMatch();

    /** Lighting, fog, ground material and prop scatter. Presentation only: it reads
        nothing from the simulation and is identical for a skirmish and a mission. */
    void ApplySceneDressing();

    /** Attaches a network channel to each joining player and, once every expected
        player is present, starts the lockstep match on all of them. */
    virtual void PostLogin(APlayerController* NewPlayer) override;

    /** How many players the match waits for before it starts. Taken from the
        ?NumPlayers URL option; 1 means single player and no networking at all. */
    int32 ExpectedPlayers = 1;

private:
    /** Assigned in join order, which is also the lockstep slot order. */
    int32 NextPlayerSlot = 0;

    bool bMatchStarted = false;

    void TryStartNetworkedMatch();
};
