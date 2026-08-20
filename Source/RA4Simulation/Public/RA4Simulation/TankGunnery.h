// Copyright (c) Red Alert 4 project. Gunnery for directly controlled vehicles.
//
// WHAT THIS IS FOR
// ----------------
// Direct control currently reuses the RTS weapon: point at a target, the shot
// lands, damage is applied from the warhead/armour table. Steering a unit in
// first person that way is not a tank game -- there is nothing between "I can see
// it" and "it is dead", so there is nothing to be good at.
//
// This header adds the two things that put skill in that gap:
//
//   Dispersion  -- the shot goes somewhere inside a cone, and the cone is wide
//                  while you move, turn the hull, traverse the turret or have
//                  just fired. It shrinks while you hold still. That is the whole
//                  "stop, let it settle, then shoot" loop.
//
//   Penetration -- a shell either goes through the plate it hit or it does not,
//                  and the plate is thicker the more obliquely you hit it. Facing
//                  matters, angling matters, flanking matters.
//
// Everything here is pure integer arithmetic on top of the project's fixed-point
// angle units, with no Unreal types and no floating point, so it is part of the
// deterministic core and is unit-tested headlessly. Nothing in this file reads
// the clock, the renderer, or anything else that differs between two machines.
//
// Units, stated once so nothing has to guess:
//   angles       -- kAngleTurn units per full turn (4096), as everywhere else
//   dispersion   -- milliradians (mrad), because that is how gun accuracy is
//                   quoted and it keeps the numbers small and readable
//   armour       -- millimetres
//   penetration  -- millimetres of armour defeated at the muzzle
#pragma once

#include <cstdint>

#include "RA4Core/Fixed.h"

namespace RA4
{

// --- Dispersion -------------------------------------------------------------

// How far the aim cone opens under each source of disturbance, and how fast it
// closes again. One of these per weapon; the defaults describe a medium tank.
struct GunneryDef
{
    // The cone the gun settles to when the vehicle is still. Smaller is better.
    int32_t AimedDispersionMrad = 30;

    // Extra cone at full speed, at full hull rotation, at full turret traverse.
    // These are what make a moving tank miss and a stopped one hit.
    int32_t MoveBloomMrad = 90;
    int32_t HullTurnBloomMrad = 70;
    int32_t TurretTurnBloomMrad = 50;

    // Extra cone applied the instant a shot leaves the barrel, so rapid fire is
    // less accurate than deliberate fire.
    int32_t FireBloomMrad = 60;

    // Per-mille of the *excess* over the aimed cone that remains after one tick.
    // 900 means 10% of the excess is removed per tick; lower converges faster.
    // Expressed this way rather than as a time so the arithmetic stays integer
    // and exact -- an exponential written as a float would not be reproducible.
    int32_t ConvergePerMille = 900;

    // Millimetres of armour this gun defeats. Compared against the effective
    // thickness of whatever plate the shell meets.
    int32_t PenetrationMm = 100;

    // Spread of penetration around the nominal value, in per-mille. 250 means the
    // roll lands in [0.75x, 1.25x]. Without it, a gun either always penetrates a
    // given plate or never does, and every duel has a foregone conclusion.
    int32_t PenetrationVariancePerMille = 250;
};

// Everything the dispersion update needs to know about this tick's motion.
// All three are per-mille of that vehicle's own maximum, so a fast scout and a
// slow heavy bloom by the same fraction when both are flat out.
struct GunneryMotion
{
    int32_t SpeedPerMille = 0;        // 0 = stopped, 1000 = full speed
    int32_t HullTurnPerMille = 0;     // absolute value of hull rotation rate
    int32_t TurretTurnPerMille = 0;   // absolute value of turret traverse rate
    bool bFiredThisTick = false;
};

// The cone the gun cannot be tighter than while the vehicle is doing this.
// Sources add rather than max: driving while traversing is worse than either.
inline int32_t DispersionFloorMrad(const GunneryDef& Def, const GunneryMotion& Motion)
{
    int64_t Floor = Def.AimedDispersionMrad;
    Floor += (int64_t(Def.MoveBloomMrad) * Motion.SpeedPerMille) / 1000;
    Floor += (int64_t(Def.HullTurnBloomMrad) * Motion.HullTurnPerMille) / 1000;
    Floor += (int64_t(Def.TurretTurnBloomMrad) * Motion.TurretTurnPerMille) / 1000;
    return int32_t(Floor);
}

// Advances the aim cone by one tick and returns the new value.
//
// The cone never drops below the floor for what the vehicle is doing right now,
// and otherwise decays geometrically toward the aimed cone. Firing adds its bloom
// on top, which is why the second shot of a burst is worse than the first.
inline int32_t UpdateDispersionMrad(const GunneryDef& Def, const GunneryMotion& Motion,
                                    int32_t CurrentMrad)
{
    const int32_t Floor = DispersionFloorMrad(Def, Motion);

    // Converge on the excess over the *aimed* cone, not over the floor: a tank
    // that stops should keep tightening all the way down, not stall at whatever
    // its last floor was.
    int32_t Next = Def.AimedDispersionMrad;
    if (CurrentMrad > Def.AimedDispersionMrad)
    {
        const int64_t Excess = int64_t(CurrentMrad) - Def.AimedDispersionMrad;
        Next = Def.AimedDispersionMrad + int32_t((Excess * Def.ConvergePerMille) / 1000);
    }

    if (Next < Floor)
    {
        Next = Floor;
    }
    if (Motion.bFiredThisTick)
    {
        Next += Def.FireBloomMrad;
    }
    return Next;
}

// True when the gun has settled far enough to be worth firing. Used for the
// reticle's "aimed" state and by the AI, so both agree on what aimed means.
inline bool IsAimed(const GunneryDef& Def, int32_t CurrentMrad)
{
    // Within a quarter of the aimed cone counts as settled. Demanding exact
    // equality would never be reached, because convergence is geometric.
    return CurrentMrad <= Def.AimedDispersionMrad + Def.AimedDispersionMrad / 4;
}

// --- Armour -----------------------------------------------------------------

enum class HitFacing : uint8_t
{
    Front,
    Side,
    Rear
};

// Per-vehicle plate thicknesses. Front is what you show the enemy on purpose;
// side and rear are what flanking is for.
struct ArmorDef
{
    int32_t FrontMm = 90;
    int32_t SideMm = 45;
    int32_t RearMm = 30;

    int32_t ForFacing(HitFacing Facing) const
    {
        switch (Facing)
        {
        case HitFacing::Front: return FrontMm;
        case HitFacing::Side:  return SideMm;
        case HitFacing::Rear:  return RearMm;
        }
        return FrontMm;
    }
};

// Which plate a shot arriving from ShotFromAngle meets on a hull facing
// HullFacing. Both are absolute angles in the project's angle units.
//
// The 45 degree boundaries are deliberate and generous to the sides: a driver who
// angles their hull should be rewarded with front armour over a wide arc, which
// is the entire skill of angling.
inline HitFacing FacingForImpact(int32_t HullFacing, int32_t ShotFromAngle)
{
    // Angle between where the hull points and where the shot came from.
    const int32_t Delta = AngleDelta(HullFacing, ShotFromAngle);
    const int32_t Abs = Delta < 0 ? -Delta : Delta;

    constexpr int32_t kEighth = kAngleTurn / 8;    // 45 degrees
    constexpr int32_t kThreeEighths = 3 * kEighth; // 135 degrees

    if (Abs <= kEighth)
    {
        return HitFacing::Front;
    }
    if (Abs >= kThreeEighths)
    {
        return HitFacing::Rear;
    }
    return HitFacing::Side;
}

// Effective thickness of a plate struck at an angle, in millimetres.
//
// A shell crossing a plate at an angle travels through more metal: the path
// length is thickness / cos(angle from the plate's normal). Returned clamped,
// because cos approaches zero at a grazing hit and the true value approaches
// infinity -- which is correct physically and useless numerically.
inline int32_t EffectiveArmorMm(int32_t BaseMm, int32_t ImpactAngleFromNormal)
{
    const int32_t Abs = ImpactAngleFromNormal < 0 ? -ImpactAngleFromNormal : ImpactAngleFromNormal;
    const Fixed C = FxCos(Abs);
    // At or past 90 degrees the shell is not entering the plate at all.
    if (C <= Fixed::FromInt(0))
    {
        return BaseMm * 10;
    }
    const int64_t Scaled = (int64_t(BaseMm) * Fixed::One().Raw) / C.Raw;
    // Ten times the nominal plate is already unpenetrable by anything that would
    // reasonably be shooting at it; going higher only risks overflow downstream.
    const int64_t Capped = Scaled > int64_t(BaseMm) * 10 ? int64_t(BaseMm) * 10 : Scaled;
    return int32_t(Capped);
}

// --- Impact resolution ------------------------------------------------------

enum class ImpactResult : uint8_t
{
    Penetration,   // through the plate; full damage
    Ricochet,      // too oblique to bite; no damage, shell deflects
    Bounce         // struck square enough but not hard enough; no damage
};

// Angle past which a shell skids off regardless of how powerful it is.
// Two thirds of a right angle, which is the convention every tank game has
// converged on because it reads clearly to players.
constexpr int32_t kAutoRicochetAngle = (kAngleTurn * 60) / 360;

// Decides what a shell does when it meets a plate.
//
// RollPerMille is a deterministic roll in [0, 999] supplied by the caller from
// the simulation's own generator -- this function does not draw randomness
// itself, so it stays pure and a test can pin every branch by choosing the roll.
inline ImpactResult ResolveImpact(const GunneryDef& Gun, int32_t BaseArmorMm,
                                  int32_t ImpactAngleFromNormal, int32_t RollPerMille)
{
    const int32_t Abs = ImpactAngleFromNormal < 0 ? -ImpactAngleFromNormal : ImpactAngleFromNormal;

    // Grazing hits skid off before thickness is even considered. This is what
    // makes angling the hull a defence against guns that would otherwise go
    // straight through.
    if (Abs >= kAutoRicochetAngle)
    {
        return ImpactResult::Ricochet;
    }

    const int32_t Effective = EffectiveArmorMm(BaseArmorMm, Abs);

    // Roll the shell's penetration around its nominal value. RollPerMille spans
    // the full variance band: 0 is the unluckiest shell, 999 the luckiest.
    const int64_t Span = (int64_t(Gun.PenetrationMm) * Gun.PenetrationVariancePerMille) / 1000;
    const int64_t Low = int64_t(Gun.PenetrationMm) - Span;
    const int64_t Rolled = Low + (Span * 2 * RollPerMille) / 1000;

    return Rolled >= Effective ? ImpactResult::Penetration : ImpactResult::Bounce;
}

// --- Shot scatter -----------------------------------------------------------

// Lateral offset of a shell at the target, in world units, for a cone of
// DispersionMrad at DistanceUnits.
//
// RollSignedPerMille is in [-1000, 1000] and is the caller's deterministic roll;
// +/-1000 is the edge of the cone. A milliradian is one unit of offset per
// thousand units of range, which is the entire reason gun accuracy is quoted in
// them -- the arithmetic is a multiply and a divide with no trig at all.
inline Fixed ShotOffsetAtRange(int32_t DispersionMrad, Fixed DistanceUnits,
                               int32_t RollSignedPerMille)
{
    const int64_t Offset =
        (int64_t(DistanceUnits.Raw) * DispersionMrad * RollSignedPerMille) / (1000 * 1000);
    return Fixed(Offset);
}

} // namespace RA4
