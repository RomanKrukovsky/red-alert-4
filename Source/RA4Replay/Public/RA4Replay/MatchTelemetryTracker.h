// Copyright (c) Red Alert 4 project. Match Telemetry and APM Tracker.
#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include "RA4Core/Command.h"
#include "RA4Core/Ids.h"
#include "RA4Simulation/SimTypes.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4REPLAY_API
#define RA4REPLAY_API
#endif

namespace RA4
{

struct PlayerTimelineSample
{
    TickIndex Tick = 0;
    float ActionsPerMinute = 0.0f;
    int32_t TotalCreditsHarvested = 0;
    int32_t CurrentCredits = 0;
    int32_t ActiveArmyValue = 0;
    int32_t UnitsLostCount = 0;
    int32_t UnitsKilledCount = 0;
};

struct PlayerMatchStats
{
    PlayerId Player = kInvalidPlayer;
    float PeakAPM = 0.0f;
    float AverageAPM = 0.0f;
    int32_t TotalCommandsIssued = 0;
    int32_t TotalHarvested = 0;
    int32_t PeakArmyValue = 0;
    int32_t TotalUnitsLost = 0;
    int32_t TotalUnitsKilled = 0;
    std::vector<PlayerTimelineSample> Timeline;
};

class RA4REPLAY_API MatchTelemetryTracker
{
public:
    MatchTelemetryTracker() = default;

    /** Initializes tracking for active players. SampleIntervalTicks specifies timeline cadence (e.g. 20 ticks = 1 sec). */
    void Initialize(uint8_t NumPlayers = 2, uint32_t SampleIntervalTicks = 20);

    /** Ingests commands executed during Tick and updates real-time APM metrics. */
    void IngestTickCommands(TickIndex Tick, const CommandFrame* Frame);

    /** Ingests SimEvents to track casualties, destruction, and resource deliveries. */
    void IngestSimEvents(const std::vector<SimEvent>& Events, const SimWorld& World);

    /** Samples world state into timeline history if sample interval reached. */
    void SampleWorldState(const SimWorld& World);

    /** Returns stats for Player. */
    const PlayerMatchStats* GetPlayerStats(PlayerId Player) const;

    /** Serializes timeline and match statistics to JSON for MatchViewer. */
    std::string ExportToJson(const SimWorld& World) const;

    void Reset();

private:
    uint8_t NumActivePlayers = 2;
    uint32_t SampleInterval = 20;

    // Rolling window of command timestamps for APM calculation: [Player -> [TickIndices]]
    std::map<PlayerId, std::deque<TickIndex>> CommandWindows;

    std::map<PlayerId, PlayerMatchStats> Stats;
};

} // namespace RA4
