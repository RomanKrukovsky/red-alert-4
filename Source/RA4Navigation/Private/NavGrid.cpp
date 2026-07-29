// Copyright (c) Red Alert 4 project.
#include "RA4Navigation/NavGrid.h"

#include <algorithm>
#include <limits>

namespace RA4
{
namespace Nav
{
namespace
{
const NavCell GInvalidCell = {NavLayer_None, 255, 0};

uint8_t IncrementClearance(uint8_t Value)
{
    return Value == std::numeric_limits<uint8_t>::max() ? Value : uint8_t(Value + 1u);
}
} // namespace

NavGrid::NavGrid(int32_t InWidth, int32_t InHeight)
    : Width(std::max(0, InWidth))
    , Height(std::max(0, InHeight))
    , Cells(size_t(Width) * size_t(Height))
{
    RebuildDerivedData();
}

bool NavGrid::IsInBounds(const TileCoord& Tile) const
{
    return Tile.X >= 0 && Tile.Y >= 0 && Tile.X < Width && Tile.Y < Height;
}

const NavCell& NavGrid::GetCell(const TileCoord& Tile) const
{
    return IsInBounds(Tile) ? Cells[size_t(ToIndex(Tile))] : GInvalidCell;
}

bool NavGrid::IsTraversable(const TileCoord& Tile, const NavQuery& Query) const
{
    const NavCell& Cell = GetCell(Tile);
    return Query.LayerMask != NavLayer_None && (Cell.PassabilityMask & Query.LayerMask) != 0 &&
           Cell.MovementCost > 0 && Cell.Clearance >= Query.RequiredClearance;
}

bool NavGrid::SetPassability(const TileCoord& Tile, uint8_t PassabilityMask)
{
    if (!IsInBounds(Tile))
    {
        return false;
    }

    NavCell& Cell = Cells[size_t(ToIndex(Tile))];
    if (Cell.PassabilityMask == PassabilityMask)
    {
        return false;
    }

    Cell.PassabilityMask = PassabilityMask;
    MarkTopologyDirty();
    return true;
}

bool NavGrid::SetMovementCost(const TileCoord& Tile, uint8_t MovementCost)
{
    if (!IsInBounds(Tile) || MovementCost == 0)
    {
        return false;
    }

    NavCell& Cell = Cells[size_t(ToIndex(Tile))];
    if (Cell.MovementCost == MovementCost)
    {
        return false;
    }

    Cell.MovementCost = MovementCost;
    MarkTopologyDirty();
    return true;
}

void NavGrid::BeginTopologyUpdate()
{
    ++UpdateDepth;
}

bool NavGrid::EndTopologyUpdate()
{
    if (UpdateDepth == 0)
    {
        return false;
    }

    --UpdateDepth;
    if (UpdateDepth == 0 && bTopologyDirty)
    {
        RebuildDerivedData();
        return true;
    }
    return false;
}

int32_t NavGrid::ToIndex(const TileCoord& Tile) const
{
    return Tile.Y * Width + Tile.X;
}

void NavGrid::MarkTopologyDirty()
{
    bTopologyDirty = true;
    if (UpdateDepth == 0)
    {
        RebuildDerivedData();
    }
}

void NavGrid::RebuildDerivedData()
{
    RebuildClearance();
    RebuildSectorsAndPortals();
    bTopologyDirty = false;
    ++TopologyRevision;
}

void NavGrid::RebuildClearance()
{
    if (Cells.empty())
    {
        return;
    }

    for (int32_t Y = 0; Y < Height; ++Y)
    {
        for (int32_t X = 0; X < Width; ++X)
        {
            const TileCoord Tile(X, Y);
            NavCell& Cell = Cells[size_t(ToIndex(Tile))];
            if (Cell.PassabilityMask == NavLayer_None)
            {
                Cell.Clearance = 0;
                continue;
            }

            const int32_t DistanceToOutside = std::min(std::min(X + 1, Y + 1),
                                                        std::min(Width - X, Height - Y));
            Cell.Clearance = static_cast<uint8_t>(std::min(DistanceToOutside, 255));
        }
    }

    for (int32_t Y = 0; Y < Height; ++Y)
    {
        for (int32_t X = 0; X < Width; ++X)
        {
            NavCell& Cell = Cells[size_t(ToIndex(TileCoord(X, Y)))];
            if (Cell.Clearance == 0)
            {
                continue;
            }

            if (X > 0)
            {
                Cell.Clearance = std::min(Cell.Clearance, IncrementClearance(GetCell(TileCoord(X - 1, Y)).Clearance));
            }
            if (Y > 0)
            {
                Cell.Clearance = std::min(Cell.Clearance, IncrementClearance(GetCell(TileCoord(X, Y - 1)).Clearance));
            }
            if (X > 0 && Y > 0)
            {
                Cell.Clearance = std::min(Cell.Clearance, IncrementClearance(GetCell(TileCoord(X - 1, Y - 1)).Clearance));
            }
            if (X + 1 < Width && Y > 0)
            {
                Cell.Clearance = std::min(Cell.Clearance, IncrementClearance(GetCell(TileCoord(X + 1, Y - 1)).Clearance));
            }
        }
    }

    for (int32_t Y = Height - 1; Y >= 0; --Y)
    {
        for (int32_t X = Width - 1; X >= 0; --X)
        {
            NavCell& Cell = Cells[size_t(ToIndex(TileCoord(X, Y)))];
            if (Cell.Clearance == 0)
            {
                continue;
            }

            if (X + 1 < Width)
            {
                Cell.Clearance = std::min(Cell.Clearance, IncrementClearance(GetCell(TileCoord(X + 1, Y)).Clearance));
            }
            if (Y + 1 < Height)
            {
                Cell.Clearance = std::min(Cell.Clearance, IncrementClearance(GetCell(TileCoord(X, Y + 1)).Clearance));
            }
            if (X + 1 < Width && Y + 1 < Height)
            {
                Cell.Clearance = std::min(Cell.Clearance, IncrementClearance(GetCell(TileCoord(X + 1, Y + 1)).Clearance));
            }
            if (X > 0 && Y + 1 < Height)
            {
                Cell.Clearance = std::min(Cell.Clearance, IncrementClearance(GetCell(TileCoord(X - 1, Y + 1)).Clearance));
            }
        }
    }
}

void NavGrid::RebuildSectorsAndPortals()
{
    Sectors.clear();
    Portals.clear();
    if (Width == 0 || Height == 0)
    {
        return;
    }

    const int32_t SectorCountX = (Width + kSectorSize - 1) / kSectorSize;
    const int32_t SectorCountY = (Height + kSectorSize - 1) / kSectorSize;
    Sectors.reserve(size_t(SectorCountX) * size_t(SectorCountY));

    for (int32_t SectorY = 0; SectorY < SectorCountY; ++SectorY)
    {
        for (int32_t SectorX = 0; SectorX < SectorCountX; ++SectorX)
        {
            NavSector Sector;
            Sector.Id = static_cast<uint16_t>(Sectors.size());
            Sector.Min = TileCoord(SectorX * kSectorSize, SectorY * kSectorSize);
            Sector.Max = TileCoord(std::min(Sector.Min.X + kSectorSize - 1, Width - 1),
                                   std::min(Sector.Min.Y + kSectorSize - 1, Height - 1));
            Sectors.push_back(Sector);
        }
    }

    for (int32_t SectorY = 0; SectorY < SectorCountY; ++SectorY)
    {
        for (int32_t SectorX = 0; SectorX < SectorCountX; ++SectorX)
        {
            const uint16_t SectorId = static_cast<uint16_t>(SectorY * SectorCountX + SectorX);
            const NavSector& Sector = Sectors[size_t(SectorId)];
            if (SectorX + 1 < SectorCountX)
            {
                AppendBoundaryPortals(SectorId, static_cast<uint16_t>(SectorId + 1u),
                                      TileCoord(Sector.Max.X, Sector.Min.Y), TileCoord(0, 1),
                                      TileCoord(1, 0), Sector.Max.Y - Sector.Min.Y + 1);
            }
            if (SectorY + 1 < SectorCountY)
            {
                AppendBoundaryPortals(SectorId, static_cast<uint16_t>(SectorId + SectorCountX),
                                      TileCoord(Sector.Min.X, Sector.Max.Y), TileCoord(1, 0),
                                      TileCoord(0, 1), Sector.Max.X - Sector.Min.X + 1);
            }
        }
    }
}

void NavGrid::AppendBoundaryPortals(uint16_t SectorA, uint16_t SectorB, const TileCoord& StartA,
                                    const TileCoord& Step, const TileCoord& OffsetToB, int32_t Length)
{
    int32_t RunStart = -1;
    uint8_t RunMask = NavLayer_None;
    for (int32_t I = 0; I <= Length; ++I)
    {
        uint8_t Mask = NavLayer_None;
        if (I < Length)
        {
            const TileCoord A(StartA.X + Step.X * I, StartA.Y + Step.Y * I);
            const TileCoord B(A.X + OffsetToB.X, A.Y + OffsetToB.Y);
            Mask = uint8_t(GetCell(A).PassabilityMask & GetCell(B).PassabilityMask);
        }

        if (Mask != NavLayer_None && Mask == RunMask)
        {
            continue;
        }

        if (RunStart >= 0)
        {
            const int32_t RunEnd = I - 1;
            const TileCoord AStart(StartA.X + Step.X * RunStart, StartA.Y + Step.Y * RunStart);
            const TileCoord AEnd(StartA.X + Step.X * RunEnd, StartA.Y + Step.Y * RunEnd);
            NavPortal Portal;
            Portal.Id = static_cast<uint16_t>(Portals.size());
            Portal.SectorA = SectorA;
            Portal.SectorB = SectorB;
            Portal.StartA = AStart;
            Portal.EndA = AEnd;
            Portal.StartB = TileCoord(AStart.X + OffsetToB.X, AStart.Y + OffsetToB.Y);
            Portal.EndB = TileCoord(AEnd.X + OffsetToB.X, AEnd.Y + OffsetToB.Y);
            Portal.PassabilityMask = RunMask;
            Portals.push_back(Portal);
            RunStart = -1;
            RunMask = NavLayer_None;
        }

        if (Mask != NavLayer_None)
        {
            RunStart = I;
            RunMask = Mask;
        }
    }
}

} // namespace Nav
} // namespace RA4
