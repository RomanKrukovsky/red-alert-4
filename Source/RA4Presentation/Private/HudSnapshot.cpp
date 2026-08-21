// Copyright (c) Red Alert 4 project.
#include "RA4Presentation/HudSnapshot.h"

#include "RA4Core/SimConfig.h"

#include <algorithm>

namespace RA4
{
namespace Presentation
{

const char* ToString(AlertType Type)
{
    switch (Type)
    {
        case AlertType::None: return "None";
        case AlertType::LowPower: return "LowPower";
        case AlertType::InsufficientFunds: return "InsufficientFunds";
        case AlertType::BaseUnderAttack: return "BaseUnderAttack";
        case AlertType::UnitsUnderAttack: return "UnitsUnderAttack";
        case AlertType::BuildingLost: return "BuildingLost";
        case AlertType::UnitLost: return "UnitLost";
        case AlertType::ConstructionComplete: return "ConstructionComplete";
        case AlertType::UnitReady: return "UnitReady";
        case AlertType::ResourcesDepleted: return "ResourcesDepleted";
        case AlertType::MCVDeployed: return "MCVDeployed";
        case AlertType::MCVUndeployed: return "MCVUndeployed";
        default: return "Unknown";
    }
}

const char* ToString(BuildBlockReason Reason)
{
    switch (Reason)
    {
        case BuildBlockReason::None: return "None";
        case BuildBlockReason::MissingPrerequisite: return "MissingPrerequisite";
        case BuildBlockReason::InsufficientCredits: return "InsufficientCredits";
        case BuildBlockReason::NoProducer: return "NoProducer";
        case BuildBlockReason::QueueFull: return "QueueFull";
        case BuildBlockReason::MatchOver: return "MatchOver";
        default: return "Unknown";
    }
}

void HudSnapshotBuilder::Initialize(PlayerId InLocalPlayer)
{
    LocalPlayer = InLocalPlayer;
    Reset();
}

void HudSnapshotBuilder::Reset()
{
    PreviousCredits = 0;
    bHasPreviousCredits = false;
    ActiveAlerts.clear();
    CachedBackground = MinimapBackground();
    BackgroundRevision = 0;
    bHasSampledBackground = false;
    // A reset means the consumer's cached copy is stale, so the next snapshot must carry a
    // full grid rather than waiting for the map to change.
    bBackgroundResendRequested = true;
    LastBackgroundTick = 0;
}

// ---------------------------------------------------------------------------
// Resources
// ---------------------------------------------------------------------------

void HudSnapshotBuilder::BuildResources(const SimWorld& World, ResourceState& Out)
{
    const PlayerState& Player = World.GetPlayer(LocalPlayer);

    Out.Credits = Player.Credits;
    // First snapshot has no previous value; reporting the whole starting balance as
    // income would make the HUD flash a huge gain on the first frame of a match.
    Out.CreditsDelta = bHasPreviousCredits ? (Player.Credits - PreviousCredits) : 0;
    PreviousCredits = Player.Credits;
    bHasPreviousCredits = true;

    Out.PowerProduced = Player.PowerProduced;
    Out.PowerConsumed = Player.PowerConsumed;
    Out.PowerRatioPercent = Player.GetPowerRatioPercent();
    Out.bPowerShortage = Out.PowerRatioPercent < 100;

    // Supply is shown in the reference HUD but not modelled by the simulation. Count
    // the units so the field is honest, and leave the cap at zero with the flag off
    // so the widget hides the counter instead of inventing a limit.
    int32_t UnitCount = 0;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (Cores[I].bAlive && Cores[I].Owner == LocalPlayer && Cores[I].Kind == EntityKind::Unit)
        {
            ++UnitCount;
        }
    }
    Out.SupplyUsed = UnitCount;
    Out.SupplyCap = 0;
    Out.bSupplyModelled = false;
}

// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------

void HudSnapshotBuilder::BuildSelection(const SimWorld& World, const std::vector<EntityId>& Selection,
                                        SelectionState& Out) const
{
    Out = SelectionState();
    if (Selection.empty() || World.GetContent() == nullptr)
    {
        return;
    }

    int32_t UnitCount = 0;
    int32_t BuildingCount = 0;

    for (const EntityId& Id : Selection)
    {
        const EntityCore* Core = World.GetCore(Id);
        if (Core == nullptr)
        {
            continue;   // died between selection prune and snapshot
        }
        ++Out.TotalCount;

        if (Core->Kind == EntityKind::Unit) { ++UnitCount; }
        else if (Core->Kind == EntityKind::Building) { ++BuildingCount; }

        // Group by content type so the grid shows "6 x T-34", not six identical
        // tiles, which is what the reference HUD does.
        auto Existing = std::find_if(Out.Groups.begin(), Out.Groups.end(),
                                     [Core](const SelectionGroup& G) { return G.Content == Core->Def; });
        const HealthComp* Health = World.GetHealth(Id);
        if (Existing == Out.Groups.end())
        {
            SelectionGroup Group;
            Group.Content = Core->Def;
            Group.Count = 1;
            Group.Representative = Id;
            Group.HealthCurrent = Health != nullptr ? Health->Current : 0;
            Group.HealthMax = Health != nullptr ? Health->Max : 0;
            Out.Groups.push_back(Group);
        }
        else
        {
            Existing->Count += 1;
            if (Health != nullptr)
            {
                Existing->HealthCurrent += Health->Current;
                Existing->HealthMax += Health->Max;
            }
        }

        // A producer in the selection takes over the production panel.
        if (Core->Kind == EntityKind::Building && !Out.ProductionSource.IsValid())
        {
            const BuildingComp* Building = World.GetBuilding(Id);
            if (Building != nullptr && Building->State == ConstructionState::Complete)
            {
                Out.ProductionSource = Id;
            }
        }
    }

    if (Out.TotalCount == 0)
    {
        Out.Kind = SelectionKind::Empty;
        return;
    }

    if (UnitCount > 0 && BuildingCount > 0) { Out.Kind = SelectionKind::Mixed; }
    else if (BuildingCount > 0) { Out.Kind = SelectionKind::SingleBuilding; }
    else { Out.Kind = UnitCount == 1 ? SelectionKind::SingleUnit : SelectionKind::MultipleUnits; }

    // The largest group drives the portrait, so a stack of tanks with one stray
    // engineer shows the tank rather than whichever entity was clicked first.
    const auto Largest = std::max_element(Out.Groups.begin(), Out.Groups.end(),
                                          [](const SelectionGroup& A, const SelectionGroup& B)
                                          { return A.Count < B.Count; });
    if (Largest != Out.Groups.end())
    {
        Out.Primary = Largest->Representative;
        Out.PrimaryContent = Largest->Content;

        const EntityCore* Core = World.GetCore(Out.Primary);
        const HealthComp* Health = World.GetHealth(Out.Primary);
        if (Core != nullptr)
        {
            Out.bPrimaryIsOwned = Core->Owner == LocalPlayer;
            if (const EntityDef* Def = World.GetContent()->FindEntity(Core->Def))
            {
                Out.PrimaryDisplayNameKey = Def->DisplayNameKey;
            }
        }
        if (Health != nullptr)
        {
            Out.PrimaryHealthCurrent = Health->Current;
            Out.PrimaryHealthMax = Health->Max;
        }

        // ADR-0013 building controls. Only meaningful for a building the local player
        // owns: the card offers a priority cycle and a repair toggle, and both are
        // commands, so the UI needs to know the current state to render them honestly.
        if (const BuildingComp* Building = World.GetBuilding(Out.Primary))
        {
            Out.bPrimaryIsBuilding = true;
            Out.PrimaryPowerPriority = Building->Priority;
            Out.bPrimaryIsRepairing = Building->bRepairing;

            if (Core != nullptr && Out.bPrimaryIsOwned)
            {
                const PowerTier Tier = World.GetPlayer(Core->Owner).GetPowerTier();
                Out.bPrimaryPowerOffline = IsPowerPriorityOffline(Building->Priority, Tier);
            }
            // Repair is offerable only on a finished, damaged building -- a half-built
            // one already gains health from construction, and paying twice for the same
            // hitpoints would be a bug the UI invited.
            Out.bPrimaryCanRepair = Out.bPrimaryIsOwned &&
                                    Building->State == ConstructionState::Complete &&
                                    Health != nullptr && Health->Current < Health->Max;
        }
    }
}

// ---------------------------------------------------------------------------
// Production
// ---------------------------------------------------------------------------

void HudSnapshotBuilder::BuildProduction(const SimWorld& World, const SelectionState& Selection,
                                         ProductionState& Out) const
{
    Out = ProductionState();
    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return;
    }

    const PlayerState& Player = World.GetPlayer(LocalPlayer);
    const bool bMatchOver = World.GetPhase() != MatchPhase::Running;

    // Index the player's completed buildings once; every option below needs to know
    // which producers exist, and scanning per option would be quadratic.
    std::vector<EntityId> CompletedBuildings;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Owner != LocalPlayer || Cores[I].Kind != EntityKind::Building)
        {
            continue;
        }
        const EntityId Id = World.MakeId(I);
        const BuildingComp* Building = World.GetBuilding(Id);
        if (Building != nullptr && Building->State == ConstructionState::Complete)
        {
            CompletedBuildings.push_back(Id);
        }
    }

    // The queue shown is the selected producer's, falling back to the first owned
    // producer that has anything queued -- so the panel is not blank just because
    // the player deselected their factory.
    EntityId QueueSource = Selection.ProductionSource;
    if (!QueueSource.IsValid())
    {
        for (const EntityId& Id : CompletedBuildings)
        {
            const BuildingComp* Building = World.GetBuilding(Id);
            if (Building != nullptr && !Building->Queue.empty())
            {
                QueueSource = Id;
                break;
            }
        }
    }
    Out.Producer = QueueSource;

    if (QueueSource.IsValid())
    {
        if (const BuildingComp* Building = World.GetBuilding(QueueSource))
        {
            for (size_t Slot = 0; Slot < Building->Queue.size(); ++Slot)
            {
                const ProductionItem& Item = Building->Queue[Slot];
                const int32_t Total = std::max(1, Item.TotalTicks) * kProductionProgressScale;
                const int32_t Clamped = std::min(Item.ProgressTicks, Total);

                QueueEntry Entry;
                Entry.Content = Item.Content;
                Entry.SlotIndex = int32_t(Slot);
                Entry.bPaused = Item.bPaused;
                Entry.PaymentState = Item.State;
                Entry.PaidCredits = Item.PaidCredits;
                Entry.TotalCost = Item.TotalCost;
                Entry.bStarvedForCredits = Item.State == FlowPaymentState::Starved;
                Entry.ProgressPercent = int32_t((int64_t(Clamped) * 100) / Total);
                Entry.RemainingTicks = (Total - Clamped) / kProductionProgressScale;

                if (const EntityDef* Def = Content->FindEntity(Item.Content))
                {
                    Entry.DisplayNameKey = Def->DisplayNameKey;
                    // A finished structure sits in the queue until the player picks a
                    // spot. The card must say "place me", not "still building".
                    Entry.bAwaitingPlacement = Def->Kind == EntityKind::Building && Clamped >= Total;
                }
                Out.Queue.push_back(Entry);
            }
        }
    }

    // Every definition the local faction could ever build, with the reason it cannot
    // be built right now. The reference HUD greys a card rather than hiding it, so
    // the reason has to be part of the data, not a lookup the widget improvises.
    for (const EntityDef& Def : Content->GetEntities())
    {
        if (Def.Production.ProducedBy.empty())
        {
            continue;   // not player-buildable
        }
        if (Def.Faction != FactionId::None && Def.Faction != Player.Faction)
        {
            continue;
        }

        BuildOption Option;
        Option.Content = Def.Id;
        Option.DisplayNameKey = Def.DisplayNameKey;
        Option.Category = Def.Production.Category;
        Option.Cost = Def.Production.Cost;
        Option.BuildTimeTicks = Def.Production.BuildTimeTicks;
        if (Def.Kind == EntityKind::Building)
        {
            Option.PowerDelta = Def.Building.PowerProduced - Def.Building.PowerConsumed;
        }
        if (!Def.Production.Prerequisites.empty())
        {
            if (const EntityDef* PrereqDef = Content->FindEntity(Def.Production.Prerequisites[0]))
            {
                Option.PrerequisiteKey = PrereqDef->DisplayNameKey;
            }
        }

        for (const EntityId& Id : CompletedBuildings)
        {
            const EntityCore* Core = World.GetCore(Id);
            if (Core == nullptr)
            {
                continue;
            }
            if (std::find(Def.Production.ProducedBy.begin(), Def.Production.ProducedBy.end(), Core->Def) !=
                Def.Production.ProducedBy.end())
            {
                Option.Producer = Id;
                break;
            }
        }

        // Reasons are reported in the order the player can act on them: tech first,
        // because no amount of money fixes a missing war factory.
        //
        // Affordability is deliberately NOT a block reason. Under ADR-0012 the
        // simulation accepts an order the player cannot yet pay for and funds it a
        // slice per tick, so greying the card out here would forbid a command the
        // simulation would happily take -- and make gradual payment unreachable
        // through the actual UI. The card stays clickable; the queue entry then
        // reports Starved so the player can see the money, not the button, is the
        // constraint.
        if (bMatchOver) { Option.BlockReason = BuildBlockReason::MatchOver; }
        else if (!World.HasPrerequisites(LocalPlayer, Def)) { Option.BlockReason = BuildBlockReason::MissingPrerequisite; }
        else if (!Option.Producer.IsValid()) { Option.BlockReason = BuildBlockReason::NoProducer; }
        else
        {
            const BuildingComp* Building = World.GetBuilding(Option.Producer);
            if (Building != nullptr && int32_t(Building->Queue.size()) >= kMaxProductionQueueLength)
            {
                Option.BlockReason = BuildBlockReason::QueueFull;
            }
            else
            {
                Option.BlockReason = BuildBlockReason::None;
                Option.bAvailable = true;
            }
        }

        Out.Options.push_back(Option);
    }

    // Sorted by category then cost: the panel's tab order is category, and within a
    // tab the cheapest item first matches the reference layout.
    std::stable_sort(Out.Options.begin(), Out.Options.end(),
                     [](const BuildOption& A, const BuildOption& B)
                     {
                         if (A.Category != B.Category)
                         {
                             return uint8_t(A.Category) < uint8_t(B.Category);
                         }
                         if (A.Cost != B.Cost)
                         {
                             return A.Cost < B.Cost;
                         }
                         return A.Content.Value < B.Content.Value;
                     });
}

// ---------------------------------------------------------------------------
// Radar
// ---------------------------------------------------------------------------

void ComputeMinimapRect(double PanelWidth, double PanelHeight,
                        double MapWidth, double MapHeight,
                        double& OutOffsetX, double& OutOffsetY,
                        double& OutWidth, double& OutHeight)
{
    OutOffsetX = 0.0;
    OutOffsetY = 0.0;
    OutWidth = PanelWidth;
    OutHeight = PanelHeight;
    if (PanelWidth <= 0.0 || PanelHeight <= 0.0 || MapWidth <= 0.0 || MapHeight <= 0.0)
    {
        return;
    }

    const double PanelAspect = PanelWidth / PanelHeight;
    const double MapAspect = MapWidth / MapHeight;
    if (MapAspect > PanelAspect)
    {
        // Wider than the panel: full width, bars above and below.
        OutWidth = PanelWidth;
        OutHeight = PanelWidth / MapAspect;
    }
    else
    {
        OutHeight = PanelHeight;
        OutWidth = PanelHeight * MapAspect;
    }
    OutOffsetX = (PanelWidth - OutWidth) * 0.5;
    OutOffsetY = (PanelHeight - OutHeight) * 0.5;
}

bool ComputeMinimapCameraFrame(
    double MapRectOffsetX, double MapRectOffsetY, double MapRectWidth, double MapRectHeight,
    double MapWorldWidth, double MapWorldHeight,
    double ViewCentreX, double ViewCentreY, double ViewExtentX, double ViewExtentY,
    double& OutLeft, double& OutTop, double& OutRight, double& OutBottom)
{
    OutLeft = OutTop = OutRight = OutBottom = 0.0;
    // A zero extent means the camera footprint is unknown -- a near-horizon corner that
    // missed the ground plane, or no camera yet. Drawing a degenerate frame would put a
    // stray line in the corner of the panel and imply the player is looking there.
    if (ViewExtentX <= 0.0 || ViewExtentY <= 0.0 ||
        MapWorldWidth <= 0.0 || MapWorldHeight <= 0.0 ||
        MapRectWidth <= 0.0 || MapRectHeight <= 0.0)
    {
        return false;
    }

    const auto Clamp01 = [](double V) { return V < 0.0 ? 0.0 : (V > 1.0 ? 1.0 : V); };

    // Clamped per edge, so a camera looking past the map boundary yields a frame flush with
    // that boundary rather than one drawn outside the widget.
    const double FracLeft = Clamp01((ViewCentreX - ViewExtentX * 0.5) / MapWorldWidth);
    const double FracRight = Clamp01((ViewCentreX + ViewExtentX * 0.5) / MapWorldWidth);
    const double FracSouth = Clamp01((ViewCentreY - ViewExtentY * 0.5) / MapWorldHeight);
    const double FracNorth = Clamp01((ViewCentreY + ViewExtentY * 0.5) / MapWorldHeight);

    OutLeft = MapRectOffsetX + FracLeft * MapRectWidth;
    OutRight = MapRectOffsetX + FracRight * MapRectWidth;
    // Y flip: the northern edge of the footprint is the top of the panel.
    OutTop = MapRectOffsetY + (1.0 - FracNorth) * MapRectHeight;
    OutBottom = MapRectOffsetY + (1.0 - FracSouth) * MapRectHeight;

    // A camera entirely off the map clamps every corner to the same bound, so the rect has
    // zero area. Guarding only the *input* extent was not enough: the contract above promises
    // false when there is nothing to draw, and returning true here put a degenerate line
    // against the map edge, implying the player was looking there when they were not.
    //
    // Found by an independent reviewer's probe, which reported ok=1 with w=0 h=0.
    if (OutRight <= OutLeft || OutBottom <= OutTop)
    {
        OutLeft = OutTop = OutRight = OutBottom = 0.0;
        return false;
    }
    return true;
}

void ComputeMinimapCellGrid(int32_t TileWidth, int32_t TileHeight,
                            int32_t& OutCellsX, int32_t& OutCellsY,
                            int32_t& OutStrideX, int32_t& OutStrideY)
{
    OutCellsX = 0;
    OutCellsY = 0;
    OutStrideX = 1;
    OutStrideY = 1;
    if (TileWidth <= 0 || TileHeight <= 0)
    {
        return;
    }

    // Ceiling division, so a map that does not divide evenly gets one extra partly-filled
    // cell rather than silently losing its last strip of tiles.
    OutStrideX = (TileWidth + kMinimapMaxCellsPerAxis - 1) / kMinimapMaxCellsPerAxis;
    OutStrideY = (TileHeight + kMinimapMaxCellsPerAxis - 1) / kMinimapMaxCellsPerAxis;
    if (OutStrideX < 1) { OutStrideX = 1; }
    if (OutStrideY < 1) { OutStrideY = 1; }

    OutCellsX = (TileWidth + OutStrideX - 1) / OutStrideX;
    OutCellsY = (TileHeight + OutStrideY - 1) / OutStrideY;
}

namespace
{

// Which terrain wins when one minimap cell covers several tiles. A single cell can be up
// to a few tiles across on a large map, and averaging them would turn a one-tile-wide
// river into nothing. Ranked by how much a player needs to see it: an ore patch or a cliff
// edge is a decision, plain ground is not.
int32_t TerrainSalience(MinimapTerrain Terrain)
{
    switch (Terrain)
    {
        case MinimapTerrain::Structure: return 5;
        case MinimapTerrain::Ore:       return 4;
        case MinimapTerrain::Cliff:     return 3;
        case MinimapTerrain::Water:     return 2;
        case MinimapTerrain::Ground:    return 1;
        case MinimapTerrain::Unknown:   return 0;
    }
    return 0;
}

MinimapTerrain TerrainForTileFlags(uint8_t Flags)
{
    // Ore is checked before water and cliff because an ore patch is the one thing on the
    // minimap the player is actively looking for; a tile flagged both is still worth
    // showing as ore.
    if ((Flags & Tile_Resource) != 0)  { return MinimapTerrain::Ore; }
    if ((Flags & Tile_Occupied) != 0)  { return MinimapTerrain::Structure; }
    if ((Flags & Tile_Water) != 0)     { return MinimapTerrain::Water; }
    if ((Flags & Tile_Cliff) != 0)     { return MinimapTerrain::Cliff; }
    return MinimapTerrain::Ground;
}

// How brightly a cell is lit. A radar blip counts, so a live contact does not sit on a dimmed
// cell -- the player is genuinely being told something is there right now.
MinimapShroud ShroudForVisibility(VisibilityState Visibility)
{
    switch (Visibility)
    {
        case VisibilityState::CurrentlyVisible:
        case VisibilityState::RadarDetected:
            return MinimapShroud::Visible;
        case VisibilityState::PreviouslySeen:
            return MinimapShroud::Remembered;
        case VisibilityState::NeverSeen:
            break;
    }
    return MinimapShroud::NeverSeen;
}

// Whether the player has learned what the *ground* looks like here. Deliberately not the same
// question as the shroud: a radar blip says something is moving there, not that anyone has seen
// the terrain under it.
//
// Answering with the shroud was a maphack. A radar swept 24 tiles of unexplored map and the
// background dutifully reported every water tile, every cliff and -- worst -- every ore patch
// inside it. Ore is the one thing on the minimap a player actively hunts for, so this handed
// out the single most valuable piece of scouting information for the price of a radar. Found by
// an independent reviewer; my own comment three lines up had already said a blip is not
// knowledge of the ground, and the code did not honour it.
bool HasLearnedTerrain(VisibilityState Visibility)
{
    return Visibility == VisibilityState::CurrentlyVisible ||
           Visibility == VisibilityState::PreviouslySeen;
}

} // namespace

bool RadarPingKindForAlert(AlertType Type, RadarPingKind& OutKind)
{
    switch (Type)
    {
        case AlertType::BaseUnderAttack:
        case AlertType::UnitsUnderAttack:
            OutKind = RadarPingKind::Attack;
            return true;
        case AlertType::BuildingLost:
        case AlertType::UnitLost:
            OutKind = RadarPingKind::Loss;
            return true;
        case AlertType::ConstructionComplete:
        case AlertType::UnitReady:
        case AlertType::MCVDeployed:
        case AlertType::MCVUndeployed:
            OutKind = RadarPingKind::Construction;
            return true;

        // Conditions rather than places. Pinging the map for "low power" would put a marker
        // somewhere arbitrary and teach the player that pings mean nothing.
        case AlertType::LowPower:
        case AlertType::InsufficientFunds:
        case AlertType::ResourcesDepleted:
        case AlertType::None:
            break;
    }
    return false;
}

int32_t RadarPingIntensityPercent(TickIndex RaisedTick, TickIndex Now, int32_t LifetimeTicks)
{
    if (LifetimeTicks <= 0)
    {
        return 0;
    }
    // A tick in the future means the caller mixed up its ordering; treat it as brand new
    // rather than returning a value above full, which would scale a marker past its cell.
    if (Now <= RaisedTick)
    {
        return 100;
    }
    const TickIndex Age = Now - RaisedTick;
    if (Age >= TickIndex(LifetimeTicks))
    {
        return 0;
    }
    // Linear: a ping is at its most visible the instant it appears and fades evenly. Integer
    // arithmetic throughout, so this cannot drift between platforms.
    return int32_t(100 - (int64_t(Age) * 100) / int64_t(LifetimeTicks));
}

void HudSnapshotBuilder::BuildMinimapBackground(const SimWorld& World, RadarState& Out)
{
    Out.bBackgroundChanged = false;
    Out.BackgroundRevision = BackgroundRevision;

    // The payload is attached only on the ticks it changed, plus whenever a consumer has asked
    // to be re-sent it. An earlier version copied it unconditionally so that a consumer created
    // mid-match would see a complete map -- but that made every tick carry the full grid, which
    // is 14792 bytes on a 256x256 map, ~296 KB/s at 20 Hz, to say "identical". An independent
    // reviewer measured exactly that and was right to call the claimed saving false.
    //
    // RequestBackgroundResend covers the case the unconditional copy was there for, without
    // charging every tick for it.
    if (bBackgroundResendRequested && bHasSampledBackground)
    {
        Out.Background = CachedBackground;
        Out.bBackgroundChanged = true;
        bBackgroundResendRequested = false;
    }

    const MapDescription& Map = World.GetMap();
    const TickIndex Tick = World.GetTick();

    // Re-sample on the first call and then only on the interval. Between those ticks the
    // cached copy above is handed out unchanged, and bBackgroundChanged stays false so the
    // UI knows it need not re-upload its texture.
    const bool bDue = !bHasSampledBackground ||
                      (Tick - LastBackgroundTick) >= TickIndex(MinimapRefreshIntervalTicks);
    if (!bDue)
    {
        return;
    }

    int32_t CellsX = 0, CellsY = 0, StrideX = 1, StrideY = 1;
    ComputeMinimapCellGrid(Map.Width, Map.Height, CellsX, CellsY, StrideX, StrideY);
    if (CellsX <= 0 || CellsY <= 0)
    {
        return;   // no map yet; leave whatever the consumer already has
    }

    MinimapBackground Sampled;
    Sampled.Width = CellsX;
    Sampled.Height = CellsY;
    Sampled.Terrain.assign(size_t(CellsX) * size_t(CellsY), uint8_t(MinimapTerrain::Unknown));
    Sampled.Shroud.assign(size_t(CellsX) * size_t(CellsY), uint8_t(MinimapShroud::NeverSeen));

    const FFogOfWarGrid* Fog = World.GetFogGrid();

    for (int32_t CellY = 0; CellY < CellsY; ++CellY)
    {
        for (int32_t CellX = 0; CellX < CellsX; ++CellX)
        {
            MinimapTerrain BestTerrain = MinimapTerrain::Unknown;
            MinimapShroud BestShroud = MinimapShroud::NeverSeen;

            const int32_t TileX0 = CellX * StrideX;
            const int32_t TileY0 = CellY * StrideY;
            for (int32_t OffsetY = 0; OffsetY < StrideY; ++OffsetY)
            {
                for (int32_t OffsetX = 0; OffsetX < StrideX; ++OffsetX)
                {
                    const int32_t TileX = TileX0 + OffsetX;
                    const int32_t TileY = TileY0 + OffsetY;
                    if (!Map.IsInBounds(TileX, TileY))
                    {
                        continue;
                    }

                    // The brightest state any covered tile is in. A cell straddling the
                    // edge of vision reads as lit, which matches what the player sees on
                    // the terrain itself.
                    const VisibilityState Visibility = Fog != nullptr
                        ? Fog->GetVisibility(int32_t(LocalPlayer), TileX, TileY)
                        : VisibilityState::CurrentlyVisible;   // no fog grid: nothing is hidden
                    const MinimapShroud Shroud = ShroudForVisibility(Visibility);
                    if (Shroud > BestShroud)
                    {
                        BestShroud = Shroud;
                    }

                    // Terrain is only recorded for ground somebody has actually looked at.
                    // Gating on the shroud instead let a radar sweep reveal the coastline and
                    // every ore patch across 24 tiles of unexplored map -- exactly the maphack
                    // the fog exists to prevent.
                    if (!HasLearnedTerrain(Visibility))
                    {
                        continue;
                    }
                    const MinimapTerrain Terrain = TerrainForTileFlags(Map.GetTile(TileX, TileY));
                    if (TerrainSalience(Terrain) > TerrainSalience(BestTerrain))
                    {
                        BestTerrain = Terrain;
                    }
                }
            }

            const size_t Index = size_t(CellY) * size_t(CellsX) + size_t(CellX);
            Sampled.Terrain[Index] = uint8_t(BestTerrain);
            Sampled.Shroud[Index] = uint8_t(BestShroud);
        }
    }

    LastBackgroundTick = Tick;
    bHasSampledBackground = true;

    // Only bump the revision when the result actually differs. A match where nothing has
    // been explored since the last sample -- which is most ticks once the map is open --
    // then produces no traffic at all, and the consumer's texture is not re-uploaded.
    if (Sampled.Width == CachedBackground.Width &&
        Sampled.Height == CachedBackground.Height &&
        Sampled.Terrain == CachedBackground.Terrain &&
        Sampled.Shroud == CachedBackground.Shroud)
    {
        return;
    }

    CachedBackground = std::move(Sampled);
    ++BackgroundRevision;
    Out.BackgroundRevision = BackgroundRevision;
    Out.bBackgroundChanged = true;
    Out.Background = CachedBackground;
}

void HudSnapshotBuilder::BuildRadar(const SimWorld& World, const std::vector<EntityId>& Selection,
                                    RadarState& Out) const
{
    Out = RadarState();
    const MapDescription& Map = World.GetMap();
    Out.MapWidthUnits = Map.Width * kTileSizeUnits;
    Out.MapHeightUnits = Map.Height * kTileSizeUnits;

    // ADR-0013's "Radar / minimap" row: the panel goes dark from Moderate. Only the radar
    // half of that row was implemented, so the overview survived a blackout intact.
    //
    // A player with no radar building at all keeps their minimap -- the row is about
    // losing a facility to a deficit, not about gating the basic overview behind tech.
    // So the panel goes dark only if the player *has* a radar and the deficit has taken
    // it, which is also what makes building one feel like it bought something.
    const PowerTier Tier = World.GetPlayer(LocalPlayer).GetPowerTier();
    bool bHasRadar = false;
    bool bHasWorkingRadar = false;
    for (uint32_t Index = 0; Index < uint32_t(World.GetAllCores().size()); ++Index)
    {
        const EntityCore& C = World.GetAllCores()[Index];
        if (!C.bAlive || C.Kind != EntityKind::Building || C.Owner != LocalPlayer)
        {
            continue;
        }
        const EntityDef* Def = World.GetContent()->FindEntity(C.Def);
        if (Def == nullptr || !Def->Building.bIsRadar)
        {
            continue;
        }
        const BuildingComp* B = World.GetBuilding(World.MakeId(Index));
        if (B == nullptr || B->State != ConstructionState::Complete)
        {
            continue;
        }
        bHasRadar = true;
        if (!IsPowerPriorityOffline(B->Priority, Tier))
        {
            bHasWorkingRadar = true;
            break;
        }
    }
    Out.bOfflineForPower = bHasRadar && !bHasWorkingRadar;
    Out.bOnline = !Out.bOfflineForPower;
    if (!Out.bOnline)
    {
        return;   // no markers at all: the panel is dark
    }

    const FFogOfWarGrid* Fog = World.GetFogGrid();
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    Out.Markers.reserve(Cores.size());

    for (uint32_t Index = 0; Index < uint32_t(Cores.size()); ++Index)
    {
        const EntityCore& Core = Cores[Index];
        if (!Core.bAlive || Core.Kind == EntityKind::Projectile)
        {
            continue;
        }

        const EntityId Id = World.MakeId(Index);
        const TransformComp* Transform = World.GetTransform(Id);
        if (Transform == nullptr)
        {
            continue;
        }

        // Own forces are always known. Enemy and neutral markers obey exactly the
        // same fog lookup as the AI view, so the minimap cannot become a maphack.
        if (Core.Owner != LocalPlayer && Fog != nullptr)
        {
            const TileCoord Tile = Map.WorldToTile(Transform->Position);
            const VisibilityState Visibility = Fog->GetVisibility(LocalPlayer, Tile.X, Tile.Y);
            if (Visibility != VisibilityState::CurrentlyVisible &&
                Visibility != VisibilityState::RadarDetected)
            {
                continue;
            }
        }

        RadarMarker Marker;
        Marker.Entity = Id;
        Marker.Position = Transform->Position;
        Marker.Owner = Core.Owner;
        Marker.Kind = Core.Kind;
        Marker.bSelected = std::find(Selection.begin(), Selection.end(), Id) != Selection.end();
        Out.Markers.push_back(Marker);
    }
}

// ---------------------------------------------------------------------------
// Match
// ---------------------------------------------------------------------------

void HudSnapshotBuilder::BuildMatch(const SimWorld& World, MatchState& Out) const
{
    Out.Tick = World.GetTick();
    Out.ElapsedSeconds = int32_t(World.GetTick() / uint32_t(kTicksPerSecond));
    Out.Phase = World.GetPhase();
    Out.Winner = World.GetWinner();
    Out.bLocalPlayerDefeated = World.GetPlayer(LocalPlayer).bDefeated;
}

// ---------------------------------------------------------------------------
// Alerts
// ---------------------------------------------------------------------------

void HudSnapshotBuilder::PushAlert(const Alert& Incoming)
{
    // Merge an identical alert that is still inside the merge window rather than
    // adding a row. Without this, one artillery barrage produces sixty
    // "units under attack" lines and the feed becomes useless exactly when it
    // matters most.
    for (Alert& Existing : ActiveAlerts)
    {
        if (Existing.Type == Incoming.Type && Existing.Content == Incoming.Content &&
            Incoming.LastTick - Existing.LastTick <= TickIndex(AlertMergeWindowTicks))
        {
            Existing.LastTick = Incoming.LastTick;
            Existing.RepeatCount += 1;
            if (Incoming.bHasLocation)
            {
                Existing.Location = Incoming.Location;
                Existing.bHasLocation = true;
            }
            return;
        }
    }
    ActiveAlerts.push_back(Incoming);
}

void HudSnapshotBuilder::AccumulateAlerts(const SimWorld& World, const ResourceState& Resources)
{
    const TickIndex Now = World.GetTick();

    auto Make = [Now](AlertType Type, AlertSeverity Severity)
    {
        Alert A;
        A.Type = Type;
        A.Severity = Severity;
        A.FirstTick = Now;
        A.LastTick = Now;
        return A;
    };

    for (const SimEvent& Event : World.GetEvents())
    {
        switch (Event.Type)
        {
            case SimEventType::DamageApplied:
            {
                const EntityCore* Core = World.GetCore(Event.Entity);
                if (Core == nullptr || Core->Owner != LocalPlayer)
                {
                    break;   // only the local player's losses are alerts
                }
                Alert A = Make(Core->Kind == EntityKind::Building ? AlertType::BaseUnderAttack
                                                                  : AlertType::UnitsUnderAttack,
                               AlertSeverity::Critical);
                A.Location = Event.Location;
                A.bHasLocation = true;
                PushAlert(A);
                break;
            }

            case SimEventType::EntityDestroyed:
            {
                if (Event.Player != LocalPlayer)
                {
                    break;
                }
                // The entity is already gone, so its kind cannot be queried. Use the
                // content id instead: a projectile carries its *weapon* id, which is
                // absent from the entity registry. Without this check every spent
                // shell the player fires is reported as a lost unit.
                const EntityDef* Def = World.GetContent() != nullptr
                                           ? World.GetContent()->FindEntity(Event.Content)
                                           : nullptr;
                if (Def == nullptr)
                {
                    break;
                }
                Alert A = Make(Def->Kind == EntityKind::Building ? AlertType::BuildingLost : AlertType::UnitLost,
                               Def->Kind == EntityKind::Building ? AlertSeverity::Critical : AlertSeverity::Warning);
                A.Content = Event.Content;
                A.Location = Event.Location;
                A.bHasLocation = true;
                PushAlert(A);
                break;
            }

            case SimEventType::BuildingCompleted:
            {
                if (Event.Player != LocalPlayer)
                {
                    break;
                }
                Alert A = Make(AlertType::ConstructionComplete, AlertSeverity::Info);
                A.Content = Event.Content;
                A.Location = Event.Location;
                A.bHasLocation = true;
                PushAlert(A);
                break;
            }

            case SimEventType::ProductionCompleted:
            {
                if (Event.Player != LocalPlayer)
                {
                    break;
                }
                Alert A = Make(AlertType::UnitReady, AlertSeverity::Info);
                A.Content = Event.Content;
                PushAlert(A);
                break;
            }

            case SimEventType::MCVDeployed:
            {
                if (Event.Player != LocalPlayer)
                {
                    break;
                }
                Alert A = Make(AlertType::MCVDeployed, AlertSeverity::Info);
                A.Content = Event.Content;
                A.Location = Event.Location;
                A.bHasLocation = true;
                PushAlert(A);
                break;
            }

            case SimEventType::MCVUndeployed:
            {
                if (Event.Player != LocalPlayer)
                {
                    break;
                }
                Alert A = Make(AlertType::MCVUndeployed, AlertSeverity::Info);
                A.Content = Event.Content;
                A.Location = Event.Location;
                A.bHasLocation = true;
                PushAlert(A);
                break;
            }

            default:
                break;
        }
    }

    // Condition-based alerts are re-asserted every tick while the condition holds and
    // merged, so they persist without flooding.
    if (Resources.bPowerShortage)
    {
        PushAlert(Make(AlertType::LowPower, AlertSeverity::Warning));
    }

    // Expire anything that has stopped repeating.
    ActiveAlerts.erase(std::remove_if(ActiveAlerts.begin(), ActiveAlerts.end(),
                                      [this, Now](const Alert& A)
                                      { return Now - A.LastTick > TickIndex(AlertLifetimeTicks); }),
                       ActiveAlerts.end());

    // Most severe first, then most recent: a superweapon warning must never be
    // pushed off screen by a stream of "unit ready".
    std::stable_sort(ActiveAlerts.begin(), ActiveAlerts.end(),
                     [](const Alert& A, const Alert& B)
                     {
                         if (A.Severity != B.Severity)
                         {
                             return uint8_t(A.Severity) > uint8_t(B.Severity);
                         }
                         return A.LastTick > B.LastTick;
                     });
}

// ---------------------------------------------------------------------------

void HudSnapshotBuilder::Build(const SimWorld& World, const std::vector<EntityId>& Selection, HudSnapshot& Out)
{
    Out = HudSnapshot();
    Out.LocalPlayer = LocalPlayer;

    BuildResources(World, Out.Resources);
    BuildSelection(World, Selection, Out.Selection);
    BuildProduction(World, Out.Selection, Out.Production);
    BuildRadar(World, Selection, Out.Radar);
    // After BuildRadar, which resets the whole RadarState. The background is produced even
    // when the panel is dark: the player is told the radar is out, not made to re-explore
    // the map once it comes back.
    BuildMinimapBackground(World, Out.Radar);
    BuildMatch(World, Out.Match);
    AccumulateAlerts(World, Out.Resources);

    Out.Alerts = ActiveAlerts;

    // Pings are derived from the alerts, so this must come after AccumulateAlerts. Alerts
    // with no location -- and conditions like low power, which have no place on a map -- do
    // not produce one.
    const TickIndex Now = World.GetTick();
    for (const Alert& A : ActiveAlerts)
    {
        if (!A.bHasLocation)
        {
            continue;
        }
        RadarPingKind Kind = RadarPingKind::Attack;
        if (!RadarPingKindForAlert(A.Type, Kind))
        {
            continue;
        }
        // Measured from LastTick, not FirstTick: a merged alert that is still firing must
        // stay lit rather than fading out while the base is still being shelled.
        const int32_t Intensity = RadarPingIntensityPercent(A.LastTick, Now, kRadarPingLifetimeTicks);
        if (Intensity <= 0)
        {
            continue;   // the alert row may outlive its ping
        }
        RadarPing Ping;
        Ping.Position = A.Location;
        Ping.Kind = Kind;
        Ping.IntensityPercent = Intensity;
        Out.Radar.Pings.push_back(Ping);
    }

    // No sort here on purpose. ActiveAlerts is already ordered most-severe-then-most-recent,
    // and the pings are appended in that order, so re-sorting them would be a second copy of
    // the same policy -- and the two would drift the moment one was changed. The invariant the
    // widget relies on is asserted by the ping-ordering tests, which read the snapshot rather
    // than any sort call, so they hold whichever layer establishes the order.

}

} // namespace Presentation
} // namespace RA4
