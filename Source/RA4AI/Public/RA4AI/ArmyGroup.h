// Copyright (c) Red Alert 4 project. Operational Army Group data structures.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Navigation/Formation.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

enum class GroupRole : uint8_t
{
    Reserve = 0,
    Scout,
    QuickResponse,
    MainAssault,
    ArtillerySupport,
    AntiAir,
    BaseGuard,
    EconRaid,
    Naval
};

enum class GroupStance : uint8_t
{
    Balanced = 0,
    Aggressive,
    Defensive,
    HoldFire,
    HoldPosition,
    ReturnFire
};

enum class GroupTaskType : uint8_t
{
    Idle = 0,
    GatherAtRally,
    Staging,
    MoveInFormation,
    AttackMove,
    DefendZone,
    EscortTarget,
    SiegePosition,
    Retreating,
    Rebuilding
};

enum class GroupFormationShape : uint8_t
{
    Line = 0,
    Column,
    Wedge,
    Spread,
    Screen,
    Circular,
    Transport
};


struct ArmyGroup
{
    uint32_t GroupId = 0;                     // Stable ID for replay & UI tracking
    std::string Name;                         // Human-readable group name
    GroupRole Role = GroupRole::MainAssault;
    GroupStance Stance = GroupStance::Balanced;
    GroupTaskType Task = GroupTaskType::Idle;

    EntityId Leader;                          // Formation anchor entity
    std::vector<EntityId> Members;            // Active entity handles
    std::map<ContentId, int32_t> TargetComposition; // Target unit count composition

    Vec2 RallyPoint = Vec2::Zero();           // Assembly/staging position
    Vec2 TargetLocation = Vec2::Zero();       // Current operational target
    EntityId TargetEntity;                    // Primary target entity

    GroupFormationShape FormationShape = GroupFormationShape::Line;
    int32_t FormationSpacing = 80;            // Distance between slots in world units


    int32_t CombatReadiness = 100;            // 0..100 percent
    int32_t MoralePercent = 100;              // 0..100 percent
    int32_t RetreatThresholdPercent = 30;     // Retreat if HP or force ratio < 30%

    TickIndex AssignedTick = 0;
    TickIndex LastOrderTick = 0;

    bool bAwaitingReinforcements = false;
};

class RA4AI_API ArmyGroupManager
{
public:
    ArmyGroup* CreateGroup(uint32_t GroupId, GroupRole Role, const std::string& Name);
    ArmyGroup* FindGroup(uint32_t GroupId);
    const ArmyGroup* FindGroup(uint32_t GroupId) const;
    void RemoveGroup(uint32_t GroupId);
    void Clear();

    const std::vector<ArmyGroup>& GetGroups() const { return Groups; }
    std::vector<ArmyGroup>& GetGroups() { return Groups; }

    uint32_t AllocateGroupId() { return NextGroupId++; }

private:
    uint32_t NextGroupId = 1;
    std::vector<ArmyGroup> Groups;
};

} // namespace AI
} // namespace RA4
