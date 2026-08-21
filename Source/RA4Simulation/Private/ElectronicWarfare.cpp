// Copyright (c) Red Alert 4 project. Electronic Warfare, Radar Jamming & Infiltration Sabotage.
#include "RA4Simulation/ElectronicWarfare.h"

#include <algorithm>

namespace RA4
{

void ElectronicWarfareSystem::RegisterJammer(EntityId EmitterId, PlayerId Owner, const Vec2& Position, Fixed Radius)
{
    for (auto& Jammer : Jammers)
    {
        if (Jammer.EmitterId == EmitterId)
        {
            Jammer.Owner = Owner;
            Jammer.Position = Position;
            Jammer.Radius = Radius;
            Jammer.bActive = true;
            return;
        }
    }

    RadarJammerEmitter NewJammer;
    NewJammer.EmitterId = EmitterId;
    NewJammer.Owner = Owner;
    NewJammer.Position = Position;
    NewJammer.Radius = Radius;
    NewJammer.bActive = true;
    Jammers.push_back(NewJammer);
}

void ElectronicWarfareSystem::RemoveJammer(EntityId EmitterId)
{
    Jammers.erase(
        std::remove_if(Jammers.begin(), Jammers.end(),
                       [&](const RadarJammerEmitter& J) { return J.EmitterId == EmitterId; }),
        Jammers.end());
}

void ElectronicWarfareSystem::ApplyInfiltrationSabotage(PlayerId TargetPlayer, bool bSabotagePower, bool bSabotageRadar, uint32_t DurationTicks)
{
    if (TargetPlayer >= kMaxPlayers) return;

    auto& State = SabotageStates[TargetPlayer];
    State.TargetPlayer = TargetPlayer;
    State.OutageTicksRemaining = DurationTicks;
    State.bPowerGridSabotaged = bSabotagePower;
    State.bRadarScrambled = bSabotageRadar;
}

bool ElectronicWarfareSystem::IsLocationJammedFor(PlayerId ViewerPlayer, const Vec2& Location) const
{
    for (const auto& Jammer : Jammers)
    {
        if (Jammer.bActive && Jammer.Owner != ViewerPlayer)
        {
            const Fixed DistSq = DistanceSquared(Jammer.Position, Location);
            if (DistSq <= Jammer.Radius * Jammer.Radius)
            {
                return true;
            }
        }
    }
    return false;
}

bool ElectronicWarfareSystem::IsPowerGridSabotaged(PlayerId Player) const
{
    if (Player >= kMaxPlayers) return false;
    return SabotageStates[Player].bPowerGridSabotaged && (SabotageStates[Player].OutageTicksRemaining > 0);
}

void ElectronicWarfareSystem::Tick(SimWorld& World)
{
    for (uint32_t P = 0; P < kMaxPlayers; ++P)
    {
        auto& State = SabotageStates[P];
        if (State.OutageTicksRemaining > 0)
        {
            --State.OutageTicksRemaining;
            if (State.OutageTicksRemaining == 0)
            {
                State.bPowerGridSabotaged = false;
                State.bRadarScrambled = false;
            }
        }
    }

    for (auto& Jammer : Jammers)
    {
        if (Jammer.EmitterId.IsValid() && !World.IsAlive(Jammer.EmitterId))
        {
            Jammer.bActive = false;
        }
    }

    Jammers.erase(
        std::remove_if(Jammers.begin(), Jammers.end(),
                       [](const RadarJammerEmitter& J) { return !J.bActive; }),
        Jammers.end());
}

} // namespace RA4
