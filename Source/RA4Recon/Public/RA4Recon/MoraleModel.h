// Copyright (c) Red Alert 4 project. The morale model: how combat becomes fear.
//
// Pure functions over MoraleComp -- no entity access, no world access -- so the
// arithmetic is unit-testable without a SimWorld and reusable by any owner of
// morale state. SimWorld owns the per-unit MoraleComp vector (it owns entity
// lifecycles); it harvests this tick's combat events into stimuli and applies
// them through these functions inside SystemRecon, before the recon layer reads
// the aggregated observer state.
//
// Owner decisions (2026-08-06): morale falls from incoming damage, suppression,
// prolonged combat, nearby allied deaths and visible enemy superiority; blackout
// fear arrives with M3. Recovery is time out of fire only (officers/base come
// after M3). Superiority MUST be computed from raw visible counts, never from
// distorted ones -- distorted counts feed fear, fear inflates counts, and the
// loop runs away.
#pragma once

#include <cstdint>

#include "RA4Core/Fixed.h"
#include "RA4Recon/ReconConfig.h"
#include "RA4Recon/ReconTypes.h"

#ifndef RA4RECON_API
#define RA4RECON_API
#endif

namespace RA4
{
namespace Recon
{

// Incoming damage on the unit itself. Penalty scales with damage relative to a
// nominal 100-point hit; suppression spikes and decays separately.
RA4RECON_API void MoraleApplyDamage(MoraleComp& M, int32_t Damage, const MoraleTuning& T);

// A friendly unit died within AllyDeathRadiusTiles. Watching a neighbour die
// costs more morale than taking a hit yourself (owner decision).
RA4RECON_API void MoraleApplyAllyDeath(MoraleComp& M, const MoraleTuning& T);

// Visible enemies outnumber nearby allies beyond the threshold ratio.
// Called once per tick while the condition holds; the per-tick penalty is small
// because it integrates. Counts must be RAW visible counts (see header note).
RA4RECON_API void MoraleApplySuperiority(MoraleComp& M, int32_t VisibleEnemies, int32_t NearbyAllies,
                                         const MoraleTuning& T);

// One tick of time passing. While under fire fatigue accrues; after
// OutOfFireDelayTicks of quiet, morale and fatigue recover and TicksUnderFire
// resets. Suppression decays every tick regardless -- ducking ends when the
// shells stop, courage takes longer.
RA4RECON_API void MoraleTickRecovery(MoraleComp& M, const MoraleTuning& T);

// Marks the unit as under fire this tick (resets the quiet counter).
RA4RECON_API void MoraleMarkUnderFire(MoraleComp& M);

} // namespace Recon
} // namespace RA4
