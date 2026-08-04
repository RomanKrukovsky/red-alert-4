// Copyright (c) Red Alert 4 project.
#include "RA4AI/ValueMap.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Fixed.h"
#include "RA4Simulation/SimWorld.h"

#include <algorithm>
#include <cmath>

namespace RA4
{
namespace AI
{

ValueMap::ValueMap(int32_t InWidth, int32_t InHeight)
{
    Init(InWidth, InHeight);
}

void ValueMap::Init(int32_t InWidth, int32_t InHeight)
{
    Width = InWidth;
    Height = InHeight;
    Cells.resize(static_cast<size_t>(Width) * static_cast<size_t>(Height));
    Clear();
}

void ValueMap::Clear()
{
    for (ValueCell& C : Cells)
    {
        C = ValueCell();
    }
}

int32_t ValueMap::CellIndex(int32_t X, int32_t Y) const
{
    return Y * Width + X;
}

bool ValueMap::InBounds(int32_t X, int32_t Y) const
{
    return X >= 0 && Y >= 0 && X < Width && Y < Height;
}

const ValueCell* ValueMap::GetCell(TileCoord Tile) const
{
    if (!InBounds(Tile.X, Tile.Y))
    {
        return nullptr;
    }
    return &Cells[static_cast<size_t>(CellIndex(Tile.X, Tile.Y))];
}

int32_t ValueMap::GetStrategicValue(TileCoord Tile) const
{
    const ValueCell* Cell = GetCell(Tile);
    return Cell != nullptr ? Cell->StrategicValue : 0;
}

int32_t ValueMap::GetEconomicValue(TileCoord Tile) const
{
    const ValueCell* Cell = GetCell(Tile);
    return Cell != nullptr ? Cell->EconomicValue : 0;
}

int32_t ValueMap::GetMilitaryValue(TileCoord Tile) const
{
    const ValueCell* Cell = GetCell(Tile);
    return Cell != nullptr ? Cell->MilitaryValue : 0;
}

void ValueMap::AddValueAtTile(int32_t X, int32_t Y, int32_t Value,
                              int32_t Econ, int32_t Mil)
{
    if (!InBounds(X, Y))
    {
        return;
    }
    ValueCell& C = Cells[static_cast<size_t>(CellIndex(X, Y))];
    C.StrategicValue += Value;
    C.EconomicValue += Econ;
    C.MilitaryValue += Mil;
}

void ValueMap::SpreadValueFromTile(int32_t CenterX, int32_t CenterY,
                                   int32_t MaxRangeTiles, int32_t Value,
                                   int32_t Econ, int32_t Mil)
{
    if (Value <= 0 || MaxRangeTiles <= 0)
    {
        return;
    }

    for (int32_t DY = -MaxRangeTiles; DY <= MaxRangeTiles; ++DY)
    {
        for (int32_t DX = -MaxRangeTiles; DX <= MaxRangeTiles; ++DX)
        {
            const int32_t Dist = std::max(std::abs(DX), std::abs(DY));
            if (Dist > MaxRangeTiles)
            {
                continue;
            }

            // Linear falloff: full value at source, zero at MaxRange.
            const int32_t Falloff = MaxRangeTiles - Dist;
            const int32_t ScaledValue = (Value * Falloff) / MaxRangeTiles;
            const int32_t ScaledEcon = (Econ * Falloff) / MaxRangeTiles;
            const int32_t ScaledMil = (Mil * Falloff) / MaxRangeTiles;

            AddValueAtTile(CenterX + DX, CenterY + DY,
                           ScaledValue, ScaledEcon, ScaledMil);
        }
    }
}

TileCoord ValueMap::FindHighestValueTarget(int32_t MinValue) const
{
    int32_t BestValue = MinValue;
    TileCoord Best(-1, -1);

    for (int32_t Y = 0; Y < Height; ++Y)
    {
        for (int32_t X = 0; X < Width; ++X)
        {
            const ValueCell& C = Cells[static_cast<size_t>(CellIndex(X, Y))];
            if (C.StrategicValue > BestValue)
            {
                BestValue = C.StrategicValue;
                Best = TileCoord(X, Y);
            }
        }
    }
    return Best;
}

TileCoord ValueMap::FindBestAttackTarget(const ThreatMap& Threats) const
{
    int32_t BestScore = 0;
    TileCoord Best(-1, -1);

    for (int32_t Y = 0; Y < Height; ++Y)
    {
        for (int32_t X = 0; X < Width; ++X)
        {
            const ValueCell& VC = Cells[static_cast<size_t>(CellIndex(X, Y))];
            if (VC.StrategicValue <= 0)
            {
                continue;
            }

            // Score = Value - Threat * 0.3. High-value, low-threat targets win.
            // The divisor (3) keeps the balance: a moderate threat can be worth
            // attacking if the value is high enough, but a heavily defended tile
            // is deprioritized.
            const int32_t ThreatAtTile = Threats.GetThreat(TileCoord(X, Y));
            const int32_t Score = VC.StrategicValue - (ThreatAtTile * 3) / 10;

            if (Score > BestScore)
            {
                BestScore = Score;
                Best = TileCoord(X, Y);
            }
        }
    }
    return Best;
}

void ValueMap::UpdateFromWorld(const SimWorld& World, PlayerId Player,
                               const std::vector<EnemyMemory>& KnownEnemies,
                               const ContentDatabase* Content,
                               TickIndex CurrentTick)
{
    Clear();

    if (Content == nullptr || Width == 0 || Height == 0)
    {
        return;
    }

    // --- Phase 1: Value from the player's own structures and positions ---
    // These are always known (no fog needed for own units).
    const Vec2 OwnBaseCenter = [&]() -> Vec2
    {
        // Find the player's construction yard for proximity calculations.
        const std::vector<EntityCore>& Cores = World.GetAllCores();
        for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
        {
            if (!Cores[I].bAlive || Cores[I].Owner != Player)
            {
                continue;
            }
            const EntityDef* Def = Content->FindEntity(Cores[I].Def);
            if (Def != nullptr && Def->Building.bIsConstructionYard)
            {
                const TransformComp* T = World.GetTransform(World.MakeId(I));
                if (T != nullptr)
                {
                    return T->Position;
                }
            }
        }
        return Vec2::Zero();
    }();

    const TileCoord OwnBaseTile = World.GetMap().WorldToTile(OwnBaseCenter);

    // --- Phase 2: Value from enemy structures/units (fog-limited) ---
    for (const EnemyMemory& Mem : KnownEnemies)
    {
        const EntityDef* Def = Content->FindEntity(Mem.DefId);
        if (Def == nullptr)
        {
            continue;
        }

        const int32_t ConfidencePercent = static_cast<int32_t>((Mem.Confidence * Fixed::FromInt(100)).ToIntRound());
        const int32_t TileX = std::max(0, std::min(Mem.Position.X, Width - 1));
        const int32_t TileY = std::max(0, std::min(Mem.Position.Y, Height - 1));

        int32_t Value = 0;
        int32_t Econ = 0;
        int32_t Mil = 0;

        if (Def->Kind == EntityKind::Building)
        {
            // Construction yard: highest strategic value (kills the base).
            if (Def->Building.bIsConstructionYard)
            {
                Value = 5000;
                Mil = 5000;
            }
            // Refinery: economic value (harvesters unload here).
            else if (Def->Building.bIsRefinery || HasRole(Def->Roles, EntityRole::Refinery))
            {
                Value = 2000;
                Econ = 2000;
            }
            // Power plant: medium value (cutting power cripples everything).
            else if (Def->Building.bIsPowerPlant || HasRole(Def->Roles, EntityRole::Power))
            {
                Value = 1000;
                Mil = 1000;
            }
            // Production building: military value.
            else if (HasRole(Def->Roles, EntityRole::Production))
            {
                Value = 1500;
                Mil = 1500;
            }
            // Defence building: moderate value (removing it opens the base).
            else if (HasRole(Def->Roles, EntityRole::Defense))
            {
                Value = 800;
                Mil = 800;
            }
            else
            {
                // Generic building: low but non-zero value.
                Value = 300;
                Mil = 300;
            }
        }
        else if (Def->Kind == EntityKind::Unit)
        {
            if (HasRole(Def->Roles, EntityRole::Harvester))
            {
                // Harvester: economic value (kills income).
                Value = 1500;
                Econ = 1500;
            }
            else if (HasRole(Def->Roles, EntityRole::Combat))
            {
                // Combat unit: low individual value but worth knowing about.
                Value = 200;
                Mil = 200;
            }
        }

        if (Value <= 0)
        {
            continue;
        }

        const int32_t ScaledValue = (Value * ConfidencePercent) / 100;
        const int32_t ScaledEcon = (Econ * ConfidencePercent) / 100;
        const int32_t ScaledMil = (Mil * ConfidencePercent) / 100;

        // Spread value within a small radius (buildings are bigger targets).
        const int32_t SpreadRadius = (Def->Kind == EntityKind::Building) ? 2 : 1;
        SpreadValueFromTile(TileX, TileY, SpreadRadius,
                            ScaledValue, ScaledEcon, ScaledMil);
    }

    // --- Phase 3: Proximity bonus ---
    // Tiles closer to own base get a small bonus for defensive value.
    if (OwnBaseTile.X >= 0 && OwnBaseTile.Y >= 0)
    {
        const int32_t ProximityRadius = 15;  // 15 tiles around own base
        for (int32_t DY = -ProximityRadius; DY <= ProximityRadius; ++DY)
        {
            for (int32_t DX = -ProximityRadius; DX <= ProximityRadius; ++DX)
            {
                const int32_t X = OwnBaseTile.X + DX;
                const int32_t Y = OwnBaseTile.Y + DY;
                if (!InBounds(X, Y))
                {
                    continue;
                }
                const int32_t Dist = std::max(std::abs(DX), std::abs(DY));
                // Small bonus that decays with distance.
                const int32_t Bonus = (ProximityRadius - Dist) * 2;
                ValueCell& C = Cells[static_cast<size_t>(CellIndex(X, Y))];
                C.StrategicValue += Bonus;
            }
        }
    }

    // Stamp update tick.
    for (ValueCell& C : Cells)
    {
        if (C.StrategicValue > 0)
        {
            C.LastUpdatedTick = CurrentTick;
        }
    }
}

} // namespace AI
} // namespace RA4
