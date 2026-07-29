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
};
