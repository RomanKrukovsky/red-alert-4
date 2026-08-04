// Copyright (c) Red Alert 4 project. Probabilistic Fog-of-War Belief State Grid.
#pragma once

#include <cstdint>
#include <vector>
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

enum class BeliefStateLevel : uint8_t
{
    Unknown = 0,
    Stale = 1,
    Probable = 2,
    Exact = 3
};

struct ObservedEntityMemory
{
    uint32_t EntityIndex = 0;
    ContentId TypeId;
    PlayerId Owner = 255;
    Vec2 LastPosition;
    Vec2 LastVelocity;
    TickIndex ObservationTick = 0;
    float Confidence = 0.0f; // 0.0 to 1.0
    BeliefStateLevel Level = BeliefStateLevel::Unknown;
};

class RA4AI_API AIBeliefGrid
{
public:
    void Initialize(int32_t InWidth, int32_t InHeight);
    void Update(const SimWorld& World, PlayerId ViewingPlayer);

    BeliefStateLevel GetTileBelief(int32_t X, int32_t Y) const;
    float GetTileConfidence(int32_t X, int32_t Y) const;

    const std::vector<ObservedEntityMemory>& GetObservedEntities() const { return ObservedEntities; }
    int32_t GetKnownEnemyUnitCount() const;
    int32_t GetKnownEnemyBuildingCount() const;

    // Derived intelligence: e.g. 0.0 to 1.0 probability that enemy has Air Tech
    float EstimateEnemyAirTechProbability() const;
    Vec2 GetSuspectedEnemyBaseCenter() const;

private:
    int32_t Width = 0;
    int32_t Height = 0;
    std::vector<float> ConfidenceGrid; // 1 per tile
    std::vector<ObservedEntityMemory> ObservedEntities;
};

} // namespace AI
} // namespace RA4
