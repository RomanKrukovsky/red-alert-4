// Copyright (c) Red Alert 4 project. Selection geometry, engine-free.
//
// The engine's job in picking is projection: turning a cursor position into a point
// on the ground plane, and a screen rectangle into a quad on that plane. Everything
// after that -- which entities the point touches, which ones the quad contains, in
// what order -- is plain geometry and lives here so it can be tested.
//
// Working on the ground plane rather than in screen space means a rotated or tilted
// camera needs no special case: the marquee is simply a quad that is no longer
// axis-aligned.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"

#ifndef RA4INPUT_API
#define RA4INPUT_API
#endif

namespace RA4
{
namespace Input
{

// One selectable entity reduced to what picking actually needs. The caller builds
// these from the simulation, which is also where the fog of war filter belongs:
// an entity the player cannot see must never reach this list.
struct PickCandidate
{
    EntityId Id;
    Vec2 Position;
    Fixed Radius = Fixed::Zero();
};

// Entities whose disc contains the point, nearest centre first. Returning all of
// them rather than just the closest lets SelectionModel apply its ownership and
// kind priorities, so a friendly tank standing on an enemy one is still selectable.
RA4INPUT_API std::vector<EntityId> PickAtPoint(const std::vector<PickCandidate>& Candidates,
                                               const Vec2& Point,
                                               Fixed ExtraTolerance = Fixed::Zero());

// Entities whose centre lies inside the convex quad, in ascending slot order so the
// result does not depend on the order the caller happened to gather candidates in.
RA4INPUT_API std::vector<EntityId> PickInQuad(const std::vector<PickCandidate>& Candidates,
                                              const Vec2 Quad[4]);

// Accepts either winding, and treats a point exactly on an edge as inside.
RA4INPUT_API bool IsPointInConvexQuad(const Vec2 Quad[4], const Vec2& Point);

// True when a drag is too short to be a marquee and should be treated as a click.
// Measured on the ground plane, so the threshold means the same thing at every zoom
// level -- a pixel threshold would make precise clicks impossible when zoomed out.
RA4INPUT_API bool IsDragSignificant(const Vec2& Start, const Vec2& End, Fixed MinimumExtent);

} // namespace Input
} // namespace RA4
