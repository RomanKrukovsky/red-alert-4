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
        if (bMatchOver) { Option.BlockReason = BuildBlockReason::MatchOver; }
        else if (!World.HasPrerequisites(LocalPlayer, Def)) { Option.BlockReason = BuildBlockReason::MissingPrerequisite; }
        else if (!Option.Producer.IsValid()) { Option.BlockReason = BuildBlockReason::NoProducer; }
        else if (Player.Credits < Def.Production.Cost) { Option.BlockReason = BuildBlockReason::InsufficientCredits; }
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

void HudSnapshotBuilder::BuildRadar(const SimWorld& World, const std::vector<EntityId>& Selection,
                                    RadarState& Out) const
{
    Out = RadarState();
    const MapDescription& Map = World.GetMap();
    Out.MapWidthUnits = Map.Width * kTileSizeUnits;
    Out.MapHeightUnits = Map.Height * kTileSizeUnits;

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
    BuildMatch(World, Out.Match);
    AccumulateAlerts(World, Out.Resources);

    Out.Alerts = ActiveAlerts;
}

} // namespace Presentation
} // namespace RA4
