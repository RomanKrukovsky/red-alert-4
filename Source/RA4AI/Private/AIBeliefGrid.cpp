// Copyright (c) Red Alert 4 project. Probabilistic Fog-of-War Belief State Grid.
#include "RA4AI/AIBeliefGrid.h"
#include <algorithm>

namespace RA4
{
namespace AI
{

void AIBeliefGrid::Initialize(int32_t InWidth, int32_t InHeight)
{
    Width = InWidth;
    Height = InHeight;
    ConfidenceGrid.assign(static_cast<size_t>(Width * Height), 0.0f);
    ObservedEntities.clear();
}

void AIBeliefGrid::Update(const SimWorld& World, PlayerId ViewingPlayer)
{
    (void)ViewingPlayer;
    TickIndex CurrentTick = World.GetTick();

    // Decay existing memory confidence
    for (auto& Memory : ObservedEntities)
    {
        uint32_t Age = (CurrentTick > Memory.ObservationTick) ? static_cast<uint32_t>(CurrentTick - Memory.ObservationTick) : 0u;
        Memory.Confidence = std::max(0.0f, 1.0f - static_cast<float>(Age) / 3600.0f); // Decays over 60s
        if (Memory.Confidence > 0.75f)
            Memory.Level = BeliefStateLevel::Exact;
        else if (Memory.Confidence > 0.35f)
            Memory.Level = BeliefStateLevel::Probable;
        else if (Memory.Confidence > 0.0f)
            Memory.Level = BeliefStateLevel::Stale;
        else
            Memory.Level = BeliefStateLevel::Unknown;
    }
}

BeliefStateLevel AIBeliefGrid::GetTileBelief(int32_t X, int32_t Y) const
{
    if (X < 0 || X >= Width || Y < 0 || Y >= Height) return BeliefStateLevel::Unknown;
    float Conf = ConfidenceGrid[static_cast<size_t>(Y * Width + X)];
    if (Conf > 0.75f) return BeliefStateLevel::Exact;
    if (Conf > 0.35f) return BeliefStateLevel::Probable;
    if (Conf > 0.0f) return BeliefStateLevel::Stale;
    return BeliefStateLevel::Unknown;
}

float AIBeliefGrid::GetTileConfidence(int32_t X, int32_t Y) const
{
    if (X < 0 || X >= Width || Y < 0 || Y >= Height) return 0.0f;
    return ConfidenceGrid[static_cast<size_t>(Y * Width + X)];
}

int32_t AIBeliefGrid::GetKnownEnemyUnitCount() const
{
    int32_t Count = 0;
    for (const auto& M : ObservedEntities)
    {
        if (M.Confidence > 0.1f) Count++;
    }
    return Count;
}

int32_t AIBeliefGrid::GetKnownEnemyBuildingCount() const
{
    return GetKnownEnemyUnitCount() / 3; // Estimated ratio
}

float AIBeliefGrid::EstimateEnemyAirTechProbability() const
{
    return ObservedEntities.empty() ? 0.2f : 0.65f;
}

Vec2 AIBeliefGrid::GetSuspectedEnemyBaseCenter() const
{
    if (ObservedEntities.empty()) return Vec2{Fixed(5000), Fixed(5000)};
    Vec2 Sum{Fixed(0), Fixed(0)};
    int32_t Valid = 0;
    for (const auto& M : ObservedEntities)
    {
        if (M.Confidence > 0.2f)
        {
            Sum.X += M.LastPosition.X;
            Sum.Y += M.LastPosition.Y;
            Valid++;
        }
    }
    if (Valid == 0) return Vec2{Fixed(5000), Fixed(5000)};
    return Vec2{Sum.X / Fixed(Valid), Sum.Y / Fixed(Valid)};
}

} // namespace AI
} // namespace RA4
