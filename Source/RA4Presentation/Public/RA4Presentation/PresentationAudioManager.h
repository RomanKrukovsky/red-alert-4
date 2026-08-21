// Copyright (c) Red Alert 4 project. Spatial 3D Audio and Voice Line Coordinator.
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <map>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/SimTypes.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4PRESENTATION_API
#define RA4PRESENTATION_API
#endif

namespace RA4
{
namespace Presentation
{

enum class SoundCategory : uint8_t
{
    SFX_Weapon = 0,
    SFX_Impact,
    SFX_Explosion,
    SFX_Engine,
    SFX_UI,
    Voice_UnitAcknowledge,
    Voice_UnitUnderFire,
    Voice_AbilityActivated,
    Voice_CommanderAlert
};

struct SoundCue
{
    uint32_t Id = 0;
    SoundCategory Category = SoundCategory::SFX_Weapon;
    std::string SoundName;
    Vec2 Location;
    float BaseVolume = 1.0f;
    float FinalVolume = 1.0f;
    float StereoPan = 0.0f; // -1.0 = full left, +1.0 = full right, 0.0 = center
    float Pitch = 1.0f;
    float FalloffRadiusCm = 5000.0f;
    uint8_t Priority = 50; // 0..100 (higher overrides lower when max voice limit reached)
    bool bSpatialized = true;
    bool bConsumed = false;
};

struct ListenerState
{
    Vec2 CameraFocus;
    float FacingDegrees = 0.0f;
    float CameraHeightCm = 1500.0f;
};

struct PendingVoiceBark
{
    PlayerId Player = kInvalidPlayer;
    ContentId UnitDef;
    SoundCategory Category = SoundCategory::Voice_UnitAcknowledge;
    std::string VoiceKey;
    uint8_t Priority = 80;
};

class RA4PRESENTATION_API PresentationAudioManager
{
public:
    PresentationAudioManager() = default;

    /** Updates camera / listener spatial anchor in world space. */
    void UpdateListener(const Vec2& CameraFocus, float FacingDegrees = 0.0f, float CameraHeightCm = 1500.0f);

    /** Ingests SimEvents and triggers tactical combat sound cues and radio voice barks. */
    void ConsumeSimEvents(const std::vector<SimEvent>& Events, const SimWorld& World, PlayerId LocalPlayer);

    /** Advances audio decay timers and flushes consumed cues. */
    void Tick(float DeltaTimeSeconds);

    /** Calculates spatial volume attenuation and stereo panning for a given world position. */
    void CalculateSpatialParameters(const Vec2& SourcePos, float FalloffRadiusCm,
                                    float& OutVolume, float& OutPan) const;

    /** Plays a generic sound cue in world space or 2D UI space. */
    uint32_t PlaySound(SoundCategory Category, const std::string& SoundName, const Vec2& Location,
                       float BaseVolume = 1.0f, float FalloffRadiusCm = 5000.0f, uint8_t Priority = 50,
                       bool bSpatialize = true);

    /** Requests a unit or commander voice line. */
    void RequestVoiceBark(PlayerId Player, ContentId UnitDef, SoundCategory Category,
                          const std::string& VoiceKey, uint8_t Priority = 80);

    const std::vector<SoundCue>& GetActiveSoundCues() const { return ActiveCues; }
    bool GetNextVoiceBark(PendingVoiceBark& OutBark);

    void SetMaxConcurrentSounds(size_t Max) { MaxConcurrentSounds = Max; }
    void SetMasterVolume(float Vol) { MasterVolume = Vol; }
    void SetSFXVolume(float Vol) { SFXVolume = Vol; }
    void SetVoiceVolume(float Vol) { VoiceVolume = Vol; }

    void Clear();

private:
    uint32_t NextCueId = 1;
    ListenerState Listener;
    size_t MaxConcurrentSounds = 64;
    float MasterVolume = 1.0f;
    float SFXVolume = 1.0f;
    float VoiceVolume = 1.0f;

    std::vector<SoundCue> ActiveCues;
    std::vector<PendingVoiceBark> PendingVoices;

    // Cooldown map to prevent voice line spam [VoiceKey -> CooldownSecondsRemaining]
    std::map<std::string, float> VoiceCooldowns;
};

} // namespace Presentation
} // namespace RA4
