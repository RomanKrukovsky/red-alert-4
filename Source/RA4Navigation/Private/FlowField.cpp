// Copyright (c) Red Alert 4 project.

#include "FlowField.h"
#include <queue>
#include <cmath>

namespace RA4
{
namespace Nav
{

FFlowField::FFlowField(const FNavGrid* InGrid, const FGridCoord& TargetLocation)
    : Grid(InGrid)
    , Target(TargetLocation)
{
    if (Grid)
    {
        Field.resize(Grid->GetWidth() * Grid->GetHeight());
    }
}

void FFlowField::CalculateField()
{
    if (!Grid || !Grid->IsValid(Target.X, Target.Y)) return;

    int32_t W = Grid->GetWidth();
    int32_t H = Grid->GetHeight();

    // 1. Reset field
    for (auto& Cell : Field)
    {
        Cell.IntegrationCost = 0xFFFF;
        Cell.DirX = 0.0f;
        Cell.DirY = 0.0f;
    }

    // 2. Integration field (Dijkstra)
    std::queue<FGridCoord> Frontier;
    
    // Setup target
    int32_t TargetIdx = Target.Y * W + Target.X;
    Field[TargetIdx].IntegrationCost = 0;
    Frontier.push(Target);

    const int32_t DX[] = { 0, 1, 0, -1, 1, 1, -1, -1 };
    const int32_t DY[] = { -1, 0, 1, 0, -1, 1, 1, -1 };
    const uint16_t Dist[] = { 10, 10, 10, 10, 14, 14, 14, 14 }; // 10 * cost

    while (!Frontier.empty())
    {
        FGridCoord Current = Frontier.front();
        Frontier.pop();

        int32_t CurrIdx = Current.Y * W + Current.X;
        uint16_t CurrCost = Field[CurrIdx].IntegrationCost;

        for (int i = 0; i < 8; ++i)
        {
            FGridCoord Neighbor = { Current.X + DX[i], Current.Y + DY[i] };
            if (Grid->IsValid(Neighbor.X, Neighbor.Y))
            {
                const FNavCell& NavCell = Grid->GetCell(Neighbor.X, Neighbor.Y);
                if (NavCell.CostMultiplier == 255) continue; // Impassable

                uint16_t NewCost = CurrCost + Dist[i] * NavCell.CostMultiplier;
                int32_t NeighborIdx = Neighbor.Y * W + Neighbor.X;

                if (NewCost < Field[NeighborIdx].IntegrationCost)
                {
                    Field[NeighborIdx].IntegrationCost = NewCost;
                    Frontier.push(Neighbor);
                }
            }
        }
    }

    // 3. Flow Field Generation
    for (int32_t Y = 0; Y < H; ++Y)
    {
        for (int32_t X = 0; X < W; ++X)
        {
            int32_t Idx = Y * W + X;
            if (Field[Idx].IntegrationCost == 0xFFFF || Field[Idx].IntegrationCost == 0) continue;

            uint16_t MinCost = Field[Idx].IntegrationCost;
            int32_t BestDX = 0;
            int32_t BestDY = 0;

            // Find lowest cost neighbor
            for (int i = 0; i < 8; ++i)
            {
                int32_t NX = X + DX[i];
                int32_t NY = Y + DY[i];
                
                if (Grid->IsValid(NX, NY))
                {
                    int32_t NIdx = NY * W + NX;
                    if (Field[NIdx].IntegrationCost < MinCost)
                    {
                        MinCost = Field[NIdx].IntegrationCost;
                        BestDX = DX[i];
                        BestDY = DY[i];
                    }
                }
            }

            // Normalize vector
            if (BestDX != 0 || BestDY != 0)
            {
                float Length = std::sqrt(BestDX * BestDX + BestDY * BestDY);
                Field[Idx].DirX = static_cast<float>(BestDX) / Length;
                Field[Idx].DirY = static_cast<float>(BestDY) / Length;
            }
        }
    }
}

void FFlowField::GetDirection(int32_t X, int32_t Y, float& OutDirX, float& OutDirY) const
{
    if (Grid && Grid->IsValid(X, Y))
    {
        int32_t Idx = Y * Grid->GetWidth() + X;
        OutDirX = Field[Idx].DirX;
        OutDirY = Field[Idx].DirY;
    }
    else
    {
        OutDirX = 0.0f;
        OutDirY = 0.0f;
    }
}

} // namespace Nav
} // namespace RA4
