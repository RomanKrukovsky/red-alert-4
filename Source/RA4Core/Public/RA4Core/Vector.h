// Copyright (c) Red Alert 4 project. Fixed-point vectors used by the simulation.
#pragma once

#include "RA4Core/Fixed.h"

namespace RA4
{

// The simulation is 2.5D: pathing, combat ranges and collision are solved on the XY
// plane, while Z is carried as a separate layer altitude (ground/air/naval). Full 3D
// vectors exist only in presentation code.
struct Vec2
{
    Fixed X;
    Fixed Y;

    constexpr Vec2() = default;
    constexpr Vec2(Fixed InX, Fixed InY) : X(InX), Y(InY) {}

    static constexpr Vec2 Zero() { return Vec2(Fixed::Zero(), Fixed::Zero()); }
    static constexpr Vec2 FromInts(int64_t InX, int64_t InY) { return Vec2(Fixed::FromInt(InX), Fixed::FromInt(InY)); }

    constexpr Vec2 operator+(const Vec2& O) const { return Vec2(X + O.X, Y + O.Y); }
    constexpr Vec2 operator-(const Vec2& O) const { return Vec2(X - O.X, Y - O.Y); }
    constexpr Vec2 operator-() const { return Vec2(-X, -Y); }
    constexpr Vec2 operator*(Fixed S) const { return Vec2(X * S, Y * S); }
    constexpr Vec2 operator/(Fixed S) const { return Vec2(X / S, Y / S); }
    Vec2& operator+=(const Vec2& O) { X += O.X; Y += O.Y; return *this; }
    Vec2& operator-=(const Vec2& O) { X -= O.X; Y -= O.Y; return *this; }
    constexpr bool operator==(const Vec2& O) const { return X == O.X && Y == O.Y; }
    constexpr bool operator!=(const Vec2& O) const { return !(*this == O); }

    // Squared length is the preferred comparison primitive: it avoids a sqrt and
    // therefore avoids both cost and rounding in the hot combat/targeting paths.
    constexpr Fixed LengthSquared() const { return X * X + Y * Y; }
    Fixed Length() const { return FxSqrt(LengthSquared()); }

    Vec2 Normalized() const
    {
        const Fixed Len = Length();
        return Len.Raw == 0 ? Vec2::Zero() : Vec2(X / Len, Y / Len);
    }

    int32_t ToAngle() const { return FxAtan2(Y, X); }

    static Vec2 FromAngle(int32_t Angle) { return Vec2(FxCos(Angle), FxSin(Angle)); }
};

inline Fixed DistanceSquared(const Vec2& A, const Vec2& B) { return (A - B).LengthSquared(); }
inline Fixed Distance(const Vec2& A, const Vec2& B) { return (A - B).Length(); }
inline Fixed Dot(const Vec2& A, const Vec2& B) { return A.X * B.X + A.Y * B.Y; }

// Integer tile coordinate. The build grid, navigation grid and fog grid all share
// this type so a cell index means the same thing in every subsystem.
struct TileCoord
{
    int32_t X = 0;
    int32_t Y = 0;

    constexpr TileCoord() = default;
    constexpr TileCoord(int32_t InX, int32_t InY) : X(InX), Y(InY) {}

    constexpr bool operator==(const TileCoord& O) const { return X == O.X && Y == O.Y; }
    constexpr bool operator!=(const TileCoord& O) const { return !(*this == O); }
};

} // namespace RA4
