// Copyright (c) Red Alert 4 project. Stable identifiers shared by every module.
#pragma once

#include <cstdint>

namespace RA4
{

// Slot + generation handle. The slot indexes the dense simulation arrays; the
// generation invalidates every copy of the handle when the slot is recycled, so a
// queued order that referenced a destroyed tank resolves to "gone" instead of
// silently retargeting whatever was built in its place.
struct EntityId
{
    // Defaults to the invalid slot, never to slot 0. A default-constructed handle
    // that happened to be valid meant "no target" resolved to whichever entity was
    // allocated first, so units opened fire on themselves and turrets shelled their
    // own headquarters.
    uint32_t Index = 0xFFFFFFFFu;
    uint32_t Generation = 0;

    constexpr EntityId() = default;
    constexpr EntityId(uint32_t InIndex, uint32_t InGeneration) : Index(InIndex), Generation(InGeneration) {}

    static constexpr EntityId Invalid() { return EntityId(0xFFFFFFFFu, 0u); }
    constexpr bool IsValid() const { return Index != 0xFFFFFFFFu; }
    constexpr bool operator==(const EntityId& O) const { return Index == O.Index && Generation == O.Generation; }
    constexpr bool operator!=(const EntityId& O) const { return !(*this == O); }

    constexpr uint64_t Packed() const { return (uint64_t(Generation) << 32) | uint64_t(Index); }
};

// Player slots are fixed for the lifetime of a match (8 players + neutral/observer).
enum : uint8_t
{
    kMaxPlayers = 8,
    kNeutralPlayer = 8,
    kInvalidPlayer = 0xFF,
};

using PlayerId = uint8_t;

// Content handles are hashes of a stable string name, not array indices, so that a
// mod adding units does not renumber existing content and invalidate replays.
struct ContentId
{
    uint32_t Value = 0;

    constexpr ContentId() = default;
    constexpr explicit ContentId(uint32_t InValue) : Value(InValue) {}
    constexpr bool IsValid() const { return Value != 0; }
    constexpr bool operator==(const ContentId& O) const { return Value == O.Value; }
    constexpr bool operator!=(const ContentId& O) const { return !(*this == O); }
    constexpr bool operator<(const ContentId& O) const { return Value < O.Value; }
};

// FNV-1a over the name. constexpr so content ids can be formed at compile time in
// C++ and at load time from data assets, producing the same value either way.
constexpr uint32_t HashName(const char* Str)
{
    uint32_t Hash = 2166136261u;
    while (*Str)
    {
        Hash ^= uint32_t(uint8_t(*Str++));
        Hash *= 16777619u;
    }
    return Hash;
}

constexpr ContentId MakeContentId(const char* Name) { return ContentId(HashName(Name)); }

// Simulation time. Tick count rather than seconds: the tick is the unit of
// authority, and any conversion to seconds happens only for display.
using TickIndex = uint32_t;

} // namespace RA4
