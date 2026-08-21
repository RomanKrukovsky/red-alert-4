// Copyright (c) Red Alert 4 project. Spatial 3D Audio and Voice Line Coordinator implementation.
#include "RA4Presentation/PresentationAudioManager.h"

#include <algorithm>
#include <cmath>

namespace RA4
{
namespace Presentation
{

void PresentationAudioManager::UpdateListener(const Vec2& CameraFocus, float FacingDegrees, float CameraHeightCm)
{
    Listener.CameraFocus = CameraFocus;
    Listener.FacingDegrees = FacingDegrees;
    Listener.CameraHeightCm = CameraHeightCm > 0.0f ? CameraHeightCm : 1500.0f;
}

void PresentationAudioManager::CalculateSpatialParameters(const Vec2& SourcePos, float FalloffRadiusCm,
                                                          float& OutVolume, float& OutPan) const
{
    const float Dx = static_cast<float>((SourcePos.X - Listener.CameraFocus.X).ToDoubleUnsafe());
    const float Dy = static_cast<float>((SourcePos.Y - Listener.CameraFocus.Y).ToDoubleUnsafe());
    const float Dz = Listener.CameraHeightCm;

    (void)Dz;
    const float EffectiveFalloff = FalloffRadiusCm > 100.0f ? FalloffRadiusCm : 5000.0f;

    // Linear distance attenuation with 3D elevation baseline
    const float Dist2D = std::sqrt(Dx * Dx + Dy * Dy);
    const float Attenuation = std::clamp(1.0f - (Dist2D / EffectiveFalloff), 0.0f, 1.0f);
    OutVolume = Attenuation * MasterVolume * SFXVolume;

    // Stereo panning based on listener orientation
    const float Rad = -Listener.FacingDegrees * 3.1415926535f / 180.0f;
    const float LocalX = Dx * std::cos(Rad) - Dy * std::sin(Rad);

    // Pan spreads across +/- 2000cm viewport width
    OutPan = std::clamp(LocalX / 2000.0f, -1.0f, 1.0f);
}

uint32_t PresentationAudioManager::PlaySound(SoundCategory Category, const std::string& SoundName,
                                             const Vec2& Location, float BaseVolume,
                                             float FalloffRadiusCm, uint8_t Priority,
                                             bool bSpatialize)
{
    if (ActiveCues.size() >= MaxConcurrentSounds)
    {
        // Find lowest priority cue to evict
        auto LowestIt = std::min_element(ActiveCues.begin(), ActiveCues.end(),
            [](const SoundCue& A, const SoundCue& B) { return A.Priority < B.Priority; });

        if (LowestIt != ActiveCues.end() && LowestIt->Priority < Priority)
        {
            *LowestIt = SoundCue{}; // replace lowest priority cue
        }
        else
        {
            return 0; // dropped due to voice concurrency limit
        }
    }

    SoundCue Cue;
    Cue.Id = NextCueId++;
    Cue.Category = Category;
    Cue.SoundName = SoundName;
    Cue.Location = Location;
    Cue.BaseVolume = BaseVolume;
    Cue.Pitch = 1.0f;
    Cue.FalloffRadiusCm = FalloffRadiusCm;
    Cue.Priority = Priority;
    Cue.bSpatialized = bSpatialize;

    if (bSpatialize)
    {
        CalculateSpatialParameters(Location, FalloffRadiusCm, Cue.FinalVolume, Cue.StereoPan);
        Cue.FinalVolume *= BaseVolume;
    }
    else
    {
        Cue.FinalVolume = BaseVolume * MasterVolume * SFXVolume;
        Cue.StereoPan = 0.0f;
    }

    ActiveCues.push_back(Cue);
    return Cue.Id;
}

void PresentationAudioManager::RequestVoiceBark(PlayerId Player, ContentId UnitDef,
                                                SoundCategory Category, const std::string& VoiceKey,
                                                uint8_t Priority)
{
    // Check if voice key is on cooldown to prevent repetitive spam
    auto It = VoiceCooldowns.find(VoiceKey);
    if (It != VoiceCooldowns.end() && It->second > 0.0f)
    {
        return;
    }

    // Set cooldown (e.g. 2.5s between identical unit voice barks)
    VoiceCooldowns[VoiceKey] = 2.5f;

    PendingVoiceBark Bark;
    Bark.Player = Player;
    Bark.UnitDef = UnitDef;
    Bark.Category = Category;
    Bark.VoiceKey = VoiceKey;
    Bark.Priority = Priority;

    PendingVoices.push_back(Bark);

    // Keep voice queue sorted by priority descending
    std::stable_sort(PendingVoices.begin(), PendingVoices.end(),
        [](const PendingVoiceBark& A, const PendingVoiceBark& B) { return A.Priority > B.Priority; });
}

bool PresentationAudioManager::GetNextVoiceBark(PendingVoiceBark& OutBark)
{
    if (PendingVoices.empty())
    {
        return false;
    }

    OutBark = PendingVoices.front();
    PendingVoices.erase(PendingVoices.begin());
    return true;
}

void PresentationAudioManager::ConsumeSimEvents(const std::vector<SimEvent>& Events,
                                                const SimWorld& World, PlayerId LocalPlayer)
{
    for (const auto& Ev : Events)
    {
        if (Ev.Type == SimEventType::WeaponFired)
        {
            PlaySound(SoundCategory::SFX_Weapon, "sfx.weapon.fire", Ev.Location, 1.0f, 6000.0f, 60);
        }
        else if (Ev.Type == SimEventType::ProjectileImpact || Ev.Type == SimEventType::DamageApplied)
        {
            PlaySound(SoundCategory::SFX_Impact, "sfx.impact.explosion", Ev.Location, 0.9f, 5000.0f, 50);
        }
        else if (Ev.Type == SimEventType::EntityDestroyed)
        {
            PlaySound(SoundCategory::SFX_Explosion, "sfx.unit.destroyed", Ev.Location, 1.0f, 8000.0f, 80);
        }
        else if (Ev.Type == SimEventType::SecondaryAbilityToggled)
        {
            if (World.IsAlive(Ev.Entity))
            {
                const auto* Core = World.GetCore(Ev.Entity);
                if (Core != nullptr && Core->Owner == LocalPlayer)
                {
                    RequestVoiceBark(LocalPlayer, Core->Def, SoundCategory::Voice_AbilityActivated,
                                     "voice.ability.activate", 90);
                }
            }
        }
        else if (Ev.Type == SimEventType::CoopPingEmitted)
        {
            PlaySound(SoundCategory::SFX_UI, "sfx.ui.coop_ping", Ev.Location, 1.0f, 10000.0f, 95, false);
        }
    }
}

void PresentationAudioManager::Tick(float DeltaTimeSeconds)
{
    if (DeltaTimeSeconds <= 0.0f)
    {
        return;
    }

    // Decay voice cooldowns
    for (auto It = VoiceCooldowns.begin(); It != VoiceCooldowns.end();)
    {
        It->second -= DeltaTimeSeconds;
        if (It->second <= 0.0f)
        {
            It = VoiceCooldowns.erase(It);
        }
        else
        {
            ++It;
        }
    }

    // Flush consumed or played sound cues
    ActiveCues.clear();
}

void PresentationAudioManager::Clear()
{
    ActiveCues.clear();
    PendingVoices.clear();
    VoiceCooldowns.clear();
}

} // namespace Presentation
} // namespace RA4
