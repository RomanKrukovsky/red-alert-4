// Copyright (c) Red Alert 4 project. Top-Secret Protocols and Commander Tree types.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4Content/ContentTypes.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"

namespace RA4
{

enum class ProtocolPowerKind : uint8_t
{
    Passive = 0,
    OrbitalStrike,        // Kinetic debris crash (heavy AOE damage)
    CryoFreeze,           // Freezes enemies in radius (slows/weakens)
    MagneticDraw,         // Displaces/damages vehicles in radius
    TimeBomb,             // Delayed explosive drop
    EmergencyRepairDrone, // AOE instant repair & shield overcharge
    ReconSurge,           // Instantly reveals fog over target area
};

struct ProtocolPowerDef
{
    std::string Id;
    std::string NameKey;
    std::string DescriptionKey;
    FactionId Faction = FactionId::None;
    uint8_t Branch = 0; // 0=Assault, 1=Special/Tactical, 2=Defense/Support
    uint8_t Tier = 1;   // 1 to 5
    std::string PrerequisiteId; // Must unlock prerequisite first if non-empty

    ProtocolPowerKind Kind = ProtocolPowerKind::Passive;
    uint32_t CooldownTicks = 1200; // 60 seconds @ 20Hz
    int32_t Damage = 0;
    Fixed Radius = Fixed::Zero();
    WarheadClass Warhead = WarheadClass::Fragmentation;


    // Passive modifiers (if Kind == Passive)
    Fixed ArmorMultiplier = Fixed::FromInt(1);
    Fixed SpeedMultiplier = Fixed::FromInt(1);
    int32_t ProductionCostDiscountPercent = 0;
};

struct PlayerProtocolState
{
    uint32_t TotalExperience = 0;
    uint32_t AvailablePoints = 0;
    uint32_t SpentPoints = 0;
    std::vector<std::string> UnlockedProtocols;
    std::vector<std::pair<std::string, TickIndex>> Cooldowns; // ProtocolId -> ReadyTick

    bool HasProtocol(const std::string& ProtocolId) const
    {
        for (const auto& P : UnlockedProtocols)
        {
            if (P == ProtocolId) return true;
        }
        return false;
    }

    bool IsOnCooldown(const std::string& ProtocolId, TickIndex CurrentTick) const
    {
        for (const auto& C : Cooldowns)
        {
            if (C.first == ProtocolId)
            {
                return CurrentTick < C.second;
            }
        }
        return false;
    }

    TickIndex GetCooldownReadyTick(const std::string& ProtocolId) const
    {
        for (const auto& C : Cooldowns)
        {
            if (C.first == ProtocolId) return C.second;
        }
        return 0;
    }
};

struct SuperweaponStatus
{
    EntityId BuildingEntity;
    PlayerId Owner = kInvalidPlayer;
    ContentId DefId;
    std::string Name;
    int32_t ChargeTicks = 0;
    int32_t TotalRechargeTicks = 0;
    int32_t ChargePercent = 0;
    bool bReady = false;
    bool bPowered = true;
};

} // namespace RA4
