// Copyright (c) Red Alert 4 project. Per-tile threat assessment grid.
//
// The ThreatMap is an engine-free, deterministic spatial model of how dangerous
// each tile on the map is, computed from fog-limited enemy sightings. It reads
// the same EnemyMemory that AICommander uses, so it never leaks information
// beyond what the fog of war allows. All arithmetic is integer-only to preserve
// determinism across replays and lockstep.
//
// Threat is derived from enemy unit/building weapon stats (DPS approximation)
// and spread across tiles within weapon range using linear falloff. Stale
// sightings contribute less through the EnemyMemory Confidence field.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4AI/AIWorldView.h"
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

struct ThreatCell
{
    int32_t TotalThreat = 0;
    int32_t AirThreat = 0;           // threat from anti-air units/buildings
    int32_t AntiArmorThreat = 0;     // threat from anti-armor units/buildings
    int32_t StructuralThreat = 0;    // threat from defensive buildings (turrets, etc.)
    TickIndex LastUpdatedTick = 0;
};

class RA4AI_API ThreatMap
{
public:
    ThreatMap() = default;
    ThreatMap(int32_t InWidth, int32_t InHeight);

    void Init(int32_t InWidth, int32_t InHeight);
    void Clear();

    // Recomputes the entire grid from scratch. Called on decision ticks to keep
    // the map in sync with the commander's fog-limited memory.
    void UpdateFromMemory(const std::vector<EnemyMemory>& KnownEnemies,
                          const ContentDatabase* Content,
                          const MapDescription& Map,
                          TickIndex CurrentTick);

    int32_t GetThreat(TileCoord Tile) const;
    int32_t GetAirThreat(TileCoord Tile) const;
    int32_t GetAntiArmorThreat(TileCoord Tile) const;
    int32_t GetStructuralThreat(TileCoord Tile) const;

    // Sum of total threat in a square area around Center (inclusive radius).
    int32_t GetAreaThreat(TileCoord Center, int32_t Radius) const;

    // Find the tile with highest total threat. If Radius > 0, only considers
    // tiles within Radius of Center. Returns (-1,-1) if no threat exists.
    TileCoord FindHighestThreatTile(int32_t Radius = 0,
                                    TileCoord Center = TileCoord(-1, -1)) const;

    int32_t GetWidth() const { return Width; }
    int32_t GetHeight() const { return Height; }
    bool IsValid() const { return Width > 0 && Height > 0; }

    const ThreatCell* GetCell(TileCoord Tile) const;

private:
    int32_t CellIndex(int32_t X, int32_t Y) const;
    bool InBounds(int32_t X, int32_t Y) const;
    void AddThreatAtTile(int32_t X, int32_t Y, int32_t Threat,
                         int32_t Air, int32_t AntiArmor, int32_t Structural);
    void SpreadThreatFromTile(int32_t CenterX, int32_t CenterY,
                              int32_t MaxRangeTiles, int32_t BaseThreat,
                              int32_t Air, int32_t AntiArmor, int32_t Structural);

    int32_t Width = 0;
    int32_t Height = 0;
    std::vector<ThreatCell> Cells;
};

} // namespace AI
} // namespace RA4
