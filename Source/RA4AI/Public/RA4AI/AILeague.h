// Copyright (c) Red Alert 4 project. AI-versus-AI self-play league.
//
// Plays real headless SimWorld matches between AI profiles and aggregates the
// results into per-pairing statistics. This is the measurement instrument the
// production plan calls for: profile balance is a number produced by running
// matches, not an opinion produced by reading configs.
//
// Deterministic by construction: a league run is fully described by (schedule,
// base seed), every match seed is derived arithmetically from the base seed, and
// the same inputs always produce the same table. That also makes regressions
// visible -- if a balance-neutral refactor changes a league table, it changed
// match outcomes and must explain itself.
//
// Engine-free and synchronous: no threads, no filesystem, no clock. Callers that
// want parallelism can shard MatchIndex ranges across processes and merge the
// MatchRecords, because record content depends only on (setup, seed).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4AI/AIStrategy.h"
#include "RA4Core/Ids.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
class ContentDatabase;

namespace AI
{

// One finished (or timed-out) match, with enough economy/army telemetry to
// diagnose WHY a profile wins, not just that it does.
struct LeagueMatchRecord
{
    uint32_t MatchIndex = 0;
    uint64_t Seed = 0;
    AIProfile Profiles[2] = {AIProfile::Balanced, AIProfile::Balanced};
    AIDifficulty Difficulty = AIDifficulty::Normal;

    // Which faction each player fielded this match. The league alternates them
    // between repeats of a pairing, because faction strength would otherwise be
    // indistinguishable from profile strength (found the hard way: an early run
    // reported player 0 winning 490:0, which was the Soviet roster, not the AI).
    bool bSwappedFactions = false;

    // kInvalidPlayer means the tick budget expired with both sides alive: a draw.
    PlayerId Winner = kInvalidPlayer;
    uint32_t DurationTicks = 0;
    bool bTimedOut = false;

    // Per-player running totals copied from PlayerState at match end.
    int32_t TotalHarvested[2] = {0, 0};
    int32_t UnitsBuilt[2] = {0, 0};
    int32_t UnitsLost[2] = {0, 0};
    int32_t BuildingsBuilt[2] = {0, 0};
    int32_t BuildingsLost[2] = {0, 0};

    // Final state checksum: two records with equal seeds and setups must match
    // bit-for-bit, so a league run doubles as a mass determinism test.
    uint64_t FinalChecksum = 0;
};

// Aggregate for one ordered pairing (A as player 0 vs B as player 1).
struct LeaguePairingStats
{
    AIProfile ProfileA = AIProfile::Balanced;
    AIProfile ProfileB = AIProfile::Balanced;
    uint32_t Matches = 0;
    uint32_t WinsA = 0;
    uint32_t WinsB = 0;
    uint32_t Draws = 0;
    uint32_t TimedOut = 0;
    uint64_t TotalDurationTicks = 0;

    int32_t WinRatePercentA() const
    {
        const uint32_t Decisive = WinsA + WinsB;
        return Decisive == 0 ? 50 : int32_t((uint64_t(WinsA) * 100) / Decisive);
    }
    uint32_t AverageDurationTicks() const
    {
        return Matches == 0 ? 0 : uint32_t(TotalDurationTicks / Matches);
    }
};

struct LeagueConfig
{
    // Matches per ordered pairing. Both orders are played so a map-side or
    // faction advantage shows up as an asymmetry instead of hiding in the noise.
    uint32_t MatchesPerPairing = 3;

    // Hard per-match tick budget. Expiry is recorded as a timeout/draw, never
    // silently truncated into a win for whoever happened to be ahead.
    int32_t MaxTicksPerMatch = 20 * 60 * 10;   // 10 minutes at 20 Hz

    AIDifficulty Difficulty = AIDifficulty::Normal;
    uint64_t BaseSeed = 20260805;

    // Alternate which profile plays which faction between repeats of a pairing,
    // so faction strength averages out of the profile comparison instead of
    // masquerading as it. Off = every match is ProfileA-as-Soviet.
    bool bAlternateFactions = true;

    // Profiles that enter the round-robin. Empty = all eight.
    std::vector<AIProfile> Roster;
};

struct LeagueResult
{
    std::vector<LeagueMatchRecord> Matches;
    std::vector<LeaguePairingStats> Pairings;

    uint32_t TotalMatches() const { return uint32_t(Matches.size()); }
    const LeaguePairingStats* FindPairing(AIProfile A, AIProfile B) const;

    // Multi-line human-readable table for logs and reports.
    std::string FormatTable() const;
};

class RA4AI_API AILeague
{
public:
    // Plays every ordered pairing from the roster, MatchesPerPairing times each,
    // on the standard skirmish bootstrap (mirrored yards + ore fields). Blocking;
    // cost is roughly Matches x MaxTicks x per-tick cost.
    static LeagueResult RunRoundRobin(const LeagueConfig& Config);

    // Plays one match and returns its record. Exposed so tests can validate a
    // single cell of the table cheaply and tools can shard big runs.
    static LeagueMatchRecord PlayMatch(AIProfile ProfileA, AIProfile ProfileB,
                                       AIDifficulty Difficulty, uint64_t Seed,
                                       int32_t MaxTicks,
                                       bool bSwapFactions = false);
};

} // namespace AI
} // namespace RA4
