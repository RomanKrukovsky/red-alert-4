// Copyright (c) Red Alert 4 project. Formation descriptors and slot assignment.
//
// Formations are authored as data (a ContentId + a list of Vec2 offsets relative to
// the leader's heading). The leader owns the macro path; members never build their
// own -- their Destination is set every tick to LeaderPos + Rotate(Offset[slot],
// LeaderFacing). This keeps a formation of N units at one flow field per sector,
// not N.
#pragma once

#include <vector>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"

#ifndef RA4NAVIGATION_API
#define RA4NAVIGATION_API
#endif

namespace RA4
{

// The named shapes an Army Group can hold. Column for travel, line to bring fire to
// bear, wedge to punch through a weak point, Spread to reduce area-damage exposure,
// ShieldScreen to keep armour between the enemy and artillery/support, Circular for
// all-round defence when surrounded, Transport for loading onto/escorting vehicles.
enum class EFormationShape : uint8_t
{
    Column = 0,
    Line,
    Wedge,
    Spread,
    ShieldScreen,
    Circular,
    Transport,
};

struct FormationDef
{
    ContentId Id;
    std::vector<Vec2> Offsets;   // slot i -> offset from leader, in leader-facing space
};

struct FormationAssignment
{
    ContentId FormationId;
    EntityId Leader;
    std::vector<EntityId> Members;   // slot i -> Members[i]
};

// Rotates an offset by a 4096-step angle (kAngleTurn). Used by the movement system
// and by the formation tests.
RA4NAVIGATION_API Vec2 RotateOffset(const Vec2& Offset, int32_t Facing);

} // namespace RA4