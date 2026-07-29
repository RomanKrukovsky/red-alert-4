// Copyright (c) Red Alert 4 project. Simulation <-> Unreal coordinate conversion.
//
// One header owns every conversion between the two spaces. The rule is simple and
// must stay that way: simulation world units are centimetres on the XY plane, which
// is exactly Unreal's unit and plane, so the mapping is identity plus a ground
// height. Angles are the only thing that genuinely differ.
#pragma once

#include "CoreMinimal.h"

#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"

namespace RA4Coords
{

// Height of the playable ground plane. A real heightmap replaces this with a terrain
// query; until then every conversion goes through this constant rather than a
// scattered set of literal zeroes.
constexpr double GroundZ = 0.0;

inline FVector ToUnreal(const RA4::Vec2& V, double Z = GroundZ)
{
    return FVector(V.X.ToDoubleUnsafe(), V.Y.ToDoubleUnsafe(), Z);
}

inline FVector2D ToUnreal2D(const RA4::Vec2& V)
{
    return FVector2D(V.X.ToDoubleUnsafe(), V.Y.ToDoubleUnsafe());
}

// Rounds to the nearest raw fixed unit rather than truncating, so a click does not
// bias consistently toward the origin.
inline RA4::Vec2 FromUnreal(const FVector& V)
{
    return RA4::Vec2(RA4::Fixed::FromRaw(FMath::RoundToInt64(V.X * double(RA4::kFixedOne))),
                     RA4::Fixed::FromRaw(FMath::RoundToInt64(V.Y * double(RA4::kFixedOne))));
}

// The simulation measures angles in 4096 units per full turn, not in degrees and
// not in 256 units. Getting this wrong rotates every unit on the field by a factor
// of sixteen, which looks like a mesh import problem and is not one.
inline double FacingToYawDegrees(int32 Facing)
{
    return double(RA4::WrapAngle(Facing)) * (360.0 / double(RA4::kAngleTurn));
}

inline int32 YawDegreesToFacing(double Yaw)
{
    return RA4::WrapAngle(int32(FMath::RoundToInt(Yaw * (double(RA4::kAngleTurn) / 360.0))));
}

// Intersects a camera ray with the ground plane. Returns false when the ray is
// parallel to the plane or points away from it, which happens at grazing angles and
// must not be treated as a click at the origin.
inline bool IntersectGroundPlane(const FVector& RayOrigin, const FVector& RayDirection, FVector& OutHit,
                                 double PlaneZ = GroundZ)
{
    constexpr double MinDenominator = 1e-6;
    if (FMath::Abs(RayDirection.Z) < MinDenominator)
    {
        return false;
    }
    const double Distance = (PlaneZ - RayOrigin.Z) / RayDirection.Z;
    if (Distance < 0.0)
    {
        return false;
    }
    OutHit = RayOrigin + RayDirection * Distance;
    return true;
}

} // namespace RA4Coords
