// Copyright (c) Red Alert 4 project. Visual effects, tracers, and impact presentation implementation.
#include "RA4Presentation/PresentationTracerFX.h"

#include <algorithm>
#include <cmath>

namespace RA4
{
namespace Presentation
{

void PresentationTracerFX::ConsumeSimEvents(const std::vector<SimEvent>& Events, const SimWorld& World)
{
    for (const auto& Ev : Events)
    {
        if (Ev.Type == SimEventType::WeaponFired)
        {
            Vec2 From = Ev.Location;
            int32_t Facing = 0;
            if (World.IsAlive(Ev.Entity))
            {
                const auto* Trans = World.GetTransform(Ev.Entity);
                if (Trans != nullptr)
                {
                    From = Trans->Position;
                    Facing = Trans->TurretFacing;
                }
            }

            Vec2 To = Ev.Location;
            if (World.IsAlive(Ev.Other))
            {
                const auto* TargetTrans = World.GetTransform(Ev.Other);
                if (TargetTrans != nullptr)
                {
                    To = TargetTrans->Position;
                }
            }

            SpawnMuzzleFlash(Ev.Entity, From, Facing);
            SpawnTracer(TracerType::StandardBullet, From, To, 4000.0f, 6.0f, 0xFFFFFFAA);
        }
        else if (Ev.Type == SimEventType::ProjectileImpact || Ev.Type == SimEventType::DamageApplied)
        {
            SpawnImpact(Ev.Location, WarheadClass::Ballistic, 100.0f, 3.0f);
        }
    }
}

void PresentationTracerFX::Tick(float DeltaTimeSeconds)
{
    if (DeltaTimeSeconds <= 0.0f)
    {
        return;
    }

    // Update active tracers
    for (auto& Tracer : Tracers)
    {
        if (!Tracer.bAlive) continue;

        Tracer.LifetimeSeconds += DeltaTimeSeconds;

        if (Tracer.bIsBeam)
        {
            if (Tracer.LifetimeSeconds >= Tracer.MaxLifetimeSeconds)
            {
                Tracer.bAlive = false;
            }
        }
        else
        {
            const float Dx = static_cast<float>((Tracer.Target.X - Tracer.Origin.X).ToDoubleUnsafe());
            const float Dy = static_cast<float>((Tracer.Target.Y - Tracer.Origin.Y).ToDoubleUnsafe());
            const float Dist = std::sqrt(Dx * Dx + Dy * Dy);

            if (Dist <= 0.001f)
            {
                Tracer.ProgressAlpha = 1.0f;
                Tracer.bAlive = false;
            }
            else
            {
                const float StepAlpha = (Tracer.SpeedCmPerSec * DeltaTimeSeconds) / Dist;
                Tracer.ProgressAlpha += StepAlpha;

                if (Tracer.ProgressAlpha >= 1.0f)
                {
                    Tracer.ProgressAlpha = 1.0f;
                    Tracer.CurrentPos = Tracer.Target;
                    Tracer.bAlive = false;
                }
                else
                {
                    const Fixed FAlpha = Fixed(static_cast<int64_t>(Tracer.ProgressAlpha * 65536.0f));
                    Tracer.CurrentPos.X = Tracer.Origin.X + (Tracer.Target.X - Tracer.Origin.X) * FAlpha;
                    Tracer.CurrentPos.Y = Tracer.Origin.Y + (Tracer.Target.Y - Tracer.Origin.Y) * FAlpha;
                }
            }
        }
    }

    // Prune dead tracers
    Tracers.erase(std::remove_if(Tracers.begin(), Tracers.end(),
                                 [](const ActiveTracer& T) { return !T.bAlive; }),
                  Tracers.end());

    // Update active impact decals
    for (auto& Impact : Impacts)
    {
        if (!Impact.bAlive) continue;

        Impact.DurationSeconds += DeltaTimeSeconds;
        if (Impact.DurationSeconds >= Impact.MaxDurationSeconds)
        {
            Impact.bAlive = false;
            Impact.Alpha = 0.0f;
        }
        else
        {
            Impact.Alpha = 1.0f - (Impact.DurationSeconds / Impact.MaxDurationSeconds);
        }
    }

    // Prune dead impacts
    Impacts.erase(std::remove_if(Impacts.begin(), Impacts.end(),
                                 [](const ImpactDecal& I) { return !I.bAlive; }),
                  Impacts.end());

    // Update muzzle flashes
    for (auto& Flash : MuzzleFlashes)
    {
        if (!Flash.bAlive) continue;

        Flash.DurationSeconds += DeltaTimeSeconds;
        if (Flash.DurationSeconds >= Flash.MaxDurationSeconds)
        {
            Flash.bAlive = false;
        }
    }

    // Prune dead muzzle flashes
    MuzzleFlashes.erase(std::remove_if(MuzzleFlashes.begin(), MuzzleFlashes.end(),
                                       [](const MuzzleFlash& M) { return !M.bAlive; }),
                        MuzzleFlashes.end());
}

uint32_t PresentationTracerFX::SpawnTracer(TracerType Type, const Vec2& From, const Vec2& To,
                                           float SpeedCmPerSec, float Width, uint32_t ColorRGBA)
{
    ActiveTracer Tracer;
    Tracer.Id = NextId++;
    Tracer.Type = Type;
    Tracer.Origin = From;
    Tracer.Target = To;
    Tracer.CurrentPos = From;
    Tracer.ProgressAlpha = 0.0f;
    Tracer.SpeedCmPerSec = SpeedCmPerSec > 0.0f ? SpeedCmPerSec : 4000.0f;
    Tracer.WidthCm = Width;
    Tracer.ColorRGBA = ColorRGBA;
    Tracer.bIsBeam = false;
    Tracer.bAlive = true;

    Tracers.push_back(Tracer);
    return Tracer.Id;
}

uint32_t PresentationTracerFX::SpawnBeam(TracerType Type, const Vec2& From, const Vec2& To,
                                         float DurationSec, float Width, uint32_t ColorRGBA)
{
    ActiveTracer Tracer;
    Tracer.Id = NextId++;
    Tracer.Type = Type;
    Tracer.Origin = From;
    Tracer.Target = To;
    Tracer.CurrentPos = From;
    Tracer.ProgressAlpha = 0.0f;
    Tracer.LifetimeSeconds = 0.0f;
    Tracer.MaxLifetimeSeconds = DurationSec > 0.0f ? DurationSec : 0.3f;
    Tracer.WidthCm = Width;
    Tracer.ColorRGBA = ColorRGBA;
    Tracer.bIsBeam = true;
    Tracer.bAlive = true;

    Tracers.push_back(Tracer);
    return Tracer.Id;
}

uint32_t PresentationTracerFX::SpawnImpact(const Vec2& Location, WarheadClass Warhead,
                                           float RadiusCm, float DurationSec)
{
    ImpactDecal Impact;
    Impact.Id = NextId++;
    Impact.Location = Location;
    Impact.Warhead = Warhead;
    Impact.RadiusCm = RadiusCm;
    Impact.DurationSeconds = 0.0f;
    Impact.MaxDurationSeconds = DurationSec > 0.0f ? DurationSec : 3.0f;
    Impact.Alpha = 1.0f;
    Impact.bAlive = true;

    Impacts.push_back(Impact);
    return Impact.Id;
}

uint32_t PresentationTracerFX::SpawnMuzzleFlash(EntityId Entity, const Vec2& Location,
                                               int32_t FacingDegrees, float Scale)
{
    MuzzleFlash Flash;
    Flash.Id = NextId++;
    Flash.Entity = Entity;
    Flash.Location = Location;
    Flash.FacingDegrees = FacingDegrees;
    Flash.DurationSeconds = 0.0f;
    Flash.MaxDurationSeconds = 0.1f;
    Flash.Scale = Scale;
    Flash.bAlive = true;

    MuzzleFlashes.push_back(Flash);
    return Flash.Id;
}

void PresentationTracerFX::Clear()
{
    Tracers.clear();
    Impacts.clear();
    MuzzleFlashes.clear();
}

} // namespace Presentation
} // namespace RA4
