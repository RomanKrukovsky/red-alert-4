// Copyright (c) Red Alert 4 project. Procedural vehicle animation and VFX driver implementation.
#include "RA4Presentation/PresentationAnimationFX.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace RA4
{

namespace
{

constexpr float kRadToDeg = 57.29577951308232f;
constexpr float kDegToRad = 0.017453292519943f;

float NormalizeAngleDeg(float Angle)
{
    while (Angle > 180.0f) Angle -= 360.0f;
    while (Angle < -180.0f) Angle += 360.0f;
    return Angle;
}

} // namespace

void TurretAimState::SetTargetAim(const Vec2& VehiclePos, float VehicleYawDegrees, const Vec2& TargetPos)
{
    const Vec2 Diff = TargetPos - VehiclePos;
    const float DX = static_cast<float>(Diff.X.Raw) / static_cast<float>(kFixedOne);
    const float DY = static_cast<float>(Diff.Y.Raw) / static_cast<float>(kFixedOne);


    if (std::abs(DX) < 1e-4f && std::abs(DY) < 1e-4f)
    {
        TargetYawDegrees = 0.0f;
        return;
    }

    const float GlobalAimYaw = std::atan2(DY, DX) * kRadToDeg;
    TargetYawDegrees = NormalizeAngleDeg(GlobalAimYaw - VehicleYawDegrees);
}

void TurretAimState::Update(float DeltaTimeSeconds, float YawSpeedDegPerSec, float PitchSpeedDegPerSec)
{
    // Shortest-arc Yaw interpolation
    const float DiffYaw = NormalizeAngleDeg(TargetYawDegrees - CurrentYawDegrees);
    const float MaxYawStep = YawSpeedDegPerSec * DeltaTimeSeconds;

    if (std::abs(DiffYaw) <= MaxYawStep)
    {
        CurrentYawDegrees = TargetYawDegrees;
    }
    else
    {
        CurrentYawDegrees = NormalizeAngleDeg(CurrentYawDegrees + (DiffYaw > 0.0f ? MaxYawStep : -MaxYawStep));
    }

    // Pitch interpolation
    const float DiffPitch = TargetPitchDegrees - CurrentPitchDegrees;
    const float MaxPitchStep = PitchSpeedDegPerSec * DeltaTimeSeconds;

    if (std::abs(DiffPitch) <= MaxPitchStep)
    {
        CurrentPitchDegrees = TargetPitchDegrees;
    }
    else
    {
        CurrentPitchDegrees += (DiffPitch > 0.0f ? MaxPitchStep : -MaxPitchStep);
    }
}

void BarrelRecoilState::Fire(float Impulse)
{
    RecoilOffset = std::min(1.0f, RecoilOffset + Impulse);
    Velocity = 0.0f;
}

void BarrelRecoilState::Update(float DeltaTimeSeconds, float SpringStiffness, float Damping)
{
    if (RecoilOffset <= 0.0f && std::abs(Velocity) < 1e-4f)
    {
        RecoilOffset = 0.0f;
        Velocity = 0.0f;
        return;
    }

    const float SpringForce = -SpringStiffness * RecoilOffset;
    const float DampingForce = -Damping * Velocity;
    const float TotalForce = SpringForce + DampingForce;

    Velocity += TotalForce * DeltaTimeSeconds;
    RecoilOffset += Velocity * DeltaTimeSeconds;

    if (RecoilOffset < 0.0f)
    {
        RecoilOffset = 0.0f;
        Velocity = 0.0f;
    }
}

void TreadTrackScrollState::Update(float ForwardSpeedUnitsPerSec, float AngularSpeedDegPerSec, float TrackGaugeUnits, float DeltaTimeSeconds)
{
    const float AngularVelocityRad = AngularSpeedDegPerSec * kDegToRad;
    const float GaugeHalf = TrackGaugeUnits * 0.5f;

    const float LeftSpeed = ForwardSpeedUnitsPerSec - (AngularVelocityRad * GaugeHalf);
    const float RightSpeed = ForwardSpeedUnitsPerSec + (AngularVelocityRad * GaugeHalf);

    constexpr float kScrollScale = 0.005f;
    LeftTrackUV = std::fmod(LeftTrackUV + LeftSpeed * DeltaTimeSeconds * kScrollScale, 1.0f);
    RightTrackUV = std::fmod(RightTrackUV + RightSpeed * DeltaTimeSeconds * kScrollScale, 1.0f);

    if (LeftTrackUV < 0.0f) LeftTrackUV += 1.0f;
    if (RightTrackUV < 0.0f) RightTrackUV += 1.0f;
}

TurretAimState& PresentationAnimationFX::GetOrCreateTurret(EntityId Id)
{
    return Turrets[Id];
}

BarrelRecoilState& PresentationAnimationFX::GetOrCreateRecoil(EntityId Id)
{
    return Recoils[Id];
}

TreadTrackScrollState& PresentationAnimationFX::GetOrCreateTreads(EntityId Id)
{
    return Treads[Id];
}

void PresentationAnimationFX::ConsumeSimEvents(const std::vector<SimEvent>& Events)
{
    for (const auto& Ev : Events)
    {
        switch (Ev.Type)
        {
        case SimEventType::WeaponFired:
        {
            if (Ev.Entity.IsValid())
            {
                GetOrCreateRecoil(Ev.Entity).Fire(1.0f);
            }
            NiagaraParticleSpawnRequest Req;
            Req.Kind = NiagaraParticleEffectKind::MuzzleFlash;
            Req.Location = Ev.Location;
            Req.Scale = 1.0f;
            QueuedParticles.push_back(Req);
            break;
        }

        case SimEventType::ProjectileImpact:
        case SimEventType::DamageApplied:
        {
            NiagaraParticleSpawnRequest Req;
            Req.Kind = NiagaraParticleEffectKind::ProjectileImpactCrater;
            Req.Location = Ev.Location;
            Req.Scale = 1.2f;
            QueuedParticles.push_back(Req);
            break;
        }

        default:
            break;
        }
    }
}

void PresentationAnimationFX::Update(float DeltaTimeSeconds)
{
    for (auto& Pair : Turrets)
    {
        Pair.second.Update(DeltaTimeSeconds);
    }
    for (auto& Pair : Recoils)
    {
        Pair.second.Update(DeltaTimeSeconds);
    }
}

std::vector<NiagaraParticleSpawnRequest> PresentationAnimationFX::PopQueuedParticleRequests()
{
    std::vector<NiagaraParticleSpawnRequest> Result = std::move(QueuedParticles);
    QueuedParticles.clear();
    return Result;
}

void PresentationAnimationFX::CleanupDeadEntities(const std::vector<EntityId>& AliveIds)
{
    std::set<EntityId> AliveSet(AliveIds.begin(), AliveIds.end());

    for (auto It = Turrets.begin(); It != Turrets.end();)
    {
        if (AliveSet.find(It->first) == AliveSet.end()) It = Turrets.erase(It);
        else ++It;
    }
    for (auto It = Recoils.begin(); It != Recoils.end();)
    {
        if (AliveSet.find(It->first) == AliveSet.end()) It = Recoils.erase(It);
        else ++It;
    }
    for (auto It = Treads.begin(); It != Treads.end();)
    {
        if (AliveSet.find(It->first) == AliveSet.end()) It = Treads.erase(It);
        else ++It;
    }
}

} // namespace RA4
