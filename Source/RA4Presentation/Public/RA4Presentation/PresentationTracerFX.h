// Copyright (c) Red Alert 4 project. Visual effects, tracers, and impact presentation.
#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/SimTypes.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4PRESENTATION_API
#define RA4PRESENTATION_API
#endif

namespace RA4
{
namespace Presentation
{

enum class TracerType : uint8_t
{
    StandardBullet = 0,
    HeavyCannonShell,
    RocketMissile,
    TeslaArc,
    LaserBeam,
    ChronoPhase
};

struct ActiveTracer
{
    uint32_t Id = 0;
    TracerType Type = TracerType::StandardBullet;
    EntityId SourceEntity;
    EntityId TargetEntity;
    Vec2 Origin;
    Vec2 Target;
    Vec2 CurrentPos;
    float ProgressAlpha = 0.0f; // 0.0 at spawn -> 1.0 at target
    float SpeedCmPerSec = 4000.0f;
    float LifetimeSeconds = 0.0f;
    float MaxLifetimeSeconds = 0.5f;
    float WidthCm = 6.0f;
    uint32_t ColorRGBA = 0xFFFFFFFF;
    bool bIsBeam = false;
    bool bAlive = true;
};

struct ImpactDecal
{
    uint32_t Id = 0;
    Vec2 Location;
    WarheadClass Warhead = WarheadClass::SmallArms;
    float RadiusCm = 80.0f;
    float DurationSeconds = 0.0f;
    float MaxDurationSeconds = 4.0f;
    float Alpha = 1.0f;
    bool bAlive = true;
};

struct MuzzleFlash
{
    uint32_t Id = 0;
    EntityId Entity;
    Vec2 Location;
    int32_t FacingDegrees = 0;
    float DurationSeconds = 0.0f;
    float MaxDurationSeconds = 0.1f;
    float Scale = 1.0f;
    bool bAlive = true;
};

class RA4PRESENTATION_API PresentationTracerFX
{
public:
    PresentationTracerFX() = default;

    /** Ingests SimEvents emitted during the last simulation tick and spawns corresponding
        tracers, beams, muzzle flashes, and impact decals. */
    void ConsumeSimEvents(const std::vector<SimEvent>& Events, const SimWorld& World);

    /** Advances all active visual effects in real time. */
    void Tick(float DeltaTimeSeconds);

    /** Spawns a moving projectile tracer between two world points. */
    uint32_t SpawnTracer(TracerType Type, const Vec2& From, const Vec2& To,
                         float SpeedCmPerSec = 4000.0f, float Width = 6.0f,
                         uint32_t ColorRGBA = 0xFFFFFFAA);

    /** Spawns a continuous beam effect (e.g. Tesla coil arc or laser). */
    uint32_t SpawnBeam(TracerType Type, const Vec2& From, const Vec2& To,
                       float DurationSec = 0.3f, float Width = 12.0f,
                       uint32_t ColorRGBA = 0x88CCFFFF);

    /** Spawns an impact explosion/crater decal on the ground. */
    uint32_t SpawnImpact(const Vec2& Location, WarheadClass Warhead,
                         float RadiusCm = 100.0f, float DurationSec = 3.0f);

    /** Spawns a muzzle flash at the barrel of a firing entity. */
    uint32_t SpawnMuzzleFlash(EntityId Entity, const Vec2& Location, int32_t FacingDegrees, float Scale = 1.0f);

    const std::vector<ActiveTracer>& GetActiveTracers() const { return Tracers; }
    const std::vector<ImpactDecal>& GetActiveImpacts() const { return Impacts; }
    const std::vector<MuzzleFlash>& GetActiveMuzzleFlashes() const { return MuzzleFlashes; }

    void Clear();

private:
    uint32_t NextId = 1;
    std::vector<ActiveTracer> Tracers;
    std::vector<ImpactDecal> Impacts;
    std::vector<MuzzleFlash> MuzzleFlashes;
};

} // namespace Presentation
} // namespace RA4
