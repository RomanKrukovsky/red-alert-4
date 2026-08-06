// Copyright (c) Red Alert 4 project.
#include "RA4Recon/MoraleModel.h"

namespace RA4
{
namespace Recon
{

namespace
{
const Fixed kOne = Fixed::FromInt(1);
const Fixed kZero = Fixed::Zero();

// The comp uses TicksUnderFire < 0 to count quiet ticks (negative = ticks of
// peace accumulated toward recovery) and >= 0 to count ticks in combat. One
// field, two directions -- keeps MoraleComp POD-small for 5000 units.
} // namespace

void MoraleApplyDamage(MoraleComp& M, int32_t Damage, const MoraleTuning& T)
{
    // Penalty proportional to the blow: a 100-damage hit costs the configured
    // fraction, a scratch costs a scratch. Integer-scaled through Fixed.
    const Fixed Scale = Fixed::FromRatio(Damage, 100);
    M.Morale = FxClamp(M.Morale - PerMilleToFixed(T.DamageMoralePenaltyPerMille) * Scale, kZero, kOne);
    M.Suppression = FxClamp(M.Suppression + PerMilleToFixed(T.DamageSuppressionPerMille) * Scale, kZero, kOne);
    MoraleMarkUnderFire(M);
}

void MoraleApplyAllyDeath(MoraleComp& M, const MoraleTuning& T)
{
    M.Morale = FxClamp(M.Morale - PerMilleToFixed(T.AllyDeathMoralePenaltyPerMille), kZero, kOne);
    MoraleMarkUnderFire(M);
}

void MoraleApplySuperiority(MoraleComp& M, int32_t VisibleEnemies, int32_t NearbyAllies,
                            const MoraleTuning& T)
{
    if (VisibleEnemies <= 0)
    {
        return;
    }
    // Ratio in per-mille against the threshold. A lone unit counts itself.
    const int32_t Allies = NearbyAllies < 1 ? 1 : NearbyAllies;
    const int64_t RatioPerMille = (int64_t(VisibleEnemies) * 1000) / Allies;
    if (RatioPerMille <= T.SuperiorityRatioThresholdPerMille)
    {
        return;
    }
    // Being outnumbered is dread, not a shellburst: it erodes morale but does
    // not reset the quiet counter -- troops still calm down when nobody shoots.
    M.Morale = FxClamp(M.Morale - PerMilleToFixed(T.SuperiorityMoralePenaltyPerTickPerMille), kZero, kOne);
}

void MoraleMarkUnderFire(MoraleComp& M)
{
    // Unconditional reset: TicksUnderFire counts ticks SINCE THE LAST stimulus.
    // Resetting only from recovery mode would let the counter climb to the
    // quiet threshold while shells are still landing, and the unit would start
    // "resting" mid-battle.
    M.TicksUnderFire = 0;
}

void MoraleTickRecovery(MoraleComp& M, const MoraleTuning& T)
{
    // Suppression decays unconditionally: heads come up as soon as fire lifts.
    M.Suppression = FxMax(M.Suppression - PerMilleToFixed(T.SuppressionDecayPerTickPerMille), kZero);

    if (M.TicksUnderFire >= 0)
    {
        // In (or just out of) contact: fatigue accrues, the counter climbs.
        // The counter flips negative below only after a full quiet delay.
        M.Fatigue = FxClamp(M.Fatigue + PerMilleToFixed(T.FatiguePerTickUnderFirePerMille), kZero, kOne);
        M.TicksUnderFire += 1;
        if (M.TicksUnderFire >= T.OutOfFireDelayTicks)
        {
            // Nothing hit us for the whole delay window (MarkUnderFire resets
            // the counter to 0 on every stimulus, so reaching the threshold
            // means an unbroken quiet streak). Switch to recovery mode.
            M.TicksUnderFire = -1;
        }
        return;
    }

    // Recovery mode: morale and fatigue drift back toward rested.
    M.Morale = FxClamp(M.Morale + PerMilleToFixed(T.MoraleRegenPerTickPerMille), kZero, kOne);
    M.Fatigue = FxMax(M.Fatigue - PerMilleToFixed(T.FatigueRegenPerTickPerMille), kZero);
}

} // namespace Recon
} // namespace RA4
