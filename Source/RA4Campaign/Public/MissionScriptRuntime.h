// Copyright (c) Red Alert 4 project. Scripted campaign triggers, actions, and cinematic engine runtime.
#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "CampaignScriptTypes.h"
#include "MissionRuntime.h"
#include "RA4Simulation/SimWorld.h"


#ifndef RA4CAMPAIGN_API
#define RA4CAMPAIGN_API
#endif

namespace RA4
{

class RA4CAMPAIGN_API MissionScriptRuntime
{
public:
    MissionScriptRuntime() = default;

    /** Initializes the script runtime attached to the given MissionRuntime. */
    void Initialize(MissionRuntime* InObjectiveRuntime);

    /** Adds a scripted trigger to the mission runtime. */
    void AddTrigger(const MissionTrigger& Trigger);

    /** Evaluates all triggers against the world and executes actions. Call after SimWorld::Tick. */
    void Tick(SimWorld& World);

    /** Returns currently active cinematic dialogue transmission, or nullptr if none. */
    const CinematicTransmission* GetActiveTransmission() const;

    /** Returns all past transmissions. */
    const std::vector<CinematicTransmission>& GetTransmissionHistory() const { return TransmissionHistory; }

    const std::vector<MissionTrigger>& GetTriggers() const { return Triggers; }

    void Reset();

private:
    bool EvaluateCondition(const MissionTrigger& Trigger, const SimWorld& World) const;
    void ExecuteAction(const ScriptTriggerAction& Action, SimWorld& World);

    MissionRuntime* ObjectiveRuntime = nullptr;
    std::vector<MissionTrigger> Triggers;

    std::deque<CinematicTransmission> TransmissionQueue;
    CinematicTransmission CurrentTransmission;
    std::vector<CinematicTransmission> TransmissionHistory;
};

} // namespace RA4
