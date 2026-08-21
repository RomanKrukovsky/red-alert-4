// Copyright (c) Red Alert 4 project. AI vs AI Self-Play Tournament League.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4AI/AICommander.h"
#include "RA4Core/Ids.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

struct MatchRecord
{
    uint32_t MatchId = 0;
    AIProfile ProfilePlayer0 = AIProfile::Balanced;
    AIProfile ProfilePlayer1 = AIProfile::Balanced;
    PlayerId Winner = kInvalidPlayer;
    uint32_t DurationTicks = 0;
    int32_t TotalHarvestedP0 = 0;
    int32_t TotalHarvestedP1 = 0;
    uint64_t FinalStateChecksum = 0;
};

struct LeagueSummary
{
    uint32_t TotalMatchesRun = 0;
    uint32_t Player0Wins = 0;
    uint32_t Player1Wins = 0;
    uint32_t Draws = 0;
    float AverageDurationSeconds = 0.0f;
    float EloRatingP0 = 1500.0f;
    float EloRatingP1 = 1500.0f;
    std::vector<MatchRecord> Matches;
};

class RA4AI_API AISelfPlayLeague
{
public:
    /** Runs a self-play tournament between two AI profiles over MatchCount matches,
        executing deterministic head-to-head simulations and tracking Elo progression. */
    static LeagueSummary RunTournament(uint32_t MatchCount, AIProfile ProfileA, AIProfile ProfileB,
                                       uint64_t BaseSeed, uint32_t MaxTicksPerMatch = 3000);

    /** Computes Elo rating adjustments based on match result. */
    static void UpdateElo(float& InOutEloA, float& InOutEloB, float ScoreA, float KFactor = 32.0f);
};

} // namespace AI
} // namespace RA4
