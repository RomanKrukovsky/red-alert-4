// Copyright (c) Red Alert 4 project.
#include "RA4Recon/DistortionPipeline.h"

#include "RA4Core/SimConfig.h"

namespace RA4
{
namespace Recon
{

namespace
{

// Uniform Fixed in [-1, +1]. Two calls to the rng keep the grain fine enough
// for per-mille math without touching floats.
Fixed NextSymmetricUnit(Random& Rng)
{
    const Fixed U = Rng.NextUnitFixed();               // [0, 1)
    return U + U - Fixed::FromInt(1);                  // [-1, +1)
}

} // namespace

Fixed StageClarity(const ObserverState& Observer, const DistortionProfile& P)
{
    if (!P.bClarityEnabled)
    {
        return Fixed::FromInt(1);
    }

    // Start from perfect sight, lose up to ClarityDistanceFalloff at max range,
    // then blend toward the observer's competence: a poor observer never reaches
    // full clarity even point-blank -- skill IS part of seeing (§4.3.1).
    const Fixed One = Fixed::FromInt(1);
    const Fixed Falloff = PerMilleToFixed(P.ClarityDistanceFalloffPerMille);
    Fixed Clarity = One - Observer.DistanceRatio * Falloff;

    // Competence blend: clarity cannot exceed (0.5 + competence/2). A rookie
    // (0.0) caps at 0.5, a veteran (1.0) is uncapped.
    const Fixed Cap = Fixed::FromRatio(1, 2) + Observer.Competence / 2;
    Clarity = FxMin(Clarity, Cap);

    return FxClamp(Clarity, Fixed::Zero(), One);
}

int32_t StageCountDistortion(int32_t TrueCount, const ObserverState& Observer,
                             const DistortionProfile& P, Random& Rng)
{
    if (!P.bCountDistortionEnabled || TrueCount <= 0)
    {
        return TrueCount;
    }

    const Fixed One = Fixed::FromInt(1);

    // Fear: grows as morale falls and suppression rises. Strictly non-negative --
    // the asymmetry the spec demands. A terrified observer reports a horde; he
    // does not report an understrength platoon (§4.3.2).
    const Fixed FearLevel = FxClamp((One - Observer.Morale) / 2 + Observer.Suppression / 2,
                                    Fixed::Zero(), One);
    const Fixed FearMax = PerMilleToFixed(P.FearCountBiasMaxPerMille);
    // Randomize within [0, FearLevel*FearMax]: fear inflates by up to the level,
    // not always by the full amount -- panic is noisy, not calibrated.
    const Fixed FearBias = FearLevel * FearMax * Rng.NextUnitFixed();

    // Incompetence: symmetric noise, spread grows as competence falls.
    const Fixed NoiseMax = PerMilleToFixed(P.CompetenceNoiseMaxPerMille);
    const Fixed NoiseSpread = (One - Observer.Competence) * NoiseMax;
    const Fixed Noise = NoiseSpread * NextSymmetricUnit(Rng);

    const Fixed Multiplier = FxMax(One + FearBias + Noise, Fixed::FromRatio(1, 4));
    const int32_t Perceived = int32_t((Multiplier * TrueCount).ToIntFloor());
    return Perceived < 1 ? 1 : Perceived; // you saw SOMETHING, that is why you reported
}

ObservedCategory StageClassification(ObservedCategory TrueCategory, Fixed Clarity,
                                     const ConfusionMatrix& M, const DistortionProfile& P, Random& Rng)
{
    if (!P.bClassificationErrorEnabled)
    {
        return TrueCategory;
    }

    // Clarity gates the roll itself: with probability = clarity the observer
    // simply gets it right, and only the remainder consults the matrix. This
    // keeps the matrix authoring independent from clarity tuning.
    const uint32_t ClarityPerMille = uint32_t(FxClamp(Clarity, Fixed::Zero(), Fixed::FromInt(1)).Raw * 1000 / kFixedOne);
    if (Rng.NextBelow(1000) < ClarityPerMille)
    {
        return TrueCategory;
    }

    const int32_t Row = int32_t(TrueCategory);
    const uint32_t Roll = Rng.NextBelow(1000); // rows are validated to sum to exactly 1000
    int32_t Accum = 0;
    for (int32_t Col = 0; Col < kObservedCategoryCount; ++Col)
    {
        Accum += M.PerMille[Row][Col];
        if (int32_t(Roll) < Accum)
        {
            return ObservedCategory(Col);
        }
    }
    return TrueCategory; // unreachable with a validated matrix; safe fallback
}

Vec2 StagePositionError(Fixed Clarity, const ObserverState& Observer,
                        const DistortionProfile& P, Random& Rng)
{
    if (!P.bPositionErrorEnabled)
    {
        return Vec2(Fixed::Zero(), Fixed::Zero());
    }

    const Fixed One = Fixed::FromInt(1);
    // Radius: max error scaled by distance and dimmed by clarity.
    const Fixed MaxRadius = Fixed::FromInt(int64_t(P.PositionErrorMaxTiles) * kTileSizeUnits);
    const Fixed Radius = MaxRadius * Observer.DistanceRatio * (One - Clarity);
    if (Radius.Raw <= 0)
    {
        return Vec2(Fixed::Zero(), Fixed::Zero());
    }

    // Uniform point in the error square, rejected into the circle. Rejection
    // sampling keeps the distribution round without trigonometry (no sin/cos
    // in fixed point, and no libm in the sim).
    for (int32_t Attempt = 0; Attempt < 8; ++Attempt)
    {
        const Fixed X = Radius * NextSymmetricUnit(Rng);
        const Fixed Y = Radius * NextSymmetricUnit(Rng);
        if (X * X + Y * Y <= Radius * Radius)
        {
            return Vec2(X, Y);
        }
    }
    return Vec2(Fixed::Zero(), Fixed::Zero()); // pathologically unlucky: no offset
}

bool StageOmission(Fixed Clarity, const ObserverState& Observer,
                   const DistortionProfile& P, Random& Rng)
{
    if (!P.bOmissionEnabled)
    {
        return false;
    }
    const Fixed One = Fixed::FromInt(1);
    // Chance grows with fatigue, shrinks with clarity. Both scaled into the
    // designer's per-mille ceiling.
    const Fixed Level = FxClamp(Observer.Fatigue / 2 + (One - Clarity) / 2, Fixed::Zero(), One);
    const Fixed Chance = PerMilleToFixed(P.OmissionChanceMaxPerMille) * Level;
    const uint32_t ChancePerMille = uint32_t(FxClamp(Chance, Fixed::Zero(), One).Raw * 1000 / kFixedOne);
    return Rng.NextBelow(1000) < ChancePerMille;
}

// --- Stage 6: fabrication -------------------------------------------------------

bool StageFabrication(const ObserverState& Observer, const DistortionProfile& P, Random& Rng)
{
    if (!P.bFabricationEnabled)
    {
        return false;
    }
    const Fixed One = Fixed::FromInt(1);

    // Dread has two ingredients and needs BOTH: how broken the unit is, and how
    // worn down it is. A unit that took one hit and a unit that has been in contact
    // for a minute can share a morale value and have very different nerves, and only
    // the second one starts seeing movement in the trees.
    //
    // Exhaustion comes from Fatigue, not from MoraleComp::TicksUnderFire. That field
    // counts ticks SINCE THE LAST stimulus (it resets to 0 on every hit and goes
    // negative once a quiet window elapses), so reading it as accumulated dread was
    // simply wrong -- it is smallest exactly when a unit is being shelled hardest.
    // Fatigue is the field that accrues in contact and decays in quiet, which is the
    // quantity this stage means. Found by the refutation test failing to produce a
    // single phantom.
    const Fixed Broken = One - Observer.Morale;
    const Fixed Endured = FxClamp(Observer.Fatigue, Fixed::Zero(), One);

    // Product, not sum: a fresh unit that is merely frightened fabricates nothing,
    // and an exhausted unit with intact morale fabricates nothing either. Phantoms
    // belong to units that are both shaken AND worn out -- that conjunction is what
    // makes a phantom mean something when the player sees one.
    const Fixed Level = Broken * Endured;

    // bIsUnderFire gates it further: a unit resting behind the lines stops seeing
    // ghosts even while its fatigue drains away.
    if (!Observer.bIsUnderFire)
    {
        return false;
    }
    const Fixed Chance = PerMilleToFixed(P.FabricationChanceMaxPerMille) * Level;
    const uint32_t ChancePerMille = uint32_t(FxClamp(Chance, Fixed::Zero(), One).Raw * 1000 / kFixedOne);
    if (ChancePerMille == 0)
    {
        // Still consume no randomness: a zero-probability roll must not shift the
        // stream, or enabling the stage would change every later draw.
        return false;
    }
    return Rng.NextBelow(1000) < ChancePerMille;
}

Vec2 StageFabricationOffset(const ObserverState& Observer, const DistortionProfile& P, Random& Rng)
{
    // A phantom appears where the observer is already looking nervously: within the
    // position-error envelope, not at a random point on the map. The offset uses
    // the same radius as stage 4 so the two agree about how vague this observer is.
    const int64_t MaxUnits = int64_t(P.PositionErrorMaxTiles) * kTileSizeUnits;
    if (MaxUnits <= 0)
    {
        return Vec2();
    }
    const Fixed Radius = Fixed::FromInt(MaxUnits) *
                         FxClamp(Observer.DistanceRatio + Fixed::FromRatio(1, 2), Fixed::Zero(), Fixed::FromInt(1));
    // Two independent draws in [-1, 1]: a square envelope, which is honest about
    // being an approximation and costs no trigonometry (and no float).
    const auto Signed = [&Rng]()
    {
        return Fixed::FromRaw(int64_t(Rng.NextUInt32() & 0xFFFF)) * 2 - Fixed::FromInt(1);
    };
    return Vec2(Radius * Signed().Raw / kFixedOne, Radius * Signed().Raw / kFixedOne);
}

// --- Stage 7: self-report bias ---------------------------------------------------

int32_t StageSelfReportStrength(int32_t TrueStrength, Fixed Discipline, const DistortionProfile& P,
                                Random& Rng)
{
    if (!P.bSelfReportBiasEnabled || TrueStrength <= 0)
    {
        return TrueStrength;
    }
    // Asymmetric like fear, in the opposite direction: a unit never reports being
    // STRONGER than it is by accident, it does so to avoid admitting a rout. So the
    // bias only ever adds, and it scales with indiscipline.
    const Fixed Indiscipline = FxClamp(Fixed::FromInt(1) - Discipline, Fixed::Zero(), Fixed::FromInt(1));
    const Fixed MaxBias = PerMilleToFixed(P.SelfReportStrengthOverstatementMaxPerMille) * Indiscipline;
    const uint32_t BiasPerMille = uint32_t(FxClamp(MaxBias, Fixed::Zero(), Fixed::FromInt(4)).Raw * 1000 / kFixedOne);
    if (BiasPerMille == 0)
    {
        return TrueStrength;
    }
    const uint32_t Roll = Rng.NextBelow(BiasPerMille + 1);
    const int64_t Inflated = int64_t(TrueStrength) * (1000 + int64_t(Roll)) / 1000;
    return int32_t(Inflated);
}

int32_t StageSelfReportLosses(int32_t TrueLosses, Fixed Discipline, const DistortionProfile& P,
                              Random& Rng)
{
    if (!P.bSelfReportBiasEnabled || TrueLosses <= 0)
    {
        return TrueLosses;
    }
    // The mirror image: losses are understated, never overstated. Clamped at zero
    // so a badly disciplined unit can claim it lost nothing, but never claim it
    // gained troops by being shot at.
    const Fixed Indiscipline = FxClamp(Fixed::FromInt(1) - Discipline, Fixed::Zero(), Fixed::FromInt(1));
    const Fixed MaxHidden = PerMilleToFixed(P.SelfReportLossUnderstatementMaxPerMille) * Indiscipline;
    const uint32_t HiddenPerMille = uint32_t(FxClamp(MaxHidden, Fixed::Zero(), Fixed::FromInt(1)).Raw * 1000 / kFixedOne);
    if (HiddenPerMille == 0)
    {
        return TrueLosses;
    }
    const uint32_t Roll = Rng.NextBelow(HiddenPerMille + 1);
    const int64_t Reported = int64_t(TrueLosses) * (1000 - int64_t(Roll)) / 1000;
    return int32_t(Reported < 0 ? 0 : Reported);
}

ObservedCategory CategorizeForConfusion(bool bIsBuilding, bool bIsAircraft, bool bIsShip,
                                        bool bIsInfantry, bool bIsHeavy)
{
    if (bIsBuilding) { return ObservedCategory::Structure; }
    if (bIsAircraft) { return ObservedCategory::Aircraft; }
    if (bIsShip) { return ObservedCategory::Ship; }
    if (bIsInfantry) { return ObservedCategory::Infantry; }
    return bIsHeavy ? ObservedCategory::HeavyVehicle : ObservedCategory::LightVehicle;
}

} // namespace Recon
} // namespace RA4
