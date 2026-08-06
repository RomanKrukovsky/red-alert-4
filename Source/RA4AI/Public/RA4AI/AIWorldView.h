// Copyright (c) Red Alert 4 project. Read-only AI World View and Fog-of-War memory.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Recon/PerceivedWorld.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

// Enemy memory tracking unit or building seen under fog of war
struct EnemyMemory
{
    EntityId Entity;
    TileCoord Position{0, 0};
    TickIndex LastSeenTick = 0;
    ContentId DefId;
    EntityKind Kind = EntityKind::Unit;
    Fixed Confidence = Fixed::FromInt(1); // 1.0 = freshly seen, decays over time in fog
};

class RA4AI_API IAIWorldView
{
public:
    virtual ~IAIWorldView() = default;

    virtual TickIndex GetCurrentTick() const = 0;
    virtual PlayerId GetPlayerId() const = 0;
    virtual FactionId GetFactionId() const = 0;
    virtual int32_t GetCredits() const = 0;
    virtual int32_t GetPowerProduced() const = 0;
    virtual int32_t GetPowerConsumed() const = 0;
    virtual int32_t GetTotalHarvested() const = 0;

    virtual const std::vector<EnemyMemory>& GetKnownEnemies() const = 0;
    virtual const ContentDatabase* GetContent() const = 0;
    virtual bool HasPrerequisites(ContentId Content) const = 0;
    virtual bool IsPlacementValid(ContentId Structure, TileCoord Tile) const = 0;
    virtual const SimWorld& GetSimWorldUnsafe() const = 0;
};

// Concrete read-only snapshot adapter wrapping SimWorld
class RA4AI_API SimWorldView : public IAIWorldView
{
public:
    SimWorldView(const SimWorld& InWorld, PlayerId InPlayer);

    TickIndex GetCurrentTick() const override { return World.GetTick(); }
    PlayerId GetPlayerId() const override { return Player; }
    FactionId GetFactionId() const override { return World.GetPlayer(Player).Faction; }
    int32_t GetCredits() const override { return World.GetPlayer(Player).Credits; }
    int32_t GetPowerProduced() const override { return World.GetPlayer(Player).PowerProduced; }
    int32_t GetPowerConsumed() const override { return World.GetPlayer(Player).PowerConsumed; }
    int32_t GetTotalHarvested() const override { return World.GetPlayer(Player).TotalHarvested; }

    const std::vector<EnemyMemory>& GetKnownEnemies() const override { return KnownEnemies; }
    const ContentDatabase* GetContent() const override { return World.GetContent(); }
    bool HasPrerequisites(ContentId Content) const override;
    bool IsPlacementValid(ContentId Structure, TileCoord Tile) const override;
    const SimWorld& GetSimWorldUnsafe() const override { return World; }

    // Observes everything currently visible, then ages what was not seen.
    void UpdateMemory(TickIndex MemoryRetentionTicks);

    // Rebuilds enemy memory from the player's perceived world instead of from
    // entity scans (I-M6). Called by UpdateMemory when the recon layer is enabled;
    // separate so the truth path stays untouched and testable on its own.
    void UpdateMemoryFromBelief(TickIndex MemoryRetentionTicks);

    // Ages memories that were not refreshed this pass: confidence falls linearly to
    // a 0.1 floor and the record is dropped once it exceeds the retention window.
    //
    // Split out from UpdateMemory because without a fog grid every enemy is visible
    // every tick, so observation always resets confidence to 1 and decay can never
    // be exercised -- or tested -- through the combined call.
    void DecayMemories(TickIndex MemoryRetentionTicks);
    void AddKnownEnemyMemory(const EnemyMemory& Mem) { KnownEnemies.push_back(Mem); }

private:
    const SimWorld& World;
    PlayerId Player;
    std::vector<EnemyMemory> KnownEnemies;
    // Scratch for the belief query; a member so repeated per-tick rebuilds allocate
    // nothing in the steady state.
    std::vector<const Recon::PerceivedTrack*> BeliefScratch;
};

} // namespace AI
} // namespace RA4
