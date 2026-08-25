// Copyright (c) Red Alert 4 project. The authoritative match simulation.
//
// SimWorld has no dependency on Unreal, rendering, UMG or audio. It can be stepped
// in a unit test, on a headless Linux dedicated server, or behind a UWorld in the
// client -- and all three produce identical state from identical inputs. That
// property is the foundation for replays, lockstep verification and server
// authority, so nothing that breaks it may be added here.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#if __has_include("HAL/Platform.h")
#include "HAL/Platform.h"
#endif

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Command.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Random.h"
#include "RA4Recon/MoraleModel.h"
#include "RA4Recon/ReconSystem.h"
#include "RA4Navigation/FlowField.h"
#include "RA4Navigation/MNavRouter.h"
#include "RA4Navigation/ReservationGrid.h"
#include "RA4Simulation/SimTypes.h"
#include "RA4Simulation/SimSnapshot.h"

#include "FogOfWarGrid.h"

namespace RA4
{




#ifndef RA4SIMULATION_API
#define RA4SIMULATION_API
#endif


struct MatchSetup
{
    uint64_t Seed = 0;
    MapDescription Map;
    struct PlayerSlot
    {
        bool bActive = false;
        uint8_t Team = 0;
        FactionId Faction = FactionId::None;
        int32_t StartingCredits = 10000;
        int32_t StartPositionIndex = 0;
    };

    PlayerSlot Players[kMaxPlayers];
};

enum class MatchPhase : uint8_t
{
    NotStarted = 0,
    Running,
    Finished,
};

struct CommandResult
{
    CommandReject Reason = CommandReject::Accepted;
    bool IsAccepted() const { return Reason == CommandReject::Accepted; }
};

class RA4SIMULATION_API SimWorld
{
public:
    SimWorld() = default;

    // --- Lifecycle ---------------------------------------------------------
    // InReconSettings is optional: nullptr (or bEnabled=false inside) means the
    // unreliable-intelligence layer is absent and the match behaves classically.
    // Additive default parameter, so no existing caller changes (ADR-0026).
    void Initialize(const ContentDatabase* InContent, const MatchSetup& Setup,
                    const Recon::ReconSettings* InReconSettings = nullptr);
    void Reset();
    void Restart();

    // Advances exactly one fixed tick. Systems run in a fixed order; see the
    // implementation for why that order is what it is.
    void Tick(const CommandFrame* Frame);

    MatchPhase GetPhase() const { return Phase; }
    TickIndex GetTick() const { return CurrentTick; }
    PlayerId GetWinner() const { return Winner; }

    // --- Entity access -----------------------------------------------------
    uint32_t GetEntityCapacity() const { return uint32_t(Core.size()); }
    // Read-only diagnostic for the parallel component-storage invariant.
    bool HasConsistentComponentStorage() const;
    bool IsAlive(EntityId Id) const;
    const EntityCore* GetCore(EntityId Id) const;
    const TransformComp* GetTransform(EntityId Id) const;
    const HealthComp* GetHealth(EntityId Id) const;
    const BuildingComp* GetBuilding(EntityId Id) const;
    const HarvesterComp* GetHarvester(EntityId Id) const;
    const ResourceNodeComp* GetResourceNode(EntityId Id) const;
    const MovementComp* GetMovement(EntityId Id) const;
    const CombatComp* GetCombat(EntityId Id) const;
    const OrderQueue* GetOrders(EntityId Id) const;
    const DirectControlComp* GetDirectControl(EntityId Id) const;
    const StatusComp* GetStatus(EntityId Id) const;
    const TransportComp* GetTransport(EntityId Id) const;
    const PassengerComp* GetPassengerOf(EntityId Id) const;
    int32_t GetConstructionProgressPerMille(EntityId Id) const;

    // Raw slot iteration for systems and for presentation sync. Callers must check
    // Core[i].bAlive; dead slots keep their component data until reused so that a
    // death event handler can still read the last known transform.
    const std::vector<EntityCore>& GetAllCores() const { return Core; }
    const std::vector<TransformComp>& GetAllTransforms() const { return Transforms; }
    EntityId MakeId(uint32_t Index) const;

    const PlayerState& GetPlayer(PlayerId Id) const;
    void AddCredits(PlayerId Id, int32_t Amount) { if (Id < kMaxPlayers) { Players[Id].Credits += Amount; } }
    const MapDescription& GetMap() const { return Map; }
    MapDescription& GetMapMutable() { return Map; }
    const ContentDatabase* GetContent() const { return Content; }
    const FFogOfWarGrid* GetFogGrid() const { return FogGrid.get(); }

    // Whether Viewer's fog currently shows the tile the entity stands on. Auto-target
    // acquisition asks this so a side does not shoot at what it cannot see; the
    // presentation layer asks it so fog actually hides things on screen (picking
    // and actor sync, V-A/V-B in VISIBILITY_CALLSITE_INVENTORY.md). Public and
    // const: it reads objective visibility, it cannot leak belief (that is
    // GetRecon()'s job) and it cannot mutate anything.
    bool IsEntityVisibleTo(PlayerId Viewer, uint32_t EntityIndex) const;
    // Whether Viewer's fog currently shows the tile containing a world point.
    // Combat events carry a location rather than a live entity (the shooter may
    // already be dead by the time presentation reads the event), so gating
    // tracers and impact markers needs this rather than the entity overload
    // above -- inventory row V-F. Same contract: true when fog is absent, and
    // CurrentlyVisible only, matching IsEntityVisibleTo (see V-A's MAJOR-2 note
    // on RadarDetected).
    bool IsLocationVisibleTo(PlayerId Viewer, const Vec2& Location) const;

    // Belief state (unreliable intelligence, ADR-0026). Read-only outside the
    // simulation; the UI and the AI commander query enemy information here and
    // never through the entity getters above once the feature is enabled.
    const Recon::ReconSystem& GetRecon() const { return ReconLayer; }


    // --- Spawning (server / mission scripts only) --------------------------
    EntityId SpawnUnit(ContentId Def, PlayerId Owner, const Vec2& Position, int32_t Facing = 0);
    EntityId SpawnBuilding(ContentId Def, PlayerId Owner, const TileCoord& OriginTile, bool bInstantComplete);
    EntityId SpawnResourceNode(ContentId Def, const TileCoord& Tile, int32_t Amount);
    void DebugDamage(EntityId TargetId, int32_t DamageAmount);
    void TeleportEntity(EntityId Id, const Vec2& NewPosition);


    // --- Commands ----------------------------------------------------------
    // Validation is total: ownership, liveness, affordability, tech, placement and
    // rate limits are all checked here, because this is the only path a client
    // packet can take into the simulation.
    CommandResult ApplyCommand(const Command& Cmd);

    bool IsPlacementValid(ContentId BuildingDef, PlayerId Owner, const TileCoord& OriginTile) const;
    bool HasPrerequisites(PlayerId Owner, const EntityDef& Def) const;

    // --- Cheat Engine Hooks --------------------------------------------------
    void CheatGrantCredits(PlayerId Owner, int32_t Amount);
    void CheatGrantPower(PlayerId Owner, int32_t PowerAmount);
    void CheatInstantBuild(PlayerId Owner);
    void CheatToggleGodMode(PlayerId Owner);

    // --- Determinism, Snapshots & Save/Restore -----------------------------
    // Hashes every value that can influence future state. Deliberately excludes
    // event lists and caches, which are outputs rather than state.
    uint64_t ComputeStateChecksum() const;
    StateHashBreakdown ComputeDetailedChecksum() const;

    SimSnapshot CaptureSnapshot() const;
    bool RestoreFromSnapshot(const SimSnapshot& Snapshot);
    void RecordSnapshot(); // Captures and stores current snapshot into ring buffer

    SnapshotRingBuffer& GetSnapshotHistory() { return SnapshotHistory; }
    const SnapshotRingBuffer& GetSnapshotHistory() const { return SnapshotHistory; }

    void Serialize(ByteWriter& W) const;
    bool Deserialize(ByteReader& R, const ContentDatabase* InContent);

    // --- Events ------------------------------------------------------------
    const std::vector<SimEvent>& GetEvents() const { return Events; }
    void ClearEvents() { Events.clear(); }

    Random& GetRandom() { return Rng; }
    bool IsHostile(PlayerId A, PlayerId B) const;



    // --- Navigation milestone diagnostics -----------------------------------
    const MovementStats& GetMovementStats() const { return Stats; }
    void ResetMovementStats() { Stats = MovementStats{}; }

private:
    // --- Systems, executed in this order every tick ------------------------
    void SystemApplyCommands(const CommandFrame* Frame);
    void SystemPower();
    // Runs after SystemPower so the throttle decisions it makes read this tick's
    // power balance, and before SystemProduction so an item funded this tick can
    // begin advancing on the same tick it becomes fully paid.
    void SystemFlowPayment();
    void SystemConstruction();
    // ADR-0013. Runs after SystemConstruction so a building that finished this tick is
    // already Complete, and before SystemCombat so a repaired structure survives the
    // shot that would otherwise have killed it at its pre-repair health.
    void SystemRepair();
    void SystemProduction();
    void SystemHarvesters();
    // Status effects: countdowns, infection damage-over-time. Runs before
    // orders/movement so a freshly stunned unit never acts on the tick it froze.
    void SystemStatusEffects();
    void SystemOrders();
    void SystemMovement();
    void SystemCombat();
    void SystemProjectiles();
    void SystemFogOfWar();
    void SystemRecon();
    void SystemVeterancy();
    void SystemFactionResources();
    void SystemDirectControl();
    void SystemDeaths();
    void SystemVictory();


    friend struct VacuumImploderState;
    friend class ExoticSuperweaponPhysics;

    // --- Internals ---------------------------------------------------------
    EntityId AllocateEntity();

    // bWasSold distinguishes a voluntary sale from a violent death. It is a
    // parameter rather than a flag on BuildingComp because the intent lasts exactly
    // one tick: a persisted flag can outlive the sale it described (a save taken
    // between the command and the death sweep reloads a building that is flagged
    // selling forever) and then silently forfeits the ADR-0012 destruction refund.
    void QueueDestroy(EntityId Id, EntityId Killer = EntityId::Invalid());
    void DestroyEntity(EntityId Id, EntityId Killer, bool bWasSold = false);
    void RemoveEntitySilently(EntityId Id);
    void ApplyDamage(EntityId TargetId, int32_t BaseDamage, WarheadClass Warhead, EntityId Source, PlayerId SourcePlayer);
    void ApplySplashDamage(const Vec2& Center, Fixed Radius, int32_t BaseDamage, WarheadClass Warhead,
                           int32_t FalloffPercent, EntityId Source, PlayerId SourcePlayer);

    void OccupyTiles(const BuildingComp& B, bool bOccupy);
    // ADR-0013 tier speed for a player, as a percentage. One place decides it so the
    // construction and production systems cannot drift apart.
    int32_t PowerSpeedPercent(PlayerId Owner) const;
    // At Critical only barracks-class producers and harvesters keep working. Which
    // buildings qualify is a content question (what can this thing produce), not a
    // hardcoded name list, so it is answered here from the definition.
    bool ProducerRunsAtCriticalPower(const EntityDef& Def) const;
    // Seeds BuildingComp::Priority at spawn. Anything ProducerRunsAtCriticalPower
    // keeps alive is Vital, so the priority bands and the Critical carve-out cannot
    // contradict each other.
    PowerPriority DefaultPowerPriorityFor(const EntityDef& Def) const;
    // The single ADR-0013 verdict on "is this queue head stopped by the power state".
    // Both SystemFlowPayment and SystemProduction ask it, because the two answering
    // the question separately is what produced a blackout deadlock: payment kept
    // charging for an item production refused to advance, and the money that should
    // have finished the power plant went into a frozen queue instead.
    bool IsProductionPowerStalled(uint32_t BuildingIndex) const;
    void BuildNavigationGrid();
    uint8_t GetNavigationPassability(const TileCoord& Tile) const;
    Nav::NavQuery MakeNavigationQuery(const EntityDef& Def) const;
    TileCoord ResolveNavigationTarget(const TileCoord& Desired, const Nav::NavQuery& Query) const;
    const Nav::FlowField* GetFlowField(const TileCoord& Target, const Nav::NavQuery& Query);
    void RefreshPlayerTech(PlayerId Owner);
    EntityId FindNearestResourceNode(const Vec2& From, PlayerId Owner) const;
    EntityId FindNearestRefinery(const Vec2& From, PlayerId Owner) const;
    // Air logistics: nearest friendly COMPLETE building whose production
    // category builds aircraft. Data-derived rearm pad -- no new flags.
    EntityId FindNearestRearmPoint(const Vec2& From, PlayerId Owner) const;
    // True when the node's definition regrows, i.e. an exhausted field must survive
    // at Amount 0 rather than be destroyed. Content-driven so mods get the behaviour
    // for free.
    bool IsRegrowingNode(EntityId NodeId) const;
    EntityId AcquireTarget(EntityId Attacker) const;

    // Uniform spatial bucket grid over live entities, rebuilt once per tick.
    //
    // WHY: AcquireTarget scanned all Core entries for every armed entity, so target
    // acquisition was O(n^2) - 4,000,000 distance checks per tick at the 2000-entity
    // budget load. Measured cost per entity grew 4.57x across a 4x entity increase
    // (TestProfile.cpp), which is that quadratic term. With buckets, a unit only
    // examines the cells its search radius actually covers.
    //
    // DETERMINISM: bucket contents are appended in ascending entity index and
    // cells are visited in a fixed order, so candidates are always considered in
    // the same sequence on every machine. AcquireTarget still resolves ties by
    // "strictly closer, else lower index", exactly as the linear scan did, so this
    // is a pure speed change and the checksum must not move.
    void RebuildSpatialGrid();
    // Appends live entity indices whose cell overlaps the square around Centre.
    void QuerySpatial(const Vec2& Centre, Fixed Radius, std::vector<uint32_t>& Out) const;

    // Scratch for FindNearestResourceNode's per-node harvester load tally; a member
    // so repeated queries reuse the allocation. Re-zeroed at the top of every call,
    // so it is derived state and deliberately absent from Serialize and checksums.
    mutable std::vector<uint32_t> ResourceLoadScratch;

    // Veterancy bookkeeping, fed from ApplyDamage where the killer is known for
    // every damage path (hitscan, projectile impact, splash). Promotion applies the
    // new rank's bonuses immediately and emits EntityVeterancyPromoted once per rank.
    void TryPromoteVeterancy(EntityId Unit);

    // Tactical mechanics.
    // The weapon an entity actually fires: its own, or -- for a multigunner
    // transport carrying an armed passenger -- the passenger's weapon.
    ContentId ResolveFireWeapon(EntityId Attacker) const;
    // Applies a weapon's on-hit status payload to the victim it wounded.
    void ApplyOnHitStatus(EntityId Victim, const WeaponDef& Weapon);
    // Boarded passengers follow their transport and die with it.
    void UpdatePassengers();
    // Passive income from captured tech buildings (oil derricks).
    void SystemTechIncome();
    // Public seam for protocol powers that mass-apply a status template (phase
    // field, EMP pulse, ...). Copies every positive countdown of Template onto
    // units in radius, keeping the larger of the two. bEnemiesOnly filters to
    // hostile targets; false buffs own side too.
    void ApplyStatusInRadius(PlayerId Caster, const Vec2& Center, Fixed Radius,
                             const StatusComp& Template, bool bEnemiesOnly);

    Vec2 FindFreeSpawnPoint(const BuildingComp& Producer, ContentId UnitDef) const;

    void EmitEvent(const SimEvent& Event) { Events.push_back(Event); }
    void FireWeapon(EntityId Attacker, EntityId TargetId, const WeaponDef& Weapon);

    // --- State -------------------------------------------------------------
    const ContentDatabase* Content = nullptr;
    MapDescription Map;
    std::unique_ptr<Nav::NavGrid> NavigationGrid;
    std::unique_ptr<FFogOfWarGrid> FogGrid;

    // Spatial bucket grid, rebuilt at the top of each tick by RebuildSpatialGrid.
    // Cell size is a multiple of the tile size so cell lookup is integer division
    // with no floating point, keeping it deterministic across platforms.
    static constexpr int32_t kSpatialCellTiles = 8;
    int32_t SpatialCellsX = 0;
    int32_t SpatialCellsY = 0;
    // One vector per cell, holding live entity indices in ascending order.
    std::vector<std::vector<uint32_t>> SpatialCells;
    // Scratch reused by AcquireTarget so a per-call allocation is not paid 2000
    // times per tick. Mutable because AcquireTarget is const and logically a query.
    mutable std::vector<uint32_t> SpatialQueryScratch;


    struct FlowFieldCacheEntry
    {
        TileCoord Target;
        Nav::NavQuery Query;
        uint32_t TopologyRevision = 0;
        TickIndex LastUsedTick = 0;
        std::unique_ptr<Nav::FlowField> Field;
    };
    std::vector<FlowFieldCacheEntry> FlowFieldCache;

    std::unique_ptr<Nav::ReservationGrid> Reservations;
    std::unique_ptr<Nav::MNavRouter> Router;
    MovementStats Stats;
    int32_t FlowFieldBuildsThisTick = 0;
    int32_t MacroPathBuildsThisTick = 0;

    MatchSetup SetupConfig;
    Random Rng;
    // Separate stream for the intel layer, seeded from the match seed. Isolation
    // is deliberate: intel draws must not shift the draw sequence of existing
    // systems, or every pre-intel replay becomes unreplayable at once.
    Random ReconRng;
    Recon::ReconSystem ReconLayer;
    // Reused per tick by SystemRecon; member so vector capacity persists and the
    // steady state allocates nothing.
    Recon::ObservationInput ReconInput;
    // Kept for Restart(), which re-runs Initialize with the original arguments.
    // Owned by the content layer, same lifetime contract as Content.
    const Recon::ReconSettings* ReconSettingsRef = nullptr;
    TickIndex CurrentTick = 0;
    MatchPhase Phase = MatchPhase::NotStarted;
    PlayerId Winner = kInvalidPlayer;

    std::vector<EntityCore> Core;
    std::vector<TransformComp> Transforms;
    std::vector<HealthComp> Healths;
    std::vector<MovementComp> Movements;
    std::vector<CombatComp> Combats;
    // Per-aircraft return/reload latch. Ammo alone cannot represent it: the first
    // reload tick raises an empty magazine above zero, but the craft must remain at
    // the pad until full instead of immediately re-entering combat.
    std::vector<uint8_t> AircraftRearming;
    std::vector<BuildingComp> Buildings;
    std::vector<HarvesterComp> Harvesters;
    std::vector<ResourceNodeComp> ResourceNodes;
    std::vector<ProjectileComp> Projectiles;
    std::vector<OrderQueue> Orders;
    std::vector<DirectControlComp> DirectControls;
    std::vector<StatusComp> Statuses;
    std::vector<TransportComp> Transports;
    std::vector<PassengerComp> PassengerOf;
    std::vector<CaptureComp> Captures;
    // Psychological state per entity (RA4Recon reads it through the aggregate
    // observer; only SystemRecon writes it). Sized with the other component
    // vectors; meaningful only for units.
    std::vector<Recon::MoraleComp> Morales;
    // Scratch for the recon visible-tile pass; a member so its capacity survives
    // across ticks (no steady-state allocation in the tick).
    mutable std::vector<const Recon::PerceivedTrack*> ReconTrackScratch;

    std::vector<uint32_t> FreeSlots;
    uint32_t HighWaterMark = 0;

    PlayerState Players[kMaxPlayers];

    // Entities queued for removal this tick. Deferring destruction keeps system
    // iteration stable: a unit dying in the combat system must not invalidate the
    // slot the movement system is about to read.
    std::vector<EntityId> PendingDestroy;
    // Parallel attribution for PendingDestroy. Each entry is the actual entity that
    // delivered the first fatal blow, or Invalid for sales, hazards and debug deaths.
    // Both vectors are consumed before Tick returns, so neither is serialized or
    // hashed; QueueDestroy is the only writer and keeps their indices aligned.
    std::vector<EntityId> PendingDestroyKillers;
    // Buildings the player sold this tick, as opposed to lost. ADR-0012 pays no
    // queue refund for a sale (the sale price is the compensation), and this is
    // tick-scoped intent rather than durable state, so it is neither serialized nor
    // hashed -- it is consumed by SystemDeaths on the tick it is written.
    std::vector<EntityId> PendingSales;

    std::vector<SimEvent> Events;

    // ADR-0012 credit-allocation scratch, reused every tick so the funding pass does
    // not allocate. Rebuilt from scratch each call, so it is not simulation state and
    // is deliberately absent from Serialize and ComputeStateChecksum.
    struct FundingCandidate
    {
        uint32_t BuildingIndex = 0;
        int32_t Priority = 0;
        PlayerId Owner = kInvalidPlayer;
    };
    std::vector<FundingCandidate> FundingCandidates;

    // Per-player command budget, reset every tick. A client that exceeds it is
    // throttled rather than trusted; see Docs/ThreatModel.md.
    int32_t CommandsThisTick[kMaxPlayers] = {};
    static constexpr int32_t kMaxCommandsPerPlayerPerTick = 64;

    SnapshotRingBuffer SnapshotHistory;
};

} // namespace RA4
