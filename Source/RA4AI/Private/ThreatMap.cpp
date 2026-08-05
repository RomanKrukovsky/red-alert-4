// Copyright (c) Red Alert 4 project.
#include "RA4AI/ThreatMap.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Fixed.h"

#include <algorithm>

namespace RA4
{
namespace AI
{

ThreatMap::ThreatMap(int32_t InWidth, int32_t InHeight)
{
    Init(InWidth, InHeight);
}

void ThreatMap::Init(int32_t InWidth, int32_t InHeight)
{
    Width = InWidth;
    Height = InHeight;
    Cells.resize(static_cast<size_t>(Width) * static_cast<size_t>(Height));
    Clear();
}

void ThreatMap::Clear()
{
    for (ThreatCell& C : Cells)
    {
        C = ThreatCell();
    }
}

int32_t ThreatMap::CellIndex(int32_t X, int32_t Y) const
{
    return Y * Width + X;
}

bool ThreatMap::InBounds(int32_t X, int32_t Y) const
{
    return X >= 0 && Y >= 0 && X < Width && Y < Height;
}

const ThreatCell* ThreatMap::GetCell(TileCoord Tile) const
{
    if (!InBounds(Tile.X, Tile.Y))
    {
        return nullptr;
    }
    return &Cells[static_cast<size_t>(CellIndex(Tile.X, Tile.Y))];
}

int32_t ThreatMap::GetThreat(TileCoord Tile) const
{
    const ThreatCell* Cell = GetCell(Tile);
    return Cell != nullptr ? Cell->TotalThreat : 0;
}

int32_t ThreatMap::GetAirThreat(TileCoord Tile) const
{
    const ThreatCell* Cell = GetCell(Tile);
    return Cell != nullptr ? Cell->AirThreat : 0;
}

int32_t ThreatMap::GetAntiArmorThreat(TileCoord Tile) const
{
    const ThreatCell* Cell = GetCell(Tile);
    return Cell != nullptr ? Cell->AntiArmorThreat : 0;
}

int32_t ThreatMap::GetStructuralThreat(TileCoord Tile) const
{
    const ThreatCell* Cell = GetCell(Tile);
    return Cell != nullptr ? Cell->StructuralThreat : 0;
}

int32_t ThreatMap::GetAreaThreat(TileCoord Center, int32_t Radius) const
{
    int32_t Sum = 0;
    for (int32_t DY = -Radius; DY <= Radius; ++DY)
    {
        for (int32_t DX = -Radius; DX <= Radius; ++DX)
        {
            const int32_t X = Center.X + DX;
            const int32_t Y = Center.Y + DY;
            if (InBounds(X, Y))
            {
                Sum += Cells[static_cast<size_t>(CellIndex(X, Y))].TotalThreat;
            }
        }
    }
    return Sum;
}

TileCoord ThreatMap::FindHighestThreatTile(int32_t Radius,
                                           TileCoord Center) const
{
    int32_t BestThreat = 0;
    TileCoord Best(-1, -1);

    const int32_t MinX = (Radius > 0 && Center.X >= 0)
        ? std::max(0, Center.X - Radius) : 0;
    const int32_t MaxX = (Radius > 0 && Center.X >= 0)
        ? std::min(Width - 1, Center.X + Radius) : Width - 1;
    const int32_t MinY = (Radius > 0 && Center.Y >= 0)
        ? std::max(0, Center.Y - Radius) : 0;
    const int32_t MaxY = (Radius > 0 && Center.Y >= 0)
        ? std::min(Height - 1, Center.Y + Radius) : Height - 1;

    for (int32_t Y = MinY; Y <= MaxY; ++Y)
    {
        for (int32_t X = MinX; X <= MaxX; ++X)
        {
            const int32_t Threat = Cells[static_cast<size_t>(CellIndex(X, Y))].TotalThreat;
            if (Threat > BestThreat)
            {
                BestThreat = Threat;
                Best = TileCoord(X, Y);
            }
        }
    }
    return Best;
}

void ThreatMap::AddThreatAtTile(int32_t X, int32_t Y, int32_t Threat,
                                int32_t Air, int32_t AntiArmor, int32_t Structural)
{
    if (!InBounds(X, Y))
    {
        return;
    }
    ThreatCell& C = Cells[static_cast<size_t>(CellIndex(X, Y))];
    C.TotalThreat += Threat;
    C.AirThreat += Air;
    C.AntiArmorThreat += AntiArmor;
    C.StructuralThreat += Structural;
}

void ThreatMap::SpreadThreatFromTile(int32_t CenterX, int32_t CenterY,
                                     int32_t MaxRangeTiles, int32_t BaseThreat,
                                     int32_t Air, int32_t AntiArmor, int32_t Structural)
{
    if (BaseThreat <= 0 || MaxRangeTiles <= 0)
    {
        return;
    }

    // Linear falloff: threat at distance d is BaseThreat * (MaxRange - d) / MaxRange.
    // The source tile gets full threat; at MaxRange tiles away it drops to zero.
    for (int32_t DY = -MaxRangeTiles; DY <= MaxRangeTiles; ++DY)
    {
        for (int32_t DX = -MaxRangeTiles; DX <= MaxRangeTiles; ++DX)
        {
            // Chebyshev distance (tile-grid appropriate, no sqrt needed).
            const int32_t Dist = std::max(std::abs(DX), std::abs(DY));
            if (Dist > MaxRangeTiles)
            {
                continue;
            }

            const int32_t Falloff = MaxRangeTiles - Dist;
            const int32_t ScaledThreat = (BaseThreat * Falloff) / MaxRangeTiles;
            const int32_t ScaledAir = (Air * Falloff) / MaxRangeTiles;
            const int32_t ScaledAntiArmor = (AntiArmor * Falloff) / MaxRangeTiles;
            const int32_t ScaledStructural = (Structural * Falloff) / MaxRangeTiles;

            AddThreatAtTile(CenterX + DX, CenterY + DY,
                            ScaledThreat, ScaledAir, ScaledAntiArmor, ScaledStructural);
        }
    }
}

void ThreatMap::UpdateFromMemory(const std::vector<EnemyMemory>& KnownEnemies,
                                 const ContentDatabase* Content,
                                 const MapDescription& Map,
                                 TickIndex CurrentTick)
{
    Clear();

    if (Content == nullptr || Width == 0 || Height == 0)
    {
        return;
    }

    for (const EnemyMemory& Mem : KnownEnemies)
    {
        const EntityDef* Def = Content->FindEntity(Mem.DefId);
        if (Def == nullptr)
        {
            continue;
        }

        // Skip unarmed entities (harvesters, builders, resource nodes).
        if (!Def->Weapon.IsValid() && !Def->SecondaryWeapon.IsValid())
        {
            continue;
        }

        // Compute confidence factor: 100 = fresh, decays linearly to 10 (0.1 floor).
        const int32_t ConfidencePercent = static_cast<int32_t>((Mem.Confidence * Fixed::FromInt(100)).ToIntRound());

        // Base threat from primary weapon DPS approximation: Damage * 100 / Cooldown.
        int32_t BaseThreat = 0;
        int32_t AirThreat = 0;
        int32_t AntiArmorThreat = 0;
        int32_t StructuralThreat = 0;

        // Range in tiles for threat spread.
        int32_t MaxRangeTiles = 3;  // minimum spread for any armed unit

        const WeaponDef* PrimaryWeapon = Content->FindWeapon(Def->Weapon);
        if (PrimaryWeapon != nullptr && PrimaryWeapon->CooldownTicks > 0)
        {
            const int32_t DPS = (PrimaryWeapon->Damage * 100) / PrimaryWeapon->CooldownTicks;
            BaseThreat += DPS;

            // Convert weapon MaxRange from world units to tiles.
            const int32_t RangeUnits = static_cast<int32_t>(PrimaryWeapon->MaxRange.ToIntFloor());
            const int32_t RangeTiles = std::max(1, RangeUnits / int32_t(MapDescription::kTileSizeUnitsLocal));
            MaxRangeTiles = std::max(MaxRangeTiles, RangeTiles);

            if (PrimaryWeapon->bCanTargetAir)
            {
                AirThreat += DPS;
            }
            if (HasRole(Def->Roles, EntityRole::AntiArmor))
            {
                AntiArmorThreat += DPS;
            }
        }

        // Secondary weapon adds its DPS to the total.
        const WeaponDef* SecondaryWeapon = Content->FindWeapon(Def->SecondaryWeapon);
        if (SecondaryWeapon != nullptr && SecondaryWeapon->CooldownTicks > 0)
        {
            const int32_t DPS = (SecondaryWeapon->Damage * 100) / SecondaryWeapon->CooldownTicks;
            BaseThreat += DPS;

            const int32_t RangeUnits = static_cast<int32_t>(SecondaryWeapon->MaxRange.ToIntFloor());
            const int32_t RangeTiles = std::max(1, RangeUnits / int32_t(MapDescription::kTileSizeUnitsLocal));
            MaxRangeTiles = std::max(MaxRangeTiles, RangeTiles);

            if (SecondaryWeapon->bCanTargetAir)
            {
                AirThreat += DPS;
            }
            if (HasRole(Def->Roles, EntityRole::AntiArmor))
            {
                AntiArmorThreat += DPS;
            }
        }

        // Defensive buildings get structural threat tag.
        if (Def->Kind == EntityKind::Building && HasRole(Def->Roles, EntityRole::Defense))
        {
            StructuralThreat = BaseThreat;
        }

        // Apply confidence scaling: fresh sighting = full threat, stale = reduced.
        const int32_t ScaledBase = (BaseThreat * ConfidencePercent) / 100;
        const int32_t ScaledAir = (AirThreat * ConfidencePercent) / 100;
        const int32_t ScaledAntiArmor = (AntiArmorThreat * ConfidencePercent) / 100;
        const int32_t ScaledStructural = (StructuralThreat * ConfidencePercent) / 100;

        // Clamp tile position to map bounds.
        const int32_t TileX = std::max(0, std::min(Mem.Position.X, Width - 1));
        const int32_t TileY = std::max(0, std::min(Mem.Position.Y, Height - 1));

        SpreadThreatFromTile(TileX, TileY, MaxRangeTiles,
                             ScaledBase, ScaledAir, ScaledAntiArmor, ScaledStructural);

        // Stamp the update tick on affected cells.
        for (ThreatCell& C : Cells)
        {
            if (C.TotalThreat > 0)
            {
                C.LastUpdatedTick = CurrentTick;
            }
        }
    }
}

} // namespace AI
} // namespace RA4
