// Copyright (c) Red Alert 4 project. Presentation models for Top-Secret Protocol Wheel, Superweapon HUD Timers & Video Comms.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4Core/Ids.h"
#include "RA4Simulation/ProtocolRuntime.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4PRESENTATION_API
#define RA4PRESENTATION_API
#endif

namespace RA4
{

struct ProtocolNodeUIData
{
    std::string Id;
    std::string NameKey;
    std::string DescriptionKey;
    uint8_t Tier = 1;
    uint8_t Branch = 0;
    bool bUnlocked = false;
    bool bCanUnlock = false;
    bool bOnCooldown = false;
    float CooldownRemainingSeconds = 0.0f;
    float CooldownProgressFraction = 0.0f; // 0.0 = ready, 1.0 = just cast
};

struct SuperweaponHUDTimerItem
{
    EntityId BuildingEntity;
    PlayerId OwnerPlayer = 0;
    std::string SuperweaponName;
    int32_t ChargePercent = 0;
    float RemainingSeconds = 0.0f;
    bool bReady = false;
    bool bPowered = true;
    bool bCriticalAlert = false; // Ready to strike
};

struct VideoCommsHUDState
{
    bool bActive = false;
    std::string TransmissionId;
    std::string SpeakerPortraitVideo;
    std::string SubtitleKey;
    float ElapsedSeconds = 0.0f;
    float DurationSeconds = 0.0f;
    float ProgressFraction = 0.0f;
};

class RA4PRESENTATION_API ProtocolAndSuperweaponUI
{
public:
    ProtocolAndSuperweaponUI() = default;

    /** Extracts UI-ready view models for the protocol wheel/tree. */
    std::vector<ProtocolNodeUIData> BuildProtocolTreeViewModel(
        PlayerId Player,
        const ProtocolRuntime& Protocols,
        TickIndex CurrentTick,
        float TickRateHz = 20.0f) const;


    /** Builds active superweapon countdown timers for the HUD header banner. */
    std::vector<SuperweaponHUDTimerItem> BuildSuperweaponTimersViewModel(
        const ProtocolRuntime& Protocols,
        const SimWorld& World,
        float TickRateHz = 20.0f) const;

    /** Formats MM:SS display string for superweapon countdown. */
    static std::string FormatCountdownMMSS(float SecondsRemaining);
};

} // namespace RA4
