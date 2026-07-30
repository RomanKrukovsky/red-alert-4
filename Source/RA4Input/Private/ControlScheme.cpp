// Copyright (c) Red Alert 4 project.
#include "RA4Input/ControlScheme.h"

namespace RA4
{
namespace Input
{

ClickFacts MakeClickFacts(const SimWorld& World, const SelectionModel& Selection, EntityId HoveredEntity,
                          bool bDragWasMarquee, bool bShift, bool bCtrl, bool bAlt, bool bAttackMoveArmed,
                          bool bPlacementArmed)
{
    ClickFacts Facts;
    Facts.bDragWasMarquee = bDragWasMarquee;
    Facts.bShift = bShift;
    Facts.bCtrl = bCtrl;
    Facts.bAlt = bAlt;
    Facts.bAttackMoveArmed = bAttackMoveArmed;
    Facts.bPlacementArmed = bPlacementArmed;

    Facts.bHoveredValid = HoveredEntity.IsValid() && World.IsAlive(HoveredEntity);
    if (Facts.bHoveredValid)
    {
        const EntityCore* Core = World.GetCore(HoveredEntity);
        Facts.bHoveredOwnedByIssuer = Core != nullptr && Core->Owner == Selection.GetLocalPlayer();
    }

    Facts.bSelectionCanAcceptOrders = Selection.HasOwnedEntities(World);
    return Facts;
}

ClickIntent RouteLeftClick(ControlScheme Scheme, const ClickFacts& Facts)
{
    // An armed mode owns the next left click in both schemes. Placement especially:
    // the player is holding a finished building and the click is where it goes.
    if (Facts.bPlacementArmed || Facts.bAttackMoveArmed)
    {
        return ClickIntent::IssueOrder;
    }

    // A drag is always a marquee. Without this, dragging across the map with units
    // selected would order them at the release point in the classic scheme.
    if (Facts.bDragWasMarquee)
    {
        return ClickIntent::SelectInMarquee;
    }

    if (Scheme == ControlScheme::Modern)
    {
        return ClickIntent::SelectAtPoint;
    }

    // --- ClassicRA -----------------------------------------------------------
    // Nothing of ours is selected, so there is no order to give: select.
    if (!Facts.bSelectionCanAcceptOrders)
    {
        return ClickIntent::SelectAtPoint;
    }

    // Ctrl is force fire and alt is force move. Both are deliberate overrides of
    // whatever the cursor is over, including our own units and bare ground.
    if (Facts.bCtrl || Facts.bAlt)
    {
        return ClickIntent::IssueOrder;
    }

    // Clicking our own stuff picks it instead of ordering the current selection into
    // it -- shift-clicking to grow the selection is the same gesture, one modifier on.
    if (Facts.bHoveredValid && Facts.bHoveredOwnedByIssuer)
    {
        return ClickIntent::SelectAtPoint;
    }

    // Ground, an enemy, a resource field, a neutral structure: all orders. Shift here
    // is a queued waypoint, which OrderResolver handles.
    return ClickIntent::IssueOrder;
}

ClickIntent RouteRightClick(ControlScheme Scheme, const ClickFacts& Facts, bool bSelectionEmpty)
{
    // Cancelling an armed mode outranks everything. Committing a building placement
    // on the button the player uses to back out is an expensive misfire.
    if (Facts.bPlacementArmed || Facts.bAttackMoveArmed)
    {
        return ClickIntent::CancelArmedMode;
    }

    if (Scheme == ControlScheme::Modern)
    {
        return bSelectionEmpty ? ClickIntent::None : ClickIntent::IssueOrder;
    }

    return bSelectionEmpty ? ClickIntent::None : ClickIntent::ClearSelection;
}

SelectionMode ResolveSelectionMode(ControlScheme Scheme, bool bShift, bool bCtrl)
{
    if (Scheme == ControlScheme::ClassicRA)
    {
        // Ctrl never reaches selection in the classic scheme: it is force fire, and
        // in the routing above a ctrl-click became an order before we got here.
        return bShift ? SelectionMode::Add : SelectionMode::Replace;
    }

    if (bCtrl)
    {
        return SelectionMode::Toggle;
    }
    return bShift ? SelectionMode::Add : SelectionMode::Replace;
}

} // namespace Input
} // namespace RA4
