// Copyright (c) Red Alert 4 project. Tactical Operation Lifecycle & Squad Manager.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

enum class OperationState : uint8_t
{
    Proposed = 0,
    Gathering,
    Staging,
    Advancing,
    Engaging,
    Retreating,
    Completed,
    Aborted
};

const char* RA4AI_API ToString(OperationState State);

struct RA4AI_API TacticalOperation
{
    uint32_t OperationId = 0;
    OperationState State = OperationState::Proposed;
    TileCoord TargetLocation{0, 0};
    TileCoord StagingPoint{0, 0};
    int32_t RequiredCombatUnits = 3;
    int32_t MinRetreatUnits = 1;
    // Alive-strength at which an engaged squad breaks off: half the commit size,
    // so a push presses its attack instead of retreating on the first losses.
    int32_t RetreatFloorUnits = 1;
    std::vector<EntityId> AssignedUnits;
    TickIndex StartTick = 0;
    TickIndex LastStateChangeTick = 0;
    // Last tick the squad roster changed -- a recruit joined or a casualty was
    // pruned. A gather whose roster has been stable below minimum for a long while
    // is waiting on reinforcements that are not coming.
    TickIndex LastSquadGrowthTick = 0;

    void TransitionTo(OperationState NewState, TickIndex CurrentTick)
    {
        State = NewState;
        LastStateChangeTick = CurrentTick;
    }
};

} // namespace AI
} // namespace RA4
