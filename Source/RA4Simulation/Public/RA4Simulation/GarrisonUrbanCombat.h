// Copyright (c) Red Alert 4 project. Garrison and Neutral Structure Urban Warfare.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/SimTypes.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4SIMULATION_API
#define RA4SIMULATION_API
#endif

namespace RA4
{

struct GarrisonStructure
{
    EntityId BuildingId;
    PlayerId Controller = kInvalidPlayer;
    std::vector<EntityId> Occupants;
    uint32_t MaxCapacity = 5;
    int32_t DamageReductionPercent = 70; // 70% damage reduction for occupants
    Fixed RangeMultiplier = Fixed::FromRatio(3, 2); // 1.5x range bonus

    bool IsFull() const { return Occupants.size() >= MaxCapacity; }
    bool IsEmpty() const { return Occupants.empty(); }
};

class RA4SIMULATION_API GarrisonUrbanCombat
{
public:
    GarrisonUrbanCombat() = default;

    /** Registers a garrisonable civilian structure. */
    void RegisterBuilding(EntityId BuildingId, uint32_t MaxCapacity = 5);

    /** Orders an infantry squad to enter and garrison the building. */
    bool EnterGarrison(EntityId BuildingId, EntityId InfantryId, SimWorld& World);

    /** Evacuates all garrisoned infantry around the building. */
    void EvacuateGarrison(EntityId BuildingId, SimWorld& World);

    /** Applies special anti-garrison weapon effects (Flame, Cryo, Toxin) clearing the structure. */
    bool ApplyAntiGarrisonAttack(EntityId BuildingId, WarheadClass Warhead, int32_t Damage, SimWorld& World);

    const GarrisonStructure* FindGarrison(EntityId BuildingId) const;
    GarrisonStructure* FindGarrisonMutable(EntityId BuildingId);

    void Tick(SimWorld& World);

private:
    std::vector<GarrisonStructure> Garrisons;
};

} // namespace RA4
