// Copyright (c) Red Alert 4 project. Mouse control schemes, engine-free.
//
// Which button does what is the single loudest thing about how an RTS feels, and the
// two conventions are incompatible: the classic Red Alert games issue orders with the
// LEFT button and clear the selection with the RIGHT one, while everything after
// StarCraft selects with left and orders with right. Routing that decision here, away
// from the PlayerController, keeps it under test -- "left-clicking my own tank while
// tanks are selected must select, not order it to attack itself" is a rule, not a
// thing to rediscover by clicking around in PIE.
//
// This layer only decides *what kind of gesture* the click was. Turning "issue an
// order" into actual commands stays in OrderResolver.
#pragma once

#include <cstdint>

#include "RA4Core/Ids.h"
#include "RA4Input/SelectionModel.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4INPUT_API
#define RA4INPUT_API
#endif

namespace RA4
{
namespace Input
{

enum class ControlScheme : uint8_t
{
    // Red Alert 1 / 2: left orders, right deselects. The project default.
    ClassicRA = 0,
    // StarCraft / Red Alert 3: left selects, right orders.
    Modern = 1,
};

enum class ClickIntent : uint8_t
{
    None = 0,
    SelectAtPoint,
    SelectInMarquee,
    IssueOrder,
    ClearSelection,
    CancelArmedMode,
};

// Everything the routing decision needs, flattened so the rules can be tested without
// a SimWorld. Fill it with MakeClickFacts.
struct ClickFacts
{
    bool bDragWasMarquee = false;

    bool bShift = false;
    bool bCtrl = false;   // force attack
    bool bAlt = false;    // force move -- ignore whatever is under the cursor

    bool bAttackMoveArmed = false;
    bool bPlacementArmed = false;

    bool bHoveredValid = false;
    bool bHoveredOwnedByIssuer = false;

    // True when the selection contains something of the local player's that can be
    // given an order. With an empty selection, or someone else's units selected,
    // every click is a selection attempt regardless of scheme.
    bool bSelectionCanAcceptOrders = false;
};

RA4INPUT_API ClickFacts MakeClickFacts(const SimWorld& World, const SelectionModel& Selection,
                                       EntityId HoveredEntity, bool bDragWasMarquee, bool bShift, bool bCtrl,
                                       bool bAlt, bool bAttackMoveArmed, bool bPlacementArmed);

RA4INPUT_API ClickIntent RouteLeftClick(ControlScheme Scheme, const ClickFacts& Facts);
RA4INPUT_API ClickIntent RouteRightClick(ControlScheme Scheme, const ClickFacts& Facts, bool bSelectionEmpty);

// Under ClassicRA the same modifier keys mean different things than under Modern:
// ctrl-click is force fire, not toggle-select, so the selection modifier has to be
// derived per scheme rather than hard-coded at the call site.
RA4INPUT_API SelectionMode ResolveSelectionMode(ControlScheme Scheme, bool bShift, bool bCtrl);

} // namespace Input
} // namespace RA4
