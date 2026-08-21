// Copyright (c) Red Alert 4 project. Garrison and Neutral Structure Urban Warfare.
#include "RA4Simulation/GarrisonUrbanCombat.h"
#include "RA4Content/ContentDatabase.h"

#include <algorithm>

namespace RA4
{

void GarrisonUrbanCombat::RegisterBuilding(EntityId BuildingId, uint32_t MaxCapacity)
{
    for (auto& G : Garrisons)
    {
        if (G.BuildingId == BuildingId)
        {
            G.MaxCapacity = MaxCapacity;
            return;
        }
    }

    GarrisonStructure G;
    G.BuildingId = BuildingId;
    G.MaxCapacity = MaxCapacity;
    G.Controller = kInvalidPlayer;
    Garrisons.push_back(G);
}

const GarrisonStructure* GarrisonUrbanCombat::FindGarrison(EntityId BuildingId) const
{
    for (const auto& G : Garrisons)
    {
        if (G.BuildingId == BuildingId)
        {
            return &G;
        }
    }
    return nullptr;
}

GarrisonStructure* GarrisonUrbanCombat::FindGarrisonMutable(EntityId BuildingId)
{
    for (auto& G : Garrisons)
    {
        if (G.BuildingId == BuildingId)
        {
            return &G;
        }
    }
    return nullptr;
}

bool GarrisonUrbanCombat::EnterGarrison(EntityId BuildingId, EntityId InfantryId, SimWorld& World)
{
    if (!World.IsAlive(BuildingId) || !World.IsAlive(InfantryId))
    {
        return false;
    }

    auto* G = FindGarrisonMutable(BuildingId);
    if (!G || G->IsFull())
    {
        return false;
    }

    const auto* InfantryCore = World.GetCore(InfantryId);
    if (!InfantryCore || !World.GetContent())
    {
        return false;
    }

    const auto* Def = World.GetContent()->FindEntity(InfantryCore->Def);
    if (!Def || Def->Unit.Layer != MovementLayer::Infantry)
    {
        return false; // Only infantry squads can garrison urban buildings
    }

    if (G->IsEmpty())
    {
        G->Controller = InfantryCore->Owner;
    }
    else if (G->Controller != InfantryCore->Owner)
    {
        return false; // Cannot enter enemy-held garrison
    }

    G->Occupants.push_back(InfantryId);
    const Vec2 BldgPos = World.GetTransform(BuildingId)->Position;
    World.TeleportEntity(InfantryId, BldgPos);

    return true;
}

void GarrisonUrbanCombat::EvacuateGarrison(EntityId BuildingId, SimWorld& World)
{
    auto* G = FindGarrisonMutable(BuildingId);
    if (!G || G->IsEmpty())
    {
        return;
    }

    const Vec2 BldgPos = World.GetTransform(BuildingId)->Position;
    for (size_t I = 0; I < G->Occupants.size(); ++I)
    {
        const EntityId Occupant = G->Occupants[I];
        if (World.IsAlive(Occupant))
        {
            const Fixed OffsetX = Fixed::FromInt(int32_t(I % 3) * 40 - 40);
            const Fixed OffsetY = Fixed::FromInt(int32_t(I / 3) * 40 + 60);
            World.TeleportEntity(Occupant, BldgPos + Vec2(OffsetX, OffsetY));
        }
    }

    G->Occupants.clear();
    G->Controller = kInvalidPlayer;
}

bool GarrisonUrbanCombat::ApplyAntiGarrisonAttack(EntityId BuildingId, WarheadClass Warhead, int32_t Damage, SimWorld& World)
{
    auto* G = FindGarrisonMutable(BuildingId);
    if (!G || G->IsEmpty())
    {
        return false;
    }

    // Flame, Cryogenic, and Siege warheads effectively flush/clear garrisons
    if (Warhead == WarheadClass::Flame || Warhead == WarheadClass::Cryogenic || Warhead == WarheadClass::Siege || Warhead == WarheadClass::HighExplosive)

    {
        for (const auto& Occupant : G->Occupants)
        {
            if (World.IsAlive(Occupant))
            {
                World.DebugDamage(Occupant, Damage);
            }
        }

        World.Tick(nullptr); // Process deaths

        G->Occupants.erase(
            std::remove_if(G->Occupants.begin(), G->Occupants.end(),
                           [&](EntityId Id) { return !World.IsAlive(Id); }),
            G->Occupants.end());

        if (G->IsEmpty())
        {
            G->Controller = kInvalidPlayer;
        }

        return true;
    }

    return false;
}

void GarrisonUrbanCombat::Tick(SimWorld& World)
{
    for (auto& G : Garrisons)
    {
        if (!World.IsAlive(G.BuildingId))
        {
            // Building collapsed -> crush all remaining occupants
            for (const auto& Occupant : G.Occupants)
            {
                if (World.IsAlive(Occupant))
                {
                    World.DebugDamage(Occupant, 9999);
                }
            }
            G.Occupants.clear();
            G.Controller = kInvalidPlayer;
        }
        else
        {
            // Clean up any dead occupants
            G.Occupants.erase(
                std::remove_if(G.Occupants.begin(), G.Occupants.end(),
                               [&](EntityId Id) { return !World.IsAlive(Id); }),
                G.Occupants.end());

            if (G.IsEmpty())
            {
                G.Controller = kInvalidPlayer;
            }
        }
    }
}

} // namespace RA4
