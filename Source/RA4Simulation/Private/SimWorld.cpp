// Copyright (c) Red Alert 4 project.
#include "RA4Simulation/SimWorld.h"

#include "RA4Core/Checksum.h"
#include "RA4Core/SimConfig.h"

#include "RA4Recon/DistortionPipeline.h"
#include "FogOfWarGrid.h"
#include "RA4Navigation/Formation.h"
#include "RA4Navigation/MNavRouter.h"
#include "RA4Navigation/ReservationGrid.h"

#include <algorithm>
#include <cassert>

namespace RA4
{

namespace
{
// Transition window (Entering/Exiting) before direct control is fully active.
constexpr TickIndex kDirectControlEnterExitTicks = 4; // 200ms at 20Hz

// Engineer capture channel: 5 seconds of standing at the door.
constexpr int32_t kCaptureChannelTicks = 100;
}

static_assert(MapDescription::kTileSizeUnitsLocal == kTileSizeUnits,
              "MapDescription tile size must match the simulation tile size");

namespace
{
// Progress is accumulated in hundredths of a tick so that low power can slow
// production proportionally without a floating point rate or a modulo trick.
constexpr int32_t kProgressScale = 100;

// A turret must be within this many angle units of the target before a weapon that
// requires alignment will fire (~5.6 degrees).
constexpr int32_t kTurretAlignTolerance = 64;

// How close a harvester must get to the *edge* of its target to dock.
constexpr int64_t kDockDistanceUnits = 260;

// Air logistics: strike aircraft carry a finite magazine and fly home to an
// air producer to rearm, exactly as a loaded harvester heads for its refinery.
constexpr int32_t kAircraftDefaultAmmo = 12;
// Reload rate while sitting inside the rearm dock radius.
constexpr int32_t kAircraftReloadPerTick = 2;
// Dock radius around the rearm pad's footprint centre, mirroring the harvester
// docking idea at a scale that suits a fast flyer.
constexpr int64_t kRearmDockDistanceUnits = 200;

// Docking is measured to the target's centre, but a building occupies tiles that no
// unit can enter. A harvester pressed against the wall of a 3x2 refinery is already
// ~400 units from its centre, so a flat 260-unit radius made docking geometrically
// impossible: the harvester ground against the building, repathed every 60 ticks and
// never unloaded. The radius therefore grows with the target's footprint.
Fixed DockRadiusFor(const EntityDef* Def)
{
    Fixed Radius = Fixed::FromInt(kDockDistanceUnits);
    if (Def != nullptr && Def->Kind == EntityKind::Building)
    {
        const int64_t SpanTiles = std::max(Def->Building.FootprintX, Def->Building.FootprintY);
        Radius += Fixed::FromInt((SpanTiles * kTileSizeUnits) / 2);
    }
    return Radius;
}

// Widens an arrive radius by the size of the group heading for the same point
// (SimTypes.h:172: "Scaled by group size ... so a hundred units do not pile onto one
// point"). SpawnUnit set ArriveRadius once from CollisionRadius and nothing ever
// scaled it.
//
// Growth is sub-linear: the radius bounds an AREA, so widening by the square root of
// the crowd keeps roughly a constant amount of room per unit. Linear growth would let
// a 200-unit order declare arrival most of a screen from the target. The result is
// clamped so a very large group cannot swallow a whole sector.
//
// Declared up here because it is used by BOTH SystemOrders and SystemMovement, and
// that sharing is load-bearing rather than tidiness: SystemOrders decides when a Move
// order is POPPED and SystemMovement decides when the unit STOPS. If the two used
// different radii, a unit would stop moving at the wide radius while its order was
// never popped at the narrow one, and SystemOrders -- which runs immediately before
// SystemMovement and re-sets bHasDestination from the queue every tick -- would keep
// it moving forever. That is a permanent stall with a full order queue.
Fixed ScaleArriveRadiusForGroup(Fixed BaseRadius, int64_t GroupSize)
{
    if (GroupSize <= 1)
    {
        return BaseRadius;
    }
    const Fixed Scaled = BaseRadius * FxSqrt(Fixed::FromInt(GroupSize));
    return FxMin(Scaled, Fixed::FromInt(MapDescription::kTileSizeUnitsLocal * 3));
}

// Packs a tile into a sortable key. The bias keeps negative coordinates from
// colliding with positive ones.
uint64_t PackDestTileKey(const TileCoord& Tile)
{
    return (uint64_t(uint32_t(Tile.X + (1 << 20))) << 32) | uint64_t(uint32_t(Tile.Y + (1 << 20)));
}

// Counts living units whose destination tile matches DestTile, from a pre-sorted key
// list. Returns at least 1 so a caller never scales by zero.
//
// A sorted vector plus binary search rather than a hash map: sorting integers is
// deterministic, whereas a hash container would bring bucket layout and iteration
// order -- both implementation-defined -- into authoritative simulation state.
int64_t GroupSizeAtDestTile(const std::vector<uint64_t>& SortedKeys, const TileCoord& DestTile)
{
    const uint64_t Key = PackDestTileKey(DestTile);
    const auto Lo = std::lower_bound(SortedKeys.begin(), SortedKeys.end(), Key);
    const auto Hi = std::upper_bound(SortedKeys.begin(), SortedKeys.end(), Key);
    const int64_t Count = int64_t(Hi - Lo);
    return Count > 0 ? Count : 1;
}

// ADR-0012 refund table. All percentages apply to PaidCredits, never to TotalCost,
// so a player can never profit by queuing and cancelling: you get back a fraction
// of what you actually spent. A building cancelled early is nearly free to back out
// of; past a quarter paid the commitment starts to cost you.
constexpr int32_t kRefundBuildingEarlyPercent = 90;   // <= 25% paid
constexpr int32_t kRefundBuildingLatePercent = 60;    // >  25% paid
constexpr int32_t kRefundUnitPercent = 80;
constexpr int32_t kRefundBuildingEarlyThresholdPercent = 25;
// The producer is gone through no fault of the queue; a partial refund softens the
// double loss without making destroyed factories a cheap way to recover credits.
constexpr int32_t kRefundProducerDestroyedPercent = 50;

int32_t FlowPaymentCancelRefund(const ProductionItem& Item, EntityKind Kind)
{
    if (Item.PaidCredits <= 0)
    {
        return 0;
    }
    int32_t Percent = kRefundUnitPercent;
    if (Kind == EntityKind::Building)
    {
        const bool bEarly = Item.TotalCost <= 0 ||
                            (int64_t(Item.PaidCredits) * 100) <=
                                (int64_t(Item.TotalCost) * kRefundBuildingEarlyThresholdPercent);
        Percent = bEarly ? kRefundBuildingEarlyPercent : kRefundBuildingLatePercent;
    }
    return int32_t((int64_t(Item.PaidCredits) * Percent) / 100);
}
} // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void SimWorld::Reset()
{
    NavigationGrid.reset();
    FogGrid.reset();
    FlowFieldCache.clear();

    if (Reservations) Reservations->Expire(0);
    if (Router) Router->InvalidateAll();
    FlowFieldBuildsThisTick = 0;
    MacroPathBuildsThisTick = 0;
    Stats = MovementStats{};
    ReconLayer.Reset();
    Core.clear();
    Transforms.clear();
    Healths.clear();
    Movements.clear();
    Combats.clear();
    AircraftRearming.clear();
    Buildings.clear();
    Harvesters.clear();
    ResourceNodes.clear();
    Projectiles.clear();
    Orders.clear();
    DirectControls.clear();
    Morales.clear();
    Statuses.clear();
    Transports.clear();
    PassengerOf.clear();
    Captures.clear();
    FreeSlots.clear();
    PendingDestroy.clear();
    PendingDestroyKillers.clear();
    PendingSales.clear();
    Events.clear();
    // Reserve the whole entity budget up front. Systems legitimately spawn entities
    // while iterating (a factory finishing a tank, a gun firing a shell), and a
    // reallocation at that moment would dangle every component reference the
    // system is holding. AllocateEntity refuses to exceed kMaxEntities, so with the
    // capacity reserved here the vectors never move.
    Core.reserve(kMaxEntities);
    Transforms.reserve(kMaxEntities);
    Healths.reserve(kMaxEntities);
    Movements.reserve(kMaxEntities);
    Combats.reserve(kMaxEntities);
    AircraftRearming.reserve(kMaxEntities);
    Buildings.reserve(kMaxEntities);
    Harvesters.reserve(kMaxEntities);
    ResourceNodes.reserve(kMaxEntities);
    Projectiles.reserve(kMaxEntities);
    Orders.reserve(kMaxEntities);
    DirectControls.reserve(kMaxEntities);
    // Morales was missed when it was added, even though AllocateEntity pushes to it
    // alongside the others. SystemCombat holds a Morales[I] reference across
    // MoraleApplyAllyDeath, so a growth reallocation there is a use-after-free -- and
    // because allocators differ between platforms the resulting garbage differs per
    // peer, making it a desync rather than a clean crash.
    Morales.reserve(kMaxEntities);
    Statuses.reserve(kMaxEntities);
    Transports.reserve(kMaxEntities);
    PassengerOf.reserve(kMaxEntities);
    Captures.reserve(kMaxEntities);

    HighWaterMark = 0;
    CurrentTick = 0;
    Phase = MatchPhase::NotStarted;
    Winner = kInvalidPlayer;
    for (PlayerState& P : Players)
    {
        P = PlayerState();
    }
}

void SimWorld::Initialize(const ContentDatabase* InContent, const MatchSetup& Setup,
                          const Recon::ReconSettings* InReconSettings)
{
    Reset();
    Content = InContent;
    SetupConfig = Setup;
    Map = Setup.Map;
    BuildNavigationGrid();
    FogGrid = std::make_unique<FFogOfWarGrid>(Map.Width, Map.Height, kMaxPlayers);
    Reservations = std::make_unique<Nav::ReservationGrid>(Map.Width, Map.Height);

    Router = std::make_unique<Nav::MNavRouter>(*NavigationGrid);
    Rng.Reset(Setup.Seed);
    // Distinct sequence constant gives the intel layer an independent stream from
    // the same match seed (see SimWorld.h for why isolation matters).
    ReconRng.Reset(Setup.Seed, 0x496e74656cULL /* "Recon" */);
    ReconSettingsRef = InReconSettings;
    ReconLayer.Initialize(InReconSettings, Map.Width, Map.Height);

    for (PlayerId I = 0; I < kMaxPlayers; ++I)
    {
        const MatchSetup::PlayerSlot& Slot = Setup.Players[I];
        PlayerState& P = Players[I];
        P.bActive = Slot.bActive;
        P.bDefeated = false;
        P.Team = Slot.Team;
        P.Faction = Slot.Faction;
        P.Credits = Slot.StartingCredits;

        switch (P.Faction)
        {
        case FactionId::Soviet: P.FactionResourceType = FactionResourceType::Mobilization; break;
        case FactionId::Alliance: P.FactionResourceType = FactionResourceType::Intelligence; break;
        case FactionId::EasternCoalition: P.FactionResourceType = FactionResourceType::Synchronization; break;
        case FactionId::ChronoLegion: P.FactionResourceType = FactionResourceType::TemporalStability; break;
        default: P.FactionResourceType = FactionResourceType::None; break;
        }
    }

    Phase = MatchPhase::Running;
}

void SimWorld::Restart()
{
    const ContentDatabase* SavedContent = Content;
    const MatchSetup SavedSetup = SetupConfig;
    const Recon::ReconSettings* SavedRecon = ReconSettingsRef;
    Initialize(SavedContent, SavedSetup, SavedRecon);
}

bool SimWorld::HasConsistentComponentStorage() const
{
    const size_t Count = Core.size();
    return Transforms.size() == Count && Healths.size() == Count && Movements.size() == Count
        && Combats.size() == Count && AircraftRearming.size() == Count
        && Buildings.size() == Count && Harvesters.size() == Count
        && ResourceNodes.size() == Count && Projectiles.size() == Count && Orders.size() == Count
        && DirectControls.size() == Count && Morales.size() == Count && Statuses.size() == Count
        && Transports.size() == Count && PassengerOf.size() == Count && Captures.size() == Count;
}

// ---------------------------------------------------------------------------
// Entity storage
// ---------------------------------------------------------------------------

EntityId SimWorld::AllocateEntity()
{
    uint32_t Index;
    if (!FreeSlots.empty())
    {
        Index = FreeSlots.back();
        FreeSlots.pop_back();
    }
    else
    {
        if (Core.size() >= kMaxEntities)
        {
            return EntityId::Invalid();
        }
        Index = uint32_t(Core.size());
        Core.emplace_back();
        Transforms.emplace_back();
        Healths.emplace_back();
        Movements.emplace_back();
        Combats.emplace_back();
        AircraftRearming.emplace_back(0);
        Buildings.emplace_back();
        Harvesters.emplace_back();
        ResourceNodes.emplace_back();
        Projectiles.emplace_back();
        Orders.emplace_back();
        DirectControls.emplace_back();
        Morales.emplace_back();
        Statuses.emplace_back();
        Transports.emplace_back();
        PassengerOf.emplace_back();
        Captures.emplace_back();
        HighWaterMark = uint32_t(Core.size());
    }

    // Reset every component: a recycled slot must be indistinguishable from a fresh
    // one, or state leaks between unrelated entities and desyncs the match.
    const uint32_t Generation = Core[Index].Generation;
    Core[Index] = EntityCore();
    Core[Index].Generation = Generation;
    Core[Index].bAlive = true;
    Transforms[Index] = TransformComp();
    Healths[Index] = HealthComp();
    Movements[Index] = MovementComp();
    Combats[Index] = CombatComp();
    AircraftRearming[Index] = 0;
    Buildings[Index] = BuildingComp();
    Harvesters[Index] = HarvesterComp();
    ResourceNodes[Index] = ResourceNodeComp();
    Projectiles[Index] = ProjectileComp();
    Orders[Index].Clear();
    DirectControls[Index] = DirectControlComp();
    Morales[Index] = Recon::MoraleComp();
    Statuses[Index] = StatusComp();
    Transports[Index] = TransportComp();
    PassengerOf[Index] = PassengerComp();
    Captures[Index] = CaptureComp();
 
    return EntityId(Index, Generation);
}

EntityId SimWorld::MakeId(uint32_t Index) const
{
    if (Index >= Core.size())
    {
        return EntityId::Invalid();
    }
    return EntityId(Index, Core[Index].Generation);
}

bool SimWorld::IsAlive(EntityId Id) const
{
    return Id.IsValid() && Id.Index < Core.size() && Core[Id.Index].bAlive &&
           Core[Id.Index].Generation == Id.Generation;
}

const EntityCore* SimWorld::GetCore(EntityId Id) const { return IsAlive(Id) ? &Core[Id.Index] : nullptr; }
const TransformComp* SimWorld::GetTransform(EntityId Id) const { return IsAlive(Id) ? &Transforms[Id.Index] : nullptr; }
const HealthComp* SimWorld::GetHealth(EntityId Id) const { return IsAlive(Id) ? &Healths[Id.Index] : nullptr; }
// The kind-specific accessors check Kind as well as liveness. Every entity owns a slot
// in every component vector, so without that check these return a valid pointer to a
// default-constructed component for the wrong kind -- and BuildingComp defaults to
// ConstructionState::Complete, so a caller using `GetBuilding(Id) != nullptr` as a
// "is this a building" test sees a live, finished building wherever a unit stands. The
// HUD did exactly that and offered a repair button on a tank.
const BuildingComp* SimWorld::GetBuilding(EntityId Id) const
{
    return (IsAlive(Id) && Core[Id.Index].Kind == EntityKind::Building) ? &Buildings[Id.Index] : nullptr;
}
const HarvesterComp* SimWorld::GetHarvester(EntityId Id) const
{
    return (IsAlive(Id) && Core[Id.Index].Kind == EntityKind::Unit) ? &Harvesters[Id.Index] : nullptr;
}
const ResourceNodeComp* SimWorld::GetResourceNode(EntityId Id) const
{
    return (IsAlive(Id) && Core[Id.Index].Kind == EntityKind::ResourceNode) ? &ResourceNodes[Id.Index] : nullptr;
}
const MovementComp* SimWorld::GetMovement(EntityId Id) const { return IsAlive(Id) ? &Movements[Id.Index] : nullptr; }
const CombatComp* SimWorld::GetCombat(EntityId Id) const { return IsAlive(Id) ? &Combats[Id.Index] : nullptr; }
const OrderQueue* SimWorld::GetOrders(EntityId Id) const { return IsAlive(Id) ? &Orders[Id.Index] : nullptr; }
const StatusComp* SimWorld::GetStatus(EntityId Id) const { return IsAlive(Id) ? &Statuses[Id.Index] : nullptr; }
const TransportComp* SimWorld::GetTransport(EntityId Id) const { return IsAlive(Id) ? &Transports[Id.Index] : nullptr; }
const PassengerComp* SimWorld::GetPassengerOf(EntityId Id) const { return IsAlive(Id) ? &PassengerOf[Id.Index] : nullptr; }
const DirectControlComp* SimWorld::GetDirectControl(EntityId Id) const { return IsAlive(Id) ? &DirectControls[Id.Index] : nullptr; }

const PlayerState& SimWorld::GetPlayer(PlayerId Id) const
{
    static const PlayerState Empty;
    return Id < kMaxPlayers ? Players[Id] : Empty;
}

int32_t SimWorld::GetConstructionProgressPerMille(EntityId Id) const
{
    const BuildingComp* B = GetBuilding(Id);
    if (B == nullptr || B->State != ConstructionState::UnderConstruction)
    {
        return 1000;
    }
    const int64_t Total = int64_t(B->ConstructionTotalTicks) * kProgressScale;
    if (Total <= 0)
    {
        return 1000;
    }
    const int64_t Clamped = std::min<int64_t>(std::max<int64_t>(B->ConstructionProgressTicks, 0), Total);
    return int32_t((Clamped * 1000) / Total);
}

// ---------------------------------------------------------------------------
// Spawning
// ---------------------------------------------------------------------------

EntityId SimWorld::SpawnUnit(ContentId Def, PlayerId Owner, const Vec2& Position, int32_t Facing)
{
    const EntityDef* D = Content ? Content->FindEntity(Def) : nullptr;
    if (D == nullptr || D->Kind != EntityKind::Unit)
    {
        return EntityId::Invalid();
    }

    const EntityId Id = AllocateEntity();
    if (!Id.IsValid())
    {
        return Id;
    }

    Core[Id.Index].Def = Def;
    Core[Id.Index].Kind = EntityKind::Unit;
    Core[Id.Index].Owner = Owner;

    Transforms[Id.Index].Position = Position;
    Transforms[Id.Index].Facing = WrapAngle(Facing);
    Transforms[Id.Index].TurretFacing = WrapAngle(Facing);

    Healths[Id.Index].Max = D->MaxHealth;
    Healths[Id.Index].Current = D->MaxHealth;

    Movements[Id.Index].ArriveRadius = FxMax(D->Unit.CollisionRadius, Fixed::FromInt(25));

    // Air logistics: aircraft spawn with a full magazine; every other layer
    // keeps the 0/0 infinite-magazine default. Uniform fields, content-derived
    // capacity -- no per-unit branching anywhere else in the simulation.
    if (D->Unit.Layer == MovementLayer::Air)
    {
        Combats[Id.Index].AmmoMax = kAircraftDefaultAmmo;
        Combats[Id.Index].AmmoCurrent = kAircraftDefaultAmmo;
    }

    if (D->Unit.bIsHarvester)
    {
        Harvesters[Id.Index].State = HarvesterState::Idle;
    }

    if (Owner < kMaxPlayers)
    {
        Players[Owner].UnitsBuilt += 1;
    }

    SimEvent Ev;
    Ev.Type = SimEventType::EntitySpawned;
    Ev.Tick = CurrentTick;
    Ev.Entity = Id;
    Ev.Player = Owner;
    Ev.Content = Def;
    Ev.Location = Position;
    EmitEvent(Ev);

    return Id;
}

EntityId SimWorld::SpawnBuilding(ContentId Def, PlayerId Owner, const TileCoord& OriginTile, bool bInstantComplete)
{
    const EntityDef* D = Content ? Content->FindEntity(Def) : nullptr;
    if (D == nullptr || D->Kind != EntityKind::Building)
    {
        return EntityId::Invalid();
    }

    const EntityId Id = AllocateEntity();
    if (!Id.IsValid())
    {
        return Id;
    }

    Core[Id.Index].Def = Def;
    Core[Id.Index].Kind = EntityKind::Building;
    Core[Id.Index].Owner = Owner;

    BuildingComp& B = Buildings[Id.Index];
    B.OriginTile = OriginTile;
    B.FootprintX = D->Building.FootprintX;
    B.FootprintY = D->Building.FootprintY;
    B.State = bInstantComplete ? ConstructionState::Complete : ConstructionState::UnderConstruction;
    B.ConstructionTotalTicks = std::max(1, D->Production.BuildTimeTicks);
    B.ConstructionProgressTicks = bInstantComplete ? B.ConstructionTotalTicks * kProgressScale : 0;
    // ADR-0013: seeded from the definition, then owned by the player. An override is a
    // command, so it must not be recomputed from content on any later tick.
    B.Priority = DefaultPowerPriorityFor(*D);

    // Position is the footprint centre so that range checks against a 3x3 factory
    // behave the same as against a 1x1 turret.
    const Vec2 Corner = Map.TileCenterToWorld(OriginTile);
    Transforms[Id.Index].Position =
        Vec2(Corner.X + Fixed::FromInt((int64_t(B.FootprintX) - 1) * kTileSizeUnits / 2),
             Corner.Y + Fixed::FromInt((int64_t(B.FootprintY) - 1) * kTileSizeUnits / 2));

    Healths[Id.Index].Max = D->MaxHealth;
    // A building under construction starts at 10% health and is finished by the
    // construction system; killing it mid-build is cheap, as it should be.
    Healths[Id.Index].Current = bInstantComplete ? D->MaxHealth : std::max(1, D->MaxHealth / 10);

    OccupyTiles(B, true);

    if (Owner < kMaxPlayers)
    {
        Players[Owner].BuildingsBuilt += 1;
    }

    SimEvent Ev;
    Ev.Type = SimEventType::BuildingPlaced;
    Ev.Tick = CurrentTick;
    Ev.Entity = Id;
    Ev.Player = Owner;
    Ev.Content = Def;
    Ev.Location = Transforms[Id.Index].Position;
    EmitEvent(Ev);

    if (bInstantComplete)
    {
        RefreshPlayerTech(Owner);
        SimEvent Done = Ev;
        Done.Type = SimEventType::BuildingCompleted;
        EmitEvent(Done);

        if (D->Building.BundledUnit.IsValid())
        {
            const Vec2 SpawnAt = FindFreeSpawnPoint(B, D->Building.BundledUnit);
            SpawnUnit(D->Building.BundledUnit, Owner, SpawnAt);
        }
    }

    return Id;
}

EntityId SimWorld::SpawnResourceNode(ContentId Def, const TileCoord& Tile, int32_t Amount)
{
    const ResourceNodeDef* D = Content ? Content->FindResourceNode(Def) : nullptr;
    if (D == nullptr)
    {
        return EntityId::Invalid();
    }

    const EntityId Id = AllocateEntity();
    if (!Id.IsValid())
    {
        return Id;
    }

    Core[Id.Index].Def = Def;
    Core[Id.Index].Kind = EntityKind::ResourceNode;
    Core[Id.Index].Owner = kNeutralPlayer;
    Transforms[Id.Index].Position = Map.TileCenterToWorld(Tile);
    ResourceNodes[Id.Index].Amount = Amount > 0 ? Amount : D->InitialAmount;
    ResourceNodes[Id.Index].Def = Def;
    Healths[Id.Index].Max = 1;
    Healths[Id.Index].Current = 1;
    Healths[Id.Index].bInvulnerable = true;

    Map.SetTileFlag(Tile.X, Tile.Y, Tile_Resource, true);

    return Id;
}

// ADR-0013. The single place that turns a power ratio into a speed, so that the
// construction and production systems cannot disagree about what "Moderate" costs.
const char* ToString(PowerTier Tier)
{
    switch (Tier)
    {
        case PowerTier::Normal:   return "Normal";
        case PowerTier::Mild:     return "Mild";
        case PowerTier::Moderate: return "Moderate";
        case PowerTier::Severe:   return "Severe";
        case PowerTier::Critical: return "Critical";
    }
    return "Unknown";
}

// ADR-0013. The single place that turns a power ratio into a speed, so that the
// construction and production systems cannot disagree about what "Moderate" costs.
int32_t SimWorld::PowerSpeedPercent(PlayerId Owner) const
{
    if (Owner >= kMaxPlayers)
    {
        return 100;
    }
    const PlayerState& P = Players[Owner];
    const int32_t Ratio = P.GetPowerRatioPercent();
    return PowerSpeedPercentForTier(PowerTierForRatio(Ratio), Ratio);
}

// At Critical the base is effectively dark: only infantry production and harvesting
// continue. Membership is decided from what the building can actually produce rather
// than from its name, so a faction that calls its barracks something else, or a mod
// that adds a second infantry building, needs no code change here.
const char* ToString(PowerPriority Priority)
{
    switch (Priority)
    {
        case PowerPriority::Vital:      return "Vital";
        case PowerPriority::Production: return "Production";
        case PowerPriority::Defense:    return "Defense";
        case PowerPriority::Auxiliary:  return "Auxiliary";
    }
    return "Unknown";
}

// ADR-0013 default power priority, read from what the building *is* rather than from a
// per-faction name table -- a mod adding a second refinery should inherit Vital without
// touching this code.
PowerPriority SimWorld::DefaultPowerPriorityFor(const EntityDef& Def) const
{
    const BuildingInfo& B = Def.Building;

    // Radar first: it is the one thing the player should lose earliest, and a radar
    // that also happens to provide a build radius must not be captured by a later rule.
    if (B.bIsRadar)
    {
        return PowerPriority::Auxiliary;
    }

    // Vital is everything a base needs to climb back out of a deficit: the yard that
    // builds a reactor, the reactor itself, the refinery that funds both, and the
    // infantry producer that ADR-0013's Critical rule keeps running. Letting these
    // degrade would turn a shortage into a death spiral -- and for the yard and the
    // barracks it would directly contradict the Critical carve-out, which exists so a
    // blacked-out base can rebuild.
    //
    // Deliberately not keyed on EntityRole::BaseBuilding: the default content sets that
    // on *every* building including turrets and factories, so using it here made almost
    // the whole base Vital and the priority table meaningless. Only the specific roles
    // discriminate.
    if (B.bIsConstructionYard || B.bIsRefinery || B.bIsPowerPlant ||
        HasRole(Def.Roles, EntityRole::Refinery) || HasRole(Def.Roles, EntityRole::Power))
    {
        return PowerPriority::Vital;
    }

    // Anything the Critical rule keeps running must be Vital, or the two rules
    // contradict each other: a Production-priority barracks would be forced offline by
    // its band at exactly the tier the carve-out says it should still work. Asking the
    // same function both rules use keeps them from drifting apart.
    //
    // The constraint this places on content: a building becomes Vital the moment it can
    // produce anything of Infantry category. Giving a war factory an infantry-class
    // product -- battle armour, a cyborg -- would silently promote it out of the
    // Critical shutdown. That is a content decision, not a code one, and
    // PowerPriority.DefaultsComeFromWhatABuildingIsNotFromItsName pins the shipped
    // answer so such an edit fails a test rather than passing unnoticed.
    if (ProducerRunsAtCriticalPower(Def))
    {
        return PowerPriority::Vital;
    }

    if (Def.Production.Category == ProductionCategory::Defense ||
        HasRole(Def.Roles, EntityRole::Defense))
    {
        return PowerPriority::Defense;
    }
    return PowerPriority::Production;
}

bool SimWorld::ProducerRunsAtCriticalPower(const EntityDef& Def) const
{
    if (Content == nullptr)
    {
        return false;
    }
    // A construction yard is included so a blacked-out base can still queue the power
    // plant that ends the blackout. Excluding it would be a deadlock: no power means
    // no yard output, and no yard output means no power.
    if (Def.Building.bIsConstructionYard)
    {
        return true;
    }
    for (const EntityDef& Product : Content->GetEntities())
    {
        if (Product.Production.Category != ProductionCategory::Infantry)
        {
            continue;
        }
        if (std::find(Product.Production.ProducedBy.begin(), Product.Production.ProducedBy.end(),
                      Def.Id) != Product.Production.ProducedBy.end())
        {
            return true;
        }
    }
    return false;
}

// ADR-0013. Whether the power state has stopped this building's queue head, for any
// of the reasons in the tier table. Deliberately one function: SystemFlowPayment and
// SystemProduction previously each decided this for themselves, and they disagreed --
// payment charged a slice every tick for a vehicle that production refused to advance
// at Critical, so the item froze at zero progress while draining the treasury that
// should have finished the power plant. The base then stayed dark forever, which is
// exactly the deadlock the Critical carve-out was written to prevent.
bool SimWorld::IsProductionPowerStalled(uint32_t BuildingIndex) const
{
    if (BuildingIndex >= Core.size() || Core[BuildingIndex].Owner >= kMaxPlayers)
    {
        return false;
    }
    const BuildingComp& B = Buildings[BuildingIndex];
    if (B.Queue.empty())
    {
        return false;
    }
    const PowerTier Tier = Players[Core[BuildingIndex].Owner].GetPowerTier();

    // ADR-0013 priority. A Vital building never goes offline whatever the tier, which
    // is what keeps a deficit recoverable; everything else has a tier at which it does.
    // This is checked before the tier shortcut below, because an Auxiliary building is
    // offline from Moderate onward -- a band that otherwise only slows things down.
    if (IsPowerPriorityOffline(B.Priority, Tier))
    {
        return true;
    }
    if (Tier < PowerTier::Severe)
    {
        return false;   // Normal, Mild and Moderate slow production; they never stop it
    }

    // Severe and Critical pause high tech outright rather than merely slowing it.
    const EntityDef* HeadDef = Content ? Content->FindEntity(B.Queue.front().Content) : nullptr;
    if (HeadDef != nullptr && HeadDef->Production.Tier >= TechTier::T2)
    {
        return true;
    }

    // At Critical, everything except infantry production and the construction yard
    // stops regardless of tier.
    if (Tier == PowerTier::Critical)
    {
        const EntityDef* ProducerDef = Content ? Content->FindEntity(Core[BuildingIndex].Def) : nullptr;
        return ProducerDef == nullptr || !ProducerRunsAtCriticalPower(*ProducerDef);
    }
    return false;
}

void SimWorld::OccupyTiles(const BuildingComp& B, bool bOccupy)
{
    if (NavigationGrid != nullptr)
    {
        NavigationGrid->BeginTopologyUpdate();
    }
    for (int32_t Y = 0; Y < B.FootprintY; ++Y)
    {
        for (int32_t X = 0; X < B.FootprintX; ++X)
        {
            const TileCoord Tile(B.OriginTile.X + X, B.OriginTile.Y + Y);
            Map.SetTileFlag(Tile.X, Tile.Y, Tile_Occupied, bOccupy);
            if (NavigationGrid != nullptr)
            {
                NavigationGrid->SetPassability(Tile, GetNavigationPassability(Tile));
            }
        }
    }
    if (NavigationGrid != nullptr)
    {
        NavigationGrid->EndTopologyUpdate();
    }

    // When placing/deploying a building, displace any units caught inside its footprint out to the perimeter
    if (bOccupy)
    {
        for (uint32_t U = 0; U < uint32_t(Core.size()); ++U)
        {
            if (Core[U].bAlive && Core[U].Kind == EntityKind::Unit)
            {
                const TileCoord UnitTile = Map.WorldToTile(Transforms[U].Position);
                if (UnitTile.X >= B.OriginTile.X && UnitTile.X < B.OriginTile.X + B.FootprintX &&
                    UnitTile.Y >= B.OriginTile.Y && UnitTile.Y < B.OriginTile.Y + B.FootprintY)
                {
                    const Vec2 FreePos = FindFreeSpawnPoint(B, Core[U].Def);
                    Transforms[U].Position = FreePos;
                }
            }
        }
    }
}

void SimWorld::BuildNavigationGrid()
{
    NavigationGrid = std::make_unique<Nav::NavGrid>(Map.Width, Map.Height);
    FlowFieldCache.clear();
    if (Router) Router->InvalidateAll();
    NavigationGrid->BeginTopologyUpdate();
    for (int32_t Y = 0; Y < Map.Height; ++Y)
    {
        for (int32_t X = 0; X < Map.Width; ++X)
        {
            const TileCoord Tile(X, Y);
            NavigationGrid->SetPassability(Tile, GetNavigationPassability(Tile));
        }
    }
    NavigationGrid->EndTopologyUpdate();
}

uint8_t SimWorld::GetNavigationPassability(const TileCoord& Tile) const
{
    if (!Map.IsInBounds(Tile.X, Tile.Y))
    {
        return Nav::NavLayer_None;
    }

    const uint8_t Flags = Map.GetTile(Tile.X, Tile.Y);
    if ((Flags & Tile_Cliff) != 0)
    {
        return Nav::NavLayer_Air;
    }
    if ((Flags & Tile_Water) != 0)
    {
        return Nav::NavLayer_Amphibious | Nav::NavLayer_Naval | Nav::NavLayer_Air;
    }
    if ((Flags & Tile_GroundPassable) == 0)
    {
        return Nav::NavLayer_Air;
    }

    uint8_t Passability = Nav::NavLayer_Infantry | Nav::NavLayer_Wheeled | Nav::NavLayer_Tracked |
                          Nav::NavLayer_Amphibious | Nav::NavLayer_Air;
    if ((Flags & Tile_Occupied) != 0)
    {
        Passability = Nav::NavLayer_Air;
    }
    return Passability;
}

Nav::NavQuery SimWorld::MakeNavigationQuery(const EntityDef& Def) const
{
    Nav::NavQuery Query;
    switch (Def.Unit.Layer)
    {
        case MovementLayer::Infantry: Query.LayerMask = Nav::NavLayer_Infantry; break;
        case MovementLayer::Wheeled: Query.LayerMask = Nav::NavLayer_Wheeled; break;
        case MovementLayer::Tracked: Query.LayerMask = Nav::NavLayer_Tracked; break;
        case MovementLayer::Amphibious: Query.LayerMask = Nav::NavLayer_Amphibious; break;
        case MovementLayer::Naval: Query.LayerMask = Nav::NavLayer_Naval; break;
        case MovementLayer::Air: Query.LayerMask = Nav::NavLayer_Air; break;
        case MovementLayer::None:
        case MovementLayer::Count: Query.LayerMask = Nav::NavLayer_None; break;
    }

    const int64_t Radius = std::max<int64_t>(0, Def.Unit.CollisionRadius.ToIntRound());
    const int64_t Diameter = Radius * 2;
    const int64_t Required = std::max<int64_t>(1, (Diameter + kTileSizeUnits - 1) / kTileSizeUnits);
    Query.RequiredClearance = static_cast<uint8_t>(std::min<int64_t>(Required, 255));
    return Query;
}

TileCoord SimWorld::ResolveNavigationTarget(const TileCoord& Desired, const Nav::NavQuery& Query) const
{
    if (NavigationGrid == nullptr || NavigationGrid->IsTraversable(Desired, Query))
    {
        return Desired;
    }

    const int32_t MaxRadius = std::max(Map.Width, Map.Height);
    for (int32_t Radius = 1; Radius <= MaxRadius; ++Radius)
    {
        const int32_t MinX = Desired.X - Radius;
        const int32_t MaxX = Desired.X + Radius;
        const int32_t MinY = Desired.Y - Radius;
        const int32_t MaxY = Desired.Y + Radius;
        for (int32_t Y = MinY; Y <= MaxY; ++Y)
        {
            for (int32_t X = MinX; X <= MaxX; ++X)
            {
                if (X != MinX && X != MaxX && Y != MinY && Y != MaxY)
                {
                    continue;
                }
                const TileCoord Candidate(X, Y);
                if (NavigationGrid->IsTraversable(Candidate, Query))
                {
                    return Candidate;
                }
            }
        }
    }
    return Desired;
}

const Nav::FlowField* SimWorld::GetFlowField(const TileCoord& Target, const Nav::NavQuery& Query)
{
    for (FlowFieldCacheEntry& Entry : FlowFieldCache)
    {
        if (Entry.Target == Target &&
            Entry.Query.LayerMask == Query.LayerMask &&
            Entry.Query.RequiredClearance == Query.RequiredClearance &&
            Entry.TopologyRevision == NavigationGrid->GetTopologyRevision())
        {
            Entry.LastUsedTick = CurrentTick;
            return Entry.Field.get();
        }
    }
    // Budget: only build if we haven't built too many this tick.
    if (FlowFieldBuildsThisTick >= kMaxFlowFieldBuildsPerTick)
    {
        // Return the most-recent field in the cache as a best-effort stale guide.
        // If the cache is empty, the caller will treat null as "blocked" and retry
        // next tick -- bounded latency, no crash, deterministic.
        if (!FlowFieldCache.empty())
        {
            return FlowFieldCache.back().Field.get();
        }
        return nullptr;
    }
    constexpr size_t kMaxCachedFlowFields = 64;
    if (FlowFieldCache.size() >= kMaxCachedFlowFields)
    {
        size_t EvictionIndex = 0;
        for (size_t I = 1; I < FlowFieldCache.size(); ++I)
        {
            const FlowFieldCacheEntry& Candidate = FlowFieldCache[I];
            const FlowFieldCacheEntry& Best = FlowFieldCache[EvictionIndex];
            if (Candidate.LastUsedTick < Best.LastUsedTick) EvictionIndex = I;
        }
        FlowFieldCache.erase(FlowFieldCache.begin() + static_cast<std::ptrdiff_t>(EvictionIndex));
    }
    FlowFieldCacheEntry Entry;
    Entry.Target = Target;
    Entry.Query = Query;
    Entry.TopologyRevision = NavigationGrid->GetTopologyRevision();
    Entry.LastUsedTick = CurrentTick;
    Entry.Field = std::make_unique<Nav::FlowField>(*NavigationGrid, Query, Target);
    Entry.Field->Rebuild();
    FlowFieldCache.push_back(std::move(Entry));
    ++FlowFieldBuildsThisTick;
    ++Stats.FlowFieldBuilds;
    return FlowFieldCache.back().Field.get();
}

Vec2 SimWorld::FindFreeSpawnPoint(const BuildingComp& Producer, ContentId /*UnitDef*/) const
{
    // Search rings outward from building footprint for the closest passable tile without other units sitting on it
    for (int32_t Ring = 1; Ring <= 4; ++Ring)
    {
        const int32_t MinX = Producer.OriginTile.X - Ring;
        const int32_t MaxX = Producer.OriginTile.X + Producer.FootprintX - 1 + Ring;
        const int32_t MinY = Producer.OriginTile.Y - Ring;
        const int32_t MaxY = Producer.OriginTile.Y + Producer.FootprintY - 1 + Ring;

        for (int32_t Y = MinY; Y <= MaxY; ++Y)
        {
            for (int32_t X = MinX; X <= MaxX; ++X)
            {
                const bool bOnRing = (X == MinX || X == MaxX || Y == MinY || Y == MaxY);
                if (!bOnRing)
                {
                    continue;
                }
                const uint8_t T = Map.GetTile(X, Y);
                if ((T & Tile_GroundPassable) != 0 && (T & Tile_Occupied) == 0 && (T & (Tile_Water | Tile_Cliff)) == 0)
                {
                    const Vec2 CandPos = Map.TileCenterToWorld(TileCoord(X, Y));
                    bool bUnitNearby = false;
                    for (size_t U = 0; U < Core.size() && U < Transforms.size(); ++U)
                    {
                        if (Core[U].bAlive && Core[U].Kind == EntityKind::Unit)
                        {
                            if (DistanceSquared(Transforms[U].Position, CandPos) < Fixed::FromInt(70 * 70))
                            {
                                bUnitNearby = true;
                                break;
                            }
                        }
                    }
                    if (!bUnitNearby)
                    {
                        return CandPos;
                    }
                }
            }
        }
    }

    // Fallback: search any passable tile on ring 1..3
    for (int32_t Ring = 1; Ring <= 3; ++Ring)
    {
        const int32_t MinX = Producer.OriginTile.X - Ring;
        const int32_t MaxX = Producer.OriginTile.X + Producer.FootprintX - 1 + Ring;
        const int32_t MinY = Producer.OriginTile.Y - Ring;
        const int32_t MaxY = Producer.OriginTile.Y + Producer.FootprintY - 1 + Ring;

        for (int32_t Y = MinY; Y <= MaxY; ++Y)
        {
            for (int32_t X = MinX; X <= MaxX; ++X)
            {
                const bool bOnRing = (X == MinX || X == MaxX || Y == MinY || Y == MaxY);
                if (!bOnRing) continue;
                const uint8_t T = Map.GetTile(X, Y);
                if ((T & Tile_GroundPassable) != 0 && (T & Tile_Occupied) == 0)
                {
                    return Map.TileCenterToWorld(TileCoord(X, Y));
                }
            }
        }
    }

    return Map.TileCenterToWorld(Producer.OriginTile);
}

// ---------------------------------------------------------------------------
// Tech and ownership helpers
// ---------------------------------------------------------------------------

void SimWorld::RefreshPlayerTech(PlayerId Owner)
{
    if (Owner >= kMaxPlayers)
    {
        return;
    }
    std::vector<ContentId>& Types = Players[Owner].CompletedBuildingTypes;
    Types.clear();
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner != Owner)
        {
            continue;
        }
        if (Buildings[I].State != ConstructionState::Complete)
        {
            continue;
        }
        if (std::find(Types.begin(), Types.end(), Core[I].Def) == Types.end())
        {
            Types.push_back(Core[I].Def);
        }
    }
    // Sorted so the checksum does not depend on entity slot order.
    std::sort(Types.begin(), Types.end());
}

bool SimWorld::HasPrerequisites(PlayerId Owner, const EntityDef& Def) const
{
    if (Owner >= kMaxPlayers)
    {
        return false;
    }
    const std::vector<ContentId>& Have = Players[Owner].CompletedBuildingTypes;

    // Legacy direct list / AllOf check
    for (const ContentId& Req : Def.Production.Prerequisites)
    {
        if (std::find(Have.begin(), Have.end(), Req) == Have.end())
        {
            return false;
        }
    }

    // Group-based AllOf check
    for (const ContentId& Req : Def.Production.PrerequisitesGroup.AllOf)
    {
        if (std::find(Have.begin(), Have.end(), Req) == Have.end())
        {
            return false;
        }
    }

    // Group-based AnyOf check
    if (!Def.Production.PrerequisitesGroup.AnyOf.empty())
    {
        bool bAnyMet = false;
        for (const ContentId& Req : Def.Production.PrerequisitesGroup.AnyOf)
        {
            if (std::find(Have.begin(), Have.end(), Req) != Have.end())
            {
                bAnyMet = true;
                break;
            }
        }
        if (!bAnyMet)
        {
            return false;
        }
    }

    // Group-based NoneOf check
    for (const ContentId& Forbidden : Def.Production.PrerequisitesGroup.NoneOf)
    {
        if (std::find(Have.begin(), Have.end(), Forbidden) != Have.end())
        {
            return false;
        }
    }

    return true;
}

bool SimWorld::IsHostile(PlayerId A, PlayerId B) const
{
    if (A == kInvalidPlayer || B == kInvalidPlayer) { return false; }
    if (A == B) { return false; }
    if (A == kNeutralPlayer || B == kNeutralPlayer) { return false; }
    if (A < kMaxPlayers && B < kMaxPlayers)
    {
        if (Players[A].Team != 0 && Players[A].Team == Players[B].Team)
        {
            return false;
        }
    }
    return true;
}


bool SimWorld::IsPlacementValid(ContentId BuildingDef, PlayerId Owner, const TileCoord& OriginTile) const
{
    const EntityDef* D = Content ? Content->FindEntity(BuildingDef) : nullptr;
    if (D == nullptr || D->Kind != EntityKind::Building)
    {
        return false;
    }

    for (int32_t Y = 0; Y < D->Building.FootprintY; ++Y)
    {
        for (int32_t X = 0; X < D->Building.FootprintX; ++X)
        {
            const int32_t TX = OriginTile.X + X;
            const int32_t TY = OriginTile.Y + Y;
            if (!Map.IsInBounds(TX, TY))
            {
                return false;
            }
            const uint8_t T = Map.GetTile(TX, TY);
            if ((T & (Tile_Cliff | Tile_Occupied | Tile_Resource)) != 0)
            {
                return false;
            }
            if (D->Building.bWaterOnly)
            {
                if ((T & Tile_Water) == 0) { return false; }
            }
            else if (D->Building.bAllowOnWater)
            {
                if ((T & (Tile_GroundPassable | Tile_Water)) == 0) { return false; }
            }
            else
            {
                if ((T & Tile_GroundPassable) == 0 || (T & Tile_Water) != 0) { return false; }
            }
        }
    }


    // Must sit inside the build radius of a completed structure that projects one.
    const Vec2 Centre = Map.TileCenterToWorld(OriginTile);
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner != Owner)
        {
            continue;
        }
        if (Buildings[I].State != ConstructionState::Complete)
        {
            continue;
        }
        const EntityDef* OtherDef = Content->FindEntity(Core[I].Def);
        if (OtherDef == nullptr || !OtherDef->Building.bProvidesBuildRadius)
        {
            continue;
        }
        const Fixed R = OtherDef->Building.BuildRadius;
        if (DistanceSquared(Centre, Transforms[I].Position) <= R * R)
        {
            return true;
        }
    }

    // If the owner holds no complete building, or is placing a Construction Yard, allow placement on clear ground
    bool bHasBase = false;
    for (uint32_t J = 0; J < Core.size(); ++J)
    {
        if (Core[J].bAlive && Core[J].Kind == EntityKind::Building && Core[J].Owner == Owner)
        {
            bHasBase = true;
            break;
        }
    }
    if (!bHasBase && D->Building.bProvidesBuildRadius)
    {
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

CommandResult SimWorld::ApplyCommand(const Command& Cmd)
{
    CommandResult Result;

    auto Reject = [&](CommandReject Reason)
    {
        Result.Reason = Reason;
        SimEvent Ev;
        Ev.Type = SimEventType::CommandRejected;
        Ev.Tick = CurrentTick;
        Ev.Player = Cmd.Issuer;
        Ev.Entity = Cmd.Primary;
        Ev.Content = Cmd.Content;
        Ev.Value = int32_t(Reason);
        EmitEvent(Ev);
        return Result;
    };

    if (Phase != MatchPhase::Running)
    {
        return Reject(CommandReject::MatchOver);
    }
    if (Cmd.Issuer >= kMaxPlayers || !Players[Cmd.Issuer].bActive || Players[Cmd.Issuer].bDefeated)
    {
        return Reject(CommandReject::NotOwner);
    }
    if (CommandsThisTick[Cmd.Issuer] >= kMaxCommandsPerPlayerPerTick)
    {
        return Reject(CommandReject::RateLimited);
    }
    CommandsThisTick[Cmd.Issuer] += 1;

    PlayerState& Player = Players[Cmd.Issuer];

    switch (Cmd.Type)
    {
        case CommandType::ResearchUpgrade:
        {
            // Routed through the producing building's queue as a ProductionItem
            // with Category::Ability, so flow payment, prerequisites and power
            // throttling all apply for free. The upgrade completes when the
            // queue item does, in SystemProduction.
            if (!IsAlive(Cmd.Primary) || Core[Cmd.Primary.Index].Owner != Cmd.Issuer
                || Core[Cmd.Primary.Index].Kind != EntityKind::Building)
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            const UpgradeDef* U = Content->FindUpgrade(Cmd.Content);
            if (U == nullptr)
            {
                return Reject(CommandReject::UnknownContent);
            }
            // Prerequisite buildings must be complete.
            for (const ContentId& Prereq : U->Prerequisites)
            {
                const EntityDef* PD = Content->FindEntity(Prereq);
                if (PD == nullptr || !HasPrerequisites(Cmd.Issuer, *PD))
                {
                    return Reject(CommandReject::TechRequirementsUnmet);
                }
            }
            bool bProducerOk = false;
            for (const ContentId& Prod : U->ProducedBy)
            {
                if (Prod == Core[Cmd.Primary.Index].Def) { bProducerOk = true; break; }
            }
            if (!bProducerOk)
            {
                return Reject(CommandReject::NoProducer);
            }
            // Already researched or already queued?
            const PlayerState& P = Players[Cmd.Issuer];
            for (const ContentId& Done : P.ResearchedUpgrades)
            {
                if (Done == Cmd.Content) { return Reject(CommandReject::TechRequirementsUnmet); }
            }
            BuildingComp& B = Buildings[Cmd.Primary.Index];
            for (const ProductionItem& Queued : B.Queue)
            {
                if (Queued.Content == Cmd.Content) { return Reject(CommandReject::QueueFull); }
            }
            ProductionItem Item;
            Item.Content = Cmd.Content;
            Item.TotalCost = U->Cost;
            Item.TotalTicks = U->BuildTimeTicks;
            Item.State = FlowPaymentState::Queued;
            Item.Priority = 0;
            if (int32_t(B.Queue.size()) >= kMaxProductionQueueLength)
            {
                return Reject(CommandReject::QueueFull);
            }
            B.Queue.push_back(Item);
            SimEvent Ev;
            Ev.Type = SimEventType::ProductionStarted;
            Ev.Tick = CurrentTick;
            Ev.Entity = Cmd.Primary;
            Ev.Player = Cmd.Issuer;
            Ev.Content = Cmd.Content;
            EmitEvent(Ev);
            return Result;
        }

        case CommandType::BoardTransport:
        case CommandType::UnloadTransport:
        case CommandType::CaptureBuilding:
        {
            if (!IsAlive(Cmd.Primary))
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            if (Core[Cmd.Primary.Index].Owner != Cmd.Issuer || Core[Cmd.Primary.Index].Kind != EntityKind::Unit)
            {
                return Reject(CommandReject::NotOwner);
            }
            Order O;
            if (Cmd.Type == CommandType::BoardTransport)
            {
                // Infantry boards a friendly transport of its own side.
                if (!IsAlive(Cmd.Target) || Core[Cmd.Target.Index].Owner != Cmd.Issuer
                    || Core[Cmd.Target.Index].Kind != EntityKind::Unit)
                {
                    return Reject(CommandReject::TargetInvalid);
                }
                const EntityDef* TD = Content->FindEntity(Core[Cmd.Target.Index].Def);
                if (TD == nullptr || Transports[Cmd.Target.Index].Passengers.size()
                                      >= size_t(std::max(0, TD->Unit.PassengerCapacity)))
                {
                    return Reject(CommandReject::TargetInvalid);
                }
                O.Type = OrderType::Board;
                O.Target = Cmd.Target;
            }
            else if (Cmd.Type == CommandType::UnloadTransport)
            {
                const EntityDef* TD = Content->FindEntity(Core[Cmd.Primary.Index].Def);
                if (TD == nullptr || Transports[Cmd.Primary.Index].Passengers.empty())
                {
                    return Reject(CommandReject::TargetInvalid);
                }
                O.Type = OrderType::Unload;
                O.Location = Transforms[Cmd.Primary.Index].Position;
            }
            else
            {
                // Engineer capture: the actor must be an engineer-class unit, and the
                // target is a building you do not already own.
                const EntityDef* ED = Content->FindEntity(Core[Cmd.Primary.Index].Def);
                if (ED == nullptr || !ED->Unit.bIsEngineer)
                {
                    return Reject(CommandReject::NotOwner);
                }
                if (!IsAlive(Cmd.Target) || Core[Cmd.Target.Index].Kind != EntityKind::Building
                    || Core[Cmd.Target.Index].Owner == Cmd.Issuer)
                {
                    return Reject(CommandReject::TargetInvalid);
                }
                O.Type = OrderType::Capture;
                O.Target = Cmd.Target;
            }
            OrderQueue& QO = Orders[Cmd.Primary.Index];
            if (Cmd.Mode == OrderMode::Replace)
            {
                QO.Clear();
            }
            if (!QO.Push(O))
            {
                return Reject(CommandReject::QueueFull);
            }
            return Result;
        }

        case CommandType::Move:
        case CommandType::AttackMove:
        case CommandType::Attack:
        case CommandType::Stop:
        case CommandType::Harvest:
        case CommandType::Guard:
        {
            if (!IsAlive(Cmd.Primary))
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            if (Core[Cmd.Primary.Index].Owner != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            if (Core[Cmd.Primary.Index].Kind != EntityKind::Unit)
            {
                return Reject(CommandReject::NoSuchEntity);
            }

            OrderQueue& Q = Orders[Cmd.Primary.Index];
            if (Cmd.Type == CommandType::Stop)
            {
                Q.Clear();
                Movements[Cmd.Primary.Index].bHasDestination = false;
                Movements[Cmd.Primary.Index].CurrentSpeed = Fixed::Zero();
                Combats[Cmd.Primary.Index].Target = EntityId::Invalid();
                Combats[Cmd.Primary.Index].bTargetIsForced = false;
                Harvesters[Cmd.Primary.Index].State = HarvesterState::Idle;
                break;
            }

            Order O;
            switch (Cmd.Type)
            {
                case CommandType::Move: O.Type = OrderType::Move; break;
                case CommandType::AttackMove: O.Type = OrderType::AttackMove; break;
                case CommandType::Attack: O.Type = OrderType::Attack; break;
                case CommandType::Harvest: O.Type = OrderType::Harvest; break;
                default: O.Type = OrderType::Guard; break;
            }
            O.Location = Cmd.Location;
            O.Target = Cmd.Target;

            if (O.Type == OrderType::Attack)
            {
                if (!IsAlive(Cmd.Target))
                {
                    return Reject(CommandReject::TargetInvalid);
                }
                if (!IsHostile(Cmd.Issuer, Core[Cmd.Target.Index].Owner))
                {
                    return Reject(CommandReject::TargetInvalid);
                }
            }

            if (Cmd.Mode == OrderMode::Replace)
            {
                Q.Clear();
                Combats[Cmd.Primary.Index].bTargetIsForced = false;
            }
            if (!Q.Push(O))
            {
                return Reject(CommandReject::QueueFull);
            }
            break;
        }

        case CommandType::SetRallyPoint:
        {
            if (!IsAlive(Cmd.Primary) || Core[Cmd.Primary.Index].Kind != EntityKind::Building)
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            if (Core[Cmd.Primary.Index].Owner != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            Buildings[Cmd.Primary.Index].RallyPoint = Cmd.Location;
            Buildings[Cmd.Primary.Index].bHasRallyPoint = true;
            break;
        }

        case CommandType::StartProduction:
        {
            const EntityDef* Item = Content ? Content->FindEntity(Cmd.Content) : nullptr;
            if (Item == nullptr)
            {
                return Reject(CommandReject::UnknownContent);
            }
            if (!HasPrerequisites(Cmd.Issuer, *Item))
            {
                return Reject(CommandReject::TechRequirementsUnmet);
            }

            // Locate the producer: either the building the player clicked, or the
            // first owned completed building of an allowed producer type.
            EntityId ProducerId = EntityId::Invalid();
            if (IsAlive(Cmd.Primary) && Core[Cmd.Primary.Index].Owner == Cmd.Issuer &&
                Core[Cmd.Primary.Index].Kind == EntityKind::Building &&
                Buildings[Cmd.Primary.Index].State == ConstructionState::Complete)
            {
                const ContentId PrimaryDef = Core[Cmd.Primary.Index].Def;
                if (std::find(Item->Production.ProducedBy.begin(), Item->Production.ProducedBy.end(), PrimaryDef) !=
                    Item->Production.ProducedBy.end())
                {
                    ProducerId = Cmd.Primary;
                }
            }
            if (!ProducerId.IsValid())
            {
                for (uint32_t I = 0; I < Core.size() && !ProducerId.IsValid(); ++I)
                {
                    if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner != Cmd.Issuer)
                    {
                        continue;
                    }
                    if (Buildings[I].State != ConstructionState::Complete)
                    {
                        continue;
                    }
                    if (std::find(Item->Production.ProducedBy.begin(), Item->Production.ProducedBy.end(),
                                  Core[I].Def) != Item->Production.ProducedBy.end())
                    {
                        ProducerId = MakeId(I);
                    }
                }
            }
            if (!ProducerId.IsValid())
            {
                return Reject(CommandReject::NoProducer);
            }

            BuildingComp& Producer = Buildings[ProducerId.Index];
            if (int32_t(Producer.Queue.size()) >= kMaxProductionQueueLength)
            {
                return Reject(CommandReject::QueueFull);
            }
            if (Item->Kind == EntityKind::Unit && Player.CommandLimitUsed + Item->Production.CommandLimit > Player.CommandLimitMax)
            {
                return Reject(CommandReject::CommandCapExceeded);
            }

            // ADR-0012: no upfront charge. Credits are drawn tick by tick in
            // SystemFlowPayment, so queuing something you cannot yet afford is a
            // legitimate plan rather than an error -- it simply funds slowly, or
            // waits in Starved until income arrives. Rejecting it here would make
            // the whole flow-payment model unobservable.
            ProductionItem QueueItem;
            QueueItem.Content = Cmd.Content;
            QueueItem.State = FlowPaymentState::Queued;
            QueueItem.TotalCost = Item->Production.Cost;
            QueueItem.PaidCredits = 0;
            QueueItem.TotalTicks = std::max(1, Item->Production.BuildTimeTicks);
            QueueItem.ProgressTicks = 0;
            QueueItem.Priority = 0;
            Producer.Queue.push_back(QueueItem);

            SimEvent Ev;
            Ev.Type = SimEventType::ProductionStarted;
            Ev.Tick = CurrentTick;
            Ev.Entity = ProducerId;
            Ev.Player = Cmd.Issuer;
            Ev.Content = Cmd.Content;
            EmitEvent(Ev);
            break;
        }

        case CommandType::CancelProduction:
        {
            if (!IsAlive(Cmd.Primary) || Core[Cmd.Primary.Index].Owner != Cmd.Issuer ||
                Core[Cmd.Primary.Index].Kind != EntityKind::Building)
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            BuildingComp& Producer = Buildings[Cmd.Primary.Index];
            const int32_t Slot = int32_t(Cmd.Slot);
            if (Slot < 0 || Slot >= int32_t(Producer.Queue.size()))
            {
                return Reject(CommandReject::QueueFull);
            }
            const ProductionItem& QueueItem = Producer.Queue[size_t(Slot)];
            const EntityDef* Item = Content->FindEntity(QueueItem.Content);
            // ADR-0012 supersedes ProductionInfo::CancelRefundPercent: the refund
            // is a function of how far the payment got, not a flat per-content
            // number, because under flow payment "cancelled" spans everything from
            // untouched-in-queue to one tick short of done.
            const int32_t Refund =
                FlowPaymentCancelRefund(QueueItem, Item ? Item->Kind : EntityKind::Unit);
            Player.Credits += Refund;

            SimEvent Ev;
            Ev.Type = SimEventType::ProductionCancelled;
            Ev.Tick = CurrentTick;
            Ev.Entity = Cmd.Primary;
            Ev.Player = Cmd.Issuer;
            Ev.Content = QueueItem.Content;
            Ev.Value = Refund;
            EmitEvent(Ev);

            Producer.Queue.erase(Producer.Queue.begin() + Slot);
            break;
        }

        case CommandType::PauseProduction:
        {
            if (!IsAlive(Cmd.Primary) || Core[Cmd.Primary.Index].Owner != Cmd.Issuer ||
                Core[Cmd.Primary.Index].Kind != EntityKind::Building)
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            BuildingComp& Producer = Buildings[Cmd.Primary.Index];
            if (Cmd.Slot < Producer.Queue.size())
            {
                ProductionItem& QueueItem = Producer.Queue[Cmd.Slot];
                // ADR-0012: unpausing returns the item to Queued rather than
                // straight to Funding, so SystemFlowPayment re-decides against
                // this tick's treasury and power instead of trusting a stale state.
                if (QueueItem.State == FlowPaymentState::ManuallyPaused)
                {
                    QueueItem.State = FlowPaymentState::Queued;
                    QueueItem.bPaused = false;
                }
                else
                {
                    QueueItem.State = FlowPaymentState::ManuallyPaused;
                    QueueItem.bPaused = true;
                }
            }
            break;
        }

        case CommandType::Deploy:
        {
            if (Cmd.Primary == EntityId::Invalid() || Cmd.Primary.Index >= Core.size())
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            const uint32_t I = Cmd.Primary.Index;
            if (!Core[I].bAlive || Core[I].Owner != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            const EntityDef* Def = Content ? Content->FindEntity(Core[I].Def) : nullptr;
            if (Def == nullptr)
            {
                return Reject(CommandReject::UnknownContent);
            }

            // Case A: MCV -> Construction Yard (Deploy)
            if (Core[I].Kind == EntityKind::Unit && Def->Unit.bIsBuilder && Def->Unit.DeploysInto.IsValid())
            {
                TileCoord TargetTile = Map.WorldToTile(Transforms[I].Position);
                if (Cmd.Tile.X > 0 || Cmd.Tile.Y > 0)
                {
                    TargetTile = Cmd.Tile;
                }

                const Vec2 TargetLoc = Map.TileCenterToWorld(TargetTile);
                const Fixed DistSq = DistanceSquared(Transforms[I].Position, TargetLoc);
                constexpr int64_t kArrivalDistUnits = 250; // ~1.25 tiles

                Orders[I].Clear();

                if (DistSq > Fixed::FromInt(kArrivalDistUnits) * Fixed::FromInt(kArrivalDistUnits))
                {
                    // The MCV is far from the target location: MOVE THERE FIRST!
                    Order MoveOrder;
                    MoveOrder.Type = OrderType::Move;
                    MoveOrder.Location = TargetLoc;
                    Orders[I].Push(MoveOrder);
                }

                Order DeployOrder;
                DeployOrder.Type = OrderType::Deploy;
                DeployOrder.Location = TargetLoc;
                Orders[I].Push(DeployOrder);
                break;
            }

            // Case B: Construction Yard -> MCV (Undeploy / Pack)
            if (Core[I].Kind == EntityKind::Building && Def->Name.find("construction_yard") != std::string::npos)
            {
                ContentId McvDefId;
                const auto& AllEntities = Content->GetEntities();
                for (const auto& E : AllEntities)
                {
                    if (E.Kind == EntityKind::Unit && E.Unit.bIsBuilder && E.Unit.DeploysInto == Core[I].Def)
                    {
                        McvDefId = E.Id;
                        break;
                    }
                }

                if (!McvDefId.IsValid())
                {
                    const PlayerState& P = GetPlayer(Cmd.Issuer);
                    std::string McvName = (P.Faction == FactionId::Alliance) ? "unit.all.mcv" : "unit.sov.mcv";
                    if (const auto* McvDef = Content->FindEntityByName(McvName))
                    {
                        McvDefId = McvDef->Id;
                    }
                }

                if (!McvDefId.IsValid())
                {
                    return Reject(CommandReject::UnknownContent);
                }

                const Vec2 ConYardLoc = Transforms[I].Position;

                // Silently remove Construction Yard without "Building Lost" event
                RemoveEntitySilently(Cmd.Primary);

                // Spawn MCV vehicle
                EntityId NewUnit = SpawnUnit(McvDefId, Cmd.Issuer, ConYardLoc);

                // Emit MCVUndeployed event
                SimEvent Ev;
                Ev.Type = SimEventType::MCVUndeployed;
                Ev.Tick = CurrentTick;
                Ev.Entity = NewUnit;
                Ev.Player = Cmd.Issuer;
                Ev.Content = McvDefId;
                Ev.Location = ConYardLoc;
                EmitEvent(Ev);
                break;
            }

            return Reject(CommandReject::TargetInvalid);
        }

        case CommandType::ToggleSecondaryAbility:
        {
            if (Cmd.Primary == EntityId::Invalid() || Cmd.Primary.Index >= Core.size())
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            const uint32_t I = Cmd.Primary.Index;
            if (!Core[I].bAlive || Core[I].Owner != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            const EntityDef* Def = Content ? Content->FindEntity(Core[I].Def) : nullptr;
            if (Def == nullptr || !Def->Unit.bHasSecondaryAbility)
            {
                return Reject(CommandReject::UnknownContent);
            }

            if (Combats[I].SecondaryAbilityCooldownTicks > 0)
            {
                return Reject(CommandReject::OnCooldown);
            }


            Combats[I].bSecondaryModeActive = !Combats[I].bSecondaryModeActive;
            if (Combats[I].bSecondaryModeActive)
            {
                if (Def->Unit.AbilityDurationTicks > 0)
                {
                    Combats[I].SecondaryAbilityDurationTicks = Def->Unit.AbilityDurationTicks;
                }
                Combats[I].SecondaryAbilityCooldownTicks = Def->Unit.AbilityCooldownTicks;
            }
            else
            {
                Combats[I].SecondaryAbilityDurationTicks = 0;
            }

            SimEvent Ev;
            Ev.Type = SimEventType::SecondaryAbilityToggled;
            Ev.Tick = CurrentTick;
            Ev.Entity = MakeId(I);
            Ev.Player = Cmd.Issuer;
            Ev.Content = Core[I].Def;
            Ev.Location = Transforms[I].Position;
            Ev.Value = Combats[I].bSecondaryModeActive ? 1 : 0;
            EmitEvent(Ev);
            break;
        }

        case CommandType::CoopPing:
        {
            SimEvent Ev;
            Ev.Type = SimEventType::CoopPingEmitted;
            Ev.Tick = CurrentTick;
            Ev.Player = Cmd.Issuer;
            Ev.Location = Cmd.Location;
            Ev.Value = Cmd.Param;
            EmitEvent(Ev);
            break;
        }

        case CommandType::PlaceBuilding:
        {
            const EntityDef* Item = Content ? Content->FindEntity(Cmd.Content) : nullptr;
            if (Item == nullptr || Item->Kind != EntityKind::Building)
            {
                return Reject(CommandReject::UnknownContent);
            }

            // The structure must already be finished in a construction yard queue.
            // This is the C&C model: you pay and build first, then choose where it
            // lands, and the server owns both halves of that transaction.
            EntityId YardId = EntityId::Invalid();
            int32_t QueueSlot = -1;
            for (uint32_t I = 0; I < Core.size() && QueueSlot < 0; ++I)
            {
                if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner != Cmd.Issuer)
                {
                    continue;
                }
                BuildingComp& Yard = Buildings[I];
                for (size_t S = 0; S < Yard.Queue.size(); ++S)
                {
                    if (Yard.Queue[S].Content == Cmd.Content &&
                        Yard.Queue[S].ProgressTicks >= Yard.Queue[S].TotalTicks * kProgressScale)
                    {
                        YardId = MakeId(I);
                        QueueSlot = int32_t(S);
                        break;
                    }
                }
            }
            if (QueueSlot < 0)
            {
                return Reject(CommandReject::NoProducer);
            }
            if (!IsPlacementValid(Cmd.Content, Cmd.Issuer, Cmd.Tile))
            {
                return Reject(CommandReject::InvalidPlacement);
            }

            Buildings[YardId.Index].Queue.erase(Buildings[YardId.Index].Queue.begin() + QueueSlot);
            SpawnBuilding(Cmd.Content, Cmd.Issuer, Cmd.Tile, /*bInstantComplete*/ false);
            break;
        }

        case CommandType::SellBuilding:
        {
            if (!IsAlive(Cmd.Primary) || Core[Cmd.Primary.Index].Owner != Cmd.Issuer ||
                Core[Cmd.Primary.Index].Kind != EntityKind::Building)
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            const EntityDef* D = Content->FindEntity(Core[Cmd.Primary.Index].Def);
            if (D != nullptr)
            {
                Player.Credits += (D->Production.Cost * D->Building.SellRefundPercent) / 100;
            }
            // ADR-0012 treats a sale as OwnershipChanged, which carries no queue
            // refund: the sale price already compensates the player, and paying the
            // destroyed-producer refund on top would make selling a loaded factory a
            // money faucet. Recorded for this tick only -- a persisted flag would
            // survive a save taken before the death sweep and permanently forfeit
            // the refund on a later, genuine destruction.
            PendingSales.push_back(Cmd.Primary);
            QueueDestroy(Cmd.Primary);
            break;
        }

        case CommandType::RepairBuilding:
        {
            if (!IsAlive(Cmd.Primary) || Core[Cmd.Primary.Index].Owner != Cmd.Issuer ||
                Core[Cmd.Primary.Index].Kind != EntityKind::Building)
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            BuildingComp& Target = Buildings[Cmd.Primary.Index];
            // A toggle rather than a one-shot: repair is a sustained paid activity, and
            // the player needs to be able to call it off when the bill outgrows the
            // building. SystemRepair also clears the flag once the structure is whole.
            Target.bRepairing = !Target.bRepairing;
            if (!Target.bRepairing)
            {
                // Drop the part-credit rather than bank it across a stop/start cycle,
                // which would let a player repair for free by toggling.
                Target.RepairCreditAccumulator = 0;
            }
            break;
        }

        case CommandType::SetPowerPriority:
        {
            if (!IsAlive(Cmd.Primary) || Core[Cmd.Primary.Index].Owner != Cmd.Issuer ||
                Core[Cmd.Primary.Index].Kind != EntityKind::Building)
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            // Param is a wire value, so it is range-checked rather than cast blindly:
            // a malformed packet must not be able to plant an out-of-range enum that
            // then falls through every switch on it.
            if (Cmd.Param < 0 || Cmd.Param > int32_t(PowerPriority::Auxiliary))
            {
                return Reject(CommandReject::TargetInvalid);
            }
            Buildings[Cmd.Primary.Index].Priority = PowerPriority(Cmd.Param);
            break;
        }

        // --- Superweapon -----------------------------------------------
        // Validated like any other order: ownership, liveness, and the building
        // actually being a charged superweapon. There is no separate "cheat"
        // path -- the AI issues this exact command through the same bus.
        case CommandType::FireSuperweapon:
        {
            if (!IsAlive(Cmd.Primary))
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            if (Core[Cmd.Primary.Index].Owner != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            const EntityDef* Def = Content ? Content->FindEntity(Core[Cmd.Primary.Index].Def) : nullptr;
            if (Def == nullptr || Def->Building.SuperweaponRechargeTicks <= 0)
            {
                return Reject(CommandReject::UnknownContent);
            }
            BuildingComp& B = Buildings[Cmd.Primary.Index];
            if (B.State != ConstructionState::Complete)
            {
                return Reject(CommandReject::SuperweaponNotReady);
            }
            if (B.SuperweaponChargeTicks < Def->Building.SuperweaponRechargeTicks)
            {
                return Reject(CommandReject::SuperweaponNotReady);
            }
            if (Player.PowerConsumed > Player.PowerProduced)
            {
                return Reject(CommandReject::SuperweaponUnpowered);
            }
            if (!Map.IsInBounds(Cmd.Tile.X, Cmd.Tile.Y))
            {
                return Reject(CommandReject::TargetInvalid);
            }

            // Spend the charge before applying damage: a rejected-after-fire path
            // would let a player fire twice if damage resolution ever throws.
            B.SuperweaponChargeTicks = 0;

            const Vec2 Impact = Map.TileCenterToWorld(Cmd.Tile);
            // Reuses the ordinary splash path, so the armour table, fog and event
            // emission behave exactly as they do for a shell.
            ApplySplashDamage(Impact, Def->Building.SuperweaponRadius,
                              Def->Building.SuperweaponDamage,
                              Def->Building.SuperweaponWarhead,
                              /*FalloffPercent*/ 50, Cmd.Primary, Cmd.Issuer);

            SimEvent Ev;
            Ev.Type = SimEventType::WeaponFired;
            Ev.Tick = CurrentTick;
            Ev.Entity = Cmd.Primary;
            Ev.Player = Cmd.Issuer;
            Ev.Content = Core[Cmd.Primary.Index].Def;
            Ev.Location = Impact;
            Ev.Value = Def->Building.SuperweaponDamage;
            EmitEvent(Ev);
            break;
        }

        case CommandType::Surrender:
        {
            Player.bDefeated = true;
            SimEvent Ev;
            Ev.Type = SimEventType::PlayerDefeated;
            Ev.Tick = CurrentTick;
            Ev.Player = Cmd.Issuer;
            EmitEvent(Ev);
            break;
        }

        // --- Direct vehicle control ----------------------------------------
        // All four commands go through the same ownership/alive checks as
        // ordinary orders. Enter/Exit mutate the DirectControlComp; Drive
        // mutates movement/turret via the profile; Fire re-uses FireWeapon so
        // damage, cooldown and ammo are identical to RTS fire.
        case CommandType::DirectControlEnter:
        {
            if (!IsAlive(Cmd.Primary))
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            if (Core[Cmd.Primary.Index].Owner != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            if (Core[Cmd.Primary.Index].Kind != EntityKind::Unit)
            {
                return Reject(CommandReject::DirectIneligibleUnit);
            }
            // Only one driver per entity.
            const DirectControlComp& Existing = DirectControls[Cmd.Primary.Index];
            if (Existing.Phase == DirectControlPhase::Active ||
                Existing.Phase == DirectControlPhase::Entering)
            {
                if (Existing.Controller == Cmd.Issuer)
                {
                    // Idempotent re-entry from same player: accept silently.
                    break;
                }
                return Reject(CommandReject::DirectAlreadyControlled);
            }
            // Reject if the entity has no weapon and no turret (eligible check).
            // We treat any armed vehicle or any vehicle with TurretTurnRate>0 as
            // direct-controllable. The presentation profile gates which units the
            // client will *offer*; the simulation only enforces safety.
            const EntityDef* Def = Content ? Content->FindEntity(Core[Cmd.Primary.Index].Def) : nullptr;
            if (Def == nullptr)
            {
                return Reject(CommandReject::UnknownContent);
            }
            DirectControlComp& Dc = DirectControls[Cmd.Primary.Index];
            Dc.Phase = DirectControlPhase::Entering;
            Dc.Controller = Cmd.Issuer;
            Dc.PhaseUntilTick = CurrentTick + kDirectControlEnterExitTicks;
            Dc.CooldownTicksPrimary = 0;
            Dc.CooldownTicksSecondary = 0;
            Dc.bOpticsZoomed = false;
            // Clear current orders; the driver is now responsible for movement.
            Orders[Cmd.Primary.Index].Clear();
            Combats[Cmd.Primary.Index].Target = EntityId::Invalid();
            Combats[Cmd.Primary.Index].bTargetIsForced = false;
            {
                SimEvent Ev;
                Ev.Type = SimEventType::DirectControlEntered;
                Ev.Tick = CurrentTick;
                Ev.Entity = Cmd.Primary;
                Ev.Player = Cmd.Issuer;
                EmitEvent(Ev);
            }
            break;
        }

        case CommandType::DirectControlExit:
        {
            if (!IsAlive(Cmd.Primary))
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            const DirectControlComp& Dc = DirectControls[Cmd.Primary.Index];
            if (Dc.Phase != DirectControlPhase::Active &&
                Dc.Phase != DirectControlPhase::Entering)
            {
                return Reject(CommandReject::DirectNotControlling);
            }
            if (Dc.Controller != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            DirectControlComp& DcMut = DirectControls[Cmd.Primary.Index];
            DcMut.Phase = DirectControlPhase::Exiting;
            DcMut.PhaseUntilTick = CurrentTick + kDirectControlEnterExitTicks;
            // Hand back to AI: leave a Stop order so the vehicle does not drift.
            Orders[Cmd.Primary.Index].Clear();
            Movements[Cmd.Primary.Index].bHasDestination = false;
            Movements[Cmd.Primary.Index].CurrentSpeed = Fixed::Zero();
            {
                SimEvent Ev;
                Ev.Type = SimEventType::DirectControlExited;
                Ev.Tick = CurrentTick;
                Ev.Entity = Cmd.Primary;
                Ev.Player = Cmd.Issuer;
                EmitEvent(Ev);
            }
            break;
        }

        case CommandType::DirectControlDrive:
        {
            if (!IsAlive(Cmd.Primary))
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            DirectControlComp& Dc = DirectControls[Cmd.Primary.Index];
            if (Dc.Phase != DirectControlPhase::Active &&
                Dc.Phase != DirectControlPhase::Entering)
            {
                return Reject(CommandReject::DirectNotControlling);
            }
            if (Dc.Controller != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            const EntityDef* Def = Content ? Content->FindEntity(Core[Cmd.Primary.Index].Def) : nullptr;
            if (Def == nullptr)
            {
                return Reject(CommandReject::UnknownContent);
            }
            // Throttle -> forward movement. We use the same MovementComp the
            // ordinary SystemMovement will integrate, so speed limits stay
            // identical to RTS control.
            const int32_t Throttle = std::clamp(int32_t(Cmd.DirectAxes.Throttle), -127, 127);
            const int32_t Steering = std::clamp(int32_t(Cmd.DirectAxes.Steering), -127, 127);
            if (Throttle > 0)
            {
                const Fixed Fwd = Def->Unit.MaxSpeed * Fixed::FromRatio(Throttle, 127);
                // Reuse the order queue with a small forward waypoint, refreshed
                // every tick. The arrive radius keeps the vehicle from fighting
                // itself.
                const Vec2 Pos = Transforms[Cmd.Primary.Index].Position;
                const int32_t Facing = Transforms[Cmd.Primary.Index].Facing;
                const Vec2 Dir = Vec2::FromAngle(Facing);
                Vec2 Dest = Pos + Dir * Fwd * Fixed::FromInt(2);
                if (Map.Width > 0 && Map.Height > 0)
                {
                    const Fixed MinMargin = Fixed::FromInt(50);
                    const Fixed MaxX = Fixed::FromInt(int64_t(Map.Width) * MapDescription::kTileSizeUnitsLocal) - MinMargin;
                    const Fixed MaxY = Fixed::FromInt(int64_t(Map.Height) * MapDescription::kTileSizeUnitsLocal) - MinMargin;
                    Dest.X = std::clamp(Dest.X, MinMargin, MaxX);
                    Dest.Y = std::clamp(Dest.Y, MinMargin, MaxY);
                }
                Orders[Cmd.Primary.Index].Clear();
                Order O;
                O.Type = OrderType::Move;
                O.Location = Dest;
                Orders[Cmd.Primary.Index].Push(O);
            }
            else if (Throttle < 0)
            {
                const Fixed Rev = Def->Unit.MaxSpeed * Fixed::FromRatio(-Throttle, 127) * Fixed::FromRatio(1, 2);
                const Vec2 Pos = Transforms[Cmd.Primary.Index].Position;
                const int32_t Facing = Transforms[Cmd.Primary.Index].Facing;
                const Vec2 Dir = Vec2::FromAngle(Facing);
                Vec2 Dest = Pos - Dir * Rev * Fixed::FromInt(2);
                if (Map.Width > 0 && Map.Height > 0)
                {
                    const Fixed MinMargin = Fixed::FromInt(50);
                    const Fixed MaxX = Fixed::FromInt(int64_t(Map.Width) * MapDescription::kTileSizeUnitsLocal) - MinMargin;
                    const Fixed MaxY = Fixed::FromInt(int64_t(Map.Height) * MapDescription::kTileSizeUnitsLocal) - MinMargin;
                    Dest.X = std::clamp(Dest.X, MinMargin, MaxX);
                    Dest.Y = std::clamp(Dest.Y, MinMargin, MaxY);
                }
                Orders[Cmd.Primary.Index].Clear();
                Order O;
                O.Type = OrderType::Move;
                O.Location = Dest;
                Orders[Cmd.Primary.Index].Push(O);
            }
            else
            {
                // No throttle: stop.
                Orders[Cmd.Primary.Index].Clear();
                Movements[Cmd.Primary.Index].CurrentSpeed = Fixed::Zero();
                Movements[Cmd.Primary.Index].bHasDestination = false;
            }
            // Steering -> hull facing delta (centi-degrees per tick).
            if (Steering != 0 && Def->Unit.TurnRatePerSecond > 0)
            {
                const int32_t TurnPerTickCenti =
                    (int32_t(Def->Unit.TurnRatePerSecond) * 100) / int32_t(kTicksPerSecond);
                Transforms[Cmd.Primary.Index].Facing +=
                    (Steering * TurnPerTickCenti) / 127;
            }
            // Turret yaw/pitch -> quantized; full integration in SystemDirectControl.
            Dc.TurretYawCentiDeg += int32_t(Cmd.DirectAxes.TurretYaw) * 16; // scale applied here
            Dc.TurretPitchCentiDeg = std::clamp(
                Dc.TurretPitchCentiDeg + int32_t(Cmd.DirectAxes.TurretPitch) * 16,
                -8000, 8000); // +/- 80 deg clamp
            // Flags handled by Fire command in the same frame, not here.
            const uint8_t Flags = Cmd.DirectAxes.Flags;
            if (Flags & 0x08) { Dc.bOpticsZoomed = !Dc.bOpticsZoomed; }
            // Promote Entering -> Active on first Drive.
            if (Dc.Phase == DirectControlPhase::Entering)
            {
                Dc.Phase = DirectControlPhase::Active;
                Dc.PhaseUntilTick = 0;
            }
            break;
        }

        case CommandType::DirectControlFire:
        {
            if (!IsAlive(Cmd.Primary))
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            const DirectControlComp& Dc = DirectControls[Cmd.Primary.Index];
            if (Dc.Phase != DirectControlPhase::Active &&
                Dc.Phase != DirectControlPhase::Entering)
            {
                return Reject(CommandReject::DirectNotControlling);
            }
            if (Dc.Controller != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            const EntityDef* Def = Content ? Content->FindEntity(Core[Cmd.Primary.Index].Def) : nullptr;
            if (Def == nullptr || !Def->Weapon.IsValid())
            {
                return Reject(CommandReject::DirectWeaponEmpty);
            }
            const WeaponDef* Wpn = Content ? Content->FindWeapon(Def->Weapon) : nullptr;
            if (Wpn == nullptr)
            {
                return Reject(CommandReject::DirectWeaponEmpty);
            }
            const uint8_t Flags = Cmd.DirectAxes.Flags;
            const bool bSecondary = (Flags & 0x02) != 0;
            const ContentId WpnId = bSecondary && Def->SecondaryWeapon.IsValid()
                ? Def->SecondaryWeapon
                : Def->Weapon;
            const WeaponDef* WpnToFire = (WpnId == Def->Weapon) ? Wpn
                : (Content ? Content->FindWeapon(WpnId) : nullptr);
            if (WpnToFire == nullptr)
            {
                return Reject(CommandReject::DirectWeaponEmpty);
            }
            DirectControlComp& DcMut = DirectControls[Cmd.Primary.Index];
            int32_t& Cooldown = (WpnId == Def->Weapon)
                ? DcMut.CooldownTicksPrimary
                : DcMut.CooldownTicksSecondary;
            if (Cooldown > 0)
            {
                return Reject(CommandReject::DirectWeaponCooldown);
            }
            // Target: use forced target from Cmd.Target if alive & hostile;
            // otherwise acquire via the existing system so first-person fire
            // can never shoot through fog of war.
            EntityId Target = Cmd.Target;
            if (!IsAlive(Target) || !IsHostile(Cmd.Issuer, Core[Target.Index].Owner))
            {
                Target = AcquireTarget(Cmd.Primary);
            }
            if (!Target.IsValid())
            {
                return Reject(CommandReject::TargetInvalid);
            }
            // Same fire path as RTS: same damage, same cooldown, same projectile.
            FireWeapon(Cmd.Primary, Target, *WpnToFire);
            Cooldown = WpnToFire->CooldownTicks;
            break;
        }

        default:
            return Reject(CommandReject::UnknownType);
    }

    return Result;
}

// ---------------------------------------------------------------------------
// Damage
// ---------------------------------------------------------------------------

void SimWorld::QueueDestroy(EntityId Id, EntityId Killer)
{
    if (std::find(PendingDestroy.begin(), PendingDestroy.end(), Id) != PendingDestroy.end())
    {
        return;
    }
    PendingDestroy.push_back(Id);
    PendingDestroyKillers.push_back(Killer);
}

void SimWorld::DebugDamage(EntityId TargetId, int32_t DamageAmount)
{
    if (IsAlive(TargetId))
    {
        HealthComp& H = Healths[TargetId.Index];
        H.Current = (DamageAmount >= H.Current) ? 0 : (H.Current - DamageAmount);
        if (H.Current == 0)
        {
            QueueDestroy(TargetId);
        }
    }
}

void SimWorld::ApplyDamage(EntityId TargetId, int32_t BaseDamage, WarheadClass Warhead, EntityId Source,
                           PlayerId SourcePlayer)
{
    if (!IsAlive(TargetId) || Healths[TargetId.Index].bInvulnerable)
    {
        return;
    }
    // Iron curtain / time belt: timed invulnerability is checked here so every
    // damage path honours it exactly like the static flag above.
    if (Statuses[TargetId.Index].InvulnerableTicks > 0)
    {
        return;
    }
    const EntityDef* D = Content->FindEntity(Core[TargetId.Index].Def);
    if (D == nullptr)
    {
        return;
    }

    int32_t Multiplier = Content->GetDamageMultiplier(Warhead, D->Armor);
    // Research upgrades: armor bonus reduces incoming damage. ArmorPercent is
    // capped at 90 in GetPlayerModifiers, so this never zeroes damage out.
    if (Core[TargetId.Index].Owner < kMaxPlayers)
    {
        const PlayerModifiers Mod = GetPlayerModifiers(Core[TargetId.Index].Owner);
        if (Mod.ArmorPercent > 0)
        {
            Multiplier = (Multiplier * (100 - Mod.ArmorPercent)) / 100;
        }
    }
    // Cryo and shrink both double incoming damage; frozen-and-shrunk stacks.
    if (Statuses[TargetId.Index].FreezeTicks > 0)
    {
        Multiplier *= 2;
    }
    if (Statuses[TargetId.Index].ShrinkTicks > 0)
    {
        Multiplier *= 2;
    }
    int32_t Damage = (BaseDamage * Multiplier) / 100;

    // Veterancy damage bonus, applied to the attacker's outgoing damage after the
    // armor matrix and before the target's own mitigations.
    const bool bSourceIsUnit = Source.IsValid() && IsAlive(Source)
                               && Core[Source.Index].Kind == EntityKind::Unit;
    if (bSourceIsUnit)
    {
        const int32_t BonusPercent =
            Content->GetVeterancy().Levels[int32_t(Healths[Source.Index].Rank)].DamageBonusPercent;
        if (BonusPercent > 0)
        {
            Damage = (Damage * (100 + BonusPercent)) / 100;
        }
        // Research upgrade: outgoing damage bonus from the attacker's owner.
        const PlayerModifiers Mod = GetPlayerModifiers(Core[Source.Index].Owner);
        if (Mod.DamagePercent > 0)
        {
            Damage = (Damage * (100 + Mod.DamagePercent)) / 100;
        }
    }

    if (Combats[TargetId.Index].bSecondaryModeActive && D->Unit.AbilityArmorBonusPercent > 0)
    {
        Damage = (Damage * (100 - D->Unit.AbilityArmorBonusPercent)) / 100;
    }
    // A weapon that is not useless against a target should always do at least one
    // point, otherwise rounding produces unkillable stalemates at low damage.
    if (Damage <= 0 && Multiplier > 0 && BaseDamage > 0)
    {
        Damage = 1;
    }
    if (Damage <= 0)
    {
        return;
    }


    HealthComp& H = Healths[TargetId.Index];
    H.Current -= Damage;

    SimEvent Ev;
    Ev.Type = SimEventType::DamageApplied;
    Ev.Tick = CurrentTick;
    Ev.Entity = TargetId;
    Ev.Other = Source;
    Ev.Player = SourcePlayer;
    Ev.Location = Transforms[TargetId.Index].Position;
    Ev.Value = Damage;
    EmitEvent(Ev);

    if (H.Current <= 0)
    {
        H.Current = 0;
        // Deferred: the killer may still be iterating and the target's components
        // must stay readable for the rest of this tick.
        const bool bFirstFatalBlow =
            std::find(PendingDestroy.begin(), PendingDestroy.end(), TargetId) == PendingDestroy.end();
        if (bFirstFatalBlow)
        {
            QueueDestroy(TargetId, Source);
        }

        // Kill-value credit for unit killers on hostile victims, once per victim.
        if (bFirstFatalBlow && bSourceIsUnit
            && (Core[TargetId.Index].Kind == EntityKind::Unit || Core[TargetId.Index].Kind == EntityKind::Building)
            && IsHostile(Core[Source.Index].Owner, Core[TargetId.Index].Owner))
        {
            HealthComp& Killer = Healths[Source.Index];
            const int64_t VictimValue = D->Production.Cost > 0 ? int64_t(D->Production.Cost) : 0;
            if (VictimValue > 0 && Killer.KillsValue <= INT32_MAX - int32_t(VictimValue))
            {
                Killer.KillsValue += int32_t(VictimValue);
                TryPromoteVeterancy(Source);
            }
        }
    }
}

void SimWorld::TryPromoteVeterancy(EntityId Unit)
{
    if (!IsAlive(Unit) || Core[Unit.Index].Kind != EntityKind::Unit)
    {
        return;
    }
    const EntityDef* D = Content->FindEntity(Core[Unit.Index].Def);
    if (D == nullptr || D->Production.Cost <= 0)
    {
        return;
    }
    const VeterancyDef& Vet = Content->GetVeterancy();
    HealthComp& H = Healths[Unit.Index];

    while (int32_t(H.Rank) + 1 < int32_t(VeterancyRank::Count))
    {
        const VeterancyLevel& Next = Vet.Levels[int32_t(H.Rank) + 1];
        const int64_t Threshold = int64_t(std::max(1, Next.CostThresholdMultiplier)) * int64_t(D->Production.Cost);
        if (H.KillsValue < Threshold)
        {
            break;
        }
        H.Rank = VeterancyRank(int32_t(H.Rank) + 1);
        const VeterancyLevel& New = Vet.Levels[int32_t(H.Rank)];
        const int32_t BaseMax = D->MaxHealth;
        const int32_t NewMax = BaseMax + (BaseMax * std::max(0, New.HpBonusPercent)) / 100;
        if (NewMax > H.Max)
        {
            H.Current = std::min(NewMax, H.Current + (NewMax - H.Max));
            H.Max = NewMax;
        }

        SimEvent Ev;
        Ev.Type = SimEventType::EntityVeterancyPromoted;
        Ev.Tick = CurrentTick;
        Ev.Entity = Unit;
        Ev.Player = Core[Unit.Index].Owner;
        Ev.Value = int32_t(H.Rank);
        EmitEvent(Ev);
    }
}

void SimWorld::TeleportEntity(EntityId Id, const Vec2& NewPosition)
{
    if (!IsAlive(Id))
    {
        return;
    }
    Transforms[Id.Index].Position = NewPosition;
}

void SimWorld::ApplySplashDamage(const Vec2& Center, Fixed Radius, int32_t BaseDamage, WarheadClass Warhead,

                                 int32_t FalloffPercent, EntityId Source, PlayerId SourcePlayer)
{
    if (Radius <= Fixed::Zero())
    {
        return;
    }
    const Fixed RadiusSq = Radius * Radius;
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind == EntityKind::Projectile || Core[I].Kind == EntityKind::ResourceNode)
        {
            continue;
        }
        const Fixed DistSq = DistanceSquared(Center, Transforms[I].Position);
        if (DistSq > RadiusSq)
        {
            continue;
        }
        // Linear falloff from full damage at the centre to FalloffPercent at the rim.
        const Fixed Dist = FxSqrt(DistSq);
        const int64_t Ratio = Radius.Raw == 0 ? 0 : (Dist.Raw * 100) / Radius.Raw;
        const int32_t Scale = 100 - int32_t((int64_t(100 - FalloffPercent) * Ratio) / 100);
        // Splash damages everything in the blast, including the attacker's own
        // units. Friendly fire is a design decision, not an oversight.
        ApplyDamage(MakeId(I), (BaseDamage * Scale) / 100, Warhead, Source, SourcePlayer);
    }
}

void SimWorld::DestroyEntity(EntityId Id, EntityId Killer, bool bWasSold)
{
    if (!IsAlive(Id))
    {
        return;
    }
    const uint32_t Index = Id.Index;
    const PlayerId Owner = Core[Index].Owner;
    const EntityKind Kind = Core[Index].Kind;

    if (Kind == EntityKind::Building)
    {
        OccupyTiles(Buildings[Index], false);
        // ADR-0012: the queue dies with its factory, but the credits already drawn
        // for it are partially returned. Without this, losing a war factory mid-run
        // silently confiscates everything paid so far -- a compounding punishment on
        // top of losing the building, and one the player has no way to see.
        //
        // A voluntary sale is excluded: that is OwnershipChanged, and the sale price
        // is already the compensation. Refunding here as well would make selling a
        // loaded factory strictly better than keeping it.
        if (Owner < kMaxPlayers && !bWasSold)
        {
            int32_t Refunded = 0;
            for (const ProductionItem& Item : Buildings[Index].Queue)
            {
                Refunded += int32_t((int64_t(Item.PaidCredits) * kRefundProducerDestroyedPercent) / 100);
            }
            if (Refunded > 0)
            {
                Players[Owner].Credits += Refunded;

                SimEvent Refund;
                Refund.Type = SimEventType::ProductionCancelled;
                Refund.Tick = CurrentTick;
                Refund.Entity = Id;
                Refund.Player = Owner;
                Refund.Content = Core[Index].Def;
                Refund.Value = Refunded;
                EmitEvent(Refund);
            }
        }
        Buildings[Index].Queue.clear();
        Buildings[Index].DockedHarvester = EntityId::Invalid();
        Buildings[Index].UnloadingQueue.clear();
        if (Owner < kMaxPlayers && !bWasSold)
        {
            Players[Owner].BuildingsLost += 1;
        }
    }
    else if (Kind == EntityKind::Unit)
    {
        if (Owner < kMaxPlayers)
        {
            Players[Owner].UnitsLost += 1;
        }
    }

    if (!bWasSold)
    {
        SimEvent Ev;
        Ev.Type = SimEventType::EntityDestroyed;
        Ev.Tick = CurrentTick;
        Ev.Entity = Id;
        Ev.Other = Killer;
        Ev.Player = Owner;
        Ev.Content = Core[Index].Def;
        Ev.Location = Transforms[Index].Position;
        EmitEvent(Ev);
    }

    // If this entity was under direct control, emit the exit event and mark
    // the phase so the presentation layer knows the player was ejected by
    // destruction rather than by an explicit Exit command.
    if (DirectControls[Index].Phase == DirectControlPhase::Active ||
        DirectControls[Index].Phase == DirectControlPhase::Entering)
    {
        SimEvent DcEv;
        DcEv.Type = SimEventType::DirectControlExited;
        DcEv.Tick = CurrentTick;
        DcEv.Entity = Id;
        DcEv.Player = DirectControls[Index].Controller;
        EmitEvent(DcEv);
        DirectControls[Index].Phase = DirectControlPhase::VehicleDestroyed;
        DirectControls[Index].PhaseUntilTick = 0;
        DirectControls[Index].Controller = kInvalidPlayer;
    }

    Core[Index].bAlive = false;
    Core[Index].Generation += 1;
    FreeSlots.push_back(Index);

    if (Kind == EntityKind::Building && Owner < kMaxPlayers)
    {
        RefreshPlayerTech(Owner);
    }
}

void SimWorld::RemoveEntitySilently(EntityId Id)
{
    if (!IsAlive(Id))
    {
        return;
    }
    const uint32_t Index = Id.Index;
    const EntityKind Kind = Core[Index].Kind;
    const PlayerId Owner = Core[Index].Owner;

    if (Kind == EntityKind::Building)
    {
        OccupyTiles(Buildings[Index], false);
        Buildings[Index].Queue.clear();
        Buildings[Index].DockedHarvester = EntityId::Invalid();
        Buildings[Index].UnloadingQueue.clear();
    }

    if (DirectControls[Index].Phase == DirectControlPhase::Active ||
        DirectControls[Index].Phase == DirectControlPhase::Entering)
    {
        SimEvent DcEv;
        DcEv.Type = SimEventType::DirectControlExited;
        DcEv.Tick = CurrentTick;
        DcEv.Entity = Id;
        DcEv.Player = DirectControls[Index].Controller;
        EmitEvent(DcEv);
        DirectControls[Index].Phase = DirectControlPhase::Inactive;
        DirectControls[Index].PhaseUntilTick = 0;
        DirectControls[Index].Controller = kInvalidPlayer;
    }

    Core[Index].bAlive = false;
    Core[Index].Generation += 1;
    FreeSlots.push_back(Index);

    if (Kind == EntityKind::Building && Owner < kMaxPlayers)
    {
        RefreshPlayerTech(Owner);
    }
}

void SimWorld::CheatGrantCredits(PlayerId Owner, int32_t Amount)
{
    if (Owner < kMaxPlayers)
    {
        Players[Owner].Credits += Amount;
    }
}

void SimWorld::CheatGrantPower(PlayerId Owner, int32_t PowerAmount)
{
    if (Owner < kMaxPlayers)
    {
        Players[Owner].PowerProduced += PowerAmount;
    }
}

void SimWorld::CheatInstantBuild(PlayerId Owner)
{
    for (size_t Index = 0; Index < Core.size(); ++Index)
    {
        if (Core[Index].bAlive && Core[Index].Owner == Owner)
        {
            if (Index < Buildings.size())
            {
                Buildings[Index].State = ConstructionState::Complete;
                Buildings[Index].ConstructionProgressTicks =
                    Buildings[Index].ConstructionTotalTicks * kProgressScale;
                if (!Buildings[Index].Queue.empty())
                {
                    // Progress is measured in hundredths of a tick, so the target is
                    // TotalTicks * kProgressScale; assigning TotalTicks alone left
                    // the item at 1% and the cheat did nothing on long builds.
                    ProductionItem& Head = Buildings[Index].Queue.front();
                    Head.ProgressTicks = Head.TotalTicks * kProgressScale;
                    // A cheat grants the item outright, so it must also be marked
                    // paid: SystemProduction only advances funded items, and
                    // SystemFlowPayment would otherwise bill for it retroactively.
                    Head.PaidCredits = Head.TotalCost;
                    Head.State = FlowPaymentState::Completed;
                }
            }
        }
    }
}

void SimWorld::CheatToggleGodMode(PlayerId Owner)
{
    for (size_t Index = 0; Index < Core.size(); ++Index)
    {
        if (Core[Index].bAlive && Core[Index].Owner == Owner)
        {
            if (Index < Healths.size())
            {
                Healths[Index].bInvulnerable = !Healths[Index].bInvulnerable;
                Healths[Index].Current = 999999;
                Healths[Index].Max = 999999;
            }
        }
    }
}

void SimWorld::CheatRevealMap(PlayerId Owner)
{
    if (FogGrid)
    {
        FogGrid->RevealRadarArea(Owner, 0, 0, 9999);
    }
}

void SimWorld::CheatKillAllEnemies(PlayerId Owner)
{
    for (size_t Index = 0; Index < Core.size(); ++Index)
    {
        if (Core[Index].bAlive && Core[Index].Owner != Owner && Core[Index].Owner < kMaxPlayers)
        {
            PendingDestroy.push_back(MakeId(uint32_t(Index)));
        }
    }
}

void SimWorld::CheatHealAll(PlayerId Owner)
{
    for (size_t Index = 0; Index < Core.size(); ++Index)
    {
        if (Core[Index].bAlive && Core[Index].Owner == Owner)
        {
            if (Index < Healths.size())
            {
                Healths[Index].Current = Healths[Index].Max;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Systems
// ---------------------------------------------------------------------------

void SimWorld::SystemApplyCommands(const CommandFrame* Frame)
{
    for (int32_t& Count : CommandsThisTick)
    {
        Count = 0;
    }
    if (Frame == nullptr)
    {
        return;
    }
    for (const Command& Cmd : Frame->Commands)
    {
        ApplyCommand(Cmd);
    }
}

void SimWorld::SystemPower()
{
    for (PlayerState& P : Players)
    {
        P.PowerProduced = 0;
        P.PowerConsumed = 0;
        P.CommandLimitMax = 50;
        P.CommandLimitUsed = 0;
    }
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Owner >= kMaxPlayers)
        {
            continue;
        }
        const EntityDef* D = Content ? Content->FindEntity(Core[I].Def) : nullptr;
        if (D == nullptr)
        {
            continue;
        }
        PlayerState& P = Players[Core[I].Owner];
        if (Core[I].Kind == EntityKind::Building && Buildings[I].State == ConstructionState::Complete)
        {
            const int64_t HealthRatio = Healths[I].Max > 0 ? (int64_t(Healths[I].Current) * 100) / Healths[I].Max : 0;
            P.PowerProduced += int32_t((int64_t(D->Building.PowerProduced) * HealthRatio) / 100);
            P.PowerConsumed += D->Building.PowerConsumed;
            P.CommandLimitMax += D->Building.CommandLimitProvided;
        }
        else if (Core[I].Kind == EntityKind::Unit)
        {
            P.CommandLimitUsed += D->Production.CommandLimit;
        }
    }
    for (PlayerState& P : Players)
    {
        P.CommandLimitMax = std::min(200, P.CommandLimitMax);
        if (P.Faction == FactionId::ChronoLegion && (CurrentTick % 40) == 0)
        {
            P.FactionResource = std::min(100, P.FactionResource + 1);
        }
    }

    // Superweapon charge, in a second pass because it depends on the power totals
    // the loop above has just finished computing. Charging requires a power
    // surplus, so cutting an opponent's power stalls their superweapon -- the
    // clock is a consequence of holding a working base, not a wall-clock timer.
    // Placed inside SystemPower rather than as a new system so the fixed system
    // order, which is part of the replay compatibility contract, is unchanged.
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building ||
            Core[I].Owner >= kMaxPlayers)
        {
            continue;
        }
        if (Buildings[I].State != ConstructionState::Complete)
        {
            continue;
        }
        const EntityDef* D = Content ? Content->FindEntity(Core[I].Def) : nullptr;
        if (D == nullptr || D->Building.SuperweaponRechargeTicks <= 0)
        {
            continue;
        }
        const PlayerState& Owner = Players[Core[I].Owner];
        if (Owner.PowerConsumed > Owner.PowerProduced)
        {
            continue;   // brownout: no charge this tick
        }
        BuildingComp& B = Buildings[I];
        if (B.SuperweaponChargeTicks < D->Building.SuperweaponRechargeTicks)
        {
            B.SuperweaponChargeTicks += 1;
        }
    }

    // ADR-0013: announce tier crossings. Edge-triggered, because a base parked at 45%
    // power would otherwise emit an event every tick and bury the alert feed -- the
    // same reason ProductionStarved is edge-triggered.
    for (PlayerId Owner = 0; Owner < kMaxPlayers; ++Owner)
    {
        PlayerState& P = Players[Owner];
        // bDefeated as well as bActive: defeat never clears bActive, and a defeated
        // player's last building dying takes both power figures to zero, which
        // GetPowerRatioPercent reports as a healthy 100%. Without this the game would
        // tell a player who just lost their base that their power had been restored.
        if (!P.bActive || P.bDefeated)
        {
            continue;
        }
        const PowerTier Tier = P.GetPowerTier();
        if (Tier == P.LastPowerTier)
        {
            continue;
        }

        SimEvent Ev;
        // "Started" reads as "the shortage got worse", so the direction is decided by
        // which way the tier moved, not by whether a shortage exists at all.
        Ev.Type = Tier > P.LastPowerTier ? SimEventType::PowerShortageStarted
                                         : SimEventType::PowerShortageEnded;
        Ev.Tick = CurrentTick;
        Ev.Player = Owner;
        Ev.Value = int32_t(Tier);
        EmitEvent(Ev);

        P.LastPowerTier = Tier;
    }
}

// ADR-0012. Draws credits for queued production a slice at a time instead of
// charging the whole price when the order is given.
//
// Two properties matter more than the mechanic itself:
//
//  * Progress never regresses. An item that runs out of money freezes and later
//    continues from exactly where it stopped. Rewinding progress would make the
//    final state depend on the *path* through funding states, which destroys the
//    "same seed plus same commands equals same state" guarantee replays rest on.
//
//  * Allocation is globally ordered, not per building. When money is scarce the
//    order in which factories get paid decides what a player ends up owning, so
//    that order must be a deterministic function of state (priority, then entity
//    index) and never of iteration or container layout.
void SimWorld::SystemFlowPayment()
{
    // Only the head of each queue draws credits. Funding the whole queue in
    // parallel would spread a thin treasury across everything and finish nothing,
    // which is precisely the failure the flow-payment model exists to avoid.
    //
    // The candidate list is a member vector rather than a fixed array: a player may
    // own any number of producing buildings, and an earlier fixed bound silently
    // dropped every producer past the ninth -- those items never paid a credit and
    // so never advanced, stalling forever with nothing shown to the player. Worse,
    // the drop happened before the priority sort, so a high-priority item at a high
    // entity index lost to low-priority ones. The storage is reused across ticks to
    // keep this allocation-free in the steady state.
    FundingCandidates.clear();

    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner >= kMaxPlayers)
        {
            continue;
        }
        BuildingComp& B = Buildings[I];
        // A half-built factory cannot yet spend the treasury on its own output.
        if (B.State != ConstructionState::Complete || B.Queue.empty())
        {
            continue;
        }
        // A building sold or already doomed this tick must stop drawing credits:
        // SystemDeaths has not run yet, so it is still alive here, and anything
        // charged now is money taken for a queue that is about to be discarded
        // without a refund.
        if (std::find(PendingDestroy.begin(), PendingDestroy.end(), MakeId(I)) != PendingDestroy.end())
        {
            continue;
        }

        ProductionItem& Head = B.Queue.front();

        // ADR-0013: anything the power state has stopped must also stop *paying*. This
        // is the same rule the PendingDestroy skip above enforces for a different
        // reason: charging for a queue that cannot advance is money taken for nothing,
        // and here it was money taken from the power plant that would have ended the
        // blackout. A player pause outranks the throttle, so that an item the player
        // deliberately stopped does not silently change its reported reason to "power".
        if (Head.State != FlowPaymentState::ManuallyPaused && IsProductionPowerStalled(I))
        {
            // Freeze in place. PaidCredits and ProgressTicks are untouched, so this is
            // a pause and not a reset -- the same guarantee starvation gives, for the
            // same determinism reason.
            Head.State = FlowPaymentState::EnergyThrottled;
            continue;
        }

        switch (Head.State)
        {
            case FlowPaymentState::Queued:
            case FlowPaymentState::Starved:
                // Both are "wants money, has none yet". Starved differs from Queued
                // only in what the UI says, so they enter funding on equal terms.
                break;
            case FlowPaymentState::Funding:
                break;
            case FlowPaymentState::EnergyThrottled:
                // Reaching this case at all means the throttle test above said no, so
                // the deficit has lifted and the item rejoins funding. Recovery is
                // therefore the exact inverse of the trigger by construction, rather
                // than an independent threshold that could disagree with it: an
                // earlier version used a separate 50% constant while the trigger fired
                // below 40%, which trapped any item stalled in the 40-49% band -- both
                // throttled and refused recovery, forever.
                break;
            case FlowPaymentState::Paying:
            case FlowPaymentState::Completed:
                // Already paid in full. SystemProduction advances these.
                continue;
            case FlowPaymentState::ManuallyPaused:
                // A deliberate player decision; never override it.
                continue;
            default:
                // Terminal states are removed by the code that sets them; if one is
                // still in a queue, leave it alone rather than paying for it.
                continue;
        }

        // A zero-cost item is funded the instant it is looked at, so it must not
        // consume an allocation slot or it would stall behind a poor treasury. This
        // also restores an already-paid item that was throttled and has now recovered:
        // it needs no more credits, only its state back.
        if (Head.IsFullyFunded())
        {
            Head.State = FlowPaymentState::Paying;
            continue;
        }

        FundingCandidates.push_back(FundingCandidate{I, Head.Priority, Core[I].Owner});
    }

    // Higher priority first, entity index breaking ties. Both keys come from
    // simulation state, so every peer produces the same order; the collection
    // order is an implementation detail, so the tie-break is an explicit key
    // rather than a reliance on sort stability.
    std::sort(FundingCandidates.begin(), FundingCandidates.end(),
              [](const FundingCandidate& A, const FundingCandidate& B)
              {
                  if (A.Owner != B.Owner) { return A.Owner < B.Owner; }
                  if (A.Priority != B.Priority) { return A.Priority > B.Priority; }
                  return A.BuildingIndex < B.BuildingIndex;
              });

    for (const FundingCandidate& Candidate : FundingCandidates)
    {
        PlayerState& Player = Players[Candidate.Owner];
        ProductionItem& Head = Buildings[Candidate.BuildingIndex].Queue.front();

        // Never charge past the remaining balance, and never overdraw the
        // treasury: Credits must not go negative, or the AI budget checks and
        // the HUD both start reporting nonsense.
        const int32_t Wanted = std::min(Head.CostPerTick(), Head.CreditsRemaining());
        const int32_t Charged = std::min(Wanted, std::max(0, Player.Credits));

        if (Charged > 0)
        {
            Player.Credits -= Charged;
            Head.PaidCredits += Charged;
        }

        if (Head.IsFullyFunded())
        {
            Head.State = FlowPaymentState::Paying;
        }
        else if (Charged < Wanted)
        {
            // Could not buy a full slice this tick. Partial payment is kept --
            // this is the "pauses rather than resets" requirement.
            //
            // Edge-triggered: the event fires on the transition into Starved, not
            // every tick it stays there, or a broke player would generate one event
            // per tick per queue and drown the alert feed.
            if (Head.State != FlowPaymentState::Starved)
            {
                SimEvent Ev;
                Ev.Type = SimEventType::ProductionStarved;
                Ev.Tick = CurrentTick;
                Ev.Entity = MakeId(Candidate.BuildingIndex);
                Ev.Player = Candidate.Owner;
                Ev.Content = Head.Content;
                Ev.Value = Head.CreditsRemaining();
                EmitEvent(Ev);
            }
            Head.State = FlowPaymentState::Starved;
        }
        else
        {
            Head.State = FlowPaymentState::Funding;
        }
    }
}


void SimWorld::SystemConstruction()
{
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building)
        {
            continue;
        }
        BuildingComp& B = Buildings[I];
        if (B.State != ConstructionState::UnderConstruction)
        {
            continue;
        }

        const PlayerId Owner = Core[I].Owner;
        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr)
        {
            continue;
        }

        // ADR-0013 tier table replaces the old flat floor. A building already under
        // construction keeps making progress at every tier, including Critical --
        // freezing it outright would strand a half-built power plant and make a
        // blackout unrecoverable, which is the one outcome the tiers must not produce.
        const int32_t Ratio = Owner < kMaxPlayers ? PowerSpeedPercent(Owner) : 100;

        const int64_t Total = std::max<int64_t>(1, int64_t(B.ConstructionTotalTicks) * kProgressScale);
        const int64_t PrevProgress = std::min<int64_t>(B.ConstructionProgressTicks, Total);
        B.ConstructionProgressTicks += Ratio;
        const int64_t NewProgress = std::min<int64_t>(B.ConstructionProgressTicks, Total);

        // Health accrues with construction progress, so a half-built structure is
        // half as tough and the presentation layer can drive a build-up effect from
        // the health ratio. Adding the delta rather than assigning the absolute
        // value means damage taken mid-build is not silently repaired next tick.
        const int64_t Gain = (int64_t(Healths[I].Max) * NewProgress) / Total -
                             (int64_t(Healths[I].Max) * PrevProgress) / Total;
        Healths[I].Current = int32_t(std::min<int64_t>(Healths[I].Max, Healths[I].Current + Gain));

        if (B.ConstructionProgressTicks >= B.ConstructionTotalTicks * kProgressScale)
        {
            B.State = ConstructionState::Complete;
            RefreshPlayerTech(Owner);

            SimEvent Ev;
            Ev.Type = SimEventType::BuildingCompleted;
            Ev.Tick = CurrentTick;
            Ev.Entity = MakeId(I);
            Ev.Player = Owner;
            Ev.Content = Core[I].Def;
            Ev.Location = Transforms[I].Position;
            EmitEvent(Ev);

            if (D->Building.BundledUnit.IsValid())
            {
                const Vec2 SpawnAt = FindFreeSpawnPoint(B, D->Building.BundledUnit);
                SpawnUnit(D->Building.BundledUnit, Owner, SpawnAt);
            }
        }
    }
}

// ADR-0013. Repairs damaged buildings the player has switched repair on for, charging
// per hitpoint restored and slowing or stopping under a power deficit.
//
// Repair is a sustained paid activity rather than a one-shot order, which is why it needs
// a system at all: the RepairBuilding command previously validated and then did nothing.
void SimWorld::SystemRepair()
{
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner >= kMaxPlayers)
        {
            continue;
        }
        BuildingComp& B = Buildings[I];
        if (!B.bRepairing)
        {
            continue;
        }
        // A building sold or already doomed this tick must not be repaired: SystemDeaths
        // has not run yet, so it is still bAlive here, and both the credits and the
        // hitpoints would go into something that is deleted before the tick ends. Same
        // check SystemFlowPayment makes, for the same reason -- selling a damaged
        // building with repair armed charged for a repair nobody ever saw.
        if (std::find(PendingDestroy.begin(), PendingDestroy.end(), MakeId(I)) != PendingDestroy.end())
        {
            continue;
        }
        // A half-built structure gains health from SystemConstruction; repairing it too
        // would pay twice for the same hitpoints.
        if (B.State != ConstructionState::Complete)
        {
            continue;
        }

        HealthComp& H = Healths[I];
        if (H.Current >= H.Max)
        {
            // Switch itself off rather than sit armed: otherwise the next scratch would
            // silently start spending again without the player asking.
            B.bRepairing = false;
            B.RepairCreditAccumulator = 0;
            continue;
        }

        PlayerState& P = Players[Core[I].Owner];
        const PowerTier Tier = P.GetPowerTier();
        // Repair is Auxiliary work in ADR-0013's table: halved at Moderate, off from
        // Severe. A building whose own priority band is offline cannot be repaired at
        // all, which is the player's own choice expressing itself.
        int32_t SpeedPercent = RepairSpeedPercentForTier(Tier);
        if (IsPowerPriorityOffline(B.Priority, Tier))
        {
            SpeedPercent = 0;
        }
        if (SpeedPercent <= 0)
        {
            continue;   // paused, not cancelled: the flag stays on and resumes on recovery
        }

        const int32_t Missing = H.Max - H.Current;
        int32_t Heal = std::max(1, (kRepairHealthPerTick * SpeedPercent) / 100);
        Heal = std::min(Heal, Missing);

        // Bill in hundredths and spend only whole credits, so a sub-credit-per-tick rate
        // is neither rounded up into extortion nor down into free repair. A tick whose
        // accumulated bill has not yet reached a whole credit still heals -- the charge
        // is deferred, not waived, and over any run of ticks the totals balance exactly.
        //
        // The exception is a player who cannot pay. Deferring a bill they will never
        // settle *is* free repair, so affordability is checked against the accumulated
        // total rather than against this tick's whole-credit slice: at Moderate the slice
        // is zero on every other tick, and billing only when it is non-zero handed out
        // health for nothing on all the others.
        const int32_t PendingCenti = B.RepairCreditAccumulator + Heal * kRepairCostPerHealthCenti;
        const int32_t Affordable = std::max(0, P.Credits) * kRepairCostScale;
        if (PendingCenti > Affordable)
        {
            // Heal only what the treasury actually covers, and take every credit of it.
            const int32_t HealableCenti = std::max(0, Affordable - B.RepairCreditAccumulator);
            Heal = HealableCenti / kRepairCostPerHealthCenti;
            if (Heal <= 0)
            {
                continue;   // cannot afford even one hitpoint
            }
        }

        B.RepairCreditAccumulator += Heal * kRepairCostPerHealthCenti;
        const int32_t Due = B.RepairCreditAccumulator / kRepairCostScale;
        if (Due > 0)
        {
            // Affordable by construction above, so this never partially pays.
            P.Credits -= Due;
            B.RepairCreditAccumulator -= Due * kRepairCostScale;
        }

        if (Heal > 0)
        {
            H.Current = std::min(H.Max, H.Current + Heal);
        }
    }
}

void SimWorld::SystemProduction()
{
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building)
        {
            continue;
        }
        BuildingComp& B = Buildings[I];
        if (B.State != ConstructionState::Complete || B.Queue.empty())
        {
            continue;
        }

        const PlayerId Owner = Core[I].Owner;
        // ADR-0013: speed follows the tier table. Whether this producer is allowed to
        // run at all at Critical is decided below -- it depends on what the building
        // is, not just on the ratio.
        const int32_t Ratio = Owner < kMaxPlayers ? PowerSpeedPercent(Owner) : 100;

        // At Critical only infantry production and the construction yard keep going.
        // The yard is deliberately included: without it a blacked-out base could not
        // build the power plant that ends the blackout, which is a deadlock rather
        // than a penalty.
        //
        // Same predicate SystemFlowPayment uses, so a stalled item is never charged
        // for a tick it did not advance. The two deciding this separately is what
        // produced a blackout the player could not escape.
        if (IsProductionPowerStalled(I))
        {
            continue;
        }

        // Only the head of the queue advances; parallel queues are per building,
        // matching the original games.
        ProductionItem& QueueItem = B.Queue.front();
        // ADR-0012: progress is bought tick by tick, so it advances while the item is
        // still Funding -- payment and construction run together, which is what makes
        // CostPerTick = TotalCost / TotalTicks add up to the full price exactly as the
        // last tick completes. Advancing only after full payment would silently double
        // every build time.
        //
        // Starved, EnergyThrottled, ManuallyPaused and Queued do not advance: an item
        // must not gain progress on a tick it failed to pay for, or a player could
        // build an army on credit they never had.
        if (QueueItem.State != FlowPaymentState::Funding &&
            QueueItem.State != FlowPaymentState::Paying &&
            QueueItem.State != FlowPaymentState::Completed)
        {
            continue;
        }
        const int32_t Complete = QueueItem.TotalTicks * kProgressScale;
        if (QueueItem.ProgressTicks >= Complete)
        {
            if (!QueueItem.IsFullyFunded())
            {
                continue;   // finished building, still finishing paying
            }
            // Structures wait here until the player picks a spot; everything else
            // pops out immediately.
            QueueItem.State = FlowPaymentState::Completed;
            const EntityDef* Item = Content->FindEntity(QueueItem.Content);
            if (Item != nullptr && Item->Kind == EntityKind::Building)
            {
                continue;
            }
        }
        else
        {
            QueueItem.ProgressTicks += Ratio;
            if (QueueItem.ProgressTicks < Complete)
            {
                continue;
            }
            // Belt and braces: nothing is ever handed over unpaid. Ceiling division
            // means payment normally lands on or before the last progress tick, and
            // Starved items are already excluded above, so this should be
            // unreachable -- but the alternative to holding back is delivering a
            // unit the player did not finish buying, so the check stays.
            if (!QueueItem.IsFullyFunded())
            {
                QueueItem.ProgressTicks = Complete - 1;
                continue;
            }
            QueueItem.State = FlowPaymentState::Completed;
        }

        // RA3-style upgrade: an Ability-category queue item whose content is an
        // UpgradeDef, not an EntityDef. It completes in place: no entity spawns,
        // the modifier lands on the player, and the queue item is consumed.
        if (Content->FindUpgrade(QueueItem.Content) != nullptr)
        {
            if (Owner < kMaxPlayers)
            {
                Players[Owner].ResearchedUpgrades.push_back(QueueItem.Content);
            }
            SimEvent Ev;
            Ev.Type = SimEventType::UpgradeResearched;
            Ev.Tick = CurrentTick;
            Ev.Entity = MakeId(I);
            Ev.Player = Owner;
            Ev.Content = QueueItem.Content;
            EmitEvent(Ev);
            B.Queue.erase(B.Queue.begin());
            continue;
        }

        const EntityDef* Item = Content->FindEntity(QueueItem.Content);
        if (Item == nullptr)
        {
            // The content vanished under us (a hot-reload or a bad save). The player
            // was charged for this a slice at a time and watched the money go, so
            // refund it in full and say so, rather than deleting the entry and
            // leaving an unexplained drain.
            if (Owner < kMaxPlayers && QueueItem.PaidCredits > 0)
            {
                Players[Owner].Credits += QueueItem.PaidCredits;

                SimEvent Lost;
                Lost.Type = SimEventType::ProductionCancelled;
                Lost.Tick = CurrentTick;
                Lost.Entity = MakeId(I);
                Lost.Player = Owner;
                Lost.Content = QueueItem.Content;
                Lost.Value = QueueItem.PaidCredits;
                EmitEvent(Lost);
            }
            B.Queue.erase(B.Queue.begin());
            continue;
        }
        if (Item->Kind == EntityKind::Building)
        {
            continue;   // awaiting placement
        }

        const Vec2 SpawnAt = FindFreeSpawnPoint(B, QueueItem.Content);
        const EntityId Spawned = SpawnUnit(QueueItem.Content, Owner, SpawnAt);
        if (!Spawned.IsValid())
        {
            continue;   // entity budget exhausted; retry next tick
        }

        if (B.bHasRallyPoint)
        {
            Order O;
            O.Type = OrderType::Move;
            O.Location = B.RallyPoint;
            Orders[Spawned.Index].Push(O);
        }

        SimEvent Ev;
        Ev.Type = SimEventType::ProductionCompleted;
        Ev.Tick = CurrentTick;
        Ev.Entity = Spawned;
        Ev.Other = MakeId(I);
        Ev.Player = Owner;
        Ev.Content = QueueItem.Content;
        Ev.Location = SpawnAt;
        EmitEvent(Ev);

        B.Queue.erase(B.Queue.begin());
    }
}

EntityId SimWorld::FindNearestResourceNode(const Vec2& From, PlayerId Owner) const
{
    // Load tally: how many friendly harvesters are already committed to each node.
    // Without it every idle harvester independently picked the same nearest field and
    // queued on one tile while an equally good field sat untouched beside it. The
    // scan is O(entities) per call, and calls happen only when a harvester goes idle
    // or loses its field -- not per harvester per tick. Deterministic: the tally is
    // a pure function of live state and candidates are visited in ascending index,
    // so every machine picks the same node.
    ResourceLoadScratch.assign(Core.size(), 0);
    if (Owner < kMaxPlayers)
    {
        for (uint32_t I = 0; I < Core.size(); ++I)
        {
            if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit || Core[I].Owner != Owner)
            {
                continue;
            }
            const HarvesterComp& H = Harvesters[I];
            // Only committed states count: an idle or returning harvester is free to
            // pick anything, and counting it would push later pickers off its old
            // target for no reason.
            if ((H.State == HarvesterState::MovingToResource || H.State == HarvesterState::Harvesting)
                && IsAlive(H.AssignedNode))
            {
                ++ResourceLoadScratch[H.AssignedNode.Index];
            }
        }
    }

    EntityId Best = EntityId::Invalid();
    uint32_t BestLoad = 0;
    Fixed BestDistSq = Fixed::Max();
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::ResourceNode || ResourceNodes[I].Amount <= 0)
        {
            continue;
        }
        const Fixed DistSq = DistanceSquared(From, Transforms[I].Position);
        const bool bBetter = !Best.IsValid() || ResourceLoadScratch[I] < BestLoad
                             || (ResourceLoadScratch[I] == BestLoad && DistSq < BestDistSq);
        if (bBetter)
        {
            BestLoad = ResourceLoadScratch[I];
            BestDistSq = DistSq;
            Best = MakeId(I);
        }
    }
    return Best;
}

bool SimWorld::IsRegrowingNode(EntityId NodeId) const
{
    if (!IsAlive(NodeId) || Core[NodeId.Index].Kind != EntityKind::ResourceNode)
    {
        return false;
    }
    const ResourceNodeDef* NodeDef = Content->FindResourceNode(Core[NodeId.Index].Def);
    return NodeDef != nullptr && NodeDef->bRegrows;
}

EntityId SimWorld::FindNearestRefinery(const Vec2& From, PlayerId Owner) const
{
    EntityId Best = EntityId::Invalid();
    Fixed BestDistSq = Fixed::Max();
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner != Owner)
        {
            continue;
        }
        if (Buildings[I].State != ConstructionState::Complete)
        {
            continue;
        }
        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr || !D->Building.bIsRefinery)
        {
            continue;
        }
        const Fixed DistSq = DistanceSquared(From, Transforms[I].Position);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = MakeId(I);
        }
    }
    return Best;
}

// Air logistics. Same scan shape as FindNearestRefinery: ascending index,
// strict-closer tie-break, so every peer picks the same pad. A rearm point is
// any COMPLETE friendly building whose production category builds aircraft --
// derived from content, never from a flag a definition could forget to set.
EntityId SimWorld::FindNearestRearmPoint(const Vec2& From, PlayerId Owner) const
{
    EntityId Best = EntityId::Invalid();
    Fixed BestDistSq = Fixed::Max();
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner != Owner)
        {
            continue;
        }
        if (Buildings[I].State != ConstructionState::Complete)
        {
            continue;
        }
        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr || D->Production.Category != ProductionCategory::Aircraft)
        {
            continue;
        }
        const Fixed DistSq = DistanceSquared(From, Transforms[I].Position);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = MakeId(I);
        }
    }
    return Best;
}

void SimWorld::SystemHarvesters()
{
    // Resource regrowth (ResourceNodeDef::bRegrows). Runs before the harvester pass
    // so a field that crept back above zero this tick is immediately a legal target.
    // Integer arithmetic only: RegrowPerTick has no fractional part by contract, so
    // no accumulator is needed and no per-node scratch state exists.
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::ResourceNode)
        {
            continue;
        }
        const ResourceNodeDef* NodeDef = Content->FindResourceNode(Core[I].Def);
        if (NodeDef == nullptr || !NodeDef->bRegrows || NodeDef->RegrowPerTick <= 0)
        {
            continue;
        }
        ResourceNodeComp& Node = ResourceNodes[I];
        if (Node.Amount < NodeDef->MaxAmount)
        {
            Node.Amount = std::min(NodeDef->MaxAmount, Node.Amount + NodeDef->RegrowPerTick);
        }
    }

    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit)
        {
            continue;
        }
        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr || !D->Unit.bIsHarvester)
        {
            continue;
        }

        HarvesterComp& H = Harvesters[I];
        const PlayerId Owner = Core[I].Owner;
        const Vec2 Pos = Transforms[I].Position;

        // An explicit Harvest order overrides whatever the automation chose.
        if (!Orders[I].IsEmpty() && Orders[I].Front().Type == OrderType::Harvest)
        {
            const Order& O = Orders[I].Front();
            if (IsAlive(O.Target) && Core[O.Target.Index].Kind == EntityKind::ResourceNode)
            {
                H.AssignedNode = O.Target;
                H.State = HarvesterState::MovingToResource;
            }
            Orders[I].PopFront();
        }
        // A manual move order suspends harvesting until it finishes.
        else if (!Orders[I].IsEmpty())
        {
            continue;
        }

        switch (H.State)
        {
            case HarvesterState::Idle:
            {
                if (H.Cargo > 0)
                {
                    H.State = HarvesterState::MovingToRefinery;
                    break;
                }
                const EntityId Node = FindNearestResourceNode(Pos, Owner);
                if (Node.IsValid())
                {
                    H.AssignedNode = Node;
                    H.State = HarvesterState::MovingToResource;
                }
                break;
            }

            case HarvesterState::MovingToResource:
            {
                if (!IsAlive(H.AssignedNode) || ResourceNodes[H.AssignedNode.Index].Amount <= 0)
                {
                    H.AssignedNode = FindNearestResourceNode(Pos, Owner);
                    if (!H.AssignedNode.IsValid())
                    {
                        H.State = HarvesterState::Idle;
                        Movements[I].bHasDestination = false;
                        break;
                    }
                }
                const Vec2 NodePos = Transforms[H.AssignedNode.Index].Position;
                const Fixed NodeDock = DockRadiusFor(Content->FindEntity(Core[H.AssignedNode.Index].Def));
                if (DistanceSquared(Pos, NodePos) <= NodeDock * NodeDock)
                {
                    H.State = HarvesterState::Harvesting;
                    Movements[I].bHasDestination = false;
                    Movements[I].CurrentSpeed = Fixed::Zero();
                }
                else
                {
                    Movements[I].Destination = NodePos;
                    Movements[I].bHasDestination = true;
                }
                break;
            }

            case HarvesterState::Harvesting:
            {
                if (!IsAlive(H.AssignedNode))
                {
                    H.State = H.Cargo > 0 ? HarvesterState::MovingToRefinery : HarvesterState::Idle;
                    break;
                }
                ResourceNodeComp& Node = ResourceNodes[H.AssignedNode.Index];
                const int32_t Space = D->Unit.CargoCapacity - H.Cargo;
                // ADR-0013: harvesting is the last thing a deficit touches -- it runs
                // at full rate until Critical, then at half. Slowing it earlier would
                // make a blackout self-reinforcing, since income is what buys the
                // power plant that ends it.
                int32_t PerTick = D->Unit.HarvestPerTick;
                if (Owner < kMaxPlayers && Players[Owner].GetPowerTier() == PowerTier::Critical)
                {
                    // Never round down to zero: a rate of nothing is a stall, not a
                    // penalty, and it would deadlock the economy.
                    PerTick = std::max(1, (PerTick * kPowerCriticalSpeedPercent) / 100);
                }
                const int32_t Take = std::min({PerTick, Space, Node.Amount});
                Node.Amount -= Take;
                H.Cargo += Take;
                // Cargo is priced by where it came from (see HarvesterComp::CargoDef),
                // so every scoop refreshes the source. Mixed cargo from two fields
                // prices at the most recent one, which matches how the classic games
                // keep one cargo value per trip.
                if (Take > 0)
                {
                    H.CargoDef = Core[H.AssignedNode.Index].Def;
                }

                if (Node.Amount <= 0 && !IsRegrowingNode(H.AssignedNode))
                {
                    // Exhausted fields stop being targets; the tile flag drives the
                    // minimap and the AI's expansion logic. A regrowing field is
                    // exempt: it stays put empty and creeps back to life, so
                    // destroying it here would delete a renewable resource forever.
                    const TileCoord T = Map.WorldToTile(Transforms[H.AssignedNode.Index].Position);
                    Map.SetTileFlag(T.X, T.Y, Tile_Resource, false);
                    QueueDestroy(H.AssignedNode);
                }
                if (H.Cargo >= D->Unit.CargoCapacity || Node.Amount <= 0)
                {
                    H.State = HarvesterState::MovingToRefinery;
                }
                break;
            }

            case HarvesterState::MovingToRefinery:
            {
                if (!IsAlive(H.AssignedRefinery) || Buildings[H.AssignedRefinery.Index].State != ConstructionState::Complete)
                {
                    H.AssignedRefinery = FindNearestRefinery(Pos, Owner);
                }
                if (!H.AssignedRefinery.IsValid())
                {
                    Movements[I].bHasDestination = false;
                    break;
                }
                BuildingComp& RefComp = Buildings[H.AssignedRefinery.Index];
                const Vec2 RefPos = Transforms[H.AssignedRefinery.Index].Position;
                const Fixed RefDock = DockRadiusFor(Content->FindEntity(Core[H.AssignedRefinery.Index].Def));
                const EntityId SelfId = MakeId(I);

                if (DistanceSquared(Pos, RefPos) <= RefDock * RefDock)
                {
                    if (!RefComp.DockedHarvester.IsValid() || !IsAlive(RefComp.DockedHarvester) || RefComp.DockedHarvester == SelfId)
                    {
                        RefComp.DockedHarvester = SelfId;
                        H.State = HarvesterState::Unloading;
                        Movements[I].bHasDestination = false;
                        Movements[I].CurrentSpeed = Fixed::Zero();
                    }
                    else
                    {
                        if (std::find(RefComp.UnloadingQueue.begin(), RefComp.UnloadingQueue.end(), SelfId) == RefComp.UnloadingQueue.end())
                        {
                            RefComp.UnloadingQueue.push_back(SelfId);
                        }
                        Movements[I].bHasDestination = false;
                        Movements[I].CurrentSpeed = Fixed::Zero();
                    }
                }
                else
                {
                    Movements[I].Destination = RefPos;
                    Movements[I].bHasDestination = true;
                }
                break;
            }

            case HarvesterState::Unloading:
            {
                const EntityId SelfId = MakeId(I);
                if (!IsAlive(H.AssignedRefinery))
                {
                    H.State = HarvesterState::MovingToRefinery;
                    break;
                }
                BuildingComp& RefComp = Buildings[H.AssignedRefinery.Index];
                const int32_t Amount = std::min(D->Unit.UnloadPerTick, H.Cargo);
                H.Cargo -= Amount;

                // The refinery pays what the cargo is worth where it was dug up, not
                // a global flat price: this used to hardcode "resource.ore_field" for
                // every delivery, which silently made rich ore worthless per unit.
                // CargoDef is invalid only for cargo gathered by a pre-CargoDef save;
                // that legacy load prices at the standard field rather than guessing.
                const ResourceNodeDef* NodeDef = Content->FindResourceNode(
                    H.CargoDef.IsValid() ? H.CargoDef : MakeContentId("resource.ore_field"));
                const int32_t Value = NodeDef ? NodeDef->ValuePerUnit : 1;
                if (Owner < kMaxPlayers)
                {
                    Players[Owner].Credits += Amount * Value;
                    Players[Owner].TotalHarvested += Amount * Value;
                }

                if (Amount > 0)
                {
                    SimEvent Ev;
                    Ev.Type = SimEventType::ResourceDelivered;
                    Ev.Tick = CurrentTick;
                    Ev.Entity = SelfId;
                    Ev.Other = H.AssignedRefinery;
                    Ev.Player = Owner;
                    Ev.Value = Amount * Value;
                    EmitEvent(Ev);
                }

                if (H.Cargo <= 0)
                {
                    if (RefComp.DockedHarvester == SelfId)
                    {
                        RefComp.DockedHarvester = EntityId::Invalid();
                        while (!RefComp.UnloadingQueue.empty())
                        {
                            EntityId NextId = RefComp.UnloadingQueue.front();
                            RefComp.UnloadingQueue.erase(RefComp.UnloadingQueue.begin());
                            if (IsAlive(NextId))
                            {
                                RefComp.DockedHarvester = NextId;
                                Harvesters[NextId.Index].State = HarvesterState::Unloading;
                                break;
                            }
                        }
                    }
                    H.State = HarvesterState::MovingToResource;
                }
                break;
            }
        }
    }
}

void SimWorld::SystemOrders()
{
    // Group-size tally for the arrive radius, built once per tick. SystemMovement
    // builds the same tally from the same state and both call
    // ScaleArriveRadiusForGroup, so the radius that pops an order here is identical
    // to the radius that stops the unit there. They must not diverge: if this system
    // demanded a tighter radius than SystemMovement, the unit would stop moving while
    // its Move order stayed queued, and the code below would re-set bHasDestination
    // every tick forever.
    //
    // Rebuilt here rather than shared through a member so this change stays inside
    // SimWorld.cpp. The two passes read the same Movements array in the same tick
    // with no intervening writes to bHasDestination or Destination -- SystemOrders is
    // the first of the pair -- so both see the same set. Note the ordering subtlety:
    // this tally is taken BEFORE this system updates destinations, so it reflects
    // last tick's assignments. That is deliberate and harmless: group size is used
    // only to widen a tolerance, and a one-tick-stale crowd count changes the radius
    // by at most one unit's worth of contribution.
    std::vector<uint64_t> DestTileKeys;
    DestTileKeys.reserve(Core.size());
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit) continue;
        if (!Movements[I].bHasDestination) continue;
        DestTileKeys.push_back(PackDestTileKey(Map.WorldToTile(Movements[I].Destination)));
    }
    std::sort(DestTileKeys.begin(), DestTileKeys.end());

    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit)
        {
            continue;
        }
        // Cargo cannot act; a stunned or frozen crew cannot take new steps.
        if (PassengerOf[I].Transport.IsValid() || !Statuses[I].bCanAct())
        {
            continue;
        }
        OrderQueue& Q = Orders[I];
        MovementComp& M = Movements[I];
        CombatComp& C = Combats[I];

        if (Q.IsEmpty())
        {
            C.bTargetIsForced = false;
            continue;
        }

        Order& O = Q.Front();
        switch (O.Type)
        {
            case OrderType::Board:
            {
                const EntityId TransportId = O.Target;
                if (!IsAlive(TransportId) || Core[TransportId.Index].Owner != Core[I].Owner)
                {
                    Q.PopFront();
                    break;
                }
                const Vec2 TPos = Transforms[TransportId.Index].Position;
                if (DistanceSquared(Transforms[I].Position, TPos) > Fixed::FromInt(150) * Fixed::FromInt(150))
                {
                    M.Destination = TPos;
                    M.bHasDestination = true;
                    break;   // keep closing until dock range
                }
                const EntityDef* TD = Content->FindEntity(Core[TransportId.Index].Def);
                TransportComp& Bay = Transports[TransportId.Index];
                if (TD == nullptr || Bay.Passengers.size() >= size_t(std::max(0, TD->Unit.PassengerCapacity)))
                {
                    Q.PopFront();
                    break;
                }
                // Board: dormant from this tick on.
                Bay.Passengers.push_back(MakeId(I));
                PassengerOf[I].Transport = TransportId;
                Orders[I].Clear();
                Movements[I].bHasDestination = false;
                break;
            }

            case OrderType::Unload:
            {
                // Eject every passenger around the transport. Positions ring out
                // deterministically from the transport's tile.
                for (EntityId& P : Transports[I].Passengers)
                {
                    if (!IsAlive(P))
                    {
                        continue;
                    }
                    PassengerOf[P.Index].Transport = EntityId::Invalid();
                }
                Transports[I].Passengers.clear();
                Q.PopFront();
                break;
            }

            case OrderType::Capture:
            {
                const EntityId TargetId = O.Target;
                if (!IsAlive(TargetId) || Core[TargetId.Index].Kind != EntityKind::Building
                    || Core[TargetId.Index].Owner == Core[I].Owner)
                {
                    Captures[I].ProgressTicks = 0;
                    Q.PopFront();
                    break;
                }
                const Vec2 TPos = Transforms[TargetId.Index].Position;
                const Fixed Dock = Fixed::FromInt(200);
                if (DistanceSquared(Transforms[I].Position, TPos) > Dock * Dock)
                {
                    Captures[I].ProgressTicks = 0;
                    M.Destination = TPos;
                    M.bHasDestination = true;
                    break;   // walk to the door first
                }
                // Channel the takeover; leaving or dying resets via the pop above.
                Captures[I].ProgressTicks += 1;
                if (Captures[I].ProgressTicks >= kCaptureChannelTicks)
                {
                    const PlayerId OldOwner = Core[TargetId.Index].Owner;
                    Core[TargetId.Index].Owner = Core[I].Owner;
                    if (OldOwner < kMaxPlayers)
                    {
                        RefreshPlayerTech(OldOwner);
                    }
                    RefreshPlayerTech(Core[I].Owner);
                    // A captured fixture may sit outside any build network; its new
                    // owner simply owns the building and its income.
                    SimEvent Ev;
                    Ev.Type = SimEventType::BuildingCaptured;
                    Ev.Tick = CurrentTick;
                    Ev.Entity = TargetId;
                    Ev.Other = MakeId(I);
                    Ev.Player = Core[I].Owner;
                    Ev.Location = TPos;
                    EmitEvent(Ev);
                    // The engineer is consumed by the capture, classic price.
                    Healths[I].Current = 0;
                    QueueDestroy(MakeId(I));
                    Captures[I].ProgressTicks = 0;
                    Q.PopFront();
                }
                break;
            }

            case OrderType::Move:
            {
                M.Destination = O.Location;
                M.bHasDestination = true;
                const Fixed D2 = DistanceSquared(Transforms[I].Position, O.Location);
                const Fixed OrderArrive = ScaleArriveRadiusForGroup(
                    M.ArriveRadius, GroupSizeAtDestTile(DestTileKeys, Map.WorldToTile(O.Location)));
                if (D2 <= OrderArrive * OrderArrive)
                {
                    M.bHasDestination = false;
                    M.CurrentSpeed = Fixed::Zero();
                    Q.PopFront();
                }
                // Give up rather than grind forever against an obstacle -- but only
                // for a unit that is genuinely stuck, never for one that is merely
                // waiting its turn in a crowd.
                //
                // WHY THIS IS NOT A PLAIN BlockedTicks THRESHOLD ANY MORE. The
                // reservation tie-break at ReservationGrid.cpp:50 is a static
                // priority: a tile is only displaced by a *strictly lower* entity
                // slot. In a dense crowd converging on one area, a high-index unit
                // therefore loses every contest to every lower-index neighbour and
                // takes the `BlockedTicks += 1` branch in SystemMovement step 7b for
                // as long as the jam lasts. That is ordinary, self-clearing
                // congestion: the units ahead do move on, and the jam drains.
                //
                // Because SystemOrders runs immediately BEFORE SystemMovement
                // (SimWorld.cpp:3628-3629), the old unconditional test here fired
                // first and cancelled the Move order at the same threshold that
                // SystemMovement uses to trigger a repath -- so the order was
                // destroyed on precisely the tick the navigation layer was about to
                // retry. Measured on 200 units sent to one rally point: 73 of them
                // had their order cancelled here between ticks 60 and 63 while still
                // 17-63 tiles away, having never been blocked by any terrain. Only
                // 127 of 200 arrived. Nothing recovers from this, because popping the
                // order is permanent while the congestion was temporary.
                //
                // The condition now requires BOTH of:
                //   (a) the unit has been blocked for the threshold, AND
                //   (b) the navigation layer has already tried to repath it and still
                //       cannot make progress -- i.e. this is a routing failure, not a
                //       queueing delay.
                // SystemMovement sets LastRepathTick only on the paths that clear a
                // dead macro path (no route found, or blocked long enough to force a
                // rebuild). A unit losing reservation contests never sets it, so
                // congestion alone can no longer pop an order.
                //
                // The extra grace window is deliberate and is what makes (b) a real
                // second opinion rather than a restatement of (a): after a repath is
                // requested the unit is given another full threshold to act on the
                // new route before its order is abandoned. A unit that repaths and
                // then moves resets BlockedTicks and never reaches this branch at all.
                //
                // Deterministic: integer tick arithmetic only, and both operands are
                // authoritative serialized state (BlockedTicks, LastRepathTick), so a
                // save/load round-trip resumes with the identical give-up decision.
                // TickIndex is unsigned, so the elapsed-time comparison is written as
                // an addition rather than a subtraction to avoid wrapping when
                // LastRepathTick is still its initial 0.
                else if (M.BlockedTicks > kTicksPerSecond * 3 &&
                         M.LastRepathTick > 0 &&
                         CurrentTick >= M.LastRepathTick +
                             static_cast<TickIndex>(kRepathBlockedTickThreshold))
                {
                    M.BlockedTicks = 0;
                    M.bHasDestination = false;
                    Q.PopFront();
                }
                break;
            }

            case OrderType::AttackMove:
            {
                const EntityId Acquired = AcquireTarget(MakeId(I));
                if (Acquired.IsValid())
                {
                    C.Target = Acquired;
                    C.bTargetIsForced = false;
                    // Sight can extend beyond weapon range. Close to the same stable
                    // 90%-range envelope as an explicit Attack order before stopping;
                    // otherwise a unit freezes the instant it sees an enemy and can
                    // remain permanently too far away to fire.
                    const ContentId FireWeaponId = ResolveFireWeapon(MakeId(I));
                    const WeaponDef* W = FireWeaponId.IsValid()
                        ? Content->FindWeapon(FireWeaponId) : nullptr;
                    if (W != nullptr)
                    {
                        const Vec2 TargetPos = Transforms[Acquired.Index].Position;
                        const Fixed Engage = (W->MaxRange * 9) / 10;
                        if (DistanceSquared(Transforms[I].Position, TargetPos) > Engage * Engage)
                        {
                            M.Destination = TargetPos;
                            M.bHasDestination = true;
                        }
                        else
                        {
                            // Stop and shoot; resume the advance once the area is clear.
                            M.bHasDestination = false;
                            M.CurrentSpeed = Fixed::Zero();
                        }
                    }
                }
                else
                {
                    M.Destination = O.Location;
                    M.bHasDestination = true;
                    const Fixed AmArrive = ScaleArriveRadiusForGroup(
                        M.ArriveRadius, GroupSizeAtDestTile(DestTileKeys, Map.WorldToTile(O.Location)));
                    if (DistanceSquared(Transforms[I].Position, O.Location) <= AmArrive * AmArrive)
                    {
                        M.bHasDestination = false;
                        Q.PopFront();
                    }
                }
                break;
            }

            case OrderType::Attack:
            {
                if (!IsAlive(O.Target))
                {
                    C.Target = EntityId::Invalid();
                    C.bTargetIsForced = false;
                    M.bHasDestination = false;
                    Q.PopFront();
                    break;
                }
                C.Target = O.Target;
                C.bTargetIsForced = true;

                const ContentId FireWeaponId = ResolveFireWeapon(MakeId(I));
                const WeaponDef* W = FireWeaponId.IsValid()
                    ? Content->FindWeapon(FireWeaponId) : nullptr;
                if (W == nullptr)
                {
                    Q.PopFront();
                    break;
                }
                const Vec2 TargetPos = Transforms[O.Target.Index].Position;
                const Fixed DistSq = DistanceSquared(Transforms[I].Position, TargetPos);
                // Close to 90% of maximum range before stopping, so that a target
                // edging away does not make the whole group oscillate.
                const Fixed Engage = (W->MaxRange * 9) / 10;
                if (DistSq > Engage * Engage)
                {
                    M.Destination = TargetPos;
                    M.bHasDestination = true;
                }
                else
                {
                    M.bHasDestination = false;
                    M.CurrentSpeed = Fixed::Zero();
                }
                break;
            }

            case OrderType::Guard:
            {
                M.bHasDestination = false;
                const EntityId Acquired = AcquireTarget(MakeId(I));
                C.Target = Acquired;
                C.bTargetIsForced = false;
                break;
            }

            case OrderType::Deploy:
            {
                const EntityDef* D = Content ? Content->FindEntity(Core[I].Def) : nullptr;
                if (D != nullptr && D->Unit.bIsBuilder && D->Unit.DeploysInto.IsValid())
                {
                    M.bHasDestination = false;
                    M.CurrentSpeed = Fixed::Zero();

                    TileCoord Tile = Map.WorldToTile(Transforms[I].Position);
                    const EntityDef* ConYardDef = Content->FindEntity(D->Unit.DeploysInto);
                    const int32_t FootX = ConYardDef ? ConYardDef->Building.FootprintX : 3;
                    const int32_t FootY = ConYardDef ? ConYardDef->Building.FootprintY : 3;

                    // Intelligent proximity search for best clear placement tile
                    TileCoord BestTile = Tile;
                    bool bFoundValidTile = false;

                    auto IsTileClearForConYard = [&](const TileCoord& T) -> bool
                    {
                        if (T.X < 1 || T.Y < 1 || T.X + FootX >= Map.Width - 1 || T.Y + FootY >= Map.Height - 1)
                        {
                            return false;
                        }
                        for (int32_t FY = 0; FY < FootY; ++FY)
                        {
                            for (int32_t FX = 0; FX < FootX; ++FX)
                            {
                                const uint8_t Flags = Map.GetTile(T.X + FX, T.Y + FY);
                                if ((Flags & Tile_GroundPassable) == 0 ||
                                    (Flags & (Tile_Water | Tile_Cliff | Tile_Resource)) != 0)
                                {
                                    return false;
                                }
                            }
                        }
                        return true;
                    };

                    if (IsTileClearForConYard(Tile))
                    {
                        BestTile = Tile;
                        bFoundValidTile = true;
                    }
                    else
                    {
                        for (int32_t Radius = 1; Radius <= 8 && !bFoundValidTile; ++Radius)
                        {
                            for (int32_t DY = -Radius; DY <= Radius && !bFoundValidTile; ++DY)
                            {
                                for (int32_t DX = -Radius; DX <= Radius && !bFoundValidTile; ++DX)
                                {
                                    if (std::abs(DX) == Radius || std::abs(DY) == Radius)
                                    {
                                        TileCoord Candidate{Tile.X + DX, Tile.Y + DY};
                                        if (IsTileClearForConYard(Candidate))
                                        {
                                            BestTile = Candidate;
                                            bFoundValidTile = true;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (bFoundValidTile)
                    {
                        const Vec2 DeployLoc = Map.TileCenterToWorld(BestTile);
                        const PlayerId Owner = Core[I].Owner;
                        const ContentId DeploysInto = D->Unit.DeploysInto;

                        // Silently remove MCV
                        RemoveEntitySilently(MakeId(I));

                        // Spawn the Construction Yard
                        EntityId NewBuilding = SpawnBuilding(DeploysInto, Owner, BestTile, /*bInstantComplete*/ true);

                        // Emit MCVDeployed event
                        SimEvent Ev;
                        Ev.Type = SimEventType::MCVDeployed;
                        Ev.Tick = CurrentTick;
                        Ev.Entity = NewBuilding;
                        Ev.Player = Owner;
                        Ev.Content = DeploysInto;
                        Ev.Location = DeployLoc;
                        EmitEvent(Ev);
                    }
                }
                Q.PopFront();
                break;
            }

            case OrderType::Harvest:
            case OrderType::DeliverToRefinery:
            case OrderType::None:
                Q.PopFront();
                break;
        }
    }
}

namespace
{
// Sentinel for "this formation has no living leader this tick".
constexpr uint32_t kNoFormationLeader = 0xFFFFFFFFu;

// Returns the entity slot leading Formation, or kNoFormationLeader.
//
// A file-local free function rather than a SimWorld method: adding a member would
// mean declaring it in SimWorld.h, and this change is confined to SimWorld.cpp.
//
// The leader is not stored anywhere. MovementComp carries only FormationId and
// FormationSlot, and there is no leader handle on the component or on SimWorld. The
// leader is therefore identified by its slot, and BOTH documented spellings of
// "leader" are accepted: SimTypes.h:183 defines -1 as "leader or unassigned", while
// Formation.h:81-83 defines slot 0 as the leader's own zero-offset slot. Accepting
// only one of the two would leave a formation assembled under the other convention
// permanently leaderless, and every member would then silently fall back to its own
// destination -- the formation would look like it simply did not work.
//
// Where several units qualify -- a malformed assignment, or a leader killed and its
// formation id reused -- the lowest entity slot wins. That is the same deterministic
// tie-break ReservationGrid uses, so every peer selects the same leader.
//
// COST: linear in the entity count, per member, per tick. That is acceptable only
// because formations are rare and small relative to kMaxEntities; if formations
// become common this wants a leader handle on the component or a per-tick index,
// which is a SimWorld.h change and therefore out of scope here.
uint32_t FindFormationLeader(const std::vector<EntityCore>& Core,
                            const std::vector<MovementComp>& Movements,
                            ContentId Formation)
{
    if (!Formation.IsValid())
    {
        return kNoFormationLeader;
    }
    for (uint32_t I = 0; I < uint32_t(Core.size()); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit)
        {
            continue;
        }
        if (Movements[I].FormationId == Formation && Movements[I].FormationSlot <= 0)
        {
            return I;
        }
    }
    return kNoFormationLeader;
}
} // namespace

void SimWorld::SystemMovement()
{
    if (Reservations) Reservations->Expire(CurrentTick);
    FlowFieldBuildsThisTick = 0;
    MacroPathBuildsThisTick = 0;

    // Shared turn / accelerate / advance step. Extracted so the final approach and
    // the flow-field path steer through identical code -- if they diverged, a unit
    // would behave differently in the last tile than it did on the way there.
    auto SteerToward = [this](uint32_t Index, const EntityDef& Def, const Vec2& Target,
                              const Nav::NavQuery& Query)
    {
        MovementComp& M = Movements[Index];
        TransformComp& T = Transforms[Index];

        const Vec2 SteeringDelta = Target - T.Position;
        if (SteeringDelta.LengthSquared().Raw == 0)
        {
            return;
        }

        const int32_t DesiredFacing = SteeringDelta.ToAngle();
        const int32_t TurnPerTick = std::max(1, Def.Unit.TurnRatePerSecond / kTicksPerSecond);
        const int32_t Diff = AngleDelta(T.Facing, DesiredFacing);
        if (Diff > TurnPerTick) { T.Facing = WrapAngle(T.Facing + TurnPerTick); }
        else if (Diff < -TurnPerTick) { T.Facing = WrapAngle(T.Facing - TurnPerTick); }
        else { T.Facing = DesiredFacing; }

        Fixed EffectiveMaxSpeed = Def.Unit.MaxSpeed;
        if (Combats[Index].bSecondaryModeActive)
        {
            EffectiveMaxSpeed = EffectiveMaxSpeed * Def.Unit.AbilitySpeedMultiplier;
        }
        const Fixed MaxSpeedPerTick = PerSecondToPerTick(EffectiveMaxSpeed);
        const Fixed AccelPerTick = PerSecondToPerTick(Def.Unit.Acceleration);
        const int32_t AlignedThreshold = kAngleTurn / 8;

        if (Diff > -AlignedThreshold && Diff < AlignedThreshold)
        {
            M.CurrentSpeed = FxMin(M.CurrentSpeed + AccelPerTick, MaxSpeedPerTick);
        }
        else
        {
            // Decelerate toward zero when not aligned with the target heading.
            // The old floor of MaxSpeedPerTick/4 made units slide at quarter speed
            // in whatever direction they happened to be facing -- including away
            // from the destination -- which is why harvesters drove past their
            // docking point and never arrived.
            M.CurrentSpeed = FxMax(M.CurrentSpeed - AccelPerTick, Fixed::Zero());
        }

        // Never overshoot the target inside one tick: at 25 units per tick a unit
        // would otherwise oscillate around a point it can never land on.
        Fixed StepLength = M.CurrentSpeed;
        const Fixed Remaining = SteeringDelta.Length();
        if (StepLength > Remaining)
        {
            StepLength = Remaining;
        }

        const Vec2 NextPos = T.Position + Vec2::FromAngle(T.Facing) * StepLength;
        const TileCoord NextPosTile = Map.WorldToTile(NextPos);
        if (NavigationGrid->IsTraversable(NextPosTile, Query))
        {
            T.Position = NextPos;
            M.BlockedTicks = 0;
        }
        else
        {
            M.CurrentSpeed = Fixed::Zero();
            M.BlockedTicks += 1;
        }

        // Invisible perimeter barrier: units can never cross or be pushed outside the playable map area
        if (Map.Width > 0 && Map.Height > 0)
        {
            const Fixed MinMargin = Fixed::FromInt(50);
            const Fixed MaxX = Fixed::FromInt(int64_t(Map.Width) * MapDescription::kTileSizeUnitsLocal) - MinMargin;
            const Fixed MaxY = Fixed::FromInt(int64_t(Map.Height) * MapDescription::kTileSizeUnitsLocal) - MinMargin;
            if (T.Position.X < MinMargin) { T.Position.X = MinMargin; M.CurrentSpeed = Fixed::Zero(); }
            else if (T.Position.X > MaxX) { T.Position.X = MaxX; M.CurrentSpeed = Fixed::Zero(); }
            if (T.Position.Y < MinMargin) { T.Position.Y = MinMargin; M.CurrentSpeed = Fixed::Zero(); }
            else if (T.Position.Y > MaxY) { T.Position.Y = MaxY; M.CurrentSpeed = Fixed::Zero(); }
        }
    };

    // --- Group-size arrive radius (SimTypes.h:172) --------------------------
    //
    // SimTypes.h documents ArriveRadius as "Scaled by group size in the navigation
    // milestone so a hundred units do not pile onto one point", but SpawnUnit set it
    // once from CollisionRadius and nothing ever scaled it. This tallies how many
    // living units share each destination tile this tick, so the radius used below can
    // widen with the crowd.
    //
    // Keyed on the destination TILE, not the exact Vec2. Two units sent to one spot by
    // a single order carry bit-identical destinations, but a unit whose destination was
    // derived (a harvester's node approach, a rally point) lands a few units off and
    // would hash to a different key while still contending for the same ground. The
    // tile is the granularity the reservation grid actually fights over, so it is the
    // granularity that matters here.
    //
    // Recomputed every tick rather than cached: group membership changes as units
    // arrive and orders are reissued, and a stale tally would leave a lone straggler
    // using a radius sized for the crowd it used to belong to.
    //
    // This is a MINOR contributor, as briefed. The dominant loss is reservation
    // priority starvation, addressed by the separation pass at the end of this
    // function. A wider arrival tolerance only stops the last few units of a genuine
    // pile-up from grinding; it cannot unfreeze a starved unit.
    std::vector<uint64_t> DestTileKeys;
    DestTileKeys.reserve(Core.size());
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || !Movements[I].bHasDestination) continue;
        DestTileKeys.push_back(PackDestTileKey(Map.WorldToTile(Movements[I].Destination)));
    }
    std::sort(DestTileKeys.begin(), DestTileKeys.end());

    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit) continue;
        // Cargo rides; a stunned or frozen crew cannot drive.
        if (!Statuses[I].bCanAct() || PassengerOf[I].Transport.IsValid()) continue;
        MovementComp& M = Movements[I];
        TransformComp& T = Transforms[I];
        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr) continue;

        // --- Formation slot derivation -------------------------------------
        // Members do not own a destination: theirs is LeaderPos + rotated slot
        // offset, recomputed every tick (the contract stated at Formation.h:2-7).
        //
        // This runs HERE, at the top of the movement pass, and not in SystemOrders,
        // because SystemOrders executes immediately before this system and writes
        // `M.Destination = O.Location` unconditionally from the order queue. Setting
        // a slot destination any earlier in the tick would simply be overwritten;
        // setting it in SystemOrders itself would fight the order queue every tick,
        // with whichever wrote last winning. Overriding after that write is the only
        // placement that yields one unambiguous destination per tick.
        //
        // It is idempotent: the slot point is a pure function of the leader's
        // position, the leader's facing and the slot offset. Nothing here reads or
        // mutates the order queue, so running it twice in a tick, or after any
        // other system, produces the same value. It does not depend on the relative
        // order of SystemOrders and SystemMovement for *correctness* of the value --
        // only for which of the two writes survives, which is the point.
        //
        // TWO CONVENTIONS FOR "LEADER", reconciled here. SimTypes.h:183 documents
        // `FormationSlot == -1` as "leader or unassigned", while Formation.h:81-83
        // documents slot 0 as the leader's own slot, always the zero offset, so that
        // slot indices line up with FormationAssignment::Members without an
        // off-by-one. Both are therefore leaders and neither may be steered as a
        // follower. Slot 0 is excluded explicitly rather than relied upon to be
        // harmless: its offset is (0,0), so treating it as a follower would order the
        // leader to its own position every tick, zeroing the destination its own order
        // had set and pinning the whole formation in place. Requiring slot >= 1 is
        // what keeps the leader on its macro path.
        bool bIsFormationMember = false;
        if (M.FormationSlot >= 1 && M.FormationId.IsValid())
        {
            const uint32_t LeaderSlot = FindFormationLeader(Core, Movements, M.FormationId);
            const FormationDef* const FDef = FindFormationDef(M.FormationId);
            // Every one of these is a fallback to the unit's own destination, never
            // a crash and never a freeze: unknown formation id, no leader alive,
            // the leader being this very unit, or a slot the authored offset table
            // is too short to describe.
            if (LeaderSlot != kNoFormationLeader && FDef != nullptr && LeaderSlot != I &&
                size_t(M.FormationSlot) < FDef->Offsets.size())
            {
                M.Destination = Transforms[LeaderSlot].Position +
                                RotateOffset(FDef->Offsets[size_t(M.FormationSlot)],
                                             Transforms[LeaderSlot].Facing);
                M.bHasDestination = true;
                bIsFormationMember = true;
            }
        }

        if (!M.bHasDestination)
        {
            M.CurrentSpeed = Fixed::Zero();
            M.BlockedTicks = 0;
            if (Reservations) Reservations->Release(I);
            continue;
        }

        // 1. Arrived?
        // A formation member intentionally does NOT latch arrival: clearing
        // bHasDestination here is harmless because the block above re-derives and
        // re-sets it at the top of the next tick. That is the desired behaviour --
        // a member parked in its slot must start moving again the instant the leader
        // does, so "arrived" can only ever be true for the current tick. The
        // arrive-radius test still matters for members: it is what stops them
        // grinding against the slot point once they are in it.
        //
        // EffectiveArriveRadius widens M.ArriveRadius by the size of the group headed
        // for the same tile (see ScaleArriveRadiusForGroup). M.ArriveRadius itself is
        // NOT mutated: it is per-unit authoritative state that is serialized and
        // restored, and writing a crowd-derived value into it would make a unit's
        // saved radius depend on who happened to be moving alongside it at save time.
        // The scale is applied where it is read instead.
        const int64_t GroupSize = GroupSizeAtDestTile(DestTileKeys, Map.WorldToTile(M.Destination));
        const Fixed EffectiveArriveRadius = ScaleArriveRadiusForGroup(M.ArriveRadius, GroupSize);
        const Fixed ArriveSq = EffectiveArriveRadius * EffectiveArriveRadius;

        const Vec2 GoalDelta = M.Destination - T.Position;
        const Fixed GoalDistSq = GoalDelta.LengthSquared();
        if (GoalDistSq <= ArriveSq)
        {
            M.bHasDestination = false;
            M.CurrentSpeed = Fixed::Zero();
            M.BlockedTicks = 0;
            if (Reservations) Reservations->Release(I);
            continue;
        }

        const Nav::NavQuery Query = MakeNavigationQuery(*D);
        if (NavigationGrid == nullptr || Query.LayerMask == Nav::NavLayer_None)
        {
            continue;   // no navigation for this unit (e.g. air, handled later)
        }

        // 1b. Formation members steer straight at their slot and return here.
        //
        // This is the whole performance rationale for formations, and it is why the
        // branch sits ABOVE the macro-path and flow-field stages rather than falling
        // through them: a member never reaches step 3, so N members cannot request N
        // macro paths or N flow fields. Only the leader -- which has FormationSlot
        // < 0 and therefore never entered the block above -- runs the pathing
        // stages, giving one path and one field per formation regardless of size.
        //
        // Steering directly is sound because the slot point tracks a leader that is
        // itself following a legal path, so the member is towed along a route the
        // navigation grid already approved. SteerToward still refuses to enter an
        // untraversable tile, so a member cannot be dragged through a cliff; it
        // stalls against it and closes again once the leader has moved on.
        //
        // Members are also excluded from tile reservations. Slot points are distinct
        // by construction, and letting members contend would reintroduce exactly the
        // priority starvation described at step 7 -- with the added twist that a
        // starved member freezes while its leader walks away, permanently.
        if (bIsFormationMember)
        {
            if (Reservations) Reservations->Release(I);
            SteerToward(I, *D, M.Destination, Query);
            continue;
        }

        const TileCoord FromTile = Map.WorldToTile(T.Position);
        const TileCoord ToTile = ResolveNavigationTarget(Map.WorldToTile(M.Destination), Query);

        // 2. Final approach. Inside the destination tile the flow field has nothing
        // left to say -- its direction at its own target is zero -- so steer at the
        // exact point instead. Without this a unit stops wherever the tile centre
        // happens to be, up to a tile-width short of where it was sent, stalls, and
        // the order system eventually gives up on it. That was the cause of
        // harvesters never docking and the vertical slice harvesting nothing.
        if (FromTile.X == ToTile.X && FromTile.Y == ToTile.Y)
        {
            if (Reservations) Reservations->Release(I);
            const Vec2 TargetPos = NavigationGrid->IsTraversable(Map.WorldToTile(M.Destination), Query)
                                       ? M.Destination
                                       : Map.TileCenterToWorld(ToTile);
            SteerToward(I, *D, TargetPos, Query);
            const Vec2 Delta = TargetPos - T.Position;
            // Same effective radius as the step-1 test above. If this one kept the
            // unscaled M.ArriveRadius the two checks would disagree: a unit could
            // satisfy step 1 next tick while failing here this tick, and inside the
            // destination tile that means grinding at the exact point it was already
            // close enough to.
            if (Delta.LengthSquared() <= ArriveSq)
            {
                M.bHasDestination = false;
                M.CurrentSpeed = Fixed::Zero();
                M.BlockedTicks = 0;
            }
            else if (M.BlockedTicks > kRepathBlockedTickThreshold)
            {
                M.CurrentMacroPath = Nav::MacroPath{};
                M.BlockedTicks = 0;
                M.LastRepathTick = CurrentTick;
            }
            continue;
        }

        // 3. Macro path (budgeted, shared, topology-aware).
        if (M.CurrentMacroPath.BuiltTopologyRevision != NavigationGrid->GetTopologyRevision() ||
            M.CurrentMacroPath.Waypoints.empty())
        {
            if (MacroPathBuildsThisTick < kMaxMacroPathBuildsPerTick && Router)
            {
                M.CurrentMacroPath = Router->Find(FromTile, ToTile, Query, /*MaxWaypoints=*/8);
                M.NextWaypointIndex = 0;
                ++MacroPathBuildsThisTick;
                ++Stats.MacroPathBuilds;
            }
            else
            {
                // Budget exhausted this tick: follow the last-known flow (if any)
                // and retry next tick. Fall through to steering with stale sub-goal.
            }
        }

        // 4. Sub-goal selection.
        const int32_t Sx = FromTile.X / Nav::NavGrid::kSectorSize;
        const int32_t Sy = FromTile.Y / Nav::NavGrid::kSectorSize;
        const int32_t Dsx = ToTile.X / Nav::NavGrid::kSectorSize;
        const int32_t Dsy = ToTile.Y / Nav::NavGrid::kSectorSize;

        if (Sx == Dsx && Sy == Dsy)
        {
            // Same sector as the goal: the macro path has nothing to contribute, so
            // head straight for the destination tile rather than a sector centre.
            M.CurrentSubGoal = ToTile;
        }
        else if (!M.CurrentMacroPath.Waypoints.empty())
        {
            const int32_t Count = int32_t(M.CurrentMacroPath.Waypoints.size());
            // Consume every waypoint whose sector we have already entered.
            while (M.NextWaypointIndex < Count)
            {
                const TileCoord& Waypoint = M.CurrentMacroPath.Waypoints[size_t(M.NextWaypointIndex)];
                if (Waypoint.X / Nav::NavGrid::kSectorSize == Sx &&
                    Waypoint.Y / Nav::NavGrid::kSectorSize == Sy)
                {
                    ++M.NextWaypointIndex;
                    continue;
                }
                break;
            }
            // Once the corridor is exhausted the goal itself becomes the sub-goal.
            // Leaving the last waypoint in place here is what parked units on sector
            // centres and made them report arrival far from their destination.
            M.CurrentSubGoal = M.NextWaypointIndex < Count
                                   ? M.CurrentMacroPath.Waypoints[size_t(M.NextWaypointIndex)]
                                   : ToTile;
        }
        else
        {
            // No macro path (unreachable or budget-stalled). Count blocked.
            M.BlockedTicks += 1;
            if (M.BlockedTicks > kRepathBlockedTickThreshold)
            {
                M.CurrentMacroPath = Nav::MacroPath{};
                M.BlockedTicks = 0;
                M.LastRepathTick = CurrentTick;
            }
            M.CurrentSpeed = Fixed::Zero();
            continue;
        }

        // 5. Flow field for the sub-goal (shared across all units heading there).
        const Nav::FlowField* Field = GetFlowField(M.CurrentSubGoal, Query);
        if (Field == nullptr || !Field->IsReachable(FromTile))
        {
            M.BlockedTicks += 1;
            M.CurrentSpeed = Fixed::Zero();
            continue;
        }

        // 6. Steering: flow direction -> desired tile.
        const Nav::FlowDirection Dir = Field->GetDirection(FromTile);
        if (Dir.X == 0 && Dir.Y == 0)
        {
            // Standing on the sub-goal tile but not yet on the goal: aim at the real
            // destination so the unit keeps closing instead of declaring itself stuck.
            SteerToward(I, *D, M.Destination, Query);
            continue;
        }
        const TileCoord DesiredTile(FromTile.X + Dir.X, FromTile.Y + Dir.Y);

        // 7. Reservation (soft, slot-order tie-break).
        // Drop last tick's claim first. The tie-break only lets a *lower* slot take
        // an occupied tile, so a unit could not re-reserve the tile it was already
        // holding: it fell through to avoidance every tick and stalled one step from
        // where it started.
        if (Reservations) Reservations->Release(I);

        TileCoord NextTile = DesiredTile;
        if (Reservations && Reservations->TryReserve(DesiredTile, I, CurrentTick, /*HoldTicks=*/2))
        {
            M.BlockedTicks = 0;
        }
        else
        {
            ++Stats.ReservationContests;
            // 7b. Local avoidance: best open neighbor by flow-direction alignment.
            // Fixed neighbor order: N,E,S,W,NE,SE,SW,NW (matches FlowField GDirections).
            static const int8_t Deltas[8][2] = {
                {0, -1}, {1, 0}, {0, 1}, {-1, 0}, {1, -1}, {1, 1}, {-1, 1}, {-1, -1},
            };
            TileCoord Best;
            Fixed BestScore = Fixed::FromInt(-1) * Fixed::FromInt(1000000);
            bool bFound = false;
            for (int32_t N = 0; N < 8; ++N)
            {
                const TileCoord C(DesiredTile.X + Deltas[N][0], DesiredTile.Y + Deltas[N][1]);
                // Never "avoid" into the tile we already occupy. The neighbours of
                // the desired tile include our own, and a unit standing on that
                // tile's centre would then steer at a point zero units away, take a
                // zero-length step and freeze there permanently -- with no blocked
                // ticks to trigger a repath, because the reservation succeeded.
                if (C.X == FromTile.X && C.Y == FromTile.Y) continue;
                if (!NavigationGrid->IsTraversable(C, Query)) continue;
                if (Reservations && !Reservations->IsFree(C, CurrentTick)) continue;
                // Score = dot of flow dir with (C - FromTile) direction.
                const Vec2 Dn(Fixed::FromInt(C.X - FromTile.X), Fixed::FromInt(C.Y - FromTile.Y));
                const Fixed Score = Fixed::FromInt(Dir.X) * Dn.X + Fixed::FromInt(Dir.Y) * Dn.Y;
                if (!bFound || Score > BestScore)
                {
                    Best = C; BestScore = Score; bFound = true;
                }
            }
            if (bFound && Reservations && Reservations->TryReserve(Best, I, CurrentTick, 2))
            {
                NextTile = Best;
                M.BlockedTicks = 0;
            }
            else
            {
                M.CurrentSpeed = Fixed::Zero();
                M.BlockedTicks += 1;
                continue;
            }
        }

        // 8. Steer toward the centre of the chosen tile.
        SteerToward(I, *D, Map.TileCenterToWorld(NextTile), Query);

        // 9. Blocked fallback: force a fresh macro path next tick.
        if (M.BlockedTicks > kRepathBlockedTickThreshold)
        {
            M.CurrentMacroPath = Nav::MacroPath{};
            M.BlockedTicks = 0;
            M.LastRepathTick = CurrentTick;
        }
    }

    // --- Soft separation ---------------------------------------------------
    //
    // Runs as a second pass, after every unit has taken its step, so it resolves a
    // settled configuration rather than a half-updated one. A lambda rather than a
    // SimWorld method because adding a member would require editing SimWorld.h, and
    // this change is confined to SimWorld.cpp.
    //
    // WHY THIS EXISTS, precisely. The reservation tie-break in ReservationGrid grants
    // a contested tile to the strictly lower slot index, and the slot index IS the
    // entity index. Priority is therefore a fixed function of identity, not a rotating
    // or randomised order, so the same high-index units lose the same contests every
    // tick. When both the reservation at step 7 and the 8-neighbour avoidance at 7b
    // fail, the loser executes `CurrentSpeed = Zero; BlockedTicks += 1; continue` and
    // stands still. That is priority starvation, and it is systematic: it is why a
    // 200-unit move to 200 *distinct* tiles lands only ~126 units. Convergence on a
    // shared point is NOT the mechanism.
    //
    // The pass attacks that directly. A frozen unit is by definition packed against
    // neighbours; nudging it out of the overlap moves it to a different tile, which
    // means a different DesiredTile and a different contest next tick. The unit stops
    // being permanently starved by the same winner.
    //
    // FIXED POINT. The naive version oscillates: A pushes B, B pushes A, both vibrate
    // forever and a distance check misreads the vibration as arrival. Three properties
    // prevent that:
    //   1. A deadband. Only overlap deeper than (radius - deadband) is corrected;
    //      pairs in the slack band above it are already at rest and left untouched, so
    //      the pass has a real rest state rather than a boundary to jitter across.
    //   2. Step < deadband, enforced by static_assert. A pair just resolved cannot be
    //      pushed back into corrective range next tick, which makes the rest state
    //      absorbing instead of a limit cycle.
    //   3. Simultaneous resolution. Offsets accumulate against the positions held at
    //      entry and are applied only afterwards, so the outcome cannot depend on
    //      visit order: each unit sees the other where it actually was and takes half
    //      the correction.
    //
    // DETERMINISM. Candidates come from the spatial grid, built in ascending index
    // order and walked row-major; each pair is filtered to J > I so it is seen exactly
    // once and antisymmetrically; accumulation is plain int64 addition and therefore
    // commutative, so the order a unit's several pushes arrive in cannot change the
    // total; the exactly-coincident case is broken by index, not iteration accident.
    // No float, no std::rand, no wall clock.
    //
    // COST. Pairs come from the spatial grid rather than a scan of all 8192 slots, so
    // this is proportional to local crowding, not the square of the entity count. The
    // query radius is padded by a whole cell for the same reason AcquireTarget pads
    // its own: the grid was built at the top of the tick and units have moved since, a
    // padded candidate set is a superset of the true one, and every candidate is still
    // distance-checked below.
    auto ApplySoftSeparation = [this]()
    {
        if (NavigationGrid == nullptr || SpatialCells.empty())
        {
            return;
        }

        const Fixed QueryRadius = Fixed::FromInt(260 + MapDescription::kTileSizeUnitsLocal * kSpatialCellTiles);

        std::vector<Vec2> Offsets(Core.size(), Vec2::Zero());
        std::vector<uint32_t> Candidates;
        bool bAnyOverlap = false;

        for (uint32_t I = 0; I < uint32_t(Core.size()); ++I)
        {
            if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit) continue;
            const EntityDef* DefI = Content ? Content->FindEntity(Core[I].Def) : nullptr;
            const Fixed RadI = DefI ? FxMax(DefI->Unit.CollisionRadius, Fixed::FromInt(30)) : Fixed::FromInt(35);

            // 1. Unit vs Unit separation
            QuerySpatial(Transforms[I].Position, QueryRadius, Candidates);
            for (const uint32_t J : Candidates)
            {
                if (J <= I) continue;
                if (!Core[J].bAlive || Core[J].Kind != EntityKind::Unit) continue;

                const EntityDef* DefJ = Content ? Content->FindEntity(Core[J].Def) : nullptr;
                const Fixed RadJ = DefJ ? FxMax(DefJ->Unit.CollisionRadius, Fixed::FromInt(30)) : Fixed::FromInt(35);

                const Fixed DesiredSeparation = (RadI + RadJ) * Fixed::FromRatio(85, 100);
                const Fixed Deadband = Fixed::FromInt(8);
                const Fixed CorrectBelow = FxMax(Fixed::FromInt(20), DesiredSeparation - Deadband);
                const Fixed CorrectBelowSq = CorrectBelow * CorrectBelow;

                const Vec2 Delta = Transforms[J].Position - Transforms[I].Position;
                const Fixed DistSq = Delta.LengthSquared();
                if (DistSq.Raw >= CorrectBelowSq.Raw)
                {
                    continue;
                }

                // Tank Crushing Infantry (Classic Red Alert combat mechanic):
                if (Core[I].Owner != Core[J].Owner && IsHostile(Core[I].Owner, Core[J].Owner))
                {
                    const bool bHeavyI = DefI && (DefI->Name.find("tank") != std::string::npos || DefI->Name.find("harvester") != std::string::npos || DefI->Name.find("mcv") != std::string::npos);
                    const bool bInfJ = DefJ && (DefJ->Name.find("conscript") != std::string::npos || DefJ->Name.find("soldier") != std::string::npos || DefJ->Name.find("infantry") != std::string::npos || DefJ->Name.find("trooper") != std::string::npos || DefJ->Unit.CollisionRadius < Fixed::FromInt(32));
                    if (bHeavyI && bInfJ && Movements[I].CurrentSpeed > Fixed::FromInt(3))
                    {
                        ApplyDamage(MakeId(J), 9999, WarheadClass::Crush, MakeId(I), Core[I].Owner);
                        continue;
                    }

                    const bool bHeavyJ = DefJ && (DefJ->Name.find("tank") != std::string::npos || DefJ->Name.find("harvester") != std::string::npos || DefJ->Name.find("mcv") != std::string::npos);
                    const bool bInfI = DefI && (DefI->Name.find("conscript") != std::string::npos || DefI->Name.find("soldier") != std::string::npos || DefI->Name.find("infantry") != std::string::npos || DefI->Name.find("trooper") != std::string::npos || DefI->Unit.CollisionRadius < Fixed::FromInt(32));
                    if (bHeavyJ && bInfI && Movements[J].CurrentSpeed > Fixed::FromInt(3))
                    {
                        ApplyDamage(MakeId(I), 9999, WarheadClass::Crush, MakeId(J), Core[J].Owner);
                        continue;
                    }
                }

                Vec2 PushDir;
                Fixed Overlap = Fixed::Zero();
                if (DistSq.Raw == 0)
                {
                    PushDir = Vec2(Fixed::One(), Fixed::Zero());
                    Overlap = CorrectBelow;
                }
                else
                {
                    const Fixed Dist = FxSqrt(DistSq);
                    if (Dist.Raw == 0) continue;
                    PushDir = Vec2(Delta.X / Dist, Delta.Y / Dist);
                    Overlap = CorrectBelow - Dist;
                }

                const Fixed HalfStep = FxClamp(Overlap / int64_t(2), Fixed::FromInt(4), Fixed::FromInt(18));
                Offsets[I] -= PushDir * HalfStep;
                Offsets[J] += PushDir * HalfStep;
                bAnyOverlap = true;
            }

            // 2. Unit vs Building/Structure separation
            const TileCoord UnitTile = Map.WorldToTile(Transforms[I].Position);
            for (int32_t DY = -1; DY <= 1; ++DY)
            {
                for (int32_t DX = -1; DX <= 1; ++DX)
                {
                    const int32_t TX = UnitTile.X + DX;
                    const int32_t TY = UnitTile.Y + DY;
                    if (TX < 0 || TY < 0 || TX >= Map.Width || TY >= Map.Height) continue;
                    if ((Map.GetTile(TX, TY) & Tile_Occupied) != 0)
                    {
                        const Vec2 BldgCenter = Map.TileCenterToWorld(TileCoord(TX, TY));
                        const Vec2 BldgDelta = Transforms[I].Position - BldgCenter;
                        const Fixed BDistSq = BldgDelta.LengthSquared();
                        const Fixed MinDist = RadI + Fixed::FromInt(MapDescription::kTileSizeUnitsLocal / 2 + 5);
                        if (BDistSq < MinDist * MinDist)
                        {
                            Vec2 OutDir;
                            if (BDistSq.Raw == 0)
                            {
                                OutDir = Vec2(Fixed::One(), Fixed::Zero());
                            }
                            else
                            {
                                const Fixed BDist = FxSqrt(BDistSq);
                                OutDir = Vec2(BldgDelta.X / BDist, BldgDelta.Y / BDist);
                            }
                            Offsets[I] += OutDir * Fixed::FromInt(12);
                            bAnyOverlap = true;
                        }
                    }
                }
            }
        }

        if (!bAnyOverlap)
        {
            return;
        }

        for (uint32_t I = 0; I < uint32_t(Core.size()); ++I)
        {
            if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit) continue;
            if (Offsets[I].LengthSquared().Raw == 0) continue;

            const EntityDef* const D = Content->FindEntity(Core[I].Def);
            if (D == nullptr) continue;
            const Nav::NavQuery Query = MakeNavigationQuery(*D);
            if (Query.LayerMask == Nav::NavLayer_None) continue;

            const Vec2 Candidate = Transforms[I].Position + Offsets[I];
            if (NavigationGrid->IsTraversable(Map.WorldToTile(Candidate), Query))
            {
                Transforms[I].Position = Candidate;
            }
            else
            {
                // Sliding fallback: try X and Y separately
                const Vec2 CandX = Transforms[I].Position + Vec2(Offsets[I].X, Fixed::Zero());
                if (NavigationGrid->IsTraversable(Map.WorldToTile(CandX), Query))
                {
                    Transforms[I].Position = CandX;
                }
                else
                {
                    const Vec2 CandY = Transforms[I].Position + Vec2(Fixed::Zero(), Offsets[I].Y);
                    if (NavigationGrid->IsTraversable(Map.WorldToTile(CandY), Query))
                    {
                        Transforms[I].Position = CandY;
                    }
                }
            }

            if (Map.Width > 0 && Map.Height > 0)
            {
                const Fixed MinMargin = Fixed::FromInt(50);
                const Fixed MaxX = Fixed::FromInt(int64_t(Map.Width) * MapDescription::kTileSizeUnitsLocal) - MinMargin;
                const Fixed MaxY = Fixed::FromInt(int64_t(Map.Height) * MapDescription::kTileSizeUnitsLocal) - MinMargin;
                if (Transforms[I].Position.X < MinMargin) { Transforms[I].Position.X = MinMargin; }
                else if (Transforms[I].Position.X > MaxX) { Transforms[I].Position.X = MaxX; }
                if (Transforms[I].Position.Y < MinMargin) { Transforms[I].Position.Y = MinMargin; }
                else if (Transforms[I].Position.Y > MaxY) { Transforms[I].Position.Y = MaxY; }
            }
        }
    };

    ApplySoftSeparation();
}

void SimWorld::RebuildSpatialGrid()
{
    // Cell edge in world units. A multiple of the tile size keeps cell lookup an
    // integer division, so there is no float rounding to diverge between platforms.
    constexpr int64_t kCellUnits = MapDescription::kTileSizeUnitsLocal * kSpatialCellTiles;

    const int32_t WantX = Map.Width > 0
        ? int32_t((int64_t(Map.Width) + kSpatialCellTiles - 1) / kSpatialCellTiles) : 1;
    const int32_t WantY = Map.Height > 0
        ? int32_t((int64_t(Map.Height) + kSpatialCellTiles - 1) / kSpatialCellTiles) : 1;

    if (WantX != SpatialCellsX || WantY != SpatialCellsY)
    {
        SpatialCellsX = WantX;
        SpatialCellsY = WantY;
        SpatialCells.assign(size_t(WantX) * size_t(WantY), std::vector<uint32_t>());
    }
    else
    {
        // clear() keeps each cell's capacity, so steady-state ticks do no allocation.
        for (std::vector<uint32_t>& Cell : SpatialCells)
        {
            Cell.clear();
        }
    }

    // Ascending index order matters: it is what makes the candidate sequence, and
    // therefore tie-breaking, identical to the old full linear scan.
    for (uint32_t I = 0; I < uint32_t(Core.size()); ++I)
    {
        if (!Core[I].bAlive)
        {
            continue;
        }
        // Projectiles and resource nodes are never acquisition candidates, so
        // keeping them out shrinks every query.
        if (Core[I].Kind == EntityKind::Projectile || Core[I].Kind == EntityKind::ResourceNode)
        {
            continue;
        }
        const Vec2& P = Transforms[I].Position;
        int64_t CX = P.X.ToIntFloor() / kCellUnits;
        int64_t CY = P.Y.ToIntFloor() / kCellUnits;
        // Clamp rather than drop: an entity nudged outside the map bounds must stay
        // targetable, otherwise it becomes silently invulnerable.
        CX = CX < 0 ? 0 : (CX >= SpatialCellsX ? SpatialCellsX - 1 : CX);
        CY = CY < 0 ? 0 : (CY >= SpatialCellsY ? SpatialCellsY - 1 : CY);
        SpatialCells[size_t(CY) * size_t(SpatialCellsX) + size_t(CX)].push_back(I);
    }
}

void SimWorld::QuerySpatial(const Vec2& Centre, Fixed Radius,
                            std::vector<uint32_t>& Out) const
{
    Out.clear();
    if (SpatialCells.empty())
    {
        return;
    }
    constexpr int64_t kCellUnits = MapDescription::kTileSizeUnitsLocal * kSpatialCellTiles;

    const int64_t R = Radius.ToIntFloor();
    const int64_t MinX = (Centre.X.ToIntFloor() - R) / kCellUnits;
    const int64_t MaxX = (Centre.X.ToIntFloor() + R) / kCellUnits;
    const int64_t MinY = (Centre.Y.ToIntFloor() - R) / kCellUnits;
    const int64_t MaxY = (Centre.Y.ToIntFloor() + R) / kCellUnits;

    const int64_t X0 = MinX < 0 ? 0 : MinX;
    const int64_t Y0 = MinY < 0 ? 0 : MinY;
    const int64_t X1 = MaxX >= SpatialCellsX ? SpatialCellsX - 1 : MaxX;
    const int64_t Y1 = MaxY >= SpatialCellsY ? SpatialCellsY - 1 : MaxY;

    // Row-major, ascending: fixed visitation order on every machine.
    for (int64_t CY = Y0; CY <= Y1; ++CY)
    {
        for (int64_t CX = X0; CX <= X1; ++CX)
        {
            const std::vector<uint32_t>& Cell =
                SpatialCells[size_t(CY) * size_t(SpatialCellsX) + size_t(CX)];
            Out.insert(Out.end(), Cell.begin(), Cell.end());
        }
    }
}

EntityId SimWorld::AcquireTarget(EntityId Attacker) const
{
    if (!IsAlive(Attacker))
    {
        return EntityId::Invalid();
    }
    const uint32_t A = Attacker.Index;
    const EntityDef* D = Content->FindEntity(Core[A].Def);
    if (D == nullptr)
    {
        return EntityId::Invalid();
    }
    const ContentId FireWeaponId = ResolveFireWeapon(Attacker);
    if (!FireWeaponId.IsValid())
    {
        return EntityId::Invalid();
    }
    const WeaponDef* W = Content->FindWeapon(FireWeaponId);
    if (W == nullptr)
    {
        return EntityId::Invalid();
    }

    const Vec2 Pos = Transforms[A].Position;
    // Units engage anything inside their vision, not just inside weapon range, so
    // that a spotted enemy is chased rather than ignored one metre out of reach.
    const Fixed SearchRange = FxMax(D->VisionRange, W->MaxRange);
    const Fixed SearchSq = SearchRange * SearchRange;

    EntityId Best = EntityId::Invalid();
    Fixed BestDistSq = Fixed::Max();

    // Only the cells the search radius actually reaches, instead of every entity in
    // the world. Candidates arrive in ascending index order within each cell and the
    // cells are walked row-major, so the "strictly closer, else lower index"
    // tie-break below resolves identically to the previous full scan. Cells never
    // contain projectiles or resource nodes, so those checks are gone from the loop.
    //
    // The radius is padded by one whole cell. The grid is built at the top of the
    // tick but SystemMovement runs before SystemCombat, so a position can be up to
    // one tick of travel stale by the time targets are acquired - at the fastest
    // content speed that is ~9% of a cell, and without the pad an entity sitting
    // just past a cell boundary could be missed. Padding cannot introduce a false
    // positive because every candidate is still range-checked against SearchSq
    // below; it only guarantees the candidate set is a superset of the true one, so
    // the result is identical to the linear scan.
    QuerySpatial(Pos, SearchRange + Fixed::FromInt(MapDescription::kTileSizeUnitsLocal * kSpatialCellTiles),
                 SpatialQueryScratch);
    for (const uint32_t I : SpatialQueryScratch)
    {
        if (!Core[I].bAlive || I == A)
        {
            continue;
        }
        if (!IsHostile(Core[A].Owner, Core[I].Owner))
        {
            continue;
        }
        const EntityDef* TargetDef = Content->FindEntity(Core[I].Def);
        if (TargetDef == nullptr)
        {
            continue;
        }
        const bool bTargetIsAir = TargetDef->Unit.Layer == MovementLayer::Air;
        if (bTargetIsAir && !W->bCanTargetAir) { continue; }
        if (!bTargetIsAir && !W->bCanTargetGround) { continue; }

        // A side does not open fire on what it cannot see. Player-level fog is also
        // what makes a spotter matter: a weapon that outranges its carrier's own
        // vision engages only what some friendly unit reveals, so artillery needs an
        // escort without any separate spotter search existing in this loop.
        if (!IsEntityVisibleTo(Core[A].Owner, I))
        {
            continue;
        }
        if (PassengerOf[I].Transport.IsValid())
        {
            continue;   // boarded passengers cannot be targeted
        }

        const Fixed DistSq = DistanceSquared(Pos, Transforms[I].Position);
        if (DistSq <= SearchSq && DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = MakeId(I);
        }
    }
    return Best;
}

bool SimWorld::IsLocationVisibleTo(PlayerId Viewer, const Vec2& Location) const
{
    // Same fog-is-optional rule as IsEntityVisibleTo: a match without a grid, and
    // every headless fixture, behaves as though the map is in the open.
    if (Viewer >= kMaxPlayers || FogGrid == nullptr)
    {
        return true;
    }
    if (int32_t(Viewer) >= FogGrid->GetNumPlayers())
    {
        return true;
    }
    const TileCoord Tile = Map.WorldToTile(Location);
    return FogGrid->GetVisibility(int32_t(Viewer), Tile.X, Tile.Y) == VisibilityState::CurrentlyVisible;
}

bool SimWorld::IsEntityVisibleTo(PlayerId Viewer, uint32_t EntityIndex) const
{
    // Liveness and range come first: a dead or out-of-range entity is visible to
    // nobody, fog or not (review MINOR-2 -- with the old order, a fogless match
    // answered "visible" for garbage indices).
    if (EntityIndex >= Core.size() || !Core[EntityIndex].bAlive)
    {
        return false;
    }
    // A unit riding in a transport is hidden inside it -- nobody can see, target
    // or shoot the passenger while it is aboard.
    if (PassengerOf[EntityIndex].Transport.IsValid())
    {
        return false;
    }
    // Fog is optional. A match configured without it, and every headless fixture that
    // never builds a grid, has to behave as though the map is in the open -- the
    // alternative is that absent fog blinds every side and combat stops entirely.
    if (Viewer >= kMaxPlayers || FogGrid == nullptr)
    {
        return true;
    }
    if (int32_t(Viewer) >= FogGrid->GetNumPlayers())
    {
        return true;
    }

    // A side always sees its own. The grid is rebuilt from unit vision every tick, so
    // asking it about a friendly unit is asking whether that unit reveals itself --
    // true today, but not something the answer should depend on.
    if (Core[EntityIndex].Owner == Viewer)
    {
        return true;
    }

    const TileCoord Tile = Map.WorldToTile(Transforms[EntityIndex].Position);
    const VisibilityState Visibility = FogGrid->GetVisibility(int32_t(Viewer), Tile.X, Tile.Y);

    // Only a live sighting is a firing solution. A radar contact is why the minimap
    // draws a blip, but it does not aim a gun: counting it as sight would let radar
    // coverage stand in for scouting and would make stealth worthless the moment any
    // side built a radar. PreviouslySeen is a memory of a tile, not of an enemy.
    return Visibility == VisibilityState::CurrentlyVisible;
}

ContentId SimWorld::ResolveFireWeapon(EntityId Attacker) const
{
    const EntityDef* D = Content->FindEntity(Core[Attacker.Index].Def);
    if (D == nullptr)
    {
        return ContentId();
    }
    // Multigunner rule: an armed passenger lends the
    // carrier its weapon. First passenger wins; capacity is small so a linear scan
    // is the whole story.
    if (D->Unit.bMultigunner)
    {
        for (const EntityId& P : Transports[Attacker.Index].Passengers)
        {
            if (!IsAlive(P))
            {
                continue;
            }
            const EntityDef* PD = Content->FindEntity(Core[P.Index].Def);
            if (PD != nullptr && PD->Weapon.IsValid())
            {
                return PD->Weapon;
            }
        }
    }
    return D->Weapon;
}

void SimWorld::ApplyOnHitStatus(EntityId Victim, const WeaponDef& Weapon)
{
    if (Weapon.StunTicksOnHit <= 0 && Weapon.FreezeTicksOnHit <= 0
        && Weapon.ShrinkTicksOnHit <= 0 && Weapon.InfectionTicksOnHit <= 0)
    {
        return;
    }
    if (!IsAlive(Victim) || Core[Victim.Index].Kind == EntityKind::Projectile)
    {
        return;
    }
    // Vehicles, ships and buildings are hard targets for EMP/cryo; infantry is the
    // classic shrink target. Content may override with StatusTargetsVehiclesOnly.
    const bool bHardTarget = Core[Victim.Index].Kind == EntityKind::Building
                             || Content->FindEntity(Core[Victim.Index].Def)->Unit.Layer != MovementLayer::Infantry;
    if (Weapon.StatusTargetsVehiclesOnly == 1 && !bHardTarget)
    {
        return;
    }
    StatusComp& S = Statuses[Victim.Index];
    if (Weapon.StunTicksOnHit > 0)      { S.StunTicks      = std::max(S.StunTicks,      Weapon.StunTicksOnHit); }
    if (Weapon.FreezeTicksOnHit > 0)    { S.FreezeTicks    = std::max(S.FreezeTicks,    Weapon.FreezeTicksOnHit); }
    if (Weapon.ShrinkTicksOnHit > 0)    { S.ShrinkTicks    = std::max(S.ShrinkTicks,    Weapon.ShrinkTicksOnHit); }
    if (Weapon.InfectionTicksOnHit > 0) { S.InfectionTicks = std::max(S.InfectionTicks, Weapon.InfectionTicksOnHit); }
}

void SimWorld::FireWeapon(EntityId Attacker, EntityId TargetId, const WeaponDef& Weapon)
{
    const uint32_t A = Attacker.Index;
    const Vec2 From = Transforms[A].Position;
    const Vec2 To = Transforms[TargetId.Index].Position;

    SimEvent Ev;
    Ev.Type = SimEventType::WeaponFired;
    Ev.Tick = CurrentTick;
    Ev.Entity = Attacker;
    Ev.Other = TargetId;
    Ev.Player = Core[A].Owner;
    Ev.Content = Weapon.Id;
    Ev.Location = From;
    EmitEvent(Ev);

    // ADR-0013: a static defence under a Severe deficit reloads at half rate. Applied
    // here rather than at the tick-down so the slowdown is visible in CooldownTicks --
    // a UI reading that field sees the real remaining time, and a tier change mid-reload
    // does not retroactively rewrite how long the shot took.
    int32_t Cooldown = Weapon.CooldownTicks;
    if (Core[A].Kind == EntityKind::Building && Core[A].Owner < kMaxPlayers)
    {
        Cooldown *= StaticDefenceCooldownMultiplierForTier(Players[Core[A].Owner].GetPowerTier());
    }
    // Research upgrades: fire rate bonus shortens the cooldown. FireRatePercent>0
    // means faster, so cooldown shrinks by the inverse ratio.
    if (Core[A].Owner < kMaxPlayers)
    {
        const PlayerModifiers Mod = GetPlayerModifiers(Core[A].Owner);
        if (Mod.FireRatePercent > 0)
        {
            Cooldown = (Cooldown * 100) / (100 + Mod.FireRatePercent);
            if (Cooldown < 1) { Cooldown = 1; }
        }
    }
    Combats[A].CooldownTicks = Cooldown;

    // Finite magazines decrement only while rounds remain. A 0/0 magazine (every
    // non-air unit) never enters this branch and fires forever; the SystemCombat
    // gate is what stops a genuinely dry aircraft from shooting at all.
    if (Combats[A].AmmoCurrent > 0)
    {
        Combats[A].AmmoCurrent -= 1;
    }

    if (Weapon.ProjectileSpeed <= Fixed::Zero())
    {
        // Hitscan: resolve immediately.
        ApplyDamage(TargetId, Weapon.Damage, Weapon.Warhead, Attacker, Core[A].Owner);
        ApplyOnHitStatus(TargetId, Weapon);
        if (Weapon.SplashRadius > Fixed::Zero())
        {
            ApplySplashDamage(To, Weapon.SplashRadius, Weapon.Damage, Weapon.Warhead,
                              Weapon.SplashFalloffPercent, Attacker, Core[A].Owner);
        }
        return;
    }

    const EntityId ProjId = AllocateEntity();
    if (!ProjId.IsValid())
    {
        return;
    }
    Core[ProjId.Index].Kind = EntityKind::Projectile;
    Core[ProjId.Index].Owner = Core[A].Owner;
    Core[ProjId.Index].Def = Weapon.Id;
    Transforms[ProjId.Index].Position = From;
    Transforms[ProjId.Index].Facing = (To - From).ToAngle();
    Healths[ProjId.Index].bInvulnerable = true;
    Healths[ProjId.Index].Max = 1;
    Healths[ProjId.Index].Current = 1;

    ProjectileComp& P = Projectiles[ProjId.Index];
    P.Source = Attacker;
    P.Target = TargetId;
    P.Weapon = Weapon.Id;
    P.OwnerPlayer = Core[A].Owner;
    P.Speed = PerSecondToPerTick(Weapon.ProjectileSpeed);

    // Scatter is applied once at launch and scales with range: long shots are less
    // accurate, which is what makes artillery a suppression weapon rather than a
    // sniper rifle.
    P.ImpactPoint = To;
    if (Weapon.ScatterAtMaxRange > Fixed::Zero())
    {
        const Fixed Dist = Distance(From, To);
        const Fixed Scatter = Weapon.MaxRange.Raw > 0
                                  ? Fixed((Weapon.ScatterAtMaxRange.Raw * Dist.Raw) / Weapon.MaxRange.Raw)
                                  : Fixed::Zero();
        if (Scatter > Fixed::Zero())
        {
            const int32_t Angle = int32_t(Rng.NextBelow(uint32_t(kAngleTurn)));
            const Fixed Radius = Fixed((int64_t(Rng.NextUInt32() & 0xFFFF) * Scatter.Raw) >> 16);
            P.ImpactPoint = To + Vec2::FromAngle(Angle) * Radius;
        }
    }
}

// Ticks an entity waits before retrying target acquisition after a search that
// found nothing. 4 ticks = 200 ms at 20 Hz. Chosen as the largest delay that is
// still below the threshold where a player would notice a unit being slow to
// return fire, while cutting fruitless searches to a quarter.
static constexpr int32_t kAcquireRetryTicks = 4;

// Engineer capture channel: 5 seconds of standing at the door.


void SimWorld::SystemCombat()
{
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive)
        {
            continue;
        }
        const EntityKind Kind = Core[I].Kind;
        if (Kind != EntityKind::Unit && Kind != EntityKind::Building)
        {
            continue;
        }

        CombatComp& C = Combats[I];
        if (C.CooldownTicks > 0)
        {
            C.CooldownTicks -= 1;
        }
        if (C.SecondaryAbilityCooldownTicks > 0)
        {
            C.SecondaryAbilityCooldownTicks -= 1;
        }
        if (C.SecondaryAbilityDurationTicks > 0)
        {
            C.SecondaryAbilityDurationTicks -= 1;
            if (C.SecondaryAbilityDurationTicks == 0)
            {
                C.bSecondaryModeActive = false;
                SimEvent Ev;
                Ev.Type = SimEventType::SecondaryAbilityToggled;
                Ev.Tick = CurrentTick;
                Ev.Entity = MakeId(I);
                Ev.Player = Core[I].Owner;
                Ev.Content = Core[I].Def;
                Ev.Location = Transforms[I].Position;
                Ev.Value = 0;
                EmitEvent(Ev);
            }
        }

        // Stunned or frozen units cannot fight; boarded units are cargo.
        if (!Statuses[I].bCanAct() || PassengerOf[I].Transport.IsValid())
        {
            continue;
        }
        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr)
        {
            continue;
        }

        // --- Air logistics -------------------------------------------------
        // A dry aircraft neither fires nor acquires a target: it breaks off and
        // flies to the nearest friendly air producer to rearm, the munitions
        // twin of the harvester's MovingToRefinery leg. The rearming latch stays
        // set after the first reload tick raises AmmoCurrent above zero; without
        // it the craft would leave after two rounds and could never become full.
        // The destination is set
        // HERE rather than in SystemOrders because the trigger is internal
        // magazine state, not a player order; with an empty order queue nothing
        // overwrites it, and SystemMovement integrates it unchanged on the next
        // tick. Boarded and stunned craft never reach this point (gate above),
        // so the movement gate for those states stays untouched.
        const bool bFiniteAircraft = Kind == EntityKind::Unit &&
                                     D->Unit.Layer == MovementLayer::Air && C.AmmoMax > 0;
        if (bFiniteAircraft && C.AmmoCurrent <= 0)
        {
            AircraftRearming[I] = 1;
        }
        if (bFiniteAircraft && AircraftRearming[I] != 0)
        {
            const Vec2 Home = Transforms[I].Position;
            const EntityId Pad = FindNearestRearmPoint(Home, Core[I].Owner);
            if (Pad.IsValid())
            {
                const Vec2 PadPos = Transforms[Pad.Index].Position;
                const Fixed DockRadius = Fixed::FromInt(kRearmDockDistanceUnits);
                if (DistanceSquared(Home, PadPos) <= DockRadius * DockRadius)
                {
                    // On the pad: top up at the fixed rate and hold position
                    // until the magazine is whole again.
                    C.AmmoCurrent = std::min(C.AmmoMax, C.AmmoCurrent + kAircraftReloadPerTick);
                    if (C.AmmoCurrent >= C.AmmoMax)
                    {
                        AircraftRearming[I] = 0;
                    }
                    Movements[I].bHasDestination = false;
                    Movements[I].CurrentSpeed = Fixed::Zero();
                }
                else
                {
                    // In transit: keep steering at the pad. Re-derived each tick
                    // so a destroyed pad re-routes to the next nearest one.
                    Movements[I].Destination = PadPos;
                    Movements[I].bHasDestination = true;
                }
            }
            else
            {
                // No pad anywhere: hover rather than grind toward nothing.
                Movements[I].bHasDestination = false;
                Movements[I].CurrentSpeed = Fixed::Zero();
            }
            // Drop any retained target; acquisition below is skipped entirely.
            C.Target = EntityId::Invalid();
            C.bTargetIsForced = false;
            continue;
        }

        const ContentId FireWeaponId = ResolveFireWeapon(MakeId(I));
        if (C.bSecondaryModeActive && D->Unit.bAbilityDisablesPrimaryWeapon)
        {
            continue;
        }

        if (Kind == EntityKind::Building && Buildings[I].State != ConstructionState::Complete)
        {
            continue;   // a turret under construction does not shoot
        }
        // ADR-0013: static defence is offline at Critical, and offline whenever its own
        // priority band is. Units are unaffected -- a tank carries its own power.
        if (Kind == EntityKind::Building && Core[I].Owner < kMaxPlayers)
        {
            const PowerTier Tier = Players[Core[I].Owner].GetPowerTier();
            if (!IsStaticDefenceOnlineAtTier(Tier) ||
                IsPowerPriorityOffline(Buildings[I].Priority, Tier))
            {
                continue;
            }
        }
        const WeaponDef* W = Content->FindWeapon(FireWeaponId);
        if (W == nullptr)
        {
            continue;
        }

        // Defences and idle units pick their own targets; ordered units keep theirs.
        if (!IsAlive(C.Target))
        {
            C.Target = EntityId::Invalid();
            C.bTargetIsForced = false;
        }
        if (!C.Target.IsValid())
        {
            // Re-searching every tick after a failed search is pure waste: the world
            // barely changes in 50 ms, so the answer is almost always "still nothing".
            // Wait a few ticks instead. 4 ticks is 200 ms at 20 Hz, well inside the
            // reaction time a player can perceive, and it cannot make a unit miss an
            // enemy that stays in range - only delay the notice by at most 200 ms.
            if (C.AcquireCooldownTicks > 0)
            {
                C.AcquireCooldownTicks -= 1;
                continue;
            }
            C.Target = AcquireTarget(MakeId(I));
            if (!C.Target.IsValid())
            {
                C.AcquireCooldownTicks = kAcquireRetryTicks;
            }
        }
        if (!C.Target.IsValid())
        {
            continue;
        }

        const Vec2 Pos = Transforms[I].Position;
        const Vec2 TargetPos = Transforms[C.Target.Index].Position;
        const Fixed DistSq = DistanceSquared(Pos, TargetPos);

        if (DistSq > W->MaxRange * W->MaxRange)
        {
            continue;
        }
        if (W->MinRange > Fixed::Zero() && DistSq < W->MinRange * W->MinRange)
        {
            continue;   // inside the artillery dead zone
        }

        // Turret tracking is independent of hull facing.
        const int32_t DesiredAngle = (TargetPos - Pos).ToAngle();
        if (D->Unit.TurretTurnRatePerSecond > 0)
        {
            const int32_t TurnPerTick = std::max(1, D->Unit.TurretTurnRatePerSecond / kTicksPerSecond);
            const int32_t Diff = AngleDelta(Transforms[I].TurretFacing, DesiredAngle);
            if (Diff > TurnPerTick) { Transforms[I].TurretFacing = WrapAngle(Transforms[I].TurretFacing + TurnPerTick); }
            else if (Diff < -TurnPerTick) { Transforms[I].TurretFacing = WrapAngle(Transforms[I].TurretFacing - TurnPerTick); }
            else { Transforms[I].TurretFacing = DesiredAngle; }
        }
        else
        {
            Transforms[I].TurretFacing = DesiredAngle;
        }

        if (W->bRequiresTurretAligned)
        {
            const int32_t Misalignment = AngleDelta(Transforms[I].TurretFacing, DesiredAngle);
            if (Misalignment > kTurretAlignTolerance || Misalignment < -kTurretAlignTolerance)
            {
                continue;
            }
        }

        if (C.CooldownTicks <= 0)
        {
            FireWeapon(MakeId(I), C.Target, *W);
        }
    }
}

void SimWorld::SystemProjectiles()
{
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Projectile)
        {
            continue;
        }
        ProjectileComp& P = Projectiles[I];
        const WeaponDef* W = Content->FindWeapon(P.Weapon);
        if (W == nullptr)
        {
            QueueDestroy(MakeId(I));
            continue;
        }

        // Guided munitions track a living target; if it dies the shot still lands
        // where the target was, which is what makes dodging meaningful.
        if (IsAlive(P.Target))
        {
            P.ImpactPoint = Transforms[P.Target.Index].Position;
        }

        const Vec2 Pos = Transforms[I].Position;
        const Vec2 Delta = P.ImpactPoint - Pos;
        const Fixed Dist = Delta.Length();

        if (Dist <= P.Speed || Dist <= Fixed::FromInt(10))
        {
            Transforms[I].Position = P.ImpactPoint;

            SimEvent Ev;
            Ev.Type = SimEventType::ProjectileImpact;
            Ev.Tick = CurrentTick;
            Ev.Entity = MakeId(I);
            Ev.Other = P.Target;
            Ev.Player = P.OwnerPlayer;
            Ev.Content = P.Weapon;
            Ev.Location = P.ImpactPoint;
            EmitEvent(Ev);

            if (IsAlive(P.Target))
            {
                ApplyDamage(P.Target, W->Damage, W->Warhead, P.Source, P.OwnerPlayer);
                ApplyOnHitStatus(P.Target, *W);
            }
            if (W->SplashRadius > Fixed::Zero())
            {
                ApplySplashDamage(P.ImpactPoint, W->SplashRadius, W->Damage, W->Warhead,
                                  W->SplashFalloffPercent, P.Source, P.OwnerPlayer);
            }
            QueueDestroy(MakeId(I));
        }
        else
        {
            const Vec2 Dir = Delta / Dist;
            Transforms[I].Position = Pos + Dir * P.Speed;
            Transforms[I].Facing = Dir.ToAngle();
        }
    }
}

void SimWorld::SystemDeaths()
{
    // QueueDestroy deduplicates victims and keeps killer attribution aligned with
    // the first fatal blow. Sales and hazards deliberately carry Invalid killers.
    for (size_t I = 0; I < PendingDestroy.size(); ++I)
    {
        const EntityId Id = PendingDestroy[I];
        const bool bWasSold =
            std::find(PendingSales.begin(), PendingSales.end(), Id) != PendingSales.end();
        DestroyEntity(Id, PendingDestroyKillers[I], bWasSold);
    }
    PendingDestroy.clear();
    PendingDestroyKillers.clear();
    PendingSales.clear();
}

void SimWorld::SystemVictory()
{
    if (Phase != MatchPhase::Running)
    {
        return;
    }

    // Does this building definition appear in ANY definition's Production.ProducedBy?
    //
    // Derived from content rather than read from a new flag, exactly as the rationale
    // below requires: the producer relationship already exists in the data, because
    // command validation resolves a producer by searching `Item->Production.ProducedBy`
    // for a candidate building's definition id. Asking the same question here keeps one
    // source of truth -- a mod that adds a producer, or removes one, is covered with no
    // further change, and a building can never be "a producer" for victory while being
    // rejected as a producer when an order is issued.
    //
    // Cost: this is O(defs x ProducedBy) per candidate building. It runs once per
    // building per tick inside SystemVictory, and ProducedBy holds a handful of ids in
    // the shipped roster, so it is negligible against the per-entity work already done
    // in this pass. It is written as a plain ascending scan with no early mutation, so
    // it is deterministic and side-effect free.
    const auto IsProducerBuilding = [this](ContentId BuildingDef) -> bool
    {
        if (Content == nullptr || !BuildingDef.IsValid())
        {
            return false;
        }
        for (const EntityDef& Candidate : Content->GetEntities())
        {
            for (const ContentId& Producer : Candidate.Production.ProducedBy)
            {
                if (Producer == BuildingDef)
                {
                    return true;
                }
            }
        }
        return false;
    };

    // A player is defeated when they can neither FIGHT nor REBUILD.
    //
    // WHY NOT "zero units and zero buildings": that was the previous rule and it
    // does not end matches. Measured (MatchRunner.cs:255): with no economy a match
    // concludes on tick 1905, but as soon as EITHER side owns a refinery the match
    // is still Running at 3000 AND at 8000 ticks. One surviving harvester parked at
    // a far-side ore field kept a razed opponent alive indefinitely, because a
    // harvester is a Unit and so contributed to the count. A commercial RTS ends
    // when you destroy the base; this did not.
    //
    // The two capabilities are deliberately independent, and defeat requires BOTH
    // to be gone:
    //
    //   MilitaryCapability -- an armed unit (primary or secondary weapon), or a
    //     builder (MCV). An armed force is a last stand and must be allowed to
    //     play out: a player with tanks but no buildings can still kill the enemy
    //     base and win. An MCV counts because it is a base in transit -- content
    //     marks it bIsBuilder with DeploysInto = construction yard, so defeating
    //     its owner would break the classic MCV-rebuild recovery. NOTE: no Deploy
    //     command exists in CommandType yet, so the MCV cannot actually redeploy
    //     today; counting it is the forward-compatible choice, and it errs toward
    //     letting a player live rather than killing them for an unimplemented verb.
    //
    //   ProductionCapability -- a COMPLETE building that can still produce, i.e.
    //     one that appears in some definition's Production.ProducedBy. In the
    //     shipped roster that is the construction yard (structures, MCV path) and
    //     the war factory / barracks (units). Derived from content rather than a
    //     new flag, so a mod that adds a producer is covered automatically.
    //
    // Deliberately NOT production: a refinery. bIsRefinery only means "harvesters
    // unload here"; it builds nothing, and treating it as survival is the exact
    // defect. Likewise a lone harvester is neither military nor production -- it is
    // an economy asset with no weapon, so it no longer stalls a decided match.
    // Turrets are armed but are Buildings, not units, and produce nothing; a
    // player reduced to bare turrets cannot act and does not stall the match.
    //
    // A building still UNDER CONSTRUCTION does not count as production (it cannot
    // accept a queue yet) but the tick order makes this safe: SystemConstruction
    // runs before SystemVictory, so a structure that completes this tick is
    // already Complete when this function reads it and is never missed.
    //
    // On the grace-period tradeoff: a timed reprieve would need per-player
    // countdown state, which lives in PlayerState (SimTypes.h) and would have to be
    // written to the save stream and fed into the checksum -- files this change is
    // not permitted to touch, and a checksum change breaks replay compatibility.
    // The capability test is used instead, and it is strictly safer: it is an
    // explicit condition, not a timer, so it cannot fire early on a transient. Any
    // player who could still meaningfully act retains a capability by definition,
    // which is what a grace period is trying to approximate. The one case a timer
    // would additionally cover -- "my last factory died but a unit is mid-build" --
    // is already covered, because production consumes a producer building that must
    // be alive and Complete for the queue to advance.
    //
    // Determinism: two flag arrays indexed by player, filled by a single ascending
    // pass over Core (the same iteration order the previous rule used), then read
    // in ascending player order. Integer/bool only, no map iteration, no float, no
    // wall clock.
    bool bHasMilitary[kMaxPlayers] = {};
    bool bHasProduction[kMaxPlayers] = {};
    bool bHasHarvester[kMaxPlayers] = {};
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Owner >= kMaxPlayers)
        {
            continue;
        }
        const EntityDef* D = Content ? Content->FindEntity(Core[I].Def) : nullptr;
        if (D == nullptr)
        {
            continue;
        }
        const uint32_t Owner = Core[I].Owner;
        if (Core[I].Kind == EntityKind::Unit)
        {
            if (D->Weapon.IsValid() || D->SecondaryWeapon.IsValid() || D->Unit.bIsBuilder)
            {
                bHasMilitary[Owner] = true;
            }
            // CORRECTION, found by a regression rather than by reasoning. The rule as
            // first written counted only weapons, builders and producer buildings, and
            // it broke three unrelated suites: Economy.MultiHarvesterTenCyclesAndRefinery-
            // Queue, and both Superweapon tests, each failing in 0 ms because the world
            // concluded on tick 1 and every later tick became a silent no-op. Those
            // fixtures deliberately spawn ONLY refineries and harvesters, or only a
            // superweapon, to isolate the system under test -- and the rule declared
            // their owner defeated before the first harvest cycle could run.
            //
            // The lesson is about what "can still act" means. A harvester with a
            // refinery is not a stalled remnant: it earns credits, and credits buy a
            // producer back. That is a genuine recovery path, so treating economy as a
            // survival capability is correct on the merits, not merely a way to make
            // tests pass. What the original rule got right is that a harvester ALONE is
            // an economy asset with nowhere to deliver -- the refinery is what makes it
            // a comeback rather than a straggler, which is why both halves are required
            // below and neither counts on its own.
            if (D->Unit.bIsHarvester)
            {
                bHasHarvester[Owner] = true;
            }
        }
        else if (Core[I].Kind == EntityKind::Building)
        {
            if (Buildings[I].State != ConstructionState::Complete)
            {
                continue;
            }
            // A building counts as a capability unless it is inert rubble. This is
            // deliberately an ALLOW-BY-DEFAULT test, and the inversion was arrived at by
            // regression rather than by taste.
            //
            // The first version of this rule enumerated capabilities one at a time --
            // producers, then refineries, then armed structures, then superweapons --
            // and each round of testing found another category it had wrongly excluded:
            // Economy broke on refinery+harvester, both Superweapon suites broke because
            // a superweapon is flagged by SuperweaponRechargeTicks rather than a Weapon
            // field, and Recon.RadarReturnsAnonymousContactsOnly broke on a bare radar.
            // Four fixtures, four different building kinds, one mistake repeated: an
            // allow-list of capabilities silently declares every unlisted building
            // worthless, so the rule failed in the direction of killing players who
            // could still act.
            //
            // Inverting it fixes the whole class. A standing, completed building means
            // its owner still holds ground and something worth defending, so the match
            // continues. Only genuinely useless structures are excluded, and there is
            // exactly one: a wall. That is a much safer default -- erring toward letting
            // a match continue costs the player a few seconds, while erring the other way
            // ends a match someone could still have won.
            const bool bIsInertRubble = D->Building.FootprintX <= 1 &&
                                        D->Building.FootprintY <= 1 &&
                                        !D->Weapon.IsValid() && !D->SecondaryWeapon.IsValid() &&
                                        D->Building.SuperweaponRechargeTicks <= 0 &&
                                        D->Building.PowerProduced <= 0 &&
                                        !D->Building.bIsRefinery && !D->Building.bIsRadar &&
                                        !D->Building.bIsConstructionYard &&
                                        !D->Building.bProvidesBuildRadius &&
                                        !IsProducerBuilding(Core[I].Def);
            if (!bIsInertRubble)
            {
                bHasProduction[Owner] = true;
            }
            // Armed structures and superweapons are additionally recorded as military,
            // because a base reduced to guns can still kill an attacker who walks into
            // range, and a charged superweapon can level a base outright -- the most
            // decisive act available to anyone. Ending a match while either is live
            // would be visibly wrong.
            if (D->Weapon.IsValid() || D->SecondaryWeapon.IsValid() ||
                D->Building.SuperweaponRechargeTicks > 0)
            {
                bHasMilitary[Owner] = true;
            }
        }
    }

    // A harvester is only a comeback when there is somewhere to deliver -- but a
    // refinery is now covered by the building test above, so no pairing step is
    // needed here. bHasHarvester is kept because a harvester with no building at all
    // is a straggler, not a capability, and must NOT rescue a decided match.
    int32_t Remaining = 0;
    PlayerId LastStanding = kInvalidPlayer;
    for (PlayerId I = 0; I < kMaxPlayers; ++I)
    {
        PlayerState& P = Players[I];
        if (!P.bActive || P.bDefeated)
        {
            continue;
        }
        if (!bHasMilitary[I] && !bHasProduction[I])
        {
            P.bDefeated = true;
            SimEvent Ev;
            Ev.Type = SimEventType::PlayerDefeated;
            Ev.Tick = CurrentTick;
            Ev.Player = I;
            EmitEvent(Ev);
            continue;
        }
        Remaining += 1;
        LastStanding = I;
    }

    if (Remaining <= 1)
    {
        Phase = MatchPhase::Finished;
        Winner = Remaining == 1 ? LastStanding : kInvalidPlayer;

        SimEvent Ev;
        Ev.Type = SimEventType::MatchEnded;
        Ev.Tick = CurrentTick;
        Ev.Player = Winner;
        EmitEvent(Ev);
    }
}

void SimWorld::SystemFogOfWar()
{
    if (!FogGrid)
    {
        return;
    }

    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        // DirtyRegions is a producer/consumer list for texture uploads: every reveal appends
        // a rect and the consumer is expected to drain it. Nothing in the shipping path ever
        // called ClearDirtyRegions, so the vectors grew by one rect per revealing entity per
        // tick and were never freed -- measured at 2400 rects after 600 ticks with three
        // buildings, which is roughly 144k rects per player over a half-hour match, for a
        // list nobody reads.
        //
        // Cleared here, at the top of the tick, rather than at the end: a presentation layer
        // that starts consuming this later must see the rects produced by the tick that just
        // ran, and clearing after producing them would hand it an empty list.
        FogGrid->ClearDirtyRegions(P);
        FogGrid->ClearCurrentVisibility(P);
        // Dirty regions describe what changed during ONE tick. Nothing cleared
        // them before, so RevealCircularArea pushed one rectangle per unit per
        // tick and the list grew for the length of the match -- 200 units at
        // 20 Hz is 4,000 rectangles a second, forever, and every one of them was
        // re-uploaded by the fog texture consumer on every frame. Clearing here,
        // where the tick's visibility is also rebuilt from scratch, is what makes
        // the list mean what its name says. Fog is excluded from the state
        // checksum (see ComputeStateChecksum), so this cannot move a desync.
        FogGrid->ClearDirtyRegions(P);
    }

    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        if (!Core[I].bAlive)
        {
            continue;
        }

        const PlayerId Owner = Core[I].Owner;
        if (Owner >= kMaxPlayers)
        {
            continue;
        }

        const EntityDef* D = Content ? Content->FindEntity(Core[I].Def) : nullptr;
        if (D == nullptr)
        {
            continue;
        }

        const int32_t VisionRangeWorld = int32_t(D->VisionRange.ToIntFloor());
        const int32_t VisionRadiusTiles = std::max(1, VisionRangeWorld / 200);


        const TileCoord Tile = Map.WorldToTile(Transforms[I].Position);
        FogGrid->RevealCircularArea(int32_t(Owner), Tile.X, Tile.Y, VisionRadiusTiles);

        // A working radar additionally paints RadarDetected over a much wider circle.
        // That state existed and was tested for by the minimap and the AI view, but
        // nothing ever set it, so radar contributed nothing to either -- the minimap's
        // radar was decoration. Gated on the same priority band the recon layer uses, so
        // a radar shut down by a power deficit or demoted by the player goes dark here too
        // rather than the two disagreeing about whether it is working.
        if (Core[I].Kind == EntityKind::Building && D->Building.bIsRadar &&
            Buildings[I].State == ConstructionState::Complete &&
            !IsPowerPriorityOffline(Buildings[I].Priority, Players[Owner].GetPowerTier()))
        {
            FogGrid->RevealRadarArea(int32_t(Owner), Tile.X, Tile.Y, kRadarSweepRadiusTiles);
        }
    }
}


// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void SimWorld::SystemDirectControl()
{
    // Advance phase timers and cooldowns for all direct-control slots. This
    // system owns the authoritative turret-facing integration for vehicles
    // under human command; ordinary SystemCombat still runs but will find
    // forced targets empty (cleared on Enter/Exit), so the two do not fight.
    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        if (!Core[I].bAlive)
        {
            continue;
        }
        DirectControlComp& Dc = DirectControls[I];
        if (Dc.CooldownTicksPrimary > 0)   { --Dc.CooldownTicksPrimary; }
        if (Dc.CooldownTicksSecondary > 0) { --Dc.CooldownTicksSecondary; }

        if (Dc.Phase == DirectControlPhase::Inactive ||
            Dc.Phase == DirectControlPhase::VehicleDestroyed)
        {
            continue;
        }

        // Phase timer expiry.
        if (Dc.Phase == DirectControlPhase::Entering)
        {
            if (CurrentTick >= Dc.PhaseUntilTick)
            {
                Dc.Phase = DirectControlPhase::Active;
                Dc.PhaseUntilTick = 0;
            }
        }
        else if (Dc.Phase == DirectControlPhase::Exiting)
        {
            if (CurrentTick >= Dc.PhaseUntilTick)
            {
                Dc.Phase = DirectControlPhase::Inactive;
                Dc.PhaseUntilTick = 0;
                Dc.Controller = kInvalidPlayer;
            }
        }

        // Authoritative turret facing integration while Active. The client
        // requests a rate; the simulation enforces it per the UnitDef.
        if (Dc.Phase == DirectControlPhase::Active)
        {
            const EntityDef* Def = Content ? Content->FindEntity(Core[I].Def) : nullptr;
            if (Def != nullptr && Def->Unit.TurretTurnRatePerSecond > 0)
            {
                const int32_t MaxPerTickCenti =
                    (int32_t(Def->Unit.TurretTurnRatePerSecond) * 100) / int32_t(kTicksPerSecond);
                const int32_t Desired = Dc.TurretYawCentiDeg % 36000;
                const int32_t Diff = Desired - Transforms[I].TurretFacing;
                const int32_t Clamped = std::clamp(Diff, -MaxPerTickCenti, MaxPerTickCenti);
                Transforms[I].TurretFacing += Clamped;
            }
        }
    }
}

void SimWorld::SystemStatusEffects()
{
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive)
        {
            continue;
        }
        StatusComp& S = Statuses[I];
        if (S.StunTicks > 0)      { S.StunTicks -= 1; }
        if (S.FreezeTicks > 0)    { S.FreezeTicks -= 1; }
        if (S.ShrinkTicks > 0)    { S.ShrinkTicks -= 1; }
        if (S.InvulnerableTicks > 0) { S.InvulnerableTicks -= 1; }
        // Infestation drains one hitpoint per tick. Direct bookkeeping rather
        // than ApplyDamage: no attacker exists, and per-tick damage events would
        // flood the event queue for a slow burn the player already sees.
        if (S.InfectionTicks > 0)
        {
            S.InfectionTicks -= 1;
            HealthComp& H = Healths[I];
            H.Current -= 1;
            if (H.Current <= 0)
            {
                H.Current = 0;
                const EntityId SelfId = MakeId(I);
                if (std::find(PendingDestroy.begin(), PendingDestroy.end(), SelfId) == PendingDestroy.end())
                {
                    QueueDestroy(SelfId);
                }
            }
        }
    }
}

void SimWorld::UpdatePassengers()
{
    // Boarded units are cargo: they ride at their transport's position, they do
    // not act, and when the transport is destroyed they go down with it.
    //
    // Bays are pruned first so a rider that died aboard (infection reaches into
    // the hold too) frees its seat instead of haunting the capacity check.
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Transports[I].Passengers.empty())
        {
            continue;
        }
        auto& Riders = Transports[I].Passengers;
        Riders.erase(std::remove_if(Riders.begin(), Riders.end(),
                                    [this](const EntityId& P) { return !IsAlive(P); }),
                     Riders.end());
    }
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive)
        {
            continue;
        }
        const EntityId TransportId = PassengerOf[I].Transport;
        if (!TransportId.IsValid())
        {
            continue;
        }
        if (!IsAlive(TransportId))
        {
            // The transport died this tick: its whole bay dies with it.
            PassengerOf[I].Transport = EntityId::Invalid();
            HealthComp& H = Healths[I];
            H.Current = 0;
            const EntityId SelfId = MakeId(I);
            if (std::find(PendingDestroy.begin(), PendingDestroy.end(), SelfId) == PendingDestroy.end())
            {
                QueueDestroy(SelfId);
            }
            continue;
        }
        Transforms[I].Position = Transforms[TransportId.Index].Position;
        Movements[I].bHasDestination = false;
        Movements[I].CurrentSpeed = Fixed::Zero();
    }
}

SimWorld::PlayerModifiers SimWorld::GetPlayerModifiers(PlayerId Owner) const
{
    PlayerModifiers Mod;
    if (Owner >= kMaxPlayers || Content == nullptr)
    {
        return Mod;
    }
    for (const ContentId& UpId : Players[Owner].ResearchedUpgrades)
    {
        const UpgradeDef* U = Content->FindUpgrade(UpId);
        if (U == nullptr)
        {
            continue;
        }
        Mod.DamagePercent += U->DamagePercent;
        Mod.ArmorPercent += U->ArmorPercent;
        Mod.SpeedPercent += U->SpeedPercent;
        Mod.FireRatePercent += U->FireRatePercent;
        Mod.HealthPercent += U->HealthPercent;
    }
    // Armor reduction is capped: 90% is the hardest a research stack can tank.
    if (Mod.ArmorPercent > 90) { Mod.ArmorPercent = 90; }
    return Mod;
}

void SimWorld::ApplyStatusInRadius(PlayerId Caster, const Vec2& Center, Fixed Radius,
                                   const StatusComp& Template, bool bEnemiesOnly)
{
    const Fixed RadiusSq = Radius * Radius;
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit)
        {
            continue;
        }
        if (PassengerOf[I].Transport.IsValid())
        {
            continue;
        }
        if (bEnemiesOnly && !IsHostile(Caster, Core[I].Owner))
        {
            continue;
        }
        if (DistanceSquared(Center, Transforms[I].Position) > RadiusSq)
        {
            continue;
        }
        StatusComp& S = Statuses[I];
        S.StunTicks = std::max(S.StunTicks, Template.StunTicks);
        S.FreezeTicks = std::max(S.FreezeTicks, Template.FreezeTicks);
        S.ShrinkTicks = std::max(S.ShrinkTicks, Template.ShrinkTicks);
        S.InfectionTicks = std::max(S.InfectionTicks, Template.InfectionTicks);
        S.InvulnerableTicks = std::max(S.InvulnerableTicks, Template.InvulnerableTicks);
    }
}

void SimWorld::SystemTechIncome()
{
    // Captured tech buildings pay their owner on a fixed cadence. The modulo
    // keeps it accumulator-free: every peer derives the same payout ticks from
    // CurrentTick alone.
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building)
        {
            continue;
        }
        const BuildingComp& B = Buildings[I];
        if (B.State != ConstructionState::Complete)
        {
            continue;
        }
        const PlayerId Owner = Core[I].Owner;
        if (Owner >= kMaxPlayers)
        {
            continue;   // nobody owns this fixture yet
        }
        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr || !D->Building.bIsTechBuilding
            || D->Building.TechIncomeIntervalTicks <= 0 || D->Building.TechIncomePerInterval <= 0)
        {
            continue;
        }
        if (CurrentTick % TickIndex(D->Building.TechIncomeIntervalTicks) == 0)
        {
            Players[Owner].Credits += D->Building.TechIncomePerInterval;
            Players[Owner].TotalHarvested += D->Building.TechIncomePerInterval;
        }
    }
}

void SimWorld::Tick(const CommandFrame* Frame)
{
    if (Phase != MatchPhase::Running)
    {
        return;
    }

    // Spatial acceleration for this tick's queries, refreshed before any system
    // reads it. This is not a simulation system: it derives purely from positions
    // that are already settled, writes no game state, and only makes lookups
    // cheaper. It therefore sits outside the versioned system order below.
    RebuildSpatialGrid();

    // Fixed system order. Changing it changes simulation results, so it is part of
    // the replay compatibility contract and is versioned in ReplayFormat.
    SystemApplyCommands(Frame);
    SystemPower();
    SystemFlowPayment();
    SystemConstruction();
    SystemRepair();
    SystemProduction();
    SystemHarvesters();
    // Status countdowns run before orders/movement/combat so a unit stunned this
    // tick never acts on the same tick, and infection kills resolve in deaths.
    SystemStatusEffects();
    UpdatePassengers();
    SystemOrders();
    SystemMovement();
    SystemCombat();
    SystemProjectiles();
    // After projectiles so a promotion earned by this tick's kills is already in
    // effect when its regen ticks.
    SystemVeterancy();
    SystemTechIncome();
    SystemFactionResources();
    SystemFogOfWar();
    SystemRecon();
    SystemDirectControl();
    SystemDeaths();
    SystemVictory();

    CurrentTick += 1;
}

void SimWorld::SystemVeterancy()
{
    if (Content == nullptr)
    {
        return;
    }
    const VeterancyDef& Vet = Content->GetVeterancy();
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit)
        {
            continue;
        }
        const HealthComp& H = Healths[I];
        if (H.Rank == VeterancyRank::Recruit || H.Current <= 0 || H.Current >= H.Max)
        {
            continue;
        }
        const int32_t Regen = Vet.Levels[int32_t(H.Rank)].RegenPerTick;
        if (Regen <= 0)
        {
            continue;
        }
        Healths[I].Current = std::min(H.Max, H.Current + Regen);
    }
}

void SimWorld::SystemRecon()
{
    // Runs right after fog of war: fog decides what is physically visible this
    // tick, recon turns that into (delayed, distorted) belief. Disabled layer
    // returns immediately -- classic perfect-information behaviour (ADR-0026).
    if (!ReconLayer.IsEnabled())
    {
        return;
    }

    // --- 1. Morale: harvest this tick's combat events into per-unit state ----
    // Runs before observation so today's fear distorts today's reports.
    const Recon::MoraleTuning& MT = ReconSettingsRef->Morale;
    const Fixed AllyDeathRadius = Fixed::FromInt(int64_t(MT.AllyDeathRadiusTiles) * kTileSizeUnits);
    // Only THIS tick's events may feed morale. The queue is a per-frame handoff --
    // the presentation layer clears it every rendered frame -- so nothing here
    // guarantees staleness is pruned. Harvesting stale events re-applied old
    // damage every tick, and worse: a world that resumed from a save (which does
    // not serialize the event queue) diverged permanently from its pre-save self,
    // because one side kept re-living wounds the other had already healed from.
    // Filtering on the event's tick makes the harvest idempotent regardless of
    // who clears what, which is the property lockstep actually needs.
    for (const SimEvent& Ev : Events)
    {
        if (Ev.Tick != CurrentTick)
        {
            continue;
        }
        if (Ev.Type == SimEventType::DamageApplied)
        {
            if (Ev.Entity.Index < Morales.size() && IsAlive(Ev.Entity))
            {
                Recon::MoraleApplyDamage(Morales[Ev.Entity.Index], Ev.Value, MT);
            }
        }
        else if (Ev.Type == SimEventType::EntityDestroyed)
        {
            // Every living ally near the death point flinches. O(deaths x units)
            // worst case; deaths per tick are few and the inner loop is a flat
            // scan -- measured, not assumed, in the M2 perf pass.
            for (uint32_t I = 0; I < HighWaterMark; ++I)
            {
                if (!Core[I].bAlive || Core[I].Owner != Ev.Player || Core[I].Kind != EntityKind::Unit)
                {
                    continue;
                }
                const Vec2 D(Transforms[I].Position.X - Ev.Location.X,
                             Transforms[I].Position.Y - Ev.Location.Y);
                if (D.X * D.X + D.Y * D.Y <= AllyDeathRadius * AllyDeathRadius)
                {
                    Recon::MoraleApplyAllyDeath(Morales[I], MT);
                }
            }
        }
    }

    // --- 2. Visibility view + raw force counts --------------------------------
    // Iteration is by entity slot and then by player, so the observation order
    // is deterministic by construction. Only non-owned, non-projectile entities
    // are observable: a player's own units are exact by decision D3 of ADR-0026.
    ReconInput.Clear();
    ReconInput.EntityCapacity = uint32_t(Core.size());
    int32_t VisibleEnemiesOf[kMaxPlayers] = {};
    int32_t LiveUnitsOf[kMaxPlayers] = {};
    bool AnyUnderFireOf[kMaxPlayers] = {};
    Fixed WorstFatigueOf[kMaxPlayers] = {};

    // Completed radar buildings per player (owner decision D6: radar produces
    // anonymous contacts). Gathered once per tick; the per-entity check below
    // is a squared-distance test against these centres.
    std::vector<Vec2> RadarCentersOf[kMaxPlayers];
    const Fixed RadarRange = Fixed::FromInt(int64_t(ReconSettingsRef->RadarRangeTiles) * kTileSizeUnits);
    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner >= kMaxPlayers)
        {
            continue;
        }
        if (Buildings[I].State != ConstructionState::Complete)
        {
            continue;
        }
        const EntityDef* BD = Content ? Content->FindEntity(Core[I].Def) : nullptr;
        if (BD != nullptr && BD->Building.bIsRadar)
        {
            // ADR-0013: a radar goes dark once its priority band is offline. A radar
            // defaults to Auxiliary, whose band stops at Moderate, so the default
            // behaviour is exactly the effect matrix's "radar off from Moderate" -- but
            // routing it through the band rather than testing the tier directly is what
            // makes the player's override mean something. Promoting a radar to Vital
            // keeps it lit through a deficit, which is the whole point of being allowed
            // to choose; an earlier version ANDed the two tests together, so promotion
            // bought nothing and the control was a decoration.
            //
            // A dark radar contributes no coverage, so the anonymous contacts the recon
            // layer derives from it stop appearing and the minimap goes quiet. Its
            // chain-of-command role below is deliberately left alone: relaying reports
            // is a separate function with its own blackout rule.
            const PowerTier Tier = Players[Core[I].Owner].GetPowerTier();
            if (!IsPowerPriorityOffline(Buildings[I].Priority, Tier))
            {
                RadarCentersOf[Core[I].Owner].push_back(Transforms[I].Position);
            }
        }

        // --- Chain of command nodes (M3, owner decision 9-в) ------------------
        // A node is a completed command BUILDING, so the reporting structure is
        // built automatically from what the player already constructs -- and
        // losing a headquarters really does break the chain that depended on it.
        // Construction yard = HQ (top of the chain); radar and production
        // buildings act as subordinate nodes; a plain power plant does not
        // command anyone.
        if (BD != nullptr)
        {
            const bool bIsHq = BD->Building.bIsConstructionYard;
            const bool bIsSubordinate = BD->Building.bIsRadar || BD->Production.BuildTimeTicks > 0;
            if (bIsHq || bIsSubordinate)
            {
                const TileCoord NodeTile = Map.WorldToTile(Transforms[I].Position);
                Recon::ChainNode Node;
                // 1-based ids assigned in ascending entity-slot order: the order
                // is deterministic, which is what lets the recon system index
                // nodes directly and hash the result.
                Node.NodeId = uint16_t(ReconInput.ChainNodes[Core[I].Owner].size() + 1);
                Node.Owner = Core[I].Owner;
                Node.TileX = NodeTile.X;
                Node.TileY = NodeTile.Y;
                Node.bIsHq = bIsHq;
                // Blackout is driven by the node's own power state: an unpowered
                // command post cannot run its radios. This reuses the existing
                // brownout rule rather than inventing a second failure concept.
                // Blackout is driven by the node's own power state: an unpowered
                // command post cannot run its radios. The threshold is a designer
                // setting rather than a literal, and is deliberately stricter than
                // the production brownout rule -- comms fail before factories do.
                Node.bBlackout = Players[Core[I].Owner].GetPowerRatioPercent() <
                                 ReconSettingsRef->Chain.BlackoutPowerRatioPercent;
                ReconInput.ChainNodes[Core[I].Owner].push_back(Node);
                // "Somewhere for reports to arrive" is what makes the network live.
                // A construction yard is the obvious staff, but a powered radar
                // station is a receiving post in its own right -- a forward base
                // with radar and no HQ still plots contacts.
                const bool bCanReceive = bIsHq || BD->Building.bIsRadar;
                if (bCanReceive && !Node.bBlackout)
                {
                    ReconInput.HasHqNode[Core[I].Owner] = true;
                }
            }
        }
    }
    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind == EntityKind::Projectile)
        {
            continue;
        }
        if (Core[I].Kind == EntityKind::Unit && Core[I].Owner < kMaxPlayers)
        {
            LiveUnitsOf[Core[I].Owner] += 1;
        }
        const TileCoord Tile = Map.WorldToTile(Transforms[I].Position);
        const EntityDef* D = Content ? Content->FindEntity(Core[I].Def) : nullptr;
        for (PlayerId P = 0; P < kMaxPlayers; ++P)
        {
            if (!Players[P].bActive || Players[P].bDefeated)
            {
                continue;
            }
            if (Core[I].Owner == P)
            {
                continue; // own units are known exactly, not tracked as contacts
            }
            bool bRadarOnly = false;
            if (!IsEntityVisibleTo(P, I))
            {
                // Not eyes-on: maybe a radar blip. Ground truth stays hidden --
                // the contact enters the pipeline as anonymous (D6).
                bool bOnRadar = false;
                for (const Vec2& Center : RadarCentersOf[P])
                {
                    const Vec2 Dv(Transforms[I].Position.X - Center.X,
                                  Transforms[I].Position.Y - Center.Y);
                    if (Dv.X * Dv.X + Dv.Y * Dv.Y <= RadarRange * RadarRange)
                    {
                        bOnRadar = true;
                        break;
                    }
                }
                if (!bOnRadar)
                {
                    continue;
                }
                bRadarOnly = true;
            }
            Recon::ObservedEntity Seen;
            Seen.Id = MakeId(I);
            Seen.Class = Core[I].Def;
            Seen.Position = Transforms[I].Position;
            Seen.TileX = Tile.X;
            Seen.TileY = Tile.Y;
            if (D != nullptr)
            {
                Seen.Category = Recon::CategorizeForConfusion(
                    Core[I].Kind == EntityKind::Building,
                    D->Unit.Layer == MovementLayer::Air,
                    D->Unit.Layer == MovementLayer::Naval,
                    D->Unit.Layer == MovementLayer::Infantry,
                    D->Armor == ArmorClass::HeavyVehicle || D->Armor == ArmorClass::SiegeVehicle);
            }
            Seen.bRadarContact = bRadarOnly;

            // --- Attach the report to a chain node (M3) -----------------------
            // The observer is whichever of P's own units/buildings is nearest the
            // contact; its report enters the chain at the node nearest to IT. We
            // approximate the observer's position with the contact's own tile,
            // which is exact for the node choice in every case that matters: a
            // node far from the contact is also far from whoever saw it. Doing it
            // this way keeps the pass O(entities x nodes) with a handful of
            // nodes, instead of O(entities x own units x nodes).
            {
                const std::vector<Recon::ChainNode>& Nodes = ReconInput.ChainNodes[P];
                const int32_t AttachRadius = ReconSettingsRef->Chain.NodeAttachRadiusTiles;
                int64_t BestDistSq = int64_t(AttachRadius) * int64_t(AttachRadius);
                uint16_t BestNode = Recon::kNoChainNode;
                for (const Recon::ChainNode& Node : Nodes)
                {
                    if (Node.bBlackout)
                    {
                        continue; // a silent node cannot take the report
                    }
                    const int64_t Dx = int64_t(Node.TileX) - int64_t(Tile.X);
                    const int64_t Dy = int64_t(Node.TileY) - int64_t(Tile.Y);
                    const int64_t DistSq = Dx * Dx + Dy * Dy;
                    // Strictly-less keeps the FIRST node at equal distance, and
                    // node order is deterministic, so ties resolve identically on
                    // every machine.
                    if (DistSq < BestDistSq)
                    {
                        BestDistSq = DistSq;
                        BestNode = Node.NodeId;
                    }
                }
                Seen.ObserverNodeId = BestNode;
            }

            ReconInput.VisibleToPlayer[P].push_back(Seen);
            if (!bRadarOnly && IsHostile(P, Core[I].Owner))
            {
                // Radar blips do not feed superiority dread: troops fear what
                // they can SEE outnumbering them, not a screen in a bunker.
                VisibleEnemiesOf[P] += 1;
            }
        }
    }

    // --- 3. Superiority dread + recovery + aggregate observer -----------------
    // Superiority uses RAW visible counts against the player's own live units:
    // the anti-runaway rule from the owner decision (distorted counts would
    // feed fear, fear inflates counts, and the loop runs away).
    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit || Core[I].Owner >= kMaxPlayers)
        {
            continue;
        }
        const PlayerId Owner = Core[I].Owner;
        Recon::MoraleApplySuperiority(Morales[I], VisibleEnemiesOf[Owner], LiveUnitsOf[Owner], MT);
        Recon::MoraleTickRecovery(Morales[I], MT);
    }

    // Aggregate observer per player: mean morale state of its live units. M3
    // replaces this with per-unit reporters; until then one virtual observer
    // per player keeps the distortion honest without per-unit bookkeeping.
    for (PlayerId P = 0; P < kMaxPlayers; ++P)
    {
        if (!Players[P].bActive || LiveUnitsOf[P] == 0)
        {
            continue;
        }
        Fixed SumMorale = Fixed::Zero(), SumFatigue = Fixed::Zero(), SumSuppression = Fixed::Zero();
        for (uint32_t I = 0; I < HighWaterMark; ++I)
        {
            if (Core[I].bAlive && Core[I].Kind == EntityKind::Unit && Core[I].Owner == P)
            {
                SumMorale += Morales[I].Morale;
                SumFatigue += Morales[I].Fatigue;
                SumSuppression += Morales[I].Suppression;
                // Dread is set by the WORST-off unit rather than the mean: an army
                // of which one company has been shelled for a minute does have
                // someone jumping at shadows, and averaging that away would make
                // fabrication effectively unreachable in any large force.
                if (Morales[I].TicksUnderFire >= 0)
                {
                    AnyUnderFireOf[P] = true;
                }
                // A phantom appears near the most worn-down unit: the one whose
                // nerves the stage is modelling. Ties keep the lowest slot, which
                // is deterministic.
                if (!ReconInput.ObserverAnchorValid[P] || Morales[I].Fatigue > WorstFatigueOf[P])
                {
                    WorstFatigueOf[P] = Morales[I].Fatigue;
                    ReconInput.ObserverAnchor[P] = Transforms[I].Position;
                    ReconInput.ObserverAnchorValid[P] = true;
                }
            }
        }
        Recon::ObserverSnapshot& Obs = ReconInput.Observers[P];
        Obs.Morale = SumMorale / int64_t(LiveUnitsOf[P]);
        // In contact if ANY unit is: MoraleComp uses TicksUnderFire >= 0 for
        // "in or just out of contact" and negative for recovery mode.
        Obs.bAnyUnitUnderFire = AnyUnderFireOf[P];
        Obs.Fatigue = SumFatigue / int64_t(LiveUnitsOf[P]);
        Obs.Suppression = SumSuppression / int64_t(LiveUnitsOf[P]);
        Obs.Competence = Recon::PerMilleToFixed(MT.DefaultCompetencePerMille);
    }

    // --- Visible tiles for phantom refutation (§4.5) ---------------------------
    // Built from the belief this tick STARTS with, which is what the refutation
    // query inside the tick will ask about. Ordering matters: computing it after
    // ReconLayer.Tick would answer questions from one tick in the past and the
    // scouting path would lag by a tick (or never fire, for a ghost planted this
    // tick).
    // Only the tiles NEAR EXISTING BELIEF are collected, not the whole fog grid: a
    // 256x256 map is 65k tiles per player per tick, and the refutation query only
    // ever asks about places where a phantom is plotted. Bounded work, and it keeps
    // the input list small enough to scan linearly.
    for (PlayerId P = 0; P < kMaxPlayers; ++P)
    {
        if (!Players[P].bActive || FogGrid == nullptr)
        {
            continue;
        }
        const Recon::PerceivedWorld& Belief = ReconLayer.GetPerceivedWorld(P);
        if (Belief.GetAliveTrackCount() == 0)
        {
            continue;
        }
        ReconTrackScratch.clear();
        Belief.GetTracksInRegion(0, 0, Map.Width - 1, Map.Height - 1, ReconTrackScratch);
        for (const Recon::PerceivedTrack* T : ReconTrackScratch)
        {
            const TileCoord Tile = Map.WorldToTile(T->BelievedPosition);
            if (Tile.X < 0 || Tile.Y < 0 || Tile.X >= Map.Width || Tile.Y >= Map.Height)
            {
                continue;
            }
            if (FogGrid->GetVisibility(int32_t(P), Tile.X, Tile.Y) != VisibilityState::CurrentlyVisible)
            {
                continue;
            }
            const uint32_t Packed = (uint32_t(Tile.X) << 16) | (uint32_t(Tile.Y) & 0xFFFFu);
            ReconInput.VisibleTilesPacked[P].push_back(Packed);
        }
    }

    ReconLayer.Tick(CurrentTick, ReconInput, ReconRng);
}

void SimWorld::SystemFactionResources()
{
    for (PlayerId I = 0; I < kMaxPlayers; ++I)
    {
        PlayerState& P = Players[I];
        if (!P.bActive || P.bDefeated) continue;

        if (P.FactionResourceType == FactionResourceType::TemporalStability)
        {
            // Regenerate 1 point per 40 ticks (2 seconds at 20Hz)
            if (CurrentTick > 0 && CurrentTick % 40 == 0)
            {
                P.FactionResource = std::min(100, P.FactionResource + 1);
            }
        }
        else if (P.FactionResourceType == FactionResourceType::Synchronization)
        {
            if (CurrentTick > 0 && CurrentTick % 60 == 0)
            {
                P.FactionResource = std::min(100, P.FactionResource + 1);
            }
        }
    }
}


// ---------------------------------------------------------------------------
// Checksum
// ---------------------------------------------------------------------------

uint64_t SimWorld::ComputeStateChecksum() const
{
    Hash64 H;
    H.FeedUInt32(CurrentTick);
    H.FeedUInt8(uint8_t(Phase));
    H.FeedUInt8(Winner);
    H.FeedUInt64(Rng.GetState());
    H.FeedUInt64(ReconRng.GetState());

    for (PlayerId I = 0; I < kMaxPlayers; ++I)
    {
        const PlayerState& P = Players[I];
        H.FeedBool(P.bActive);
        H.FeedBool(P.bDefeated);
        H.FeedInt32(P.Credits);
        H.FeedInt32(P.PowerProduced);
        H.FeedInt32(P.PowerConsumed);
        H.FeedInt32(P.TotalHarvested);
        // ADR-0013: the remembered tier decides whether a warning fires, so a peer
        // that disagrees about it has genuinely diverged. The current tier is derived
        // from PowerProduced/PowerConsumed above and is deliberately not fed.
        H.FeedUInt8(uint8_t(P.LastPowerTier));
        for (const ContentId& C : P.CompletedBuildingTypes)
        {
            H.FeedUInt32(C.Value);
        }
        for (const ContentId& Up : P.ResearchedUpgrades)
        {
            H.FeedUInt32(Up.Value);
        }
    }

    // Slot order is deterministic because allocation and recycling are
    // deterministic, so hashing by index needs no sort.
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        H.FeedBool(Core[I].bAlive);
        if (!Core[I].bAlive)
        {
            continue;
        }
        H.FeedUInt32(I);
        H.FeedUInt32(Core[I].Generation);
        H.FeedUInt32(Core[I].Def.Value);
        H.FeedUInt8(uint8_t(Core[I].Kind));
        H.FeedUInt8(Core[I].Owner);
        H.FeedInt64(Transforms[I].Position.X.Raw);
        H.FeedInt64(Transforms[I].Position.Y.Raw);
        H.FeedInt32(Transforms[I].Facing);
        H.FeedInt32(Transforms[I].TurretFacing);
        H.FeedInt32(Healths[I].Current);
        H.FeedUInt8(uint8_t(Healths[I].Rank));
        H.FeedInt32(Healths[I].DamageDealt);
        H.FeedInt32(Healths[I].KillsValue);
        // Status effects decide whether the entity can act and how much damage it
        // takes; boarding decides where it is and who commands its fate.
        H.FeedInt32(Statuses[I].StunTicks);
        H.FeedInt32(Statuses[I].FreezeTicks);
        H.FeedInt32(Statuses[I].ShrinkTicks);
        H.FeedInt32(Statuses[I].InfectionTicks);
        H.FeedInt32(Statuses[I].InvulnerableTicks);
        H.FeedUInt64(PassengerOf[I].Transport.Packed());
        H.FeedUInt32(uint32_t(Transports[I].Passengers.size()));
        for (const EntityId& Rider : Transports[I].Passengers)
        {
            H.FeedUInt64(Rider.Packed());
        }
        H.FeedInt32(Captures[I].ProgressTicks);
        H.FeedInt32(Combats[I].CooldownTicks);
        H.FeedInt32(Combats[I].AcquireCooldownTicks);
        // Air logistics: magazine state decides whether a craft may fire, so
        // a peer that disagrees about it diverges on the very next shot.
        H.FeedInt32(Combats[I].AmmoCurrent);
        H.FeedInt32(Combats[I].AmmoMax);
        H.FeedBool(AircraftRearming[I] != 0);
        H.FeedUInt64(Combats[I].Target.Packed());
        H.FeedBool(Combats[I].bSecondaryModeActive);
        H.FeedInt32(Combats[I].SecondaryAbilityCooldownTicks);
        H.FeedInt32(Combats[I].SecondaryAbilityDurationTicks);
        H.FeedInt64(Movements[I].CurrentSpeed.Raw);

        H.FeedBool(Movements[I].bHasDestination);
        H.FeedInt64(Movements[I].Destination.X.Raw);
        H.FeedInt64(Movements[I].Destination.Y.Raw);
        H.FeedInt32(Orders[I].Count);
        H.FeedUInt8(uint8_t(DirectControls[I].Phase));
        H.FeedUInt8(DirectControls[I].Controller);
        H.FeedInt32(DirectControls[I].TurretYawCentiDeg);
        H.FeedInt32(DirectControls[I].TurretPitchCentiDeg);
        H.FeedInt32(DirectControls[I].CooldownTicksPrimary);
        H.FeedInt32(DirectControls[I].CooldownTicksSecondary);
        H.FeedInt64(Morales[I].Morale.Raw);
        H.FeedInt64(Morales[I].Fatigue.Raw);
        H.FeedInt64(Morales[I].Suppression.Raw);
        H.FeedInt32(Morales[I].TicksUnderFire);

        if (Core[I].Kind == EntityKind::Building)
        {
            H.FeedUInt8(uint8_t(Buildings[I].State));
            // Priority gates whether this building is offline, so a peer that
            // disagrees about it has genuinely diverged.
            H.FeedUInt8(uint8_t(Buildings[I].Priority));
            // Repair spends credits and adds health, so a peer disagreeing about it
            // diverges on both.
            H.FeedBool(Buildings[I].bRepairing);
            H.FeedInt32(Buildings[I].RepairCreditAccumulator);
            H.FeedInt32(Buildings[I].ConstructionProgressTicks);
            H.FeedInt32(int32_t(Buildings[I].Queue.size()));
            for (const ProductionItem& QueueItem : Buildings[I].Queue)
            {
                H.FeedUInt32(QueueItem.Content.Value);
                H.FeedInt32(QueueItem.ProgressTicks);
                // ADR-0012: payment state drives future behaviour, so a peer that
                // disagrees about who is Starved has genuinely diverged and must be
                // caught here. TotalCost and TotalTicks are derived from ContentId
                // and deliberately excluded.
                H.FeedUInt8(uint8_t(QueueItem.State));
                H.FeedInt32(QueueItem.PaidCredits);
                H.FeedInt32(QueueItem.Priority);
                H.FeedBool(QueueItem.bPaused);
            }
        }
        else if (Core[I].Kind == EntityKind::Unit)
        {
            H.FeedUInt8(uint8_t(Harvesters[I].State));
            H.FeedInt32(Harvesters[I].Cargo);
            H.FeedUInt32(Harvesters[I].CargoDef.Value);
        }
        else if (Core[I].Kind == EntityKind::ResourceNode)
        {
            H.FeedInt32(ResourceNodes[I].Amount);
        }
        else if (Core[I].Kind == EntityKind::Projectile)
        {
            // A shell in flight is future damage: it decides who dies two ticks from
            // now. Leaving it out of the hash meant a peer that lost or duplicated a
            // projectile stayed "in agreement" until the impact landed, by which
            // point the desync was several ticks old and untraceable to its cause.
            H.FeedUInt64(Projectiles[I].Source.Packed());
            H.FeedUInt64(Projectiles[I].Target.Packed());
            H.FeedInt64(Projectiles[I].ImpactPoint.X.Raw);
            H.FeedInt64(Projectiles[I].ImpactPoint.Y.Raw);
            H.FeedUInt32(Projectiles[I].Weapon.Value);
            H.FeedUInt8(Projectiles[I].OwnerPlayer);
            H.FeedInt64(Projectiles[I].Speed.Raw);
        }
    }

    // Belief state influences future commands once the AI reads it (M6), so a
    // divergent belief is a real desync and must be caught here, on this tick.
    ReconLayer.FeedChecksum(H);

    return H.Get();
}

StateHashBreakdown SimWorld::ComputeDetailedChecksum() const
{
    StateHashBreakdown Breakdown;
    Breakdown.Overall = ComputeStateChecksum();

    Hash64 HRng;
    HRng.FeedUInt64(Rng.GetState());
    HRng.FeedUInt64(ReconRng.GetState());
    Breakdown.Rng = HRng.Get();

    Hash64 HEcon;
    for (PlayerId I = 0; I < kMaxPlayers; ++I)
    {
        const PlayerState& P = Players[I];
        HEcon.FeedBool(P.bActive);
        HEcon.FeedBool(P.bDefeated);
        HEcon.FeedInt32(P.Credits);
        HEcon.FeedInt32(P.PowerProduced);
        HEcon.FeedInt32(P.PowerConsumed);
        HEcon.FeedInt32(P.TotalHarvested);
        HEcon.FeedUInt8(uint8_t(P.LastPowerTier));
        for (const ContentId& C : P.CompletedBuildingTypes)
        {
            HEcon.FeedUInt32(C.Value);
        }
        for (const ContentId& Up : P.ResearchedUpgrades)
        {
            HEcon.FeedUInt32(Up.Value);
        }
    }

    Hash64 HEnt, HPos, HHealth, HCombat, HProd, HOrders;

    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        HEnt.FeedBool(Core[I].bAlive);
        if (!Core[I].bAlive)
        {
            continue;
        }

        HEnt.FeedUInt32(I);
        HEnt.FeedUInt32(Core[I].Generation);
        HEnt.FeedUInt32(Core[I].Def.Value);
        HEnt.FeedUInt8(uint8_t(Core[I].Kind));
        HEnt.FeedUInt8(Core[I].Owner);

        HPos.FeedInt64(Transforms[I].Position.X.Raw);
        HPos.FeedInt64(Transforms[I].Position.Y.Raw);
        HPos.FeedInt32(Transforms[I].Facing);
        HPos.FeedInt32(Transforms[I].TurretFacing);
        HPos.FeedInt64(Movements[I].CurrentSpeed.Raw);
        HPos.FeedBool(Movements[I].bHasDestination);
        HPos.FeedInt64(Movements[I].Destination.X.Raw);
        HPos.FeedInt64(Movements[I].Destination.Y.Raw);

        HHealth.FeedInt32(Healths[I].Current);
        HHealth.FeedUInt8(uint8_t(Healths[I].Rank));
        HHealth.FeedInt32(Healths[I].DamageDealt);
        HHealth.FeedInt32(Healths[I].KillsValue);

        HCombat.FeedInt32(Combats[I].CooldownTicks);
        HCombat.FeedInt32(Combats[I].AcquireCooldownTicks);
        HCombat.FeedInt32(Combats[I].AmmoCurrent);
        HCombat.FeedInt32(Combats[I].AmmoMax);
        HCombat.FeedBool(AircraftRearming[I] != 0);
        HCombat.FeedUInt64(Combats[I].Target.Packed());
        HCombat.FeedBool(Combats[I].bSecondaryModeActive);
        HCombat.FeedInt32(Combats[I].SecondaryAbilityCooldownTicks);
        HCombat.FeedInt32(Combats[I].SecondaryAbilityDurationTicks);
        HCombat.FeedInt64(Morales[I].Morale.Raw);

        HCombat.FeedInt64(Morales[I].Fatigue.Raw);
        HCombat.FeedInt64(Morales[I].Suppression.Raw);
        HCombat.FeedInt32(Morales[I].TicksUnderFire);

        HOrders.FeedInt32(Orders[I].Count);
        HOrders.FeedUInt8(uint8_t(DirectControls[I].Phase));
        HOrders.FeedUInt8(DirectControls[I].Controller);
        HOrders.FeedInt32(DirectControls[I].TurretYawCentiDeg);
        HOrders.FeedInt32(DirectControls[I].TurretPitchCentiDeg);
        HOrders.FeedInt32(DirectControls[I].CooldownTicksPrimary);
        HOrders.FeedInt32(DirectControls[I].CooldownTicksSecondary);

        if (Core[I].Kind == EntityKind::Building)
        {
            HProd.FeedUInt8(uint8_t(Buildings[I].State));
            HProd.FeedUInt8(uint8_t(Buildings[I].Priority));
            HProd.FeedBool(Buildings[I].bRepairing);
            HProd.FeedInt32(Buildings[I].RepairCreditAccumulator);
            HProd.FeedInt32(Buildings[I].ConstructionProgressTicks);
            HProd.FeedInt32(int32_t(Buildings[I].Queue.size()));
            for (const ProductionItem& QueueItem : Buildings[I].Queue)
            {
                HProd.FeedUInt32(QueueItem.Content.Value);
                HProd.FeedInt32(QueueItem.ProgressTicks);
                HProd.FeedUInt8(uint8_t(QueueItem.State));
                HProd.FeedInt32(QueueItem.PaidCredits);
                HProd.FeedInt32(QueueItem.Priority);
                HProd.FeedBool(QueueItem.bPaused);
            }
        }
        else if (Core[I].Kind == EntityKind::Unit)
        {
            HEcon.FeedUInt8(uint8_t(Harvesters[I].State));
            HEcon.FeedInt32(Harvesters[I].Cargo);
        }
        else if (Core[I].Kind == EntityKind::ResourceNode)
        {
            HEcon.FeedInt32(ResourceNodes[I].Amount);
        }
        else if (Core[I].Kind == EntityKind::Projectile)
        {
            HCombat.FeedUInt64(Projectiles[I].Source.Packed());
            HCombat.FeedUInt64(Projectiles[I].Target.Packed());
            HCombat.FeedInt64(Projectiles[I].ImpactPoint.X.Raw);
            HCombat.FeedInt64(Projectiles[I].ImpactPoint.Y.Raw);
            HCombat.FeedUInt32(Projectiles[I].Weapon.Value);
            HCombat.FeedUInt8(Projectiles[I].OwnerPlayer);
            HCombat.FeedInt64(Projectiles[I].Speed.Raw);
        }
    }

    Breakdown.Economy = HEcon.Get();
    Breakdown.Entities = HEnt.Get();
    Breakdown.Positions = HPos.Get();
    Breakdown.Health = HHealth.Get();
    Breakdown.Combat = HCombat.Get();
    Breakdown.Production = HProd.Get();
    Breakdown.Orders = HOrders.Get();

    return Breakdown;
}

SimSnapshot SimWorld::CaptureSnapshot() const
{
    SimSnapshot Snapshot;
    Snapshot.Tick = CurrentTick;
    Snapshot.Checksum = ComputeStateChecksum();
    Snapshot.Breakdown = ComputeDetailedChecksum();

    ByteWriter Writer;
    Serialize(Writer);
    const auto& Buffer = Writer.GetBuffer();
    Snapshot.StateBuffer.assign(Buffer.begin(), Buffer.end());
    Snapshot.bValid = true;
    return Snapshot;
}

bool SimWorld::RestoreFromSnapshot(const SimSnapshot& Snapshot)
{
    if (!Snapshot.bValid || Snapshot.StateBuffer.empty())
    {
        return false;
    }

    ByteReader Reader(Snapshot.StateBuffer);
    const bool bSuccess = Deserialize(Reader, Content);
    if (bSuccess)
    {
        CurrentTick = Snapshot.Tick;
    }
    return bSuccess;
}

void SimWorld::RecordSnapshot()
{
    SnapshotHistory.Push(CaptureSnapshot());
}

// ---------------------------------------------------------------------------
// Serialization & Restoration
// ---------------------------------------------------------------------------


constexpr uint32_t kSimSaveMagic = 0x52413453u; // "RA4S"
constexpr uint32_t kSimSaveVersion = 13;
// v13: aircraft rearm latch. A partially reloaded magazine is still committed to
// the pad, so this bit affects future movement and firing and must survive saves.
constexpr uint32_t kSimSaveVersionAircraftRearm = 13;
// v13: per-player researched upgrades (RA3-style global research).
constexpr uint32_t kSimSaveVersionUpgrades = 13;
// v12: aircraft logistics -- magazines (CombatComp::AmmoMax/AmmoCurrent).
// Written unconditionally in Serialize next to the combat fields; older saves
// load with both at 0, which the behaviour reads as an infinite magazine, so
// pre-logistics saves migrate by construction with no conversion pass.
constexpr uint32_t kSimSaveVersionAmmo = 12;
// v11: tactical state -- status effects, transport bays and boarding,
// engineer capture channel progress.
constexpr uint32_t kSimSaveVersionStatus = 11;
// v10: veterancy (rank/damage dealt/kill value per entity) and harvester cargo
// provenance. Older saves migrate by leaving the defaults in place: everyone is a
// Recruit with no history, and legacy cargo prices at the standard ore field.
constexpr uint32_t kSimSaveVersionVeterancy = 10;
constexpr uint32_t kSimSaveVersionPowerTier = 5;
constexpr uint32_t kSimSaveVersionPowerPriority = 7;
constexpr uint32_t kSimSaveVersionRepair = 8;
constexpr uint32_t kSimSaveVersionProjectiles = 9;
constexpr uint32_t kSimSaveVersionMinSupported = 5;

void SimWorld::Serialize(ByteWriter& W) const
{
    W.WriteUInt32(kSimSaveMagic);
    W.WriteUInt32(kSimSaveVersion);
    W.WriteUInt32(CurrentTick);
    W.WriteUInt8(static_cast<uint8_t>(Phase));
    W.WriteUInt8(Winner);
    W.WriteUInt64(Rng.GetState());
    W.WriteUInt64(Rng.GetIncrement());
    W.WriteUInt64(ReconRng.GetState());
    W.WriteUInt64(ReconRng.GetIncrement());

    // Map description
    W.WriteString(Map.Name);
    W.WriteInt32(Map.Width);
    W.WriteInt32(Map.Height);
    W.WriteUInt32(static_cast<uint32_t>(Map.Tiles.size()));
    for (uint8_t Tile : Map.Tiles)
    {
        W.WriteUInt8(Tile);
    }

    // Players
    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        const PlayerState& S = Players[P];
        W.WriteBool(S.bActive);
        W.WriteBool(S.bDefeated);
        W.WriteUInt8(static_cast<uint8_t>(S.Faction));
        W.WriteInt32(S.Credits);
        W.WriteInt32(S.PowerProduced);
        W.WriteInt32(S.PowerConsumed);
        W.WriteInt32(S.UnitsBuilt);
        W.WriteInt32(S.BuildingsBuilt);
        W.WriteInt32(S.UnitsLost);
        W.WriteInt32(S.BuildingsLost);
        W.WriteInt32(S.TotalHarvested);
        // ADR-0013: the tier itself is derived, but the *remembered* tier is what makes
        // the deficit warning edge-triggered, so it has to survive a reload.
        W.WriteUInt8(static_cast<uint8_t>(S.LastPowerTier));
        W.WriteUInt32(static_cast<uint32_t>(S.CompletedBuildingTypes.size()));
        for (const ContentId& C : S.CompletedBuildingTypes)
        {
            W.WriteUInt32(C.Value);
        }
        // v13: researched upgrades.
        W.WriteUInt32(static_cast<uint32_t>(S.ResearchedUpgrades.size()));
        for (const ContentId& Up : S.ResearchedUpgrades)
        {
            W.WriteUInt32(Up.Value);
        }
    }

    // Free slots & High water mark
    W.WriteUInt32(HighWaterMark);
    W.WriteUInt32(static_cast<uint32_t>(FreeSlots.size()));
    for (uint32_t Slot : FreeSlots)
    {
        W.WriteUInt32(Slot);
    }

    // Entity arrays
    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        const EntityCore& C = Core[I];
        W.WriteBool(C.bAlive);
        W.WriteUInt32(C.Generation);
        W.WriteUInt32(C.Def.Value);
        W.WriteUInt8(static_cast<uint8_t>(C.Kind));
        W.WriteUInt8(C.Owner);

        const TransformComp& T = Transforms[I];
        W.WriteInt64(T.Position.X.Raw);
        W.WriteInt64(T.Position.Y.Raw);
        W.WriteInt32(T.Facing);
        W.WriteInt32(T.TurretFacing);

        const HealthComp& H = Healths[I];
        W.WriteInt32(H.Current);
        W.WriteInt32(H.Max);
        W.WriteBool(H.bInvulnerable);
        W.WriteUInt8(static_cast<uint8_t>(H.Rank));
        W.WriteInt32(H.DamageDealt);
        W.WriteInt32(H.KillsValue);
        // v11 status payload.
        const StatusComp& St = Statuses[I];
        W.WriteInt32(St.StunTicks);
        W.WriteInt32(St.FreezeTicks);
        W.WriteInt32(St.ShrinkTicks);
        W.WriteInt32(St.InfectionTicks);
        W.WriteInt32(St.InvulnerableTicks);
        W.WriteUInt64(PassengerOf[I].Transport.Packed());
        W.WriteUInt32(static_cast<uint32_t>(Transports[I].Passengers.size()));
        for (const EntityId& Rider : Transports[I].Passengers)
        {
            W.WriteUInt64(Rider.Packed());
        }
        W.WriteInt32(Captures[I].ProgressTicks);

        const MovementComp& M = Movements[I];
        W.WriteInt64(M.Destination.X.Raw);
        W.WriteInt64(M.Destination.Y.Raw);
        W.WriteBool(M.bHasDestination);
        W.WriteInt64(M.CurrentSpeed.Raw);
        W.WriteInt64(M.ArriveRadius.Raw);
        W.WriteInt32(M.BlockedTicks);

        const CombatComp& Cm = Combats[I];
        W.WriteUInt32(Cm.Target.Index);
        W.WriteUInt32(Cm.Target.Generation);
        W.WriteInt32(Cm.CooldownTicks);
        W.WriteInt32(Cm.AcquireCooldownTicks);
        // v12: aircraft magazine. Written unconditionally since v12; the reader
        // gates on kSimSaveVersionAmmo and keeps the 0/0 infinite default for
        // older saves. Read order matches this write order exactly.
        W.WriteInt32(Cm.AmmoMax);
        W.WriteInt32(Cm.AmmoCurrent);
        W.WriteBool(AircraftRearming[I] != 0);
        W.WriteBool(Cm.bTargetIsForced);

        const BuildingComp& B = Buildings[I];
        W.WriteInt32(B.OriginTile.X);
        W.WriteInt32(B.OriginTile.Y);
        W.WriteInt32(B.FootprintX);
        W.WriteInt32(B.FootprintY);
        W.WriteUInt8(static_cast<uint8_t>(B.State));
        W.WriteInt32(B.ConstructionProgressTicks);
        W.WriteInt32(B.ConstructionTotalTicks);
        W.WriteInt64(B.RallyPoint.X.Raw);
        W.WriteInt64(B.RallyPoint.Y.Raw);
        W.WriteBool(B.bHasRallyPoint);
        // ADR-0013: a player override, so it is authoritative state rather than
        // something re-derivable from content on load.
        W.WriteUInt8(static_cast<uint8_t>(B.Priority));
        // ADR-0013 repair: a sustained activity the player switched on, so it and its
        // part-paid credit both have to survive a reload.
        W.WriteBool(B.bRepairing);
        W.WriteInt32(B.RepairCreditAccumulator);
        W.WriteUInt32(static_cast<uint32_t>(B.Queue.size()));
        for (const ProductionItem& Item : B.Queue)
        {
            W.WriteUInt32(Item.Content.Value);
            W.WriteInt32(Item.ProgressTicks);
            W.WriteInt32(Item.TotalTicks);
            W.WriteInt32(Item.PaidCredits);
            W.WriteBool(Item.bPaused);
            // Save format v2 (ADR-0012). TotalCost is written even though it is
            // derivable, because a save must still load correctly after a content
            // balance pass changes a price: the player is mid-build at the old cost.
            W.WriteUInt8(static_cast<uint8_t>(Item.State));
            W.WriteInt32(Item.TotalCost);
            W.WriteInt32(Item.Priority);
        }

        const HarvesterComp& Hv = Harvesters[I];
        W.WriteUInt8(static_cast<uint8_t>(Hv.State));
        W.WriteInt32(Hv.Cargo);
        W.WriteUInt32(Hv.AssignedNode.Index);
        W.WriteUInt32(Hv.AssignedNode.Generation);
        W.WriteUInt32(Hv.AssignedRefinery.Index);
        W.WriteUInt32(Hv.AssignedRefinery.Generation);
        W.WriteUInt32(Hv.CargoDef.Value);

        const ResourceNodeComp& Rn = ResourceNodes[I];
        W.WriteInt32(Rn.Amount);
        W.WriteUInt32(Rn.Def.Value);

        const OrderQueue& Oq = Orders[I];
        W.WriteInt32(Oq.Count);
        for (int32_t K = 0; K < Oq.Count; ++K)
        {
            const Order& Ord = Oq.Orders[K];
            W.WriteUInt8(static_cast<uint8_t>(Ord.Type));
            W.WriteInt64(Ord.Location.X.Raw);
            W.WriteInt64(Ord.Location.Y.Raw);
            W.WriteUInt32(Ord.Target.Index);
            W.WriteUInt32(Ord.Target.Generation);
        }

        const DirectControlComp& Dc = DirectControls[I];
        W.WriteUInt8(static_cast<uint8_t>(Dc.Phase));
        W.WriteUInt8(Dc.Controller);
        W.WriteUInt32(uint32_t(Dc.PhaseUntilTick));
        W.WriteInt32(Dc.TurretYawCentiDeg);
        W.WriteInt32(Dc.TurretPitchCentiDeg);
        W.WriteInt32(Dc.CooldownTicksPrimary);
        W.WriteInt32(Dc.CooldownTicksSecondary);
        W.WriteUInt8(Dc.bOpticsZoomed ? 1 : 0);
        // v4: morale (M2) -- fear drives distortion rolls, so it saves and
        // hashes like any other future-state-shaping component.
        const Recon::MoraleComp& Mo = Morales[I];
        W.WriteInt64(Mo.Morale.Raw);
        W.WriteInt64(Mo.Fatigue.Raw);
        W.WriteInt64(Mo.Suppression.Raw);
        W.WriteInt32(Mo.TicksUnderFire);
        // v6: projectiles. Every other component array was written here; this one
        // was not, so a save taken while any shell was in flight silently dropped
        // it -- Deserialize resized Projectiles to the high-water mark and left it
        // default-constructed. The reloaded entity was still Kind::Projectile and
        // still alive, so SystemProjectiles found Weapon == ContentId() (invalid),
        // failed the FindWeapon lookup and destroyed it: damage already "spent" by
        // the firing unit's cooldown never arrived.
        const ProjectileComp& Pr = Projectiles[I];
        W.WriteUInt32(Pr.Source.Index);
        W.WriteUInt32(Pr.Source.Generation);
        W.WriteUInt32(Pr.Target.Index);
        W.WriteUInt32(Pr.Target.Generation);
        W.WriteInt64(Pr.ImpactPoint.X.Raw);
        W.WriteInt64(Pr.ImpactPoint.Y.Raw);
        W.WriteUInt32(Pr.Weapon.Value);
        W.WriteUInt8(Pr.OwnerPlayer);
        W.WriteInt64(Pr.Speed.Raw);
    }

    // Belief state (ADR-0026). Serialized with the match: a save/load cycle that
    // diverged GT from PS would be a critical bug, and this is what prevents it.
    ReconLayer.Serialize(W);
}

bool SimWorld::Deserialize(ByteReader& R, const ContentDatabase* InContent)
{
    if (R.ReadUInt32() != kSimSaveMagic)
    {
        return false;
    }
    const uint32_t Version = R.ReadUInt32();
    // A range, with each remaining field-level change gated on its own named constant
    // below. The lower bound is not 1: v4 and older were stamped by two branches on
    // incompatible byte layouts, and no reader can serve both, so they are refused
    // rather than loaded into a silently corrupt world.
    if (Version < kSimSaveVersionMinSupported || Version > kSimSaveVersion)
    {
        return false;
    }

    Reset();
    Content = InContent;

    CurrentTick = R.ReadUInt32();
    Phase = static_cast<MatchPhase>(R.ReadUInt8());
    Winner = R.ReadUInt8();
    const uint64_t RState = R.ReadUInt64();
    const uint64_t RInc = R.ReadUInt64();
    Rng.SetState(RState, RInc);
    // Present in every loadable version (the recon layer landed in v3 and the minimum
    // supported is v5), so no gate is needed.
    const uint64_t ReconState = R.ReadUInt64();
    const uint64_t ReconInc = R.ReadUInt64();
    ReconRng.SetState(ReconState, ReconInc);

    Map.Name = R.ReadString();
    Map.Width = R.ReadInt32();
    Map.Height = R.ReadInt32();
    const uint32_t TileCount = R.ReadUInt32();
    Map.Tiles.resize(TileCount);
    for (uint32_t I = 0; I < TileCount; ++I)
    {
        Map.Tiles[I] = R.ReadUInt8();
    }

    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        PlayerState& S = Players[P];
        S.bActive = R.ReadBool();
        S.bDefeated = R.ReadBool();
        S.Faction = static_cast<FactionId>(R.ReadUInt8());
        S.Credits = R.ReadInt32();
        S.PowerProduced = R.ReadInt32();
        S.PowerConsumed = R.ReadInt32();
        S.UnitsBuilt = R.ReadInt32();
        S.BuildingsBuilt = R.ReadInt32();
        S.UnitsLost = R.ReadInt32();
        S.BuildingsLost = R.ReadInt32();
        S.TotalHarvested = R.ReadInt32();
        if (Version >= kSimSaveVersionPowerTier)
        {
            S.LastPowerTier = static_cast<PowerTier>(R.ReadUInt8());
        }
        else
        {
            // Older saves have no memory of the tier. Deriving it from the power
            // figures just read is better than defaulting to Normal: that would
            // re-announce an ongoing deficit the player already knows about.
            S.LastPowerTier = S.GetPowerTier();
        }
        const uint32_t TechCount = R.ReadUInt32();
        S.CompletedBuildingTypes.resize(TechCount);
        for (uint32_t T = 0; T < TechCount; ++T)
        {
            S.CompletedBuildingTypes[T].Value = R.ReadUInt32();
        }
        if (Version >= kSimSaveVersionUpgrades)
        {
            const uint32_t UpCount = R.ReadUInt32();
            S.ResearchedUpgrades.resize(UpCount);
            for (uint32_t U = 0; U < UpCount; ++U)
            {
                S.ResearchedUpgrades[U].Value = R.ReadUInt32();
            }
        }
    }

    HighWaterMark = R.ReadUInt32();
    const uint32_t FreeCount = R.ReadUInt32();
    FreeSlots.resize(FreeCount);
    for (uint32_t I = 0; I < FreeCount; ++I)
    {
        FreeSlots[I] = R.ReadUInt32();
    }

    Core.resize(HighWaterMark);
    Transforms.resize(HighWaterMark);
    Healths.resize(HighWaterMark);
    Movements.resize(HighWaterMark);
    Combats.resize(HighWaterMark);
    AircraftRearming.resize(HighWaterMark);
    Morales.resize(HighWaterMark);
    Buildings.resize(HighWaterMark);
    Harvesters.resize(HighWaterMark);
    ResourceNodes.resize(HighWaterMark);
    Projectiles.resize(HighWaterMark);
    Orders.resize(HighWaterMark);
    DirectControls.resize(HighWaterMark);
    Statuses.resize(HighWaterMark);
    Transports.resize(HighWaterMark);
    PassengerOf.resize(HighWaterMark);
    Captures.resize(HighWaterMark);

    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        EntityCore& C = Core[I];
        C.bAlive = R.ReadBool();
        C.Generation = R.ReadUInt32();
        C.Def.Value = R.ReadUInt32();
        C.Kind = static_cast<EntityKind>(R.ReadUInt8());
        C.Owner = R.ReadUInt8();

        TransformComp& T = Transforms[I];
        T.Position.X.Raw = R.ReadInt64();
        T.Position.Y.Raw = R.ReadInt64();
        T.Facing = R.ReadInt32();
        T.TurretFacing = R.ReadInt32();

        HealthComp& H = Healths[I];
        H.Current = R.ReadInt32();
        H.Max = R.ReadInt32();
        H.bInvulnerable = R.ReadBool();
        if (Version >= kSimSaveVersionVeterancy)
        {
            H.Rank = static_cast<VeterancyRank>(R.ReadUInt8());
            H.DamageDealt = R.ReadInt32();
            H.KillsValue = R.ReadInt32();
        }
        if (Version >= kSimSaveVersionStatus)
        {
            StatusComp& St = Statuses[I];
            St.StunTicks = R.ReadInt32();
            St.FreezeTicks = R.ReadInt32();
            St.ShrinkTicks = R.ReadInt32();
            St.InfectionTicks = R.ReadInt32();
            St.InvulnerableTicks = R.ReadInt32();
            PassengerOf[I].Transport.Packed(R.ReadUInt64());
            const uint32_t RiderCount = R.ReadUInt32();
            Transports[I].Passengers.clear();
            for (uint32_t Rd = 0; Rd < RiderCount; ++Rd)
            {
                EntityId Rider;
                Rider.Packed(R.ReadUInt64());
                Transports[I].Passengers.push_back(Rider);
            }
            Captures[I].ProgressTicks = R.ReadInt32();
        }

        MovementComp& M = Movements[I];
        M.Destination.X.Raw = R.ReadInt64();
        M.Destination.Y.Raw = R.ReadInt64();
        M.bHasDestination = R.ReadBool();
        M.CurrentSpeed.Raw = R.ReadInt64();
        M.ArriveRadius.Raw = R.ReadInt64();
        M.BlockedTicks = R.ReadInt32();

        CombatComp& Cm = Combats[I];
        Cm.Target.Index = R.ReadUInt32();
        Cm.Target.Generation = R.ReadUInt32();
        Cm.CooldownTicks = R.ReadInt32();
        // v5 added the acquisition retry countdown. A v4 save has no such field, and
        // defaulting it to 0 is the correct migration: it means "may search this
        // tick", which is exactly the behaviour v4 had unconditionally.
        Cm.AcquireCooldownTicks = (Version >= 5) ? R.ReadInt32() : 0;
        // v12: aircraft magazine. Mirrors the unconditional Serialize write above,
        // field for field. Older saves keep the resize() defaults (0/0 == infinite
        // magazine), which is exactly how those matches behaved before logistics.
        if (Version >= kSimSaveVersionAmmo)
        {
            Cm.AmmoMax = R.ReadInt32();
            Cm.AmmoCurrent = R.ReadInt32();
        }
        if (Version >= kSimSaveVersionAircraftRearm)
        {
            AircraftRearming[I] = R.ReadBool() ? 1 : 0;
        }
        Cm.bTargetIsForced = R.ReadBool();

        BuildingComp& B = Buildings[I];
        B.OriginTile.X = R.ReadInt32();
        B.OriginTile.Y = R.ReadInt32();
        B.FootprintX = R.ReadInt32();
        B.FootprintY = R.ReadInt32();
        B.State = static_cast<ConstructionState>(R.ReadUInt8());
        B.ConstructionProgressTicks = R.ReadInt32();
        B.ConstructionTotalTicks = R.ReadInt32();
        B.RallyPoint.X.Raw = R.ReadInt64();
        B.RallyPoint.Y.Raw = R.ReadInt64();
        B.bHasRallyPoint = R.ReadBool();
        if (Version >= kSimSaveVersionPowerPriority)
        {
            B.Priority = static_cast<PowerPriority>(R.ReadUInt8());
        }
        else
        {
            // Older saves have no priority byte, so it has to be derived -- and it must
            // be derived from the definition, not left at the BuildingComp default.
            // That default is Production, which goes offline at Critical: a v6 save
            // would have loaded with its construction yard and barracks in a band that
            // stops them exactly where ADR-0013's carve-out says they must keep
            // working, recreating the inescapable blackout this package exists to fix.
            // Core[I].Def was read earlier in this same loop, so the definition is
            // available here.
            const EntityDef* Def = Content ? Content->FindEntity(Core[I].Def) : nullptr;
            B.Priority = Def != nullptr ? DefaultPowerPriorityFor(*Def)
                                        : PowerPriority::Production;
        }
        if (Version >= kSimSaveVersionRepair)
        {
            B.bRepairing = R.ReadBool();
            B.RepairCreditAccumulator = R.ReadInt32();
        }
        // Older saves predate repair entirely, so the resize() default of "not
        // repairing" is exactly right -- nothing to reconstruct.
        const uint32_t QueueCount = R.ReadUInt32();
        B.Queue.resize(QueueCount);
        for (uint32_t Q = 0; Q < QueueCount; ++Q)
        {
            ProductionItem& Item = B.Queue[Q];
            Item.Content.Value = R.ReadUInt32();
            Item.ProgressTicks = R.ReadInt32();
            Item.TotalTicks = R.ReadInt32();
            Item.PaidCredits = R.ReadInt32();
            Item.bPaused = R.ReadBool();
            Item.State = static_cast<FlowPaymentState>(R.ReadUInt8());
            Item.TotalCost = R.ReadInt32();
            Item.Priority = R.ReadInt32();
        }

        HarvesterComp& Hv = Harvesters[I];
        Hv.State = static_cast<HarvesterState>(R.ReadUInt8());
        Hv.Cargo = R.ReadInt32();
        Hv.AssignedNode.Index = R.ReadUInt32();
        Hv.AssignedNode.Generation = R.ReadUInt32();
        Hv.AssignedRefinery.Index = R.ReadUInt32();
        Hv.AssignedRefinery.Generation = R.ReadUInt32();
        if (Version >= kSimSaveVersionVeterancy)
        {
            Hv.CargoDef.Value = R.ReadUInt32();
        }

        ResourceNodeComp& Rn = ResourceNodes[I];
        Rn.Amount = R.ReadInt32();
        Rn.Def.Value = R.ReadUInt32();

        OrderQueue& Oq = Orders[I];
        Oq.Count = R.ReadInt32();
        for (int32_t K = 0; K < Oq.Count; ++K)
        {
            Order& Ord = Oq.Orders[K];
            Ord.Type = static_cast<OrderType>(R.ReadUInt8());
            Ord.Location.X.Raw = R.ReadInt64();
            Ord.Location.Y.Raw = R.ReadInt64();
            Ord.Target.Index = R.ReadUInt32();
            Ord.Target.Generation = R.ReadUInt32();
        }

        DirectControlComp& Dc = DirectControls[I];
        Dc.Phase = static_cast<DirectControlPhase>(R.ReadUInt8());
        Dc.Controller = R.ReadUInt8();
        Dc.PhaseUntilTick = static_cast<TickIndex>(R.ReadUInt32());
        Dc.TurretYawCentiDeg = R.ReadInt32();
        Dc.TurretPitchCentiDeg = R.ReadInt32();
        Dc.CooldownTicksPrimary = R.ReadInt32();
        Dc.CooldownTicksSecondary = R.ReadInt32();
        Dc.bOpticsZoomed = (R.ReadUInt8() != 0);
        Recon::MoraleComp& Mo = Morales[I];
        Mo.Morale = Fixed::FromRaw(R.ReadInt64());
        Mo.Fatigue = Fixed::FromRaw(R.ReadInt64());
        Mo.Suppression = Fixed::FromRaw(R.ReadInt64());
        Mo.TicksUnderFire = R.ReadInt32();
        if (Version >= kSimSaveVersionProjectiles)
        {
            ProjectileComp& Pr = Projectiles[I];
            Pr.Source.Index = R.ReadUInt32();
            Pr.Source.Generation = R.ReadUInt32();
            Pr.Target.Index = R.ReadUInt32();
            Pr.Target.Generation = R.ReadUInt32();
            Pr.ImpactPoint.X.Raw = R.ReadInt64();
            Pr.ImpactPoint.Y.Raw = R.ReadInt64();
            Pr.Weapon.Value = R.ReadUInt32();
            Pr.OwnerPlayer = R.ReadUInt8();
            Pr.Speed.Raw = R.ReadInt64();
        }
    }

    BuildNavigationGrid();
    Reservations = std::make_unique<Nav::ReservationGrid>(Map.Width, Map.Height);
    Router = std::make_unique<Nav::MNavRouter>(*NavigationGrid);
    FogGrid = std::make_unique<FFogOfWarGrid>(Map.Width, Map.Height, kMaxPlayers);

    for (PlayerId P = 0; P < kMaxPlayers; ++P)
    {
        if (Players[P].bActive)
        {
            RefreshPlayerTech(P);
        }
    }

    // Reset() cleared the intel layer; re-arm it with the settings this session
    // was initialized with before reading the belief payload. Enabled-ness must
    // match the save or ReconSystem::Deserialize refuses (see its comment).
    ReconLayer.Initialize(ReconSettingsRef, Map.Width, Map.Height);
    {
        // Present in every loadable version, so no gate and no pre-recon fallback:
        // a save old enough to lack a belief payload is already refused up front.
        if (!ReconLayer.Deserialize(R))
        {
            return false;
        }
    }

    // Prime fog of war visibility for the current state so systems running on the
    // next tick (such as SystemCombat/AcquireTarget) see the correct visibility state.
    SystemFogOfWar();

    return !R.HasError();
}

} // namespace RA4
