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

    // Raw slot iteration for systems and for presentation sync. Callers must check
    // Core[i].bAlive; dead slots keep their component data until reused so that a
    // death event handler can still read the last known transform.
    const std::vector<EntityCore>& GetAllCores() const { return Core; }
    const std::vector<TransformComp>& GetAllTransforms() const { return Transforms; }
    EntityId MakeId(uint32_t Index) const;

    const PlayerState& GetPlayer(PlayerId Id) const;
    const MapDescription& GetMap() const { return Map; }
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

    // --- Determinism & Save/Restore -----------------------------------------
    // Hashes every value that can influence future state. Deliberately excludes
    // event lists and caches, which are outputs rather than state.
    uint64_t ComputeStateChecksum() const;

    void Serialize(ByteWriter& W) const;
    bool Deserialize(ByteReader& R, const ContentDatabase* InContent);

    // --- Events ------------------------------------------------------------
    const std::vector<SimEvent>& GetEvents() const { return Events; }
    void ClearEvents() { Events.clear(); }

    Random& GetRandom() { return Rng; }

    // --- Navigation milestone diagnostics -----------------------------------
    const MovementStats& GetMovementStats() const { return Stats; }
    void ResetMovementStats() { Stats = MovementStats{}; }

private:
    // --- Systems, executed in this order every tick ------------------------
    void SystemApplyCommands(const CommandFrame* Frame);
    void SystemPower();
    void SystemConstruction();
    void SystemProduction();
    void SystemHarvesters();
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


    // --- Internals ---------------------------------------------------------
    EntityId AllocateEntity();
    void DestroyEntity(EntityId Id, EntityId Killer);
    void ApplyDamage(EntityId TargetId, int32_t BaseDamage, WarheadClass Warhead, EntityId Source, PlayerId SourcePlayer);
    void ApplySplashDamage(const Vec2& Center, Fixed Radius, int32_t BaseDamage, WarheadClass Warhead,
                           int32_t FalloffPercent, EntityId Source, PlayerId SourcePlayer);

    void OccupyTiles(const BuildingComp& B, bool bOccupy);
    void BuildNavigationGrid();
    uint8_t GetNavigationPassability(const TileCoord& Tile) const;
    Nav::NavQuery MakeNavigationQuery(const EntityDef& Def) const;
    TileCoord ResolveNavigationTarget(const TileCoord& Desired, const Nav::NavQuery& Query) const;
    const Nav::FlowField* GetFlowField(const TileCoord& Target, const Nav::NavQuery& Query);
    void RefreshPlayerTech(PlayerId Owner);
    EntityId FindNearestResourceNode(const Vec2& From, PlayerId Owner) const;
    EntityId FindNearestRefinery(const Vec2& From, PlayerId Owner) const;
    EntityId AcquireTarget(EntityId Attacker) const;
    bool IsHostile(PlayerId A, PlayerId B) const;
    Vec2 FindFreeSpawnPoint(const BuildingComp& Producer, ContentId UnitDef) const;
    void EmitEvent(const SimEvent& Event) { Events.push_back(Event); }
    void FireWeapon(EntityId Attacker, EntityId TargetId, const WeaponDef& Weapon);

    // --- State -------------------------------------------------------------
    const ContentDatabase* Content = nullptr;
    MapDescription Map;
    std::unique_ptr<Nav::NavGrid> NavigationGrid;
    std::unique_ptr<FFogOfWarGrid> FogGrid;


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
    std::vector<BuildingComp> Buildings;
    std::vector<HarvesterComp> Harvesters;
    std::vector<ResourceNodeComp> ResourceNodes;
    std::vector<ProjectileComp> Projectiles;
    std::vector<OrderQueue> Orders;
    std::vector<DirectControlComp> DirectControls;
    // Psychological state per entity (RA4Recon reads it through the aggregate
    // observer; only SystemRecon writes it). Sized with the other component
    // vectors; meaningful only for units.
    std::vector<Recon::MoraleComp> Morales;

    std::vector<uint32_t> FreeSlots;
    uint32_t HighWaterMark = 0;

    PlayerState Players[kMaxPlayers];

    // Entities queued for removal this tick. Deferring destruction keeps system
    // iteration stable: a unit dying in the combat system must not invalidate the
    // slot the movement system is about to read.
    std::vector<EntityId> PendingDestroy;

    std::vector<SimEvent> Events;

    // Per-player command budget, reset every tick. A client that exceeds it is
    // throttled rather than trusted; see Docs/ThreatModel.md.
    int32_t CommandsThisTick[kMaxPlayers] = {};
    static constexpr int32_t kMaxCommandsPerPlayerPerTick = 64;
};

} // namespace RA4
