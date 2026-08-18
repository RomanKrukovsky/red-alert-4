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

// Stable content keys for the seven built-in shapes. Spelled as strings and hashed
// through MakeContentId like every other content handle, so a mod inserting its own
// formation cannot renumber these and invalidate a stored replay.
constexpr ContentId kFormationColumn = MakeContentId("formation.column");
constexpr ContentId kFormationLine = MakeContentId("formation.line");
constexpr ContentId kFormationWedge = MakeContentId("formation.wedge");
constexpr ContentId kFormationSpread = MakeContentId("formation.spread");
constexpr ContentId kFormationShieldScreen = MakeContentId("formation.shield_screen");
constexpr ContentId kFormationCircular = MakeContentId("formation.circular");
constexpr ContentId kFormationTransport = MakeContentId("formation.transport");

// Maps an EFormationShape onto the content key above. The enum is the authoring-side
// spelling; the ContentId is what the simulation stores and checksums.
RA4NAVIGATION_API ContentId FormationShapeToContentId(EFormationShape Shape);

// Looks up a built-in formation. Returns nullptr for an unknown id rather than a
// default shape: silently substituting Line would hide the content error and still
// pile every member onto one point, which is exactly the failure being designed out.
//
// The returned pointer is to immutable static storage with permanent lifetime, so
// callers may cache it across ticks. Offsets are in leader-facing space: +X is the
// leader's forward axis and +Y is 90 degrees clockwise of it (this sim's Y-down
// convention), matching Vec2::FromAngle. Feed each offset through RotateOffset with
// the leader's facing to get a world delta.
//
// Slot 0 is the LEADER's own slot and is always the zero offset -- the leader is a
// member of its own formation, so Members[0] is the leader and slot indices line up
// with FormationAssignment::Members without an off-by-one at the call site.
//
// Every offset within a shape is distinct (see the static_asserts in Formation.cpp).
// Two slots resolving to one point would order two units to the identical
// Destination, so they would shove each other forever and neither would arrive.
RA4NAVIGATION_API const FormationDef* FindFormationDef(ContentId Id);

// Number of slots a shape provides, i.e. the largest group it can seat including the
// leader. 0 for an unknown id, so a caller can distinguish "no such formation" from
// "formation with no slots" without a second lookup.
RA4NAVIGATION_API int32_t FormationSlotCount(ContentId Id);

} // namespace RA4