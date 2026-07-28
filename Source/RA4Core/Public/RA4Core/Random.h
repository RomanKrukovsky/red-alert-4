// Copyright (c) Red Alert 4 project. Deterministic RNG for the simulation.
#pragma once

#include <cstdint>

#include "RA4Core/Fixed.h"

namespace RA4
{

// PCG-XSH-RR 64/32. Chosen over std::mt19937 because the C++ standard fixes the
// algorithm but not the distribution implementations, and over a raw LCG because
// the low bits of an LCG are badly correlated -- which shows up immediately when
// you use them to pick scatter offsets for artillery.
//
// Every stochastic decision in the simulation draws from the single match stream so
// that a replay reproduces them exactly. Presentation-only randomness (debris
// direction, muzzle flash variation) must NOT use this object.
class Random
{
public:
    Random() = default;
    explicit Random(uint64_t Seed, uint64_t Sequence = 0xda3e39cb94b95bdbULL) { Reset(Seed, Sequence); }

    void Reset(uint64_t Seed, uint64_t Sequence = 0xda3e39cb94b95bdbULL)
    {
        Increment = (Sequence << 1u) | 1u;
        State = 0;
        NextUInt32();
        State += Seed;
        NextUInt32();
    }

    uint32_t NextUInt32()
    {
        const uint64_t Old = State;
        State = Old * 6364136223846793005ULL + Increment;
        const uint32_t Xorshifted = uint32_t(((Old >> 18u) ^ Old) >> 27u);
        const uint32_t Rot = uint32_t(Old >> 59u);
        return (Xorshifted >> Rot) | (Xorshifted << ((~Rot + 1u) & 31u));
    }

    // Uniform in [0, Bound). Uses rejection sampling rather than a modulo so the
    // distribution has no bias -- biased rolls are invisible in testing but change
    // balance measurably over a long match.
    uint32_t NextBelow(uint32_t Bound)
    {
        if (Bound == 0)
        {
            return 0;
        }
        const uint32_t Threshold = (~Bound + 1u) % Bound;
        for (;;)
        {
            const uint32_t R = NextUInt32();
            if (R >= Threshold)
            {
                return R % Bound;
            }
        }
    }

    // Inclusive range, matching how designers express values in data tables.
    int32_t NextRange(int32_t MinInclusive, int32_t MaxInclusive)
    {
        if (MaxInclusive <= MinInclusive)
        {
            return MinInclusive;
        }
        return MinInclusive + int32_t(NextBelow(uint32_t(MaxInclusive - MinInclusive + 1)));
    }

    // Fixed value in [0, 1).
    Fixed NextUnitFixed() { return Fixed(int64_t(NextUInt32() & 0xFFFF)); }

    bool NextChancePercent(int32_t Percent) { return int32_t(NextBelow(100)) < Percent; }

    uint64_t GetState() const { return State; }
    uint64_t GetIncrement() const { return Increment; }
    void SetState(uint64_t InState, uint64_t InIncrement) { State = InState; Increment = InIncrement; }

private:
    uint64_t State = 0x853c49e6748fea9bULL;
    uint64_t Increment = 0xda3e39cb94b95bdbULL;
};

} // namespace RA4
