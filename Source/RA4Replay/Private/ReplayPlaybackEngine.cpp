// Copyright (c) Red Alert 4 project. Replay playback engine implementation.
#include "RA4Replay/ReplayPlaybackEngine.h"

#include <algorithm>
#include <cmath>

namespace RA4
{

namespace
{

MatchSetup MakeSetupFromHeader(const ReplayHeader& Header)
{
    MatchSetup Setup;
    Setup.Seed = Header.Seed;
    Setup.Map.Name = Header.MapName;
    Setup.Map.Resize(Header.MapWidth, Header.MapHeight, Tile_GroundPassable);
    if (!Header.MapTiles.empty() && Header.MapTiles.size() == Setup.Map.Tiles.size())
    {
        Setup.Map.Tiles = Header.MapTiles;
    }

    for (PlayerId P = 0; P < kMaxPlayers; ++P)
    {
        Setup.Players[P].bActive = Header.Players[P].bActive;
        Setup.Players[P].Team = Header.Players[P].Team;
        Setup.Players[P].Faction = static_cast<FactionId>(Header.Players[P].Faction);
        Setup.Players[P].StartingCredits = Header.Players[P].StartingCredits;
        Setup.Players[P].StartPositionIndex = Header.Players[P].StartPositionIndex;
    }

    return Setup;
}

} // namespace

bool ReplayPlaybackEngine::Load(const ReplayData& InReplay, const ContentDatabase& Content,
                                uint32_t KeyframeIntervalTicks)
{
    Replay = InReplay;
    ContentDb = &Content;
    KeyframeInterval = KeyframeIntervalTicks > 0 ? KeyframeIntervalTicks : 50;

    FrameIndexByTick.clear();
    for (size_t I = 0; I < Replay.Frames.size(); ++I)
    {
        FrameIndexByTick[Replay.Frames[I].Tick] = I;
    }

    Keyframes.clear();

    MatchSetup Setup = MakeSetupFromHeader(Replay.Header);
    World = std::make_unique<SimWorld>();
    World->Initialize(ContentDb, Setup);

    CaptureKeyframe(0);

    State = PlaybackState::Paused;
    SpeedMultiplier = 1.0f;
    AccumulatedTimeSeconds = 0.0f;

    return true;
}

bool ReplayPlaybackEngine::LoadWithInitialSnapshot(const ReplayData& InReplay, const ContentDatabase& Content,
                                                   const SimSnapshot& InitialSnapshot, uint32_t KeyframeIntervalTicks)
{
    if (!Load(InReplay, Content, KeyframeIntervalTicks))
    {
        return false;
    }

    if (InitialSnapshot.bValid && !InitialSnapshot.StateBuffer.empty())
    {
        World->RestoreFromSnapshot(InitialSnapshot);
        Keyframes[0] = InitialSnapshot;
    }

    return true;
}

void ReplayPlaybackEngine::Play()
{
    if (State != PlaybackState::Completed)
    {
        State = PlaybackState::Playing;
    }
}

void ReplayPlaybackEngine::Pause()
{
    if (State == PlaybackState::Playing)
    {
        State = PlaybackState::Paused;
    }
}

void ReplayPlaybackEngine::Stop()
{
    State = PlaybackState::Stopped;
    SeekToTick(0);
}

float ReplayPlaybackEngine::GetProgress() const
{
    if (Replay.FinalTick == 0 || !World)
    {
        return 0.0f;
    }
    return std::clamp(static_cast<float>(World->GetTick()) / static_cast<float>(Replay.FinalTick), 0.0f, 1.0f);
}

const CommandFrame* ReplayPlaybackEngine::FindFrameForTick(TickIndex Tick) const
{
    auto It = FrameIndexByTick.find(Tick);
    if (It != FrameIndexByTick.end() && It->second < Replay.Frames.size())
    {
        return &Replay.Frames[It->second];
    }
    return nullptr;
}

void ReplayPlaybackEngine::CaptureKeyframe(TickIndex Tick)
{
    if (!World) return;
    if (Keyframes.find(Tick) == Keyframes.end())
    {
        Keyframes[Tick] = World->CaptureSnapshot();
    }
}

bool ReplayPlaybackEngine::RestoreClosestKeyframe(TickIndex TargetTick)
{
    if (Keyframes.empty() || !World)
    {
        return false;
    }

    // Find upper bound and step back one to get largest keyframe <= TargetTick
    auto It = Keyframes.upper_bound(TargetTick);
    if (It != Keyframes.begin())
    {
        --It;
        return World->RestoreFromSnapshot(It->second);
    }

    // Fallback to first keyframe (tick 0)
    return World->RestoreFromSnapshot(Keyframes.begin()->second);
}

bool ReplayPlaybackEngine::SeekToTick(TickIndex TargetTick)
{
    if (!World)
    {
        return false;
    }

    const TickIndex ClampedTarget = std::min(TargetTick, Replay.FinalTick);

    if (ClampedTarget < World->GetTick())
    {
        if (!RestoreClosestKeyframe(ClampedTarget))
        {
            return false;
        }
    }

    // Simulate forward to exact target tick
    while (World->GetTick() < ClampedTarget && World->GetPhase() == MatchPhase::Running)
    {
        const TickIndex CurrentT = World->GetTick();
        if (CurrentT % KeyframeInterval == 0)
        {
            CaptureKeyframe(CurrentT);
        }

        const CommandFrame* Frame = FindFrameForTick(CurrentT);
        const TickIndex OldT = World->GetTick();
        World->Tick(Frame);
        if (World->GetTick() == OldT)
        {
            // Simulation didn't advance (e.g. match completed)
            break;
        }
    }

    if (World->GetTick() >= Replay.FinalTick || World->GetPhase() != MatchPhase::Running)
    {
        State = PlaybackState::Completed;
    }

    return true;
}

uint32_t ReplayPlaybackEngine::Step(float DeltaTimeSeconds)
{
    if (State != PlaybackState::Playing || !World || DeltaTimeSeconds <= 0.0f)
    {
        return 0;
    }

    AccumulatedTimeSeconds += DeltaTimeSeconds * SpeedMultiplier;
    constexpr float kTickDuration = 0.05f; // 20 Hz
    constexpr float kEpsilon = 1e-4f;
    uint32_t TicksAdvanced = 0;

    while ((AccumulatedTimeSeconds + kEpsilon) >= kTickDuration && World->GetPhase() == MatchPhase::Running)
    {
        AccumulatedTimeSeconds -= kTickDuration;
        if (AccumulatedTimeSeconds < 0.0f)
        {
            AccumulatedTimeSeconds = 0.0f;
        }

        const TickIndex CurrentT = World->GetTick();
        if (CurrentT >= Replay.FinalTick)
        {
            State = PlaybackState::Completed;
            break;
        }


        if (CurrentT % KeyframeInterval == 0)
        {
            CaptureKeyframe(CurrentT);
        }

        const CommandFrame* Frame = FindFrameForTick(CurrentT);
        const TickIndex OldT = World->GetTick();
        World->Tick(Frame);
        if (World->GetTick() == OldT)
        {
            State = PlaybackState::Completed;
            break;
        }
        TicksAdvanced++;
    }

    if (World->GetPhase() != MatchPhase::Running)
    {
        State = PlaybackState::Completed;
    }

    return TicksAdvanced;
}

} // namespace RA4
