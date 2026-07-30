// Copyright (c) Red Alert 4 project.
#include "RA4Input/OrderResolver.h"

namespace RA4
{
namespace Input
{

namespace
{

const EntityDef* GetDef(const SimWorld& World, EntityId Id)
{
    const EntityCore* Core = World.GetCore(Id);
    if (Core == nullptr || World.GetContent() == nullptr)
    {
        return nullptr;
    }
    return World.GetContent()->FindEntity(Core->Def);
}

bool IsHostileTo(const SimWorld& World, PlayerId Issuer, EntityId Target)
{
    const EntityCore* Core = World.GetCore(Target);
    if (Core == nullptr)
    {
        return false;
    }
    if (Core->Owner == Issuer || Core->Owner == kNeutralPlayer || Core->Owner == kInvalidPlayer)
    {
        return false;
    }
    return true;
}

bool IsArmed(const SimWorld& World, EntityId Id)
{
    const EntityDef* Def = GetDef(World, Id);
    return Def != nullptr && Def->Weapon.IsValid();
}

bool IsHarvester(const SimWorld& World, EntityId Id)
{
    const EntityDef* Def = GetDef(World, Id);
    return Def != nullptr && Def->Kind == EntityKind::Unit && Def->Unit.bIsHarvester;
}

bool IsRefinery(const SimWorld& World, EntityId Id)
{
    const EntityDef* Def = GetDef(World, Id);
    return Def != nullptr && Def->Kind == EntityKind::Building && Def->Building.bIsRefinery;
}

bool IsResourceNode(const SimWorld& World, EntityId Id)
{
    const EntityCore* Core = World.GetCore(Id);
    return Core != nullptr && Core->Kind == EntityKind::ResourceNode;
}

bool IsMobileUnit(const SimWorld& World, EntityId Id, PlayerId Issuer)
{
    const EntityCore* Core = World.GetCore(Id);
    if (Core == nullptr || Core->Owner != Issuer || Core->Kind != EntityKind::Unit)
    {
        return false;
    }
    const EntityDef* Def = GetDef(World, Id);
    return Def != nullptr && Def->Unit.MaxSpeed > Fixed::Zero();
}

Command MakeOrder(CommandType Type, PlayerId Issuer, EntityId Primary, bool bQueue)
{
    Command C;
    C.Type = Type;
    C.Issuer = Issuer;
    C.Primary = Primary;
    C.Mode = bQueue ? OrderMode::Queue : OrderMode::Replace;
    return C;
}

// Buildings in the selection do not move, but a right-click on the ground is still
// meaningful for them: it sets the rally point for whatever they produce.
std::vector<Command> ResolveForBuildings(const SimWorld& World, const SelectionModel& Selection,
                                         const OrderContext& Context)
{
    std::vector<Command> Out;
    for (const EntityId& Id : Selection.Get())
    {
        const EntityCore* Core = World.GetCore(Id);
        if (Core == nullptr || Core->Owner != Context.Issuer || Core->Kind != EntityKind::Building)
        {
            continue;
        }
        const EntityDef* Def = GetDef(World, Id);
        // Only production buildings have a rally point worth setting.
        if (Def == nullptr || Def->Building.bIsConstructionYard == false)
        {
            const bool bProduces = Def != nullptr && !Def->Building.bIsPowerPlant;
            if (!bProduces)
            {
                continue;
            }
        }
        Command C = MakeOrder(CommandType::SetRallyPoint, Context.Issuer, Id, false);
        C.Location = Context.WorldLocation;
        Out.push_back(C);
    }
    return Out;
}

// Force move rewrites the gesture before any rule sees it: there is deliberately
// nothing under the cursor any more, so every downstream branch -- order and cursor
// hint alike -- naturally collapses to a plain move and the two cannot disagree.
OrderContext ApplyForceMove(const OrderContext& Context)
{
    if (!Context.bForceMove)
    {
        return Context;
    }
    OrderContext Adjusted = Context;
    Adjusted.HoveredEntity = EntityId::Invalid();
    Adjusted.bForceAttack = false;
    Adjusted.bAttackMoveMode = false;
    return Adjusted;
}

} // namespace

std::vector<Command> ResolveOrder(const SimWorld& World, const SelectionModel& Selection,
                                  const OrderContext& RawContext)
{
    const OrderContext Context = ApplyForceMove(RawContext);
    std::vector<Command> Out;

    // --- placing a finished structure ---------------------------------------
    if (Context.bPlacementMode)
    {
        if (Context.PlacementContent.IsValid())
        {
            Command C;
            C.Type = CommandType::PlaceBuilding;
            C.Issuer = Context.Issuer;
            C.Content = Context.PlacementContent;
            C.Tile = Context.Tile;
            Out.push_back(C);
        }
        return Out;
    }

    if (Selection.IsEmpty())
    {
        return Out;
    }

    const bool bTargetValid = Context.HoveredEntity.IsValid() && World.IsAlive(Context.HoveredEntity);
    const bool bTargetHostile = bTargetValid && IsHostileTo(World, Context.Issuer, Context.HoveredEntity);
    const bool bTargetIsResource = bTargetValid && IsResourceNode(World, Context.HoveredEntity);
    const bool bTargetIsOwnRefinery = bTargetValid && IsRefinery(World, Context.HoveredEntity) &&
                                      World.GetCore(Context.HoveredEntity)->Owner == Context.Issuer;

    for (const EntityId& Id : Selection.Get())
    {
        if (!IsMobileUnit(World, Id, Context.Issuer))
        {
            continue;
        }

        // Force attack wins over everything: ctrl-clicking an ally or bare ground
        // means the player deliberately wants fire there.
        if (Context.bForceAttack && bTargetValid && IsArmed(World, Id))
        {
            Command C = MakeOrder(CommandType::Attack, Context.Issuer, Id, Context.bQueueOrder);
            C.Target = Context.HoveredEntity;
            C.Location = Context.WorldLocation;
            Out.push_back(C);
            continue;
        }

        // Attack-move: armed by pressing A, then clicking anywhere.
        if (Context.bAttackMoveMode)
        {
            Command C = MakeOrder(CommandType::AttackMove, Context.Issuer, Id, Context.bQueueOrder);
            C.Location = Context.WorldLocation;
            Out.push_back(C);
            continue;
        }

        // Enemy under the cursor: armed units attack it, unarmed ones (harvesters,
        // an MCV) move there instead of standing still looking confused.
        if (bTargetHostile)
        {
            if (IsArmed(World, Id))
            {
                Command C = MakeOrder(CommandType::Attack, Context.Issuer, Id, Context.bQueueOrder);
                C.Target = Context.HoveredEntity;
                C.Location = Context.WorldLocation;
                Out.push_back(C);
            }
            else
            {
                Command C = MakeOrder(CommandType::Move, Context.Issuer, Id, Context.bQueueOrder);
                C.Location = Context.WorldLocation;
                Out.push_back(C);
            }
            continue;
        }

        // Ore under the cursor: harvesters gather it, everyone else drives there.
        if (bTargetIsResource && IsHarvester(World, Id))
        {
            Command C = MakeOrder(CommandType::Harvest, Context.Issuer, Id, Context.bQueueOrder);
            C.Target = Context.HoveredEntity;
            C.Location = Context.WorldLocation;
            Out.push_back(C);
            continue;
        }

        // A loaded harvester sent to a refinery should unload, which the harvester
        // state machine already does on arrival -- so a plain move is the correct
        // command, and the cursor tells the player it will be a delivery.
        if (bTargetIsOwnRefinery && IsHarvester(World, Id))
        {
            Command C = MakeOrder(CommandType::Move, Context.Issuer, Id, Context.bQueueOrder);
            C.Location = Context.WorldLocation;
            Out.push_back(C);
            continue;
        }

        Command C = MakeOrder(CommandType::Move, Context.Issuer, Id, Context.bQueueOrder);
        C.Location = Context.WorldLocation;
        Out.push_back(C);
    }

    // Nothing mobile was selected: fall back to treating this as a rally point set.
    if (Out.empty() && !Context.bAttackMoveMode && !bTargetHostile)
    {
        Out = ResolveForBuildings(World, Selection, Context);
    }

    return Out;
}

CursorHint ResolveCursorHint(const SimWorld& World, const SelectionModel& Selection,
                             const OrderContext& RawContext)
{
    const OrderContext Context = ApplyForceMove(RawContext);

    if (Context.bPlacementMode)
    {
        return Context.PlacementContent.IsValid() &&
                       World.IsPlacementValid(Context.PlacementContent, Context.Issuer, Context.Tile)
                   ? CursorHint::Move
                   : CursorHint::NoEntry;
    }

    if (Selection.IsEmpty() || !Selection.HasOwnedEntities(World))
    {
        return CursorHint::Select;
    }

    const bool bTargetValid = Context.HoveredEntity.IsValid() && World.IsAlive(Context.HoveredEntity);

    if (!Selection.ContainsAnyUnit(World))
    {
        // Only buildings selected: the ground click sets a rally point.
        return bTargetValid ? CursorHint::Select : CursorHint::SetRallyPoint;
    }

    if (Context.bForceAttack && bTargetValid)
    {
        return CursorHint::ForceAttack;
    }
    if (Context.bAttackMoveMode)
    {
        return CursorHint::Attack;
    }

    if (bTargetValid)
    {
        if (IsHostileTo(World, Context.Issuer, Context.HoveredEntity))
        {
            // Report what will actually happen: a selection of pure harvesters
            // cannot attack, and the cursor must not claim otherwise.
            for (const EntityId& Id : Selection.Get())
            {
                if (IsMobileUnit(World, Id, Context.Issuer) && IsArmed(World, Id))
                {
                    return CursorHint::Attack;
                }
            }
            return CursorHint::Move;
        }

        if (IsResourceNode(World, Context.HoveredEntity))
        {
            for (const EntityId& Id : Selection.Get())
            {
                if (IsHarvester(World, Id))
                {
                    return CursorHint::Harvest;
                }
            }
            return CursorHint::Move;
        }

        const EntityCore* TargetCore = World.GetCore(Context.HoveredEntity);
        if (TargetCore != nullptr && TargetCore->Owner == Context.Issuer &&
            IsRefinery(World, Context.HoveredEntity))
        {
            for (const EntityId& Id : Selection.Get())
            {
                if (IsHarvester(World, Id))
                {
                    return CursorHint::Deliver;
                }
            }
        }
    }

    return CursorHint::Move;
}

} // namespace Input
} // namespace RA4
