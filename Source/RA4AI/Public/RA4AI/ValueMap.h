// Copyright (c) Red Alert 4 project. Per-tile strategic value assessment grid.
//
// The ValueMap is an engine-free, deterministic spatial model of how valuable
// each tile on the map is from the owning player's perspective. It combines
// economic value (resource nodes, refineries, harvesters), military value
// (production buildings, construction yard), and proximity bonus (distance to
// own base) into a single integer score per tile.
//
// Unlike ThreatMap which models danger from the enemy, ValueMap models
// opportunity: where to attack for maximum strategic impact, and what to
// defend. Both maps feed into the Director subsystem and battle prediction.
//
// All arithmetic is integer-only to preserve determinism.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4AI/ThreatMap.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/SimTypes.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

struct ValueCell
{
    int32_t StrategicValue = 0;   // overall value score
    int32_t EconomicValue = 0;    // resource nodes, refineries, harvesters
    int32_t MilitaryValue = 0;    // production, tech, construction yard
    TickIndex LastUpdatedTick = 0;
};

class RA4AI_API ValueMap
{
public:
    ValueMap() = default;
    ValueMap(int32_t InWidth, int32_t InHeight);

    void Init(int32_t InWidth, int32_t InHeight);
    void Clear();

    // Recomputes the entire grid from world state and fog-limited memory.
    void UpdateFromWorld(const SimWorld& World, PlayerId Player,
                         const std::vector<EnemyMemory>& KnownEnemies,
                         const ContentDatabase* Content,
                         TickIndex CurrentTick);

    int32_t GetStrategicValue(TileCoord Tile) const;
    int32_t GetEconomicValue(TileCoord Tile) const;
    int32_t GetMilitaryValue(TileCoord Tile) const;

    // Find the tile with highest strategic value. Returns (-1,-1) if none.
    TileCoord FindHighestValueTarget(int32_t MinValue = 0) const;

    // Find the best attack target: high value, ideally low threat (favor
    // undefended high-value targets). Returns (-1,-1) if no suitable target.
    TileCoord FindBestAttackTarget(const ThreatMap& Threats) const;

    int32_t GetWidth() const { return Width; }
    int32_t GetHeight() const { return Height; }
    bool IsValid() const { return Width > 0 && Height > 0; }

    const ValueCell* GetCell(TileCoord Tile) const;

private:
    int32_t CellIndex(int32_t X, int32_t Y) const;
    bool InBounds(int32_t X, int32_t Y) const;
    void AddValueAtTile(int32_t X, int32_t Y, int32_t Value,
                        int32_t Econ, int32_t Mil);
    void SpreadValueFromTile(int32_t CenterX, int32_t CenterY,
                             int32_t MaxRangeTiles, int32_t Value,
                             int32_t Econ, int32_t Mil);

    int32_t Width = 0;
    int32_t Height = 0;
    std::vector<ValueCell> Cells;
};

} // namespace AI
} // namespace RA4
