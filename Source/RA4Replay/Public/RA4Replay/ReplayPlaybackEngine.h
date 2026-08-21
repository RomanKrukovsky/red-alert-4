// Copyright (c) Red Alert 4 project. Replay playback engine with snapshot scrubbing.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include <string>

#include "RA4Core/Ids.h"
#include "RA4Replay/Replay.h"
#include "RA4Simulation/SimSnapshot.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4REPLAY_API
#define RA4REPLAY_API
#endif

namespace RA4
{

enum class PlaybackState : uint8_t
{
    Stopped = 0,
    Playing,
    Paused,
    Completed
};

struct ReplayKeyframe
{
    TickIndex Tick = 0;
    SimSnapshot Snapshot;
};

class RA4REPLAY_API ReplayPlaybackEngine
{
public:
    ReplayPlaybackEngine() = default;

    /** Loads replay data and initializes simulation world to tick 0.
        KeyframeIntervalTicks determines snapshot caching cadence for instant scrubbing. */
    bool Load(const ReplayData& InReplay, const ContentDatabase& Content,
              uint32_t KeyframeIntervalTicks = 50);

    /** Loads replay data and applies an initial snapshot representing the state at tick 0. */
    bool LoadWithInitialSnapshot(const ReplayData& InReplay, const ContentDatabase& Content,
                                 const SimSnapshot& InitialSnapshot, uint32_t KeyframeIntervalTicks = 50);

    void Play();
    void Pause();
    void Stop();

    void SetPlaybackSpeed(float Multiplier) { SpeedMultiplier = Multiplier > 0.0f ? Multiplier : 1.0f; }
    float GetPlaybackSpeed() const { return SpeedMultiplier; }
    PlaybackState GetState() const { return State; }

    /** Advances replay by DeltaTimeSeconds (respecting SpeedMultiplier). Returns number of ticks simulated. */
    uint32_t Step(float DeltaTimeSeconds);

    /** Instantaneously seeks to TargetTick. If TargetTick < CurrentTick, restores from closest
        prior snapshot keyframe and simulates forward. */
    bool SeekToTick(TickIndex TargetTick);

    TickIndex GetCurrentTick() const { return World ? World->GetTick() : 0; }
    TickIndex GetTotalTicks() const { return Replay.FinalTick; }
    float GetProgress() const;

    const SimWorld* GetWorld() const { return World.get(); }
    const ReplayData& GetReplayData() const { return Replay; }

private:
    void CaptureKeyframe(TickIndex Tick);
    bool RestoreClosestKeyframe(TickIndex TargetTick);
    const CommandFrame* FindFrameForTick(TickIndex Tick) const;

    ReplayData Replay;
    const ContentDatabase* ContentDb = nullptr;
    std::unique_ptr<SimWorld> World;

    PlaybackState State = PlaybackState::Stopped;
    float SpeedMultiplier = 1.0f;
    float AccumulatedTimeSeconds = 0.0f;
    uint32_t KeyframeInterval = 50;

    // Cache of CommandFrames indexed by tick for fast lookup
    std::map<TickIndex, size_t> FrameIndexByTick;

    // Snapshot keyframes for instant seek scrubbing: [Tick -> Snapshot]
    std::map<TickIndex, SimSnapshot> Keyframes;
};

} // namespace RA4
