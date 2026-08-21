// Copyright (c) Red Alert 4 project. Electronic Warfare, Radar Jamming & Infiltration Sabotage.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/SimTypes.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4SIMULATION_API
#define RA4SIMULATION_API
#endif

namespace RA4
{

struct RadarJammerEmitter
{
    EntityId EmitterId;
    PlayerId Owner = kInvalidPlayer;
    Vec2 Position;
    Fixed Radius = Fixed::FromInt(500);
    bool bActive = true;
};

struct InfiltrationSabotageEffect
{
    PlayerId TargetPlayer = kInvalidPlayer;
    uint32_t OutageTicksRemaining = 0;
    bool bPowerGridSabotaged = false;
    bool bRadarScrambled = false;
};

class RA4SIMULATION_API ElectronicWarfareSystem
{
public:
    ElectronicWarfareSystem() = default;

    /** Registers or updates an active Mobile Radar Jammer / Gap Generator. */
    void RegisterJammer(EntityId EmitterId, PlayerId Owner, const Vec2& Position, Fixed Radius);

    /** Removes a destroyed or deactivated jammer. */
    void RemoveJammer(EntityId EmitterId);

    /** Infiltrates enemy infrastructure (Power Plant or Radar) causing temporary sabotage outage. */
    void ApplyInfiltrationSabotage(PlayerId TargetPlayer, bool bSabotagePower, bool bSabotageRadar, uint32_t DurationTicks);

    /** Checks if a world position is obscured from a viewer by enemy radar jamming. */
    bool IsLocationJammedFor(PlayerId ViewerPlayer, const Vec2& Location) const;

    /** Checks if target player is currently suffering from a sabotaged power grid outage. */
    bool IsPowerGridSabotaged(PlayerId Player) const;

    /** Advances sabotage countdowns. */
    void Tick(SimWorld& World);

    const std::vector<RadarJammerEmitter>& GetActiveJammers() const { return Jammers; }

private:
    std::vector<RadarJammerEmitter> Jammers;
    InfiltrationSabotageEffect SabotageStates[kMaxPlayers];
};

} // namespace RA4
