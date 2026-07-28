// Copyright (c) Red Alert 4 project.
//
// Integer-only implementations of sqrt / sin / cos / atan2. No libm call appears
// anywhere in the simulation, so results are identical on every target.
#include "RA4Core/Fixed.h"

namespace RA4
{

Fixed FxSqrt(Fixed V)
{
    if (V.Raw <= 0)
    {
        return Fixed::Zero();
    }

    // sqrt(x * 2^16) in raw space equals sqrt(raw) * 2^8, so compute the integer
    // square root of (raw << 16) to land back in 16-bit fractional space.
    // Guard against overflow: raw < 2^47 keeps (raw << 16) inside int64.
    uint64_t Value = uint64_t(V.Raw);
    int32_t ExtraShift = 0;
    while (Value >= (uint64_t(1) << 47))
    {
        Value >>= 2;      // dividing radicand by 4 ...
        ExtraShift += 1;  // ... halves the root, restored below
    }
    Value <<= kFixedShift;

    // Classic restoring integer sqrt, bit at a time from the highest even power.
    uint64_t Result = 0;
    uint64_t Bit = uint64_t(1) << 62;
    while (Bit > Value)
    {
        Bit >>= 2;
    }
    while (Bit != 0)
    {
        if (Value >= Result + Bit)
        {
            Value -= Result + Bit;
            Result = (Result >> 1) + Bit;
        }
        else
        {
            Result >>= 1;
        }
        Bit >>= 2;
    }

    return Fixed(int64_t(Result) << ExtraShift);
}

namespace
{
// CORDIC works in a finer angle space than the public 4096-step unit so that the
// public unit is exact rather than the limit of precision.
constexpr int32_t kFineBits = 24;
constexpr int32_t kFineTurn = 1 << kFineBits;
constexpr int32_t kFineFromPublic = kFineBits - kAngleBits; // 12

constexpr int32_t kCordicIterations = 20;

// atan(2^-i) expressed in fine angle units (round(atan(2^-i)/(2*pi) * 2^24)).
constexpr int32_t kCordicAtan[kCordicIterations] = {
    2097152, 1238021, 654136, 332050, 166669, 83416, 41718, 20860, 10430, 5215,
    2608,    1304,    652,    326,    163,    81,    41,    20,    10,    5};

// 1 / product(sqrt(1 + 2^-2i)) in 16.16, pre-applied so the rotated vector is unit length.
constexpr int64_t kCordicInvGain = 39797;

// Rotates (kCordicInvGain, 0) by FineAngle, which must lie in [-kFineTurn/4, kFineTurn/4].
void CordicRotate(int32_t FineAngle, int64_t& OutCos, int64_t& OutSin)
{
    int64_t X = kCordicInvGain;
    int64_t Y = 0;
    int32_t Z = FineAngle;

    for (int32_t I = 0; I < kCordicIterations; ++I)
    {
        const int64_t Xi = X >> I;
        const int64_t Yi = Y >> I;
        if (Z >= 0)
        {
            X -= Yi;
            Y += Xi;
            Z -= kCordicAtan[I];
        }
        else
        {
            X += Yi;
            Y -= Xi;
            Z += kCordicAtan[I];
        }
    }

    OutCos = X;
    OutSin = Y;
}

// Reduces any angle to the first quadrant and restores the sign afterwards.
void SinCos(int32_t Angle, int64_t& OutSin, int64_t& OutCos)
{
    const int32_t Wrapped = WrapAngle(Angle);
    const int32_t Quadrant = Wrapped >> (kAngleBits - 2);          // 0..3
    const int32_t InQuadrant = Wrapped & ((1 << (kAngleBits - 2)) - 1);
    int64_t C = 0;
    int64_t S = 0;
    if (InQuadrant == 0)
    {
        // CORDIC leaves a residual of a few raw units even at zero rotation. The
        // cardinal directions are exactly the ones units travel along most often, so
        // snap them: a unit ordered due east must not drift north over a thousand
        // ticks.
        C = kFixedOne;
        S = 0;
    }
    else
    {
        CordicRotate(InQuadrant << kFineFromPublic, C, S);
    }

    switch (Quadrant)
    {
        case 0: OutCos = C;  OutSin = S;  break;
        case 1: OutCos = -S; OutSin = C;  break;
        case 2: OutCos = -C; OutSin = -S; break;
        default: OutCos = S; OutSin = -C; break;
    }
}
} // namespace

Fixed FxSin(int32_t Angle)
{
    int64_t S = 0;
    int64_t C = 0;
    SinCos(Angle, S, C);
    return Fixed(S);
}

Fixed FxCos(int32_t Angle)
{
    int64_t S = 0;
    int64_t C = 0;
    SinCos(Angle, S, C);
    return Fixed(C);
}

int32_t FxAtan2(Fixed Y, Fixed X)
{
    if (X.Raw == 0 && Y.Raw == 0)
    {
        return 0;
    }

    // Fold into the first octant-pair the CORDIC vectoring mode can handle
    // (x > 0), tracking the quadrant offset separately.
    int64_t Vx = X.Raw;
    int64_t Vy = Y.Raw;
    int32_t Offset = 0;

    if (Vx < 0)
    {
        if (Vy >= 0)
        {
            // Rotate -90 degrees: (x,y) -> (y,-x)
            const int64_t T = Vx;
            Vx = Vy;
            Vy = -T;
            Offset = kFineTurn / 4;
        }
        else
        {
            // Rotate +90 degrees: (x,y) -> (-y,x)
            const int64_t T = Vx;
            Vx = -Vy;
            Vy = T;
            Offset = -kFineTurn / 4;
        }
    }

    // Keep magnitudes small enough that the shifts below cannot overflow.
    while (Vx > (int64_t(1) << 40) || Vy > (int64_t(1) << 40) || Vy < -(int64_t(1) << 40))
    {
        Vx >>= 1;
        Vy >>= 1;
    }

    int32_t Z = 0;
    for (int32_t I = 0; I < kCordicIterations; ++I)
    {
        const int64_t Xi = Vx >> I;
        const int64_t Yi = Vy >> I;
        if (Vy > 0)
        {
            Vx += Yi;
            Vy -= Xi;
            Z += kCordicAtan[I];
        }
        else
        {
            Vx -= Yi;
            Vy += Xi;
            Z -= kCordicAtan[I];
        }
    }

    const int32_t Fine = Z + Offset;
    // Round-to-nearest when converting back to the coarse public unit.
    const int32_t Public = (Fine + (1 << (kFineFromPublic - 1))) >> kFineFromPublic;
    return WrapAngle(Public);
}

} // namespace RA4
