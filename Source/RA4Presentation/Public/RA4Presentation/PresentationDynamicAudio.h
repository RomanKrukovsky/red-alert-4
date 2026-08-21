// Copyright (c) Red Alert 4 project. Dynamic interactive music director and unit voice bark matrix.
#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include "RA4Core/Command.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"

#include "RA4Core/Vector.h"
#include "RA4Simulation/SimTypes.h"

#ifndef RA4PRESENTATION_API
#define RA4PRESENTATION_API
#endif

namespace RA4
{

enum class MusicIntensityLayer : uint8_t
{
    Ambient = 0,
    Suspense,
    HeavyCombat,
    Count
};

enum class VoiceBarkKind : uint8_t
{
    Selected = 0,
    MoveOrdered,
    AttackOrdered,
    UnderFire,
    UnitPanicking,
    VictoryCheer,
    DeathCry
};

struct ActiveVoiceBark
{
    std::string SoundCueId;
    std::string SubtitleTextKey;
    VoiceBarkKind Kind = VoiceBarkKind::Selected;
    uint8_t Priority = 1; // 1 (lowest) to 10 (highest, e.g. base under attack)
    float DurationSeconds = 2.0f;
    float ElapsedSeconds = 0.0f;
    Vec2 WorldLocation;
    bool bIs3D = false;
};

class RA4PRESENTATION_API PresentationDynamicAudio
{
public:
    PresentationDynamicAudio();

    /** Consumes SimEvents to update combat heat and trigger reactive voice barks. */
    void ConsumeSimEvents(const std::vector<SimEvent>& Events, PlayerId LocalPlayer);

    /** Ingests player command input to emit order feedback barks. */
    void OnCommandIssued(PlayerId Player, const Command& Cmd, const std::string& UnitVoiceProfile);

    /** Advances dynamic music layer weights and active voice line playback. */
    void Update(float DeltaTimeSeconds);

    /** Returns current volume weight [0.0..1.0] for the given music layer. */
    float GetLayerVolume(MusicIntensityLayer Layer) const;

    /** Returns currently playing voice bark, or nullptr if none. */
    const ActiveVoiceBark* GetCurrentVoiceBark() const;

    /** Manually queues a voice bark with priority arbitration. */
    bool QueueVoiceBark(const ActiveVoiceBark& Bark);

    float GetCombatHeat() const { return CombatHeat; }

private:
    float LayerVolumes[static_cast<size_t>(MusicIntensityLayer::Count)];
    float CombatHeat = 0.0f; // Decays over time; boosted by damage/fire events

    std::deque<ActiveVoiceBark> BarkQueue;
    ActiveVoiceBark CurrentBark;
    bool bBarkPlaying = false;
    float LastBarkTimestampPerKind[8];
    float GlobalTime = 0.0f;
};

} // namespace RA4
