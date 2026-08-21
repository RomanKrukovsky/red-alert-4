// Copyright (c) Red Alert 4 project. Dynamic interactive music director and unit voice bark matrix implementation.
#include "RA4Presentation/PresentationDynamicAudio.h"

#include <algorithm>
#include <cmath>

namespace RA4
{

PresentationDynamicAudio::PresentationDynamicAudio()
{
    LayerVolumes[0] = 1.0f;
    LayerVolumes[1] = 0.0f;
    LayerVolumes[2] = 0.0f;

    for (int I = 0; I < 8; ++I)
    {
        LastBarkTimestampPerKind[I] = -100.0f;
    }
}

void PresentationDynamicAudio::ConsumeSimEvents(const std::vector<SimEvent>& Events, PlayerId LocalPlayer)
{
    for (const auto& Ev : Events)
    {
        switch (Ev.Type)
        {
        case SimEventType::DamageApplied:
        case SimEventType::WeaponFired:
        {
            CombatHeat = std::min(100.0f, CombatHeat + 15.0f);
            if (Ev.Player == LocalPlayer)
            {
                ActiveVoiceBark Bark;
                Bark.SoundCueId = "vo.unit.under_fire";
                Bark.SubtitleTextKey = "subtitles.unit.under_fire";
                Bark.Kind = VoiceBarkKind::UnderFire;
                Bark.Priority = 8;
                Bark.DurationSeconds = 1.5f;
                Bark.WorldLocation = Ev.Location;
                Bark.bIs3D = true;
                QueueVoiceBark(Bark);
            }
            break;
        }

        case SimEventType::EntityDestroyed:
        {
            CombatHeat = std::min(100.0f, CombatHeat + 25.0f);
            if (Ev.Player == LocalPlayer)
            {
                ActiveVoiceBark Bark;
                Bark.SoundCueId = "vo.unit.death_cry";
                Bark.SubtitleTextKey = "subtitles.unit.death_cry";
                Bark.Kind = VoiceBarkKind::DeathCry;
                Bark.Priority = 6;
                Bark.DurationSeconds = 1.2f;
                Bark.WorldLocation = Ev.Location;
                Bark.bIs3D = true;
                QueueVoiceBark(Bark);
            }
            break;
        }

        default:
            break;
        }
    }
}

void PresentationDynamicAudio::OnCommandIssued(PlayerId Player, const Command& Cmd, const std::string& UnitVoiceProfile)
{
    (void)Player;
    (void)UnitVoiceProfile;

    ActiveVoiceBark Bark;
    if (Cmd.Type == CommandType::Move)
    {
        Bark.SoundCueId = "vo.unit.move_ack";
        Bark.SubtitleTextKey = "subtitles.unit.move_ack";
        Bark.Kind = VoiceBarkKind::MoveOrdered;
        Bark.Priority = 2;
        Bark.DurationSeconds = 1.0f;
        QueueVoiceBark(Bark);
    }
    else if (Cmd.Type == CommandType::Attack)
    {
        Bark.SoundCueId = "vo.unit.attack_ack";
        Bark.SubtitleTextKey = "subtitles.unit.attack_ack";
        Bark.Kind = VoiceBarkKind::AttackOrdered;
        Bark.Priority = 4;
        Bark.DurationSeconds = 1.2f;
        QueueVoiceBark(Bark);
    }
}

bool PresentationDynamicAudio::QueueVoiceBark(const ActiveVoiceBark& Bark)
{
    const size_t KindIdx = static_cast<size_t>(Bark.Kind);
    if (KindIdx < 8)
    {
        if (GlobalTime - LastBarkTimestampPerKind[KindIdx] < 1.0f)
        {
            return false; // Rate-limited
        }
        LastBarkTimestampPerKind[KindIdx] = GlobalTime;
    }

    if (bBarkPlaying)
    {
        if (Bark.Priority > CurrentBark.Priority)
        {
            // Preempt current lower-priority bark
            CurrentBark = Bark;
            CurrentBark.ElapsedSeconds = 0.0f;
            bBarkPlaying = true;
            return true;
        }

        if (BarkQueue.size() < 4)
        {
            BarkQueue.push_back(Bark);
            return true;
        }
        return false;
    }

    CurrentBark = Bark;
    CurrentBark.ElapsedSeconds = 0.0f;
    bBarkPlaying = true;
    return true;
}

void PresentationDynamicAudio::Update(float DeltaTimeSeconds)
{
    GlobalTime += DeltaTimeSeconds;
    CombatHeat = std::max(0.0f, CombatHeat - DeltaTimeSeconds * 5.0f);

    float TargetVolumes[3] = {0.0f, 0.0f, 0.0f};
    if (CombatHeat < 20.0f)
    {
        TargetVolumes[0] = 1.0f;
    }
    else if (CombatHeat < 60.0f)
    {
        const float Alpha = (CombatHeat - 20.0f) / 40.0f;
        TargetVolumes[0] = 1.0f - Alpha;
        TargetVolumes[1] = 1.0f;
        TargetVolumes[2] = Alpha * 0.5f;
    }
    else
    {
        const float Alpha = std::min(1.0f, (CombatHeat - 60.0f) / 40.0f);
        TargetVolumes[0] = 0.0f;
        TargetVolumes[1] = 1.0f - Alpha;
        TargetVolumes[2] = 1.0f;
    }

    const float BlendRate = DeltaTimeSeconds * 3.0f;
    for (int I = 0; I < 3; ++I)
    {
        LayerVolumes[I] += (TargetVolumes[I] - LayerVolumes[I]) * std::min(1.0f, BlendRate);
    }

    if (bBarkPlaying)
    {
        CurrentBark.ElapsedSeconds += DeltaTimeSeconds;
        if (CurrentBark.ElapsedSeconds >= CurrentBark.DurationSeconds)
        {
            if (!BarkQueue.empty())
            {
                CurrentBark = BarkQueue.front();
                BarkQueue.pop_front();
                CurrentBark.ElapsedSeconds = 0.0f;
            }
            else
            {
                bBarkPlaying = false;
            }
        }
    }
}

float PresentationDynamicAudio::GetLayerVolume(MusicIntensityLayer Layer) const
{
    const size_t Idx = static_cast<size_t>(Layer);
    if (Idx < static_cast<size_t>(MusicIntensityLayer::Count))
    {
        return LayerVolumes[Idx];
    }
    return 0.0f;
}

const ActiveVoiceBark* PresentationDynamicAudio::GetCurrentVoiceBark() const
{
    if (bBarkPlaying)
    {
        return &CurrentBark;
    }
    return nullptr;
}

} // namespace RA4
