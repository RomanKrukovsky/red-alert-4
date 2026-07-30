// Copyright (c) Red Alert 4 project. Deterministic fixed-point arithmetic.
//
// The simulation must produce bit-identical results on Windows/Linux/macOS and on
// x86-64/arm64. Floating point cannot guarantee that (FMA contraction, x87 excess
// precision, differing libm implementations of sin/sqrt), so the entire simulation
// layer uses this 48.16 fixed-point type. Presentation code may use float freely.
#pragma once

#include <cstdint>
#include <limits>

#if __has_include("HAL/Platform.h")
#include "HAL/Platform.h"
#endif

#ifndef RA4CORE_API
#define RA4CORE_API
#endif

namespace RA4
{

// 16 fractional bits: resolution 1/65536 of a world unit.
// World unit == 1 centimetre, matching Unreal. A 512x512 m map is 51200 units,
// well inside the int64 range even after multiplication intermediates.
constexpr int32_t kFixedShift = 16;
constexpr int64_t kFixedOne = int64_t(1) << kFixedShift;
constexpr int64_t kFixedHalf = kFixedOne >> 1;

// Multiplies two raw fixed values through a 128-bit intermediate and shifts back.
//
// A plain (A * B) >> 16 in int64 overflows once both operands exceed about 3.0e9
// raw, i.e. 46 km... except that squared distances hit that at a separation of only
// ~460 m. Every LengthSquared() across a full-size map was silently wrapping. The
// widened intermediate is exact for the entire coordinate range and still bit-exact
// across compilers, because integer arithmetic has no implementation freedom.
inline int64_t FixedMulRaw(int64_t A, int64_t B)
{
#if defined(__SIZEOF_INT128__)
    return int64_t((__int128(A) * __int128(B)) >> kFixedShift);
#else
    // Portable 64x64 -> 128 fallback. Arithmetic shift right floors, so a negative
    // product must round away from zero to match the __int128 path exactly.
    const bool bNegative = (A < 0) != (B < 0);
    const uint64_t UA = uint64_t(A < 0 ? -A : A);
    const uint64_t UB = uint64_t(B < 0 ? -B : B);

    const uint64_t A0 = UA & 0xFFFFFFFFull, A1 = UA >> 32;
    const uint64_t B0 = UB & 0xFFFFFFFFull, B1 = UB >> 32;

    const uint64_t P00 = A0 * B0;
    const uint64_t P01 = A0 * B1;
    const uint64_t P10 = A1 * B0;
    const uint64_t P11 = A1 * B1;

    const uint64_t Middle = (P00 >> 32) + (P01 & 0xFFFFFFFFull) + (P10 & 0xFFFFFFFFull);
    const uint64_t Low = (Middle << 32) | (P00 & 0xFFFFFFFFull);
    const uint64_t High = P11 + (P01 >> 32) + (P10 >> 32) + (Middle >> 32);

    const uint64_t Shifted = (High << (64 - kFixedShift)) | (Low >> kFixedShift);
    const bool bHasRemainder = (Low & ((uint64_t(1) << kFixedShift) - 1)) != 0;

    return bNegative ? -int64_t(Shifted + (bHasRemainder ? 1u : 0u)) : int64_t(Shifted);
#endif
}

// Divides two raw fixed values, widening the numerator so that the 16-bit rescale
// cannot overflow.
inline int64_t FixedDivRaw(int64_t A, int64_t B)
{
    if (B == 0)
    {
        // Division by zero is a content or logic error, not a runtime condition the
        // simulation can recover from mid-tick. Returning zero keeps every peer in
        // agreement; the offending data is caught by ContentDatabase::Validate.
        return 0;
    }
#if defined(__SIZEOF_INT128__)
    return int64_t((__int128(A) * __int128(kFixedOne)) / __int128(B));
#else
    // Without a 128-bit type the numerator must fit in int64 after scaling, which
    // bounds operands to ~2.1e9 world units (21000 km) -- far beyond any map.
    return (A * kFixedOne) / B;
#endif
}

struct Fixed
{
    int64_t Raw = 0;

    constexpr Fixed() = default;
    constexpr explicit Fixed(int64_t InRaw) : Raw(InRaw) {}

    // Multiplication rather than a shift: left-shifting a negative value is
    // undefined behaviour before C++20, and UBSan rightly flags it.
    static constexpr Fixed FromInt(int64_t V) { return Fixed(V * kFixedOne); }
    // Exact rational construction; used by content loading so that authored decimal
    // values become a single canonical fixed value on every platform.
    static constexpr Fixed FromRatio(int64_t Num, int64_t Den) { return Fixed(Den == 0 ? 0 : (Num * kFixedOne) / Den); }
    static constexpr Fixed FromRaw(int64_t V) { return Fixed(V); }
    static constexpr Fixed Zero() { return Fixed(0); }
    static constexpr Fixed One() { return Fixed(kFixedOne); }
    static constexpr Fixed Max() { return Fixed(std::numeric_limits<int64_t>::max() / 2); }

    // Truncation toward negative infinity keeps grid indexing consistent for
    // negative coordinates (C++ integer division truncates toward zero).
    constexpr int64_t ToIntFloor() const { return Raw >> kFixedShift; }
    constexpr int64_t ToIntRound() const { return (Raw + kFixedHalf) >> kFixedShift; }
    // Only for logging / rendering. Never feed the result back into simulation state.
    double ToDoubleUnsafe() const { return double(Raw) / double(kFixedOne); }

    constexpr Fixed operator-() const { return Fixed(-Raw); }
    constexpr Fixed operator+(Fixed O) const { return Fixed(Raw + O.Raw); }
    constexpr Fixed operator-(Fixed O) const { return Fixed(Raw - O.Raw); }
    Fixed operator*(Fixed O) const { return Fixed(FixedMulRaw(Raw, O.Raw)); }
    Fixed operator/(Fixed O) const { return Fixed(FixedDivRaw(Raw, O.Raw)); }
    constexpr Fixed operator*(int64_t S) const { return Fixed(Raw * S); }
    constexpr Fixed operator/(int64_t S) const { return Fixed(S == 0 ? 0 : Raw / S); }

    Fixed& operator+=(Fixed O) { Raw += O.Raw; return *this; }
    Fixed& operator-=(Fixed O) { Raw -= O.Raw; return *this; }
    Fixed& operator*=(Fixed O) { Raw = FixedMulRaw(Raw, O.Raw); return *this; }

    constexpr bool operator==(Fixed O) const { return Raw == O.Raw; }
    constexpr bool operator!=(Fixed O) const { return Raw != O.Raw; }
    constexpr bool operator<(Fixed O) const { return Raw < O.Raw; }
    constexpr bool operator<=(Fixed O) const { return Raw <= O.Raw; }
    constexpr bool operator>(Fixed O) const { return Raw > O.Raw; }
    constexpr bool operator>=(Fixed O) const { return Raw >= O.Raw; }
};

constexpr Fixed operator""_fx(unsigned long long V) { return Fixed::FromInt(int64_t(V)); }

inline constexpr Fixed FxAbs(Fixed V) { return V.Raw < 0 ? Fixed(-V.Raw) : V; }
inline constexpr Fixed FxMin(Fixed A, Fixed B) { return A.Raw < B.Raw ? A : B; }
inline constexpr Fixed FxMax(Fixed A, Fixed B) { return A.Raw > B.Raw ? A : B; }
inline constexpr Fixed FxClamp(Fixed V, Fixed Lo, Fixed Hi) { return FxMin(FxMax(V, Lo), Hi); }

// Integer Newton-Raphson square root. Deterministic on every platform, unlike std::sqrt.
RA4CORE_API Fixed FxSqrt(Fixed V);

// Angle unit: 1 turn == kAngleTurn. Power of two so wrapping is a mask, never a modulo
// of a negative number (which is implementation-defined before C++11 and error-prone after).
constexpr int32_t kAngleBits = 12;
constexpr int32_t kAngleTurn = 1 << kAngleBits; // 4096 steps per full rotation
constexpr int32_t kAngleMask = kAngleTurn - 1;

inline constexpr int32_t WrapAngle(int32_t A) { return A & kAngleMask; }
// Shortest signed delta in (-kAngleTurn/2, kAngleTurn/2].
inline constexpr int32_t AngleDelta(int32_t From, int32_t To)
{
    int32_t D = WrapAngle(To - From);
    return D > kAngleTurn / 2 ? D - kAngleTurn : D;
}

RA4CORE_API Fixed FxSin(int32_t Angle);
RA4CORE_API Fixed FxCos(int32_t Angle);
// Full-circle arctangent returning the fixed-point angle unit above.
RA4CORE_API int32_t FxAtan2(Fixed Y, Fixed X);

} // namespace RA4
