// Copyright (c) Red Alert 4 project. Data-oriented presentation interpolator.
//
// Smoothly interpolates discrete 20Hz simulation ticks into high-frequency (60-144+ FPS)
// visual transforms for Unreal Engine actors, Mass Entity instances, and VAT meshes.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4PRESENTATION_API
#define RA4PRESENTATION_API
#endif

namespace RA4
{
namespace Presentation
{

// Flat presentation state representation of an entity, ready for GPU instancing or Actor transforms.
struct InterpolatedEntityState
{
    EntityId Id;
    uint32_t Index = 0;
    ContentId Def;
    EntityKind Kind = EntityKind::Unit;
    PlayerId Owner = kInvalidPlayer;
    bool bAlive = false;

    // World coordinates (cm, 1:1 with Unreal Engine world space)
    float WorldX = 0.0f;
    float WorldY = 0.0f;
    float WorldZ = 0.0f;

    // Rotations in degrees [0, 360)
    float HullYawDegrees = 0.0f;
    float TurretYawDegrees = 0.0f;

    // Health & status
    int32_t HealthCurrent = 0;
    int32_t HealthMax = 0;
    float HealthPercent = 1.0f;
    bool bIsUnderFire = false;
    bool bMoving = false;
};

// Internal sample point per entity for a specific simulation tick.
struct EntitySample
{
    Vec2 Position = Vec2::Zero();
    Vec2 Velocity = Vec2::Zero();
    int32_t HullAngle = 0;
    int32_t TurretAngle = 0;
    int32_t Health = 0;
    int32_t HealthMax = 0;
    ContentId Def;
    EntityKind Kind = EntityKind::Unit;
    PlayerId Owner = kInvalidPlayer;
    uint32_t Generation = 0;
    bool bAlive = false;
    bool bUnderFire = false;
};


// High-performance double-buffered interpolator.
class RA4PRESENTATION_API PresentationInterpolator
{
public:
    PresentationInterpolator() = default;

    // Ingests the current simulation world state. Call this immediately after each SimWorld::Tick.
    void IngestSimTick(const SimWorld& World);

    // Interpolates all active entities at fraction Alpha in [0.0, 1.0] between Previous and Current tick.
    void InterpolateAll(float Alpha, std::vector<InterpolatedEntityState>& OutStates) const;

    // Interpolates a single entity by ID. Returns false if entity is not found or dead.
    bool GetInterpolatedEntity(EntityId Id, float Alpha, InterpolatedEntityState& OutState) const;

    // Clears stored samples.
    void Reset();

    TickIndex GetCurrentSimTick() const { return CurrentSimTick; }
    size_t GetActiveEntityCount() const { return ActiveCount; }

private:
    TickIndex CurrentSimTick = 0;
    size_t ActiveCount = 0;

    std::vector<EntitySample> PrevSamples;
    std::vector<EntitySample> CurrSamples;
};

} // namespace Presentation
} // namespace RA4
