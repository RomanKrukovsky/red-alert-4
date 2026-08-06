// Copyright (c) Red Alert 4 project.
#include "RA4Input/SelectionModel.h"

#include <algorithm>

namespace RA4
{
namespace Input
{

namespace
{
// Priority bands. The gaps leave room to insert kinds later without renumbering.
constexpr int32_t kPriorityOwnUnit = 0;
constexpr int32_t kPriorityOwnBuilding = 100;
constexpr int32_t kPriorityOtherUnit = 200;
constexpr int32_t kPriorityOtherBuilding = 300;
constexpr int32_t kPriorityResource = 400;
constexpr int32_t kPriorityUnselectable = 1000;
} // namespace

int32_t GetSelectionPriority(const SimWorld& World, EntityId Id, PlayerId LocalPlayer)
{
    const EntityCore* Core = World.GetCore(Id);
    if (Core == nullptr)
    {
        return kPriorityUnselectable;
    }

    // Projectiles are simulation entities but are never selectable.
    if (Core->Kind == EntityKind::Projectile)
    {
        return kPriorityUnselectable;
    }
    if (Core->Kind == EntityKind::ResourceNode)
    {
        return kPriorityResource;
    }

    const bool bOwned = Core->Owner == LocalPlayer;
    if (Core->Kind == EntityKind::Unit)
    {
        return bOwned ? kPriorityOwnUnit : kPriorityOtherUnit;
    }
    return bOwned ? kPriorityOwnBuilding : kPriorityOtherBuilding;
}

void SelectionModel::Clear()
{
    Selected.clear();
}

bool SelectionModel::IsSelected(EntityId Id) const
{
    return std::find(Selected.begin(), Selected.end(), Id) != Selected.end();
}

void SelectionModel::SortAndCap(const SimWorld& World)
{
    // Sort by priority, then by slot index. The slot tiebreak keeps the primary
    // entity stable frame to frame, so the command card does not flicker between
    // two equally ranked units.
    const PlayerId Player = LocalPlayer;
    std::stable_sort(Selected.begin(), Selected.end(),
                     [&World, Player](const EntityId& A, const EntityId& B)
                     {
                         const int32_t PA = GetSelectionPriority(World, A, Player);
                         const int32_t PB = GetSelectionPriority(World, B, Player);
                         if (PA != PB)
                         {
                             return PA < PB;
                         }
                         return A.Index < B.Index;
                     });

    if (int32_t(Selected.size()) > kMaxSelectedEntities)
    {
        Selected.resize(size_t(kMaxSelectedEntities));
    }
}

void SelectionModel::ApplyMode(const std::vector<EntityId>& Incoming, SelectionMode Mode)
{
    switch (Mode)
    {
        case SelectionMode::Replace:
            Selected = Incoming;
            break;

        case SelectionMode::Add:
            for (const EntityId& Id : Incoming)
            {
                if (!IsSelected(Id))
                {
                    Selected.push_back(Id);
                }
            }
            break;

        case SelectionMode::Toggle:
            for (const EntityId& Id : Incoming)
            {
                const auto It = std::find(Selected.begin(), Selected.end(), Id);
                if (It != Selected.end())
                {
                    Selected.erase(It);
                }
                else
                {
                    Selected.push_back(Id);
                }
            }
            break;
    }
}

void SelectionModel::SelectAtCursor(const SimWorld& World, const std::vector<EntityId>& Candidates,
                                    SelectionMode Mode)
{
    EntityId Best = EntityId::Invalid();
    int32_t BestPriority = kPriorityUnselectable;

    for (const EntityId& Id : Candidates)
    {
        if (!World.IsAlive(Id))
        {
            continue;
        }
        const int32_t Priority = GetSelectionPriority(World, Id, LocalPlayer);
        if (Priority >= kPriorityUnselectable)
        {
            continue;
        }
        // Strict less-than plus a slot tiebreak: overlapping candidates must resolve
        // the same way every time, not by hit-test iteration order.
        if (Priority < BestPriority || (Priority == BestPriority && Best.IsValid() && Id.Index < Best.Index))
        {
            BestPriority = Priority;
            Best = Id;
        }
    }

    if (!Best.IsValid())
    {
        // Clicking empty ground clears the selection, but shift-clicking it must
        // not throw away what the player has spent ten seconds gathering.
        if (Mode == SelectionMode::Replace)
        {
            Clear();
        }
        return;
    }

    ApplyMode({Best}, Mode);
    SortAndCap(World);
}

void SelectionModel::SelectInMarquee(const SimWorld& World, const std::vector<EntityId>& Candidates,
                                     SelectionMode Mode)
{
    // A box drag is a "give me my army" gesture. If it caught any of the player's
    // own units, those are the only thing that ends up selected -- enemy scouts and
    // the player's own barracks inside the same box are ignored. Only when the box
    // contains nothing of the player's does it fall back to showing what is there.
    std::vector<EntityId> OwnUnits;
    std::vector<EntityId> OwnBuildings;
    std::vector<EntityId> Others;

    for (const EntityId& Id : Candidates)
    {
        if (!World.IsAlive(Id))
        {
            continue;
        }
        const EntityCore* Core = World.GetCore(Id);
        if (Core == nullptr || Core->Kind == EntityKind::Projectile || Core->Kind == EntityKind::ResourceNode)
        {
            continue;
        }
        if (Core->Owner == LocalPlayer)
        {
            (Core->Kind == EntityKind::Unit ? OwnUnits : OwnBuildings).push_back(Id);
        }
        else
        {
            Others.push_back(Id);
        }
    }

    std::vector<EntityId> Incoming;
    if (!OwnUnits.empty())
    {
        Incoming = OwnUnits;
    }
    else if (!OwnBuildings.empty())
    {
        // Buildings are not a group: dragging over a base selects one structure,
        // because "sell" or "set rally point" on twelve buildings at once is never
        // what was meant.
        Incoming.push_back(*std::min_element(OwnBuildings.begin(), OwnBuildings.end(),
                                             [](const EntityId& A, const EntityId& B)
                                             { return A.Index < B.Index; }));
    }
    else if (!Others.empty())
    {
        Incoming.push_back(*std::min_element(Others.begin(), Others.end(),
                                             [](const EntityId& A, const EntityId& B)
                                             { return A.Index < B.Index; }));
    }

    if (Incoming.empty())
    {
        if (Mode == SelectionMode::Replace)
        {
            Clear();
        }
        return;
    }

    ApplyMode(Incoming, Mode);
    SortAndCap(World);
}

void SelectionModel::SelectSameType(const SimWorld& World, EntityId Prototype,
                                    const std::vector<EntityId>& Visible, SelectionMode Mode)
{
    const EntityCore* ProtoCore = World.GetCore(Prototype);
    if (ProtoCore == nullptr || ProtoCore->Owner != LocalPlayer || ProtoCore->Kind != EntityKind::Unit)
    {
        // Double-clicking a building or an enemy behaves like a single click.
        SelectAtCursor(World, {Prototype}, Mode);
        return;
    }

    std::vector<EntityId> Incoming;
    for (const EntityId& Id : Visible)
    {
        const EntityCore* Core = World.GetCore(Id);
        if (Core != nullptr && Core->Owner == LocalPlayer && Core->Kind == EntityKind::Unit &&
            Core->Def == ProtoCore->Def)
        {
            Incoming.push_back(Id);
        }
    }
    if (Incoming.empty())
    {
        Incoming.push_back(Prototype);
    }

    ApplyMode(Incoming, Mode);
    SortAndCap(World);
}

bool SelectionModel::AssignControlGroup(int32_t GroupIndex)
{
    if (GroupIndex < 0 || GroupIndex >= kControlGroupCount)
    {
        return false;
    }
    ControlGroups[GroupIndex] = Selected;
    return true;
}

bool SelectionModel::AddToControlGroup(int32_t GroupIndex)
{
    if (GroupIndex < 0 || GroupIndex >= kControlGroupCount)
    {
        return false;
    }
    std::vector<EntityId>& Group = ControlGroups[GroupIndex];
    for (const EntityId& Id : Selected)
    {
        if (std::find(Group.begin(), Group.end(), Id) == Group.end())
        {
            Group.push_back(Id);
        }
    }
    return true;
}

bool SelectionModel::RecallControlGroup(int32_t GroupIndex, const SimWorld& World)
{
    if (GroupIndex < 0 || GroupIndex >= kControlGroupCount)
    {
        return false;
    }

    std::vector<EntityId>& Group = ControlGroups[GroupIndex];
    // Compact the stored group as a side effect, so a group whose units died in a
    // battle does not keep resurrecting stale handles on every recall.
    Group.erase(std::remove_if(Group.begin(), Group.end(),
                               [&World](const EntityId& Id) { return !World.IsAlive(Id); }),
                Group.end());

    if (Group.empty())
    {
        return false;
    }

    Selected = Group;
    SortAndCap(World);
    return true;
}

int32_t SelectionModel::GetControlGroupSize(int32_t GroupIndex, const SimWorld& World) const
{
    if (GroupIndex < 0 || GroupIndex >= kControlGroupCount)
    {
        return 0;
    }
    int32_t Count = 0;
    for (const EntityId& Id : ControlGroups[GroupIndex])
    {
        if (World.IsAlive(Id))
        {
            ++Count;
        }
    }
    return Count;
}

void SelectionModel::PruneDead(const SimWorld& World)
{
    // Dead entities leave the selection -- and so do enemies that slipped back
    // into fog (review MAJOR-1 on V-A/V-B): a selected enemy used to keep
    // feeding the HUD live HP through the fog after its actor was hidden,
    // which is an intel leak through the selection panel. Own units are always
    // visible to their owner, so this only ever removes enemies.
    Selected.erase(std::remove_if(Selected.begin(), Selected.end(),
                                  [&World, this](const EntityId& Id)
                                  {
                                      if (!World.IsAlive(Id))
                                      {
                                          return true;
                                      }
                                      return !World.IsEntityVisibleTo(LocalPlayer, Id.Index);
                                  }),
                   Selected.end());
}

bool SelectionModel::HasOwnedEntities(const SimWorld& World) const
{
    for (const EntityId& Id : Selected)
    {
        const EntityCore* Core = World.GetCore(Id);
        if (Core != nullptr && Core->Owner == LocalPlayer)
        {
            return true;
        }
    }
    return false;
}

bool SelectionModel::ContainsAnyUnit(const SimWorld& World) const
{
    for (const EntityId& Id : Selected)
    {
        const EntityCore* Core = World.GetCore(Id);
        if (Core != nullptr && Core->Owner == LocalPlayer && Core->Kind == EntityKind::Unit)
        {
            return true;
        }
    }
    return false;
}

void SelectionModel::SelectIdleUnits(const SimWorld& World, SelectionMode Mode)
{
    std::vector<EntityId> IdleUnits;
    for (uint32_t i = 0; i < World.GetEntityCapacity(); ++i)
    {
        EntityId Id = World.MakeId(i);
        if (!World.IsAlive(Id)) continue;
        const EntityCore* Core = World.GetCore(Id);
        if (Core && Core->Owner == LocalPlayer && Core->Kind == EntityKind::Unit)
        {
            const OrderQueue* Orders = World.GetOrders(Id);
            if (Orders && Orders->Count == 0)
            {
                IdleUnits.push_back(Id);
            }
        }
    }
    ApplyMode(IdleUnits, Mode);
    SortAndCap(World);
}

void SelectionModel::SelectWoundedUnits(const SimWorld& World, int32_t HealthPercentThreshold, SelectionMode Mode)
{
    std::vector<EntityId> WoundedUnits;
    for (uint32_t i = 0; i < World.GetEntityCapacity(); ++i)
    {
        EntityId Id = World.MakeId(i);
        if (!World.IsAlive(Id)) continue;
        const EntityCore* Core = World.GetCore(Id);
        if (Core && Core->Owner == LocalPlayer && Core->Kind == EntityKind::Unit)
        {
            const HealthComp* Health = World.GetHealth(Id);
            if (Health && Health->Max > 0)
            {
                int32_t Percent = (Health->Current * 100) / Health->Max;
                if (Percent < HealthPercentThreshold)
                {
                    WoundedUnits.push_back(Id);
                }
            }
        }
    }
    ApplyMode(WoundedUnits, Mode);
    SortAndCap(World);
}


} // namespace Input
} // namespace RA4

