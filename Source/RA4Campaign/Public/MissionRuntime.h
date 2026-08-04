// Copyright (c) Red Alert 4 project. Evaluates campaign objectives against the
// simulation.
//
// This is what turns a CampaignMissionDef from a row in a database into a mission
// that can be won and lost. It has no dependency on Unreal: it reads SimWorld, which
// is itself engine-free, so a mission can be played to completion in a unit test at
// full speed and produce the same result it would produce behind a UWorld.
//
// The runtime never writes to the world. Objectives are predicates over state the
// simulation already owns, which is what keeps a mission replay-safe -- replaying a
// recorded command stream reproduces the same objective transitions at the same
// ticks, because nothing about the evaluation feeds back into the simulation.
#pragma once

#include <string>
#include <vector>

#include "CampaignTypes.h"
#include "RA4Simulation/SimWorld.h"

namespace RA4
{

// See CampaignDatabase.h: empty outside an Unreal build, the module's export
// attribute inside one.
#ifndef RA4CAMPAIGN_API
#define RA4CAMPAIGN_API
#endif

enum class MissionStatus : uint8_t
{
    NotStarted = 0,
    InProgress,
    Won,
    Lost,
};

/** Fired when an objective changes state, so the HUD can announce it without
    diffing the objective list itself every tick. */
struct ObjectiveTransition
{
    std::string ObjectiveId;
    ObjectiveState NewState = ObjectiveState::Hidden;
    TickIndex Tick = 0;
};

class RA4CAMPAIGN_API MissionRuntime
{
public:
    /** Copies the mission's objectives and arms the runtime. LocalPlayer is the slot
        the human occupies; it decides which side a simulation-declared victory counts
        for. Safe to call again to restart the same mission. */
    void Begin(const CampaignMissionDef& Mission, PlayerId LocalPlayer, TickIndex StartTick = 0);

    /** Evaluates every armed objective and failure condition once. Call after
        SimWorld::Tick, so the objectives judge the state the next frame will act on.
        Returns the status after this evaluation. */
    MissionStatus Evaluate(const SimWorld& World);

    MissionStatus GetStatus() const { return Status; }
    bool IsFinished() const { return Status == MissionStatus::Won || Status == MissionStatus::Lost; }

    const std::vector<MissionObjective>& GetObjectives() const { return Objectives; }
    const MissionObjective* FindObjective(const std::string& Id) const;

    /** Reveals an objective that shipped Hidden. A hidden objective is not evaluated
        and is not required for victory, which is how a mission stages its goals
        without the briefing giving away the second half. */
    bool RevealObjective(const std::string& Id);

    const std::vector<ObjectiveTransition>& GetTransitions() const { return Transitions; }
    void ClearTransitions() { Transitions.clear(); }

    /** Which failure condition ended the mission, or -1. Index into the mission's
        FailureConditions; kept so the defeat screen can name the reason. */
    int32_t GetTriggeredFailure() const { return TriggeredFailure; }

private:
    void SetObjectiveState(MissionObjective& Objective, ObjectiveState NewState, TickIndex Tick);
    bool EvaluateCondition(const SimWorld& World, const ObjectiveCondition& Condition) const;
    bool AllPrimariesComplete() const;

    std::vector<MissionObjective> Objectives;
    std::vector<ObjectiveCondition> FailureConditions;
    std::vector<ObjectiveTransition> Transitions;

    PlayerId Player = kInvalidPlayer;
    TickIndex StartTick = 0;
    MissionStatus Status = MissionStatus::NotStarted;
    int32_t TriggeredFailure = -1;
};

// --- Bringing a mission up ---------------------------------------------------

/** Translates the mission's declared starting conditions into the MatchSetup that
    SimWorld::Initialize takes. Kept separate from the runtime because setting a
    match up and judging it are different jobs, and the loader is also what a map
    editor or a test fixture wants on its own. */
MatchSetup RA4CAMPAIGN_API BuildMissionMatchSetup(const CampaignMissionDef& Mission);

/** Places the mission's opening units and buildings into an already-initialized
    world. Returns how many spawns were placed; a spawn whose ContentId is not in the
    content database is skipped rather than aborting the mission, and the shortfall in
    the return value is what a test asserts on. */
int32_t RA4CAMPAIGN_API SpawnMissionEntities(SimWorld& World, const CampaignMissionDef& Mission);

} // namespace RA4
