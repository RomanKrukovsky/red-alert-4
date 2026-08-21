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

// Subsystem-isolated breakdown of the simulation state checksum.
// When a desync occurs in multiplayer, comparing these individual subsystem hashes
// immediately pinpoints the exact culprit subsystem without binary diffing.
struct StateHashBreakdown
{
    uint64_t Overall = 0;
    uint64_t Entities = 0;
    uint64_t Positions = 0;
    uint64_t Health = 0;
    uint64_t Economy = 0;
    uint64_t Combat = 0;
    uint64_t Production = 0;
    uint64_t Orders = 0;
    uint64_t Rng = 0;

    bool operator==(const StateHashBreakdown& O) const
    {
        return Overall == O.Overall &&
               Entities == O.Entities &&
               Positions == O.Positions &&
               Health == O.Health &&
               Economy == O.Economy &&
               Combat == O.Combat &&
               Production == O.Production &&
               Orders == O.Orders &&
               Rng == O.Rng;
    }

    bool operator!=(const StateHashBreakdown& O) const { return !(*this == O); }

    // Returns false if identical; returns true and appends diff explanations if diverged.
    bool FindDivergence(const StateHashBreakdown& Peer, char* OutBuffer, size_t BufferSize) const
    {
        if (*this == Peer)
        {
            return false;
        }

        if (!OutBuffer || BufferSize == 0)
        {
            return true;
        }

        size_t Written = 0;
        auto Append = [&](const char* Subsystem, uint64_t LocalVal, uint64_t PeerVal)
        {
            if (LocalVal != PeerVal && Written < BufferSize - 1)
            {
                // Simple append without snprintf dependencies
                const char* Prefix = (Written == 0) ? "Desync in: " : ", ";
                while (*Prefix && Written < BufferSize - 1) OutBuffer[Written++] = *Prefix++;
                while (*Subsystem && Written < BufferSize - 1) OutBuffer[Written++] = *Subsystem++;
            }
        };

        Append("Entities", Entities, Peer.Entities);
        Append("Positions", Positions, Peer.Positions);
        Append("Health", Health, Peer.Health);
        Append("Economy", Economy, Peer.Economy);
        Append("Combat", Combat, Peer.Combat);
        Append("Production", Production, Peer.Production);
        Append("Orders", Orders, Peer.Orders);
        Append("Rng", Rng, Peer.Rng);

        OutBuffer[Written] = '\0';
        return true;
    }
};

} // namespace RA4
