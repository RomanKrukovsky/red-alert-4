// Copyright (c) Red Alert 4 project. Context-sensitive orders, engine-free.
//
// Turns "the player right-clicked there with this selected" into the command list
// the server will validate. One right button doing the correct thing in a dozen
// situations is most of what makes an RTS feel good, and it is pure decision logic,
// so it is tested here rather than discovered by playing.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/Command.h"
#include "RA4Input/SelectionModel.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4INPUT_API
#define RA4INPUT_API
#endif

namespace RA4
{
namespace Input
{

// What the cursor should look like before the click happens. The HUD reads this
// every frame on hover; the same function decides the order, so the cursor can
// never promise something the click will not do.
enum class CursorHint : uint8_t
{
    None = 0,
    Select,
    Move,
    NoEntry,        // ordered somewhere the selection cannot go
    Attack,
    ForceAttack,
    Harvest,
    Deliver,
    Repair,
    Capture,
    SetRallyPoint,
};

struct OrderContext
{
    PlayerId Issuer = 0;

    // The entity under the cursor, if any. Invalid means bare ground.
    EntityId HoveredEntity;
    Vec2 WorldLocation;
    TileCoord Tile;

    bool bQueueOrder = false;      // shift held
    bool bForceAttack = false;     // ctrl held: attack whatever is there, ally or ground
    bool bAttackMoveMode = false;  // the player armed attack-move and is now clicking

    // Set while the player is placing a finished structure from the sidebar.
    bool bPlacementMode = false;
    ContentId PlacementContent;
};

// Produces the commands for a right-click (or for a left-click while placement or
// attack-move mode is armed). Returns an empty list when the gesture means nothing,
// which the caller should treat as "play the negative sound", not as an error.
RA4INPUT_API std::vector<Command> ResolveOrder(const SimWorld& World, const SelectionModel& Selection,
                                               const OrderContext& Context);

// The hover state matching ResolveOrder, for cursor and tooltip rendering.
RA4INPUT_API CursorHint ResolveCursorHint(const SimWorld& World, const SelectionModel& Selection,
                                          const OrderContext& Context);

} // namespace Input
} // namespace RA4
