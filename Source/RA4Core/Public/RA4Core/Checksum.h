// Copyright (c) Red Alert 4 project. Desync detection primitives.
#pragma once

#include <cstdint>
#include <cstddef>

namespace RA4
{

// FNV-1a 64. Not cryptographic -- the job is to notice that two machines diverged,
// not to resist an attacker forging a matching state. Clients exchange these every
// N ticks; a mismatch triggers the desync dump described in Docs/ADR/0002.
class Hash64
{
public:
    void Feed(const void* Data, size_t Size)
    {
        const uint8_t* Bytes = static_cast<const uint8_t*>(Data);
        for (size_t I = 0; I < Size; ++I)
        {
            Value ^= uint64_t(Bytes[I]);
            Value *= 1099511628211ULL;
        }
    }

    void FeedUInt32(uint32_t V) { Feed(&V, sizeof(V)); }
    void FeedInt32(int32_t V) { Feed(&V, sizeof(V)); }
    void FeedUInt64(uint64_t V) { Feed(&V, sizeof(V)); }
    void FeedInt64(int64_t V) { Feed(&V, sizeof(V)); }
    void FeedUInt8(uint8_t V) { Feed(&V, sizeof(V)); }
    void FeedBool(bool V) { const uint8_t B = V ? 1u : 0u; Feed(&B, 1); }

    uint64_t Get() const { return Value; }
    void Reset() { Value = 14695981039346656037ULL; }

private:
    uint64_t Value = 14695981039346656037ULL;
};

inline uint64_t HashBytes(const void* Data, size_t Size)
{
    Hash64 H;
    H.Feed(Data, Size);
    return H.Get();
}

} // namespace RA4
