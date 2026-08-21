// Copyright (c) Red Alert 4 project. Top-Secret Protocols and Global Commander Powers runtime.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/ProtocolTypes.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4SIMULATION_API
#define RA4SIMULATION_API
#endif

namespace RA4
{

class RA4SIMULATION_API ProtocolRuntime
{
public:
    ProtocolRuntime();

    /** Registers a protocol definition into the global protocol database. */
    void RegisterProtocol(const ProtocolPowerDef& Def);

    /** Finds a registered protocol definition, or nullptr. */
    const ProtocolPowerDef* FindProtocol(const std::string& ProtocolId) const;

    /** Populates the standard RA3-style protocol tech tree. */
    void RegisterDefaultProtocols();

    /** Resets runtime state for all players. */
    void Reset();

    /** Ingests SimEvents to automatically award protocol experience for combat. */
    void ProcessSimEvents(const std::vector<SimEvent>& Events);

    /** Manually awards protocol XP to a player. */
    void AwardExperience(PlayerId Player, uint32_t Exp);

    /** Returns whether the player can unlock the given protocol. */
    bool CanUnlockProtocol(PlayerId Player, const std::string& ProtocolId) const;

    /** Unlocks a protocol for the player, deducting 1 point. Returns true on success. */
    bool UnlockProtocol(PlayerId Player, const std::string& ProtocolId);

    /** Checks if a global power can be cast by the player at the target location. */
    bool CanCastPower(PlayerId Player, const std::string& ProtocolId, const Vec2& Location, const SimWorld& World) const;

    /** Executes a global commander power on the simulation world. Returns true on success. */
    bool CastPower(PlayerId Player, const std::string& ProtocolId, const Vec2& Location, SimWorld& World);

    /** Queries the state of all superweapons currently built across all players. */
    std::vector<SuperweaponStatus> GetSuperweaponStatuses(const SimWorld& World) const;

    const PlayerProtocolState& GetPlayerState(PlayerId Player) const;
    PlayerProtocolState& GetPlayerStateMutable(PlayerId Player);

    const std::map<std::string, ProtocolPowerDef>& GetCatalog() const { return ProtocolCatalog; }

    /** Ticks cooldowns and passive updates. */
    void Tick(const SimWorld& World);


private:
    void ExecutePowerEffect(const ProtocolPowerDef& Def, PlayerId Player, const Vec2& Location, SimWorld& World);

    std::map<std::string, ProtocolPowerDef> ProtocolCatalog;
    PlayerProtocolState PlayerStates[kMaxPlayers];
};

} // namespace RA4
