// Copyright (c) Red Alert 4 project. Exotic superweapon mechanics (Vacuum Imploder, Iron Curtain, Chrono Teleport).
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

struct VacuumImploderState
{
    Vec2 Epicenter;
    Fixed Radius = Fixed::FromInt(600);
    uint32_t TotalTicks = 40; // 2.0s pull duration @ 20Hz
    uint32_t ElapsedTicks = 0;
    Fixed PullSpeed = Fixed::FromInt(120); // Displacement per second
    int32_t FinalDamage = 3000;
    bool bCompleted = false;

    void Update(SimWorld& World);
};

class RA4SIMULATION_API ExoticSuperweaponPhysics
{
public:
    ExoticSuperweaponPhysics() = default;

    /** Spawns a vacuum imploder gravitational singularity. */
    void TriggerVacuumImploder(const Vec2& Epicenter, Fixed Radius, int32_t Damage = 3000);

    /** Applies Iron Curtain invulnerability to vehicles while destroying infantry. */
    static void TriggerIronCurtain(const Vec2& Center, Fixed Radius, uint32_t DurationTicks, SimWorld& World);

    /** Teleports a group of vehicles to target location. Sinks non-amphibious vehicles in water. */
    static void TriggerChronoSphere(const Vec2& SourceCenter, const Vec2& TargetCenter, Fixed Radius, SimWorld& World);

    /** Advances all active gravitational singularities. */
    void Tick(SimWorld& World);

    const std::vector<VacuumImploderState>& GetActiveImploders() const { return ActiveImploders; }

private:
    std::vector<VacuumImploderState> ActiveImploders;
};

} // namespace RA4
