// Copyright (c) Red Alert 4 project. Lockstep Deterministic Command Bus implementation.
#include "RA4Simulation/CommandBus.h"
#include "RA4Simulation/SimWorld.h"

namespace RA4
{

void CommandBus::EnqueueCommand(TickIndex TargetTick, const Command& Cmd)
{
    CommandFrame& Frame = FrameBuffer[TargetTick];
    Frame.Tick = TargetTick;
    Frame.Commands.push_back(Cmd);
}

void CommandBus::EnqueueFrame(const CommandFrame& Frame)
{
    CommandFrame& Target = FrameBuffer[Frame.Tick];
    Target.Tick = Frame.Tick;
    for (const auto& Cmd : Frame.Commands)
    {
        Target.Commands.push_back(Cmd);
    }
}

CommandFrame CommandBus::FetchFrameForTick(TickIndex Tick) const
{
    auto It = FrameBuffer.find(Tick);
    if (It != FrameBuffer.end())
    {
        return It->second;
    }
    CommandFrame EmptyFrame;
    EmptyFrame.Tick = Tick;
    return EmptyFrame;
}

int32_t CommandBus::DispatchTick(TickIndex Tick, SimWorld& World)
{
    CommandFrame Frame = FetchFrameForTick(Tick);
    int32_t ExecutedCount = 0;
    
    // Apply each command in deterministic frame order
    for (const Command& Cmd : Frame.Commands)
    {
        CommandResult Res = World.ApplyCommand(Cmd);
        if (Res.IsAccepted())
        {
            ExecutedCount++;
        }
    }

    // Step simulation tick with the applied frame
    World.Tick(&Frame);

    return ExecutedCount;
}

void CommandBus::ClearUpToTick(TickIndex Tick)
{
    auto It = FrameBuffer.begin();
    while (It != FrameBuffer.end())
    {
        if (It->first <= Tick)
        {
            It = FrameBuffer.erase(It);
        }
        else
        {
            ++It;
        }
    }
}

size_t CommandBus::GetPendingCommandCount() const
{
    size_t Total = 0;
    for (const auto& Pair : FrameBuffer)
    {
        Total += Pair.second.Commands.size();
    }
    return Total;
}

} // namespace RA4
