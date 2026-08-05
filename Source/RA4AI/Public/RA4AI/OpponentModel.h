// Copyright (c) Red Alert 4 project. Probabilistic opponent behavior model.
//
// Tracks per-player behavioral statistics derived from observed actions: attack
// frequency, army composition, expansion rate, aggression level, preferred
// attack direction, etc.  Fully deterministic: uses only integer arithmetic
// and exponential moving averages with fixed-point alpha.  No engine
// dependency — reads SimEvents, never SimWorld.
#pragma once

#include <cstdint>

#include "RA4Simulation/SimTypes.h"

namespace RA4
{
namespace AI
{

// Maximum number of opponent players tracked simultaneously.
constexpr int32_t kMaxTrackedPlayers = 16;

// Flat, copyable profile for one observed opponent.  All ratio fields are
// 0..100 (percentage), direction is quadrant index 0..3.
struct OpponentProfile
{
    // --- Behavioral ratios (0..100) ---
    int32_t Aggressiveness = 0;      // attack frequency / damage output
    int32_t ExpansionRate = 0;       // building completion rate
    int32_t AirPreference = 0;       // proportion of air units produced
    int32_t ArmorRatio = 0;          // proportion of armored units
    int32_t HarasserTendency = 0;    // attacks on economy / unprotected targets
    int32_t PreferredAttackDirection = 0; // quadrant index 0=NW, 1=NE, 2=SW, 3=SE

    // --- Raw counters ---
    int32_t AttacksObserved = 0;     // total attack events seen
    int32_t BuildingsObserved = 0;   // total building completions seen
    int32_t UnitsObserved = 0;       // total unit productions seen
    int32_t TotalUnitsLost = 0;      // total units destroyed

    // --- Timestamps ---
    TickIndex LastObservationTick = 0;
    TickIndex LastAttackTick = 0;
};

// Probabilistic model of an opponent's behaviour, built from observed SimEvents.
// Engine-free: only integer arithmetic, fully deterministic, testable in isolation.
class OpponentModel
{
public:
    OpponentModel() = default;

    // Process a batch of SimEvents and update the profile for the owning player.
    // Self is the player ID of the AI that owns this model.  Events from Self
    // are ignored.  Only DamageApplied, BuildingCompleted, ProductionCompleted,
    // EntityDestroyed, and WeaponFired are used.
    void UpdateFromEvents(const SimEvent* Events, int32_t EventCount,
                          PlayerId Self, TickIndex Now);

    // Get the profile for a specific enemy player.
    const OpponentProfile& GetProfile(PlayerId Enemy) const;

    // Convenience queries for directors.
    bool EnemyPrefersAir(PlayerId Enemy) const;
    bool EnemyPrefersArmor(PlayerId Enemy) const;
    bool EnemyIsAggressive(PlayerId Enemy) const;
    int32_t GetPreferredAttackDirection(PlayerId Enemy) const;

    // Reset all profiles to defaults.
    void Reset();

    // Classify a movement direction into a quadrant index (0..3).
    // Used internally and exposed for testing.
    static int32_t ClassifyDirection(int32_t AttackerX, int32_t AttackerY,
                                     int32_t TargetX, int32_t TargetY);

private:
    // Exponential moving average helper: (Old*9 + NewSample) / 10.
    static int32_t EmaUpdate(int32_t Old, int32_t NewSample);

    OpponentProfile Profiles[kMaxTrackedPlayers];
};

} // namespace AI
} // namespace RA4
