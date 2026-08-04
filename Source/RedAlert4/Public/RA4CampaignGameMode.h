// Copyright (c) Red Alert 4 project. Playable campaign mission.
#pragma once

#include "CoreMinimal.h"
#include "RA4SkirmishGameMode.h"

#include "RA4CampaignGameMode.generated.h"

/** Starts one campaign mission instead of a skirmish.

    Everything about presentation -- the controller, the camera pawn, the HUD, the
    lighting and terrain setup -- is the skirmish mode's, because a mission is the
    same game with a different starting position and a list of objectives. What
    differs is only where the match comes from: the mission's own MissionSetupDef
    rather than the skirmish bootstrap, and a MissionRuntime evaluating objectives
    against the simulation every tick.

    Opened with ?Mission=<id>, e.g. ?Mission=sov_mission_1. */
UCLASS()
class REDALERT4_API ARA4CampaignGameMode : public ARA4SkirmishGameMode
{
    GENERATED_BODY()

public:
    ARA4CampaignGameMode();

protected:
    virtual void StartSimulationMatch() override;
};
