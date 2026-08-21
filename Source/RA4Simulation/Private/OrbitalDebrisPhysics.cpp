// Copyright (c) Red Alert 4 project. Magnetic Satellite & Orbital Debris Re-entry Physics.
#include "RA4Simulation/OrbitalDebrisPhysics.h"
#include "RA4Content/ContentDatabase.h"

#include <algorithm>

namespace RA4
{

void MagneticBeamState::Update(SimWorld& World)
{
    if (!bActive) return;

    ++ElapsedTicks;

    const auto& Cores = World.GetAllCores();
    const auto& Transforms = World.GetAllTransforms();
    const Fixed RadiusSq = Radius * Radius;

    for (uint32_t I = 0; I < Cores.size(); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Kind != EntityKind::Unit)
        {
            continue;
        }

        const Vec2 Pos = Transforms[I].Position;
        if (DistanceSquared(Pos, Center) <= RadiusSq)
        {
            const EntityId Id = World.MakeId(I);
            if (World.GetContent())
            {
                const auto* Def = World.GetContent()->FindEntity(Cores[I].Def);
                if (Def && Def->Unit.Layer != MovementLayer::Infantry)
                {
                    // Mechanical vehicles/vessels are sucked up into orbit
                    World.DebugDamage(Id, 999999);
                    ++LiftedCount;
                }
            }
        }
    }

    if (ElapsedTicks >= TotalTicks)
    {
        bActive = false;
    }
}

void OrbitalDebrisImpact::Update(SimWorld& World)
{
    if (bImpacted) return;

    ++ElapsedTicks;

    if (ElapsedTicks >= FallDelayTicks)
    {
        const auto& Cores = World.GetAllCores();
        const auto& Transforms = World.GetAllTransforms();
        const Fixed RadiusSq = ImpactRadius * ImpactRadius;

        for (uint32_t I = 0; I < Cores.size(); ++I)
        {
            if (!Cores[I].bAlive || Cores[I].Kind == EntityKind::Projectile || Cores[I].Kind == EntityKind::ResourceNode)
            {
                continue;
            }

            const Vec2 Pos = Transforms[I].Position;
            if (DistanceSquared(Pos, Target) <= RadiusSq)
            {
                World.DebugDamage(World.MakeId(I), BaseDamage);
            }
        }

        bImpacted = true;
    }
}

void OrbitalDebrisPhysics::TriggerMagneticSatellite(const Vec2& Center, Fixed Radius, uint32_t DurationTicks)
{
    MagneticBeamState Beam;
    Beam.Center = Center;
    Beam.Radius = Radius;
    Beam.TotalTicks = DurationTicks;
    ActiveBeams.push_back(Beam);
}

void OrbitalDebrisPhysics::TriggerOrbitalDrop(const Vec2& Target, Fixed ImpactRadius, int32_t BaseDamage)
{
    OrbitalDebrisImpact Impact;
    Impact.Target = Target;
    Impact.ImpactRadius = ImpactRadius;
    Impact.BaseDamage = BaseDamage;
    PendingImpacts.push_back(Impact);
}

void OrbitalDebrisPhysics::Tick(SimWorld& World)
{
    for (auto& Beam : ActiveBeams)
    {
        Beam.Update(World);
    }

    for (auto& Impact : PendingImpacts)
    {
        Impact.Update(World);
    }

    ActiveBeams.erase(
        std::remove_if(ActiveBeams.begin(), ActiveBeams.end(),
                       [](const MagneticBeamState& B) { return !B.bActive; }),
        ActiveBeams.end());

    PendingImpacts.erase(
        std::remove_if(PendingImpacts.begin(), PendingImpacts.end(),
                       [](const OrbitalDebrisImpact& I) { return I.bImpacted; }),
        PendingImpacts.end());
}

} // namespace RA4
