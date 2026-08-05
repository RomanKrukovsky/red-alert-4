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
