// Copyright (c) Red Alert 4 project. Magnetic Satellite & Orbital Debris Re-entry Physics.
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

struct MagneticBeamState
{
    Vec2 Center;
    Fixed Radius = Fixed::FromInt(300);
    uint32_t TotalTicks = 40;
    uint32_t ElapsedTicks = 0;
    uint32_t LiftedCount = 0;
    bool bActive = true;

    void Update(SimWorld& World);
};

struct OrbitalDebrisImpact
{
    Vec2 Target;
    Fixed ImpactRadius = Fixed::FromInt(250);
    int32_t BaseDamage = 1500;
    uint32_t FallDelayTicks = 20; // 1.0s re-entry flight time
    uint32_t ElapsedTicks = 0;
    bool bImpacted = false;

    void Update(SimWorld& World);
};

class RA4SIMULATION_API OrbitalDebrisPhysics
{
public:
    OrbitalDebrisPhysics() = default;

    /** Spawns a Magnetic Satellite attraction beam lifting vehicles into low orbit. */
    void TriggerMagneticSatellite(const Vec2& Center, Fixed Radius = Fixed::FromInt(300), uint32_t DurationTicks = 40);

    /** Triggers an orbital drop of kinetic space scrap / meteorite impacting target area. */
    void TriggerOrbitalDrop(const Vec2& Target, Fixed ImpactRadius = Fixed::FromInt(250), int32_t BaseDamage = 1500);

    void Tick(SimWorld& World);

    const std::vector<MagneticBeamState>& GetActiveBeams() const { return ActiveBeams; }
    const std::vector<OrbitalDebrisImpact>& GetPendingImpacts() const { return PendingImpacts; }

private:
    std::vector<MagneticBeamState> ActiveBeams;
    std::vector<OrbitalDebrisImpact> PendingImpacts;
};

} // namespace RA4
