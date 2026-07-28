// Copyright (c) Red Alert 4 project. Constants that define the simulation contract.
#pragma once

#include <cstdint>

#include "RA4Core/Fixed.h"

namespace RA4
{

// 20 Hz. Chosen over 30 Hz because command latency in an RTS is dominated by the
// network round trip and the input buffer, not the tick, while the per-tick cost of
// 2000 entities is what actually limits us. Presentation interpolates between ticks.
constexpr int32_t kTicksPerSecond = 20;
constexpr int64_t kMillisecondsPerTick = 1000 / kTicksPerSecond;

// Converts an authored "per second" rate into the per-tick delta the systems apply.
inline constexpr Fixed PerSecondToPerTick(Fixed PerSecond) { return PerSecond / int64_t(kTicksPerSecond); }
inline constexpr int32_t SecondsToTicks(int32_t Seconds) { return Seconds * kTicksPerSecond; }

// World units are centimetres, matching Unreal's default so that presentation needs
// no scaling factor. One build tile is 2 metres.
constexpr int64_t kTileSizeUnits = 200;
constexpr Fixed kTileSize = Fixed::FromInt(kTileSizeUnits);

// Hard ceilings. Exceeding them is a content or map authoring error, not a runtime
// condition, so they are asserted rather than handled.
constexpr uint32_t kMaxEntities = 8192;
constexpr int32_t kMaxMapTiles = 512;
constexpr int32_t kMaxProductionQueueLength = 9;

// Simulation state checksums are exchanged on this cadence. Every tick would cost
// bandwidth for no benefit; too rare and the desync report points nowhere near the
// actual divergence.
constexpr int32_t kChecksumIntervalTicks = 20;

} // namespace RA4
