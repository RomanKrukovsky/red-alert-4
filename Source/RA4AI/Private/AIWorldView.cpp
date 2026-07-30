// Copyright (c) Red Alert 4 project.
#include "RA4AI/AIWorldView.h"
#include "FogOfWarGrid.h"

namespace RA4
{
namespace AI
{

SimWorldView::SimWorldView(const SimWorld& InWorld, PlayerId InPlayer)
    : World(InWorld)
    , Player(InPlayer)
{
}

bool SimWorldView::HasPrerequisites(ContentId Content) const
{
    const ContentDatabase* Db = World.GetContent();
    if (Db == nullptr)
    {
        return false;
    }
    const EntityDef* Def = Db->FindEntity(Content);
    if (Def == nullptr)
    {
        return false;
    }
    return World.HasPrerequisites(Player, *Def);
}

bool SimWorldView::IsPlacementValid(ContentId Structure, TileCoord Tile) const
{
    return World.IsPlacementValid(Structure, Player, Tile);
}

void SimWorldView::UpdateMemory(TickIndex MemoryRetentionTicks)
{
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    const TickIndex CurrentTick = World.GetTick();
    const FFogOfWarGrid* Fog = World.GetFogGrid();

    // 1. Update freshly visible enemy entities
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        const EntityCore& Core = Cores[I];
        if (!Core.bAlive || Core.Owner == Player || Core.Owner >= kMaxPlayers)
        {
            continue;
        }

        const EntityId Id = World.MakeId(I);
        const TransformComp* Transform = World.GetTransform(Id);
        if (Transform == nullptr)
        {
            continue;
        }

        // Tile indices, not world units. FFogOfWarGrid is sized Map.Width x Map.Height
        // (tiles), so feeding it raw world coordinates put every lookup out of bounds,
        // where GetVisibility answers NeverSeen -- with fog active the AI then observed
        // nothing whatsoever, including enemies standing next to its own base.
        const TileCoord Tile = World.GetMap().WorldToTile(Transform->Position);

        // If fog grid is active, only update/reveal entities currently visible or detected on radar
        if (Fog != nullptr)
        {
            const VisibilityState Vis = Fog->GetVisibility(Player, Tile.X, Tile.Y);
            if (Vis != VisibilityState::CurrentlyVisible && Vis != VisibilityState::RadarDetected)
            {
                continue;
            }
        }

        // Find existing memory or append new
        bool bFound = false;
        for (EnemyMemory& Mem : KnownEnemies)
        {
            if (Mem.Entity == Id)
            {
                Mem.Position = Tile;
                Mem.LastSeenTick = CurrentTick;
                Mem.Confidence = Fixed::FromInt(1);
                bFound = true;
                break;
            }
        }

        if (!bFound)
        {
            EnemyMemory Mem;
            Mem.Entity = Id;
            Mem.Position = Tile;
            Mem.LastSeenTick = CurrentTick;
            Mem.DefId = Core.Def;
            Mem.Kind = Core.Kind;
            Mem.Confidence = Fixed::FromInt(1);
            KnownEnemies.push_back(Mem);
        }
    }

    DecayMemories(MemoryRetentionTicks);
}

void SimWorldView::DecayMemories(TickIndex MemoryRetentionTicks)
{
    const TickIndex CurrentTick = World.GetTick();

    for (auto It = KnownEnemies.begin(); It != KnownEnemies.end();)
    {
        const EntityCore* Core = World.GetCore(It->Entity);
        if (Core == nullptr || !Core->bAlive)
        {
            It = KnownEnemies.erase(It);
            continue;
        }

        const TickIndex Elapsed = CurrentTick >= It->LastSeenTick ? CurrentTick - It->LastSeenTick : 0;
        if (MemoryRetentionTicks > 0 && Elapsed > MemoryRetentionTicks)
        {
            It = KnownEnemies.erase(It);
            continue;
        }

        // Linear decay to a 0.1 floor: a stale sighting is worth less than a fresh
        // one but never worthless, because an enemy base does not usually move.
        if (MemoryRetentionTicks > 0)
        {
            const int32_t Ratio = 100 - static_cast<int32_t>((Elapsed * 90) / MemoryRetentionTicks);
            It->Confidence = Fixed::FromRatio(std::max(10, Ratio), 100);
        }

        ++It;
    }
}

} // namespace AI
} // namespace RA4
