// Copyright (c) Red Alert 4 project. Lockstep Deterministic Command Bus.
#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "RA4Core/Command.h"
#include "RA4Core/Ids.h"

namespace RA4
{

class SimWorld;

#ifndef RA4SIMULATION_API
#define RA4SIMULATION_API
#endif

class RA4SIMULATION_API CommandBus
{
public:
    CommandBus() = default;

    /** Enqueues a single command for execution at TargetTick. */
    void EnqueueCommand(TickIndex TargetTick, const Command& Cmd);

    /** Enqueues an entire CommandFrame from network/replay. */
    void EnqueueFrame(const CommandFrame& Frame);

    /** Fetches the assembled CommandFrame for the given tick. */
    CommandFrame FetchFrameForTick(TickIndex Tick) const;

    /** Dispatches and consumes the buffered frame for Tick exactly once, returning
        how many commands were accepted by validation. */
    int32_t DispatchTick(TickIndex Tick, SimWorld& World);

    /** Clears processed commands up to and including Tick. */
    void ClearUpToTick(TickIndex Tick);

    /** Returns current total pending command count across all buffered ticks. */
    size_t GetPendingCommandCount() const;

private:
    std::map<TickIndex, CommandFrame> FrameBuffer;
};

} // namespace RA4
