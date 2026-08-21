// Copyright (c) Red Alert 4 project. Procedural vehicle animation (turret aiming, barrel recoil, tread scroll) and VFX driver.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/SimTypes.h"

#ifndef RA4PRESENTATION_API
#define RA4PRESENTATION_API
#endif

namespace RA4
{

/** Procedural turret aiming state for independent chassis-to-turret rotation. */
struct TurretAimState
{
    float CurrentYawDegrees = 0.0f;
    float TargetYawDegrees = 0.0f;
    float CurrentPitchDegrees = 0.0f;
    float TargetPitchDegrees = 0.0f;

    void SetTargetAim(const Vec2& VehiclePos, float VehicleYawDegrees, const Vec2& TargetPos);
    void Update(float DeltaTimeSeconds, float YawSpeedDegPerSec = 180.0f, float PitchSpeedDegPerSec = 90.0f);
};

/** Spring-damper recoil simulation for vehicle gun barrels. */
struct BarrelRecoilState
{
    float RecoilOffset = 0.0f; // 0.0 = resting, 1.0 = fully kicked back
    float Velocity = 0.0f;

    void Fire(float Impulse = 1.0f);
    void Update(float DeltaTimeSeconds, float SpringStiffness = 120.0f, float Damping = 18.0f);
};

/** Differential tread track UV scroll simulation for tracked vehicles. */
struct TreadTrackScrollState
{
    float LeftTrackUV = 0.0f;
    float RightTrackUV = 0.0f;

    void Update(float ForwardSpeedUnitsPerSec, float AngularSpeedDegPerSec, float TrackGaugeUnits, float DeltaTimeSeconds);
};

enum class NiagaraParticleEffectKind : uint8_t
{
    MuzzleFlash = 0,
    ProjectileImpactCrater,
    TeslaLightningArc,
    WaterWakeSplashes,
    ExplosionShockwave,
};

struct NiagaraParticleSpawnRequest
{
    NiagaraParticleEffectKind Kind = NiagaraParticleEffectKind::MuzzleFlash;
    Vec2 Location;
    float RotationDegrees = 0.0f;
    float Scale = 1.0f;
    uint32_t ColorRGBA = 0xFFFFFFFF;
};

class RA4PRESENTATION_API PresentationAnimationFX
{
public:
    PresentationAnimationFX() = default;

    /** Updates or initializes vehicle turret aim state. */
    TurretAimState& GetOrCreateTurret(EntityId Id);

    /** Updates or initializes barrel recoil state. */
    BarrelRecoilState& GetOrCreateRecoil(EntityId Id);

    /** Updates or initializes tread scroll state. */
    TreadTrackScrollState& GetOrCreateTreads(EntityId Id);

    /** Ingests SimEvents to trigger recoils and spawn Niagara VFX requests. */
    void ConsumeSimEvents(const std::vector<SimEvent>& Events);

    /** Advances all procedural animations. */
    void Update(float DeltaTimeSeconds);

    /** Retrieves and clears all queued particle requests. */
    std::vector<NiagaraParticleSpawnRequest> PopQueuedParticleRequests();

    void CleanupDeadEntities(const std::vector<EntityId>& AliveIds);

private:
    std::map<EntityId, TurretAimState> Turrets;
    std::map<EntityId, BarrelRecoilState> Recoils;
    std::map<EntityId, TreadTrackScrollState> Treads;
    std::vector<NiagaraParticleSpawnRequest> QueuedParticles;
};

} // namespace RA4
