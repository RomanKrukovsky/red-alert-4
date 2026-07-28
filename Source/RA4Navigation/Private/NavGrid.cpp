// Copyright (c) Red Alert 4 project.

#include "NavGrid.h"
#include <algorithm>

namespace RA4
{
namespace Nav
{

FNavGrid::FNavGrid(int32_t InWidth, int32_t InHeight)
    : Width(InWidth)
    , Height(InHeight)
{
    Cells.resize(Width * Height);
}

void FNavGrid::Initialize(const std::vector<uint8_t>& InImpassableMask)
{
    // Initialize cells from mask
    for (int32_t Y = 0; Y < Height; ++Y)
    {
        for (int32_t X = 0; X < Width; ++X)
        {
            FNavCell& Cell = GetCellMutable(X, Y);
            int32_t Index = Y * Width + X;
            
            if (Index < InImpassableMask.size() && InImpassableMask[Index] > 0)
            {
                Cell.CostMultiplier = 255;
                Cell.LayerMask = 0; // Impassable
            }
            else
            {
                Cell.CostMultiplier = 1;
                Cell.LayerMask = 0xFF; // Passable
            }
        }
    }

    BuildSectors();
    BuildPortals();
}

const FNavCell& FNavGrid::GetCell(int32_t X, int32_t Y) const
{
    // Note: Assuming coordinates are validated by caller or bounded
    return Cells[Y * Width + X];
}

FNavCell& FNavGrid::GetCellMutable(int32_t X, int32_t Y)
{
    return Cells[Y * Width + X];
}

bool FNavGrid::IsValid(int32_t X, int32_t Y) const
{
    return X >= 0 && X < Width && Y >= 0 && Y < Height;
}

void FNavGrid::BuildSectors()
{
    int32_t NumSectorsX = (Width + kSectorSize - 1) / kSectorSize;
    int32_t NumSectorsY = (Height + kSectorSize - 1) / kSectorSize;

    Sectors.clear();
    Sectors.reserve(NumSectorsX * NumSectorsY);

    for (int32_t SY = 0; SY < NumSectorsY; ++SY)
    {
        for (int32_t SX = 0; SX < NumSectorsX; ++SX)
        {
            FSector Sector;
            Sector.Id = static_cast<uint16_t>(Sectors.size());
            Sector.MinBounds.X = SX * kSectorSize;
            Sector.MinBounds.Y = SY * kSectorSize;
            Sector.MaxBounds.X = std::min((SX + 1) * kSectorSize - 1, Width - 1);
            Sector.MaxBounds.Y = std::min((SY + 1) * kSectorSize - 1, Height - 1);
            
            Sectors.push_back(Sector);

            // Assign cells to this sector
            for (int32_t Y = Sector.MinBounds.Y; Y <= Sector.MaxBounds.Y; ++Y)
            {
                for (int32_t X = Sector.MinBounds.X; X <= Sector.MaxBounds.X; ++X)
                {
                    GetCellMutable(X, Y).SectorId = Sector.Id;
                }
            }
        }
    }
}

void FNavGrid::BuildPortals()
{
    // Dummy implementation for portal extraction logic across sector boundaries
    // A complete implementation would identify contiguous open segments between adjacent sectors
    Portals.clear();
}

} // namespace Nav
} // namespace RA4
