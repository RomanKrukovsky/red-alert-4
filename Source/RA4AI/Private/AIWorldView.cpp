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
    // --- I-M6: when the recon layer is on, the AI reads BELIEF, not truth ---------
    //
    // This is the whole point of the milestone. The AI's zero-cheat property stops
    // being a promise enforced by review and becomes a consequence of the code: the
    // commander cannot see through fog or past distortion because the only enemy
    // information reaching it is the same staff map the human player looks at.
    //
    // A phantom therefore fools the AI exactly as it fools a person, and an
    // over-count makes it over-prepare. That is not a side effect to be tolerated --
    // it is the behaviour the layer exists to produce.
    if (World.GetRecon().IsEnabled())
    {
        UpdateMemoryFromBelief(MemoryRetentionTicks);
        return;
    }

    const std::vector<EntityCore>& Cores = World.GetAllCores();
    const TickIndex CurrentTick = World.GetTick();
    const FFogOfWarGrid* Fog = World.GetFogGrid();

    // 1. Update freshly visible enemy entities
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        const EntityCore& Core = Cores[I];
        if (!Core.bAlive || Core.Owner >= kMaxPlayers || !World.IsHostile(Player, Core.Owner))
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

void SimWorldView::UpdateMemoryFromBelief(TickIndex MemoryRetentionTicks)
{
    const Recon::PerceivedWorld& Belief = World.GetRecon().GetPerceivedWorld(Player);

    BeliefScratch.clear();
    Belief.GetTracksInRegion(0, 0, World.GetMap().Width - 1, World.GetMap().Height - 1, BeliefScratch);

    // Belief is authoritative for the AI here, so the memory list is REPLACED rather
    // than merged. Keeping stale entries alongside would quietly re-introduce
    // knowledge the staff map no longer holds -- including contacts the player's own
    // scouts have already disproved.
    KnownEnemies.clear();
    for (const Recon::PerceivedTrack* Track : BeliefScratch)
    {
        EnemyMemory Mem;
        // No EntityId: a track has none, by design (INVARIANT 10). Downstream AI code
        // uses Entity only to re-look-up truth, which is exactly what must stop, so
        // leaving it invalid makes any such attempt fail loudly instead of silently
        // working.
        Mem.Entity = EntityId{};
        Mem.Position = World.GetMap().WorldToTile(Track->BelievedPosition);
        Mem.LastSeenTick = Track->LastUpdateTick;
        // A believed CATEGORY is not a content id, and inventing one would hand the
        // AI an identification the staff map never made. Invalid means "unidentified";
        // consumers already treat an invalid def as an unknown contact.
        Mem.DefId = Track->BelievedClass;
        Mem.Kind = Track->BelievedCategory == Recon::ObservedCategory::Structure ? EntityKind::Building
                                                                                : EntityKind::Unit;
        // Confidence comes from the staff map rather than from a decay clock the AI
        // runs itself: the layer already models how sure the HQ is, and a second
        // opinion would drift from the one the player sees.
        Mem.Confidence = Track->Confidence;
        KnownEnemies.push_back(Mem);
    }
    (void)MemoryRetentionTicks; // belief lifetime is the recon layer's business
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
