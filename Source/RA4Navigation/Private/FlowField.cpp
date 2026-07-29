// Copyright (c) Red Alert 4 project.
#include "RA4Navigation/FlowField.h"

#include <limits>
#include <queue>

namespace RA4
{
namespace Nav
{
namespace
{
struct QueueEntry
{
    uint32_t Cost = 0;
    int32_t Index = 0;
};

struct QueueEntryGreater
{
    bool operator()(const QueueEntry& A, const QueueEntry& B) const
    {
        return A.Cost != B.Cost ? A.Cost > B.Cost : A.Index > B.Index;
    }
};

constexpr int8_t GDirections[8][2] = {
    {0, -1}, {1, 0}, {0, 1}, {-1, 0}, {1, -1}, {1, 1}, {-1, 1}, {-1, -1},
};
constexpr uint32_t GStepCosts[8] = {10u, 10u, 10u, 10u, 14u, 14u, 14u, 14u};
} // namespace

FlowField::FlowField(const NavGrid& InGrid, const NavQuery& InQuery, const TileCoord& InTarget)
    : Grid(InGrid)
    , Query(InQuery)
    , Target(InTarget)
    , IntegrationCosts(size_t(InGrid.GetWidth()) * size_t(InGrid.GetHeight()), kUnreachableCost)
    , Directions(size_t(InGrid.GetWidth()) * size_t(InGrid.GetHeight()))
{
}

void FlowField::Rebuild()
{
    std::fill(IntegrationCosts.begin(), IntegrationCosts.end(), kUnreachableCost);
    std::fill(Directions.begin(), Directions.end(), FlowDirection{});
    BuiltTopologyRevision = Grid.GetTopologyRevision();

    if (!Grid.IsTraversable(Target, Query))
    {
        return;
    }

    const int32_t TargetIndex = ToIndex(Target);
    IntegrationCosts[size_t(TargetIndex)] = 0;
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, QueueEntryGreater> Open;
    Open.push(QueueEntry{0, TargetIndex});

    while (!Open.empty())
    {
        const QueueEntry Current = Open.top();
        Open.pop();
        if (Current.Cost != IntegrationCosts[size_t(Current.Index)])
        {
            continue;
        }

        const TileCoord CurrentTile(Current.Index % Grid.GetWidth(), Current.Index / Grid.GetWidth());
        const uint32_t TerrainCost = static_cast<uint32_t>(Grid.GetCell(CurrentTile).MovementCost);
        for (int32_t DirectionIndex = 0; DirectionIndex < 8; ++DirectionIndex)
        {
            const TileCoord Previous(CurrentTile.X + GDirections[DirectionIndex][0],
                                     CurrentTile.Y + GDirections[DirectionIndex][1]);
            if (!CanStep(Previous, CurrentTile))
            {
                continue;
            }

            const uint32_t StepCost = GStepCosts[DirectionIndex] * TerrainCost;
            if (Current.Cost > std::numeric_limits<uint32_t>::max() - StepCost)
            {
                continue;
            }

            const uint32_t NewCost = Current.Cost + StepCost;
            const int32_t PreviousIndex = ToIndex(Previous);
            if (NewCost < IntegrationCosts[size_t(PreviousIndex)])
            {
                IntegrationCosts[size_t(PreviousIndex)] = NewCost;
                Open.push(QueueEntry{NewCost, PreviousIndex});
            }
        }
    }

    for (int32_t Y = 0; Y < Grid.GetHeight(); ++Y)
    {
        for (int32_t X = 0; X < Grid.GetWidth(); ++X)
        {
            const TileCoord Tile(X, Y);
            const int32_t Index = ToIndex(Tile);
            if (IntegrationCosts[size_t(Index)] == kUnreachableCost || Tile == Target)
            {
                continue;
            }

            uint32_t BestCost = IntegrationCosts[size_t(Index)];
            int32_t BestIndex = std::numeric_limits<int32_t>::max();
            FlowDirection BestDirection;
            for (int32_t DirectionIndex = 0; DirectionIndex < 8; ++DirectionIndex)
            {
                const TileCoord Next(Tile.X + GDirections[DirectionIndex][0], Tile.Y + GDirections[DirectionIndex][1]);
                if (!CanStep(Tile, Next))
                {
                    continue;
                }

                const int32_t NextIndex = ToIndex(Next);
                const uint32_t NextCost = IntegrationCosts[size_t(NextIndex)];
                if (NextCost < BestCost || (NextCost == BestCost && NextIndex < BestIndex))
                {
                    BestCost = NextCost;
                    BestIndex = NextIndex;
                    BestDirection = FlowDirection{GDirections[DirectionIndex][0], GDirections[DirectionIndex][1]};
                }
            }
            Directions[size_t(Index)] = BestDirection;
        }
    }
}

bool FlowField::IsReachable(const TileCoord& Tile) const
{
    return GetIntegrationCost(Tile) != kUnreachableCost;
}

uint32_t FlowField::GetIntegrationCost(const TileCoord& Tile) const
{
    return Grid.IsInBounds(Tile) ? IntegrationCosts[size_t(ToIndex(Tile))] : kUnreachableCost;
}

FlowDirection FlowField::GetDirection(const TileCoord& Tile) const
{
    return Grid.IsInBounds(Tile) ? Directions[size_t(ToIndex(Tile))] : FlowDirection{};
}

bool FlowField::CanStep(const TileCoord& From, const TileCoord& To) const
{
    if (!Grid.IsTraversable(From, Query) || !Grid.IsTraversable(To, Query))
    {
        return false;
    }

    const int32_t DeltaX = To.X - From.X;
    const int32_t DeltaY = To.Y - From.Y;
    if ((DeltaX == 0 && DeltaY == 0) || DeltaX < -1 || DeltaX > 1 || DeltaY < -1 || DeltaY > 1)
    {
        return false;
    }

    if (DeltaX != 0 && DeltaY != 0)
    {
        return Grid.IsTraversable(TileCoord(From.X + DeltaX, From.Y), Query) &&
               Grid.IsTraversable(TileCoord(From.X, From.Y + DeltaY), Query);
    }
    return true;
}

int32_t FlowField::ToIndex(const TileCoord& Tile) const
{
    return Tile.Y * Grid.GetWidth() + Tile.X;
}

} // namespace Nav
} // namespace RA4
