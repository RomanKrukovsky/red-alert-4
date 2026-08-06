// Copyright (c) Red Alert 4 project. Read-only projection of the match for the HUD.
//
// The HUD needs numbers the simulation already owns, in a shape UMG can consume, at a
// cadence that is not "every frame". This module produces one immutable snapshot per
// simulation tick; the UI diffs it and pushes only what changed into its ViewModels.
//
// It is engine-free on purpose: every rule about what counts as "low power", what
// blocks a production card, and how a mixed selection is summarised is decided here
// and covered by headless tests, instead of being discovered by staring at a widget.
//
// Direction of dependency is one way and must stay that way:
//     RA4Simulation  <--  RA4Presentation  <--  RA4UI
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4Core/Ids.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4PRESENTATION_API
#define RA4PRESENTATION_API
#endif

namespace RA4
{
namespace Presentation
{

// Production progress is accumulated by the simulation in hundredths of a tick so
// that low power can slow it proportionally. Mirrored here to turn it back into a
// 0..1 fraction; asserted against real queue behaviour in the tests.
constexpr int32_t kProductionProgressScale = 100;

// --- Resources -------------------------------------------------------------

struct ResourceState
{
    int32_t Credits = 0;
    int32_t CreditsDelta = 0;          // change since the previous snapshot

    int32_t PowerProduced = 0;
    int32_t PowerConsumed = 0;
    int32_t PowerRatioPercent = 100;   // 100 == fully supplied
    bool bPowerShortage = false;

    // The reference HUD shows a supply counter (e.g. "88 / 200"). The simulation has
    // no population cap, so Cap stays 0 and bSupplyModelled is false: the widget must
    // hide the counter rather than display a fabricated limit.
    int32_t SupplyUsed = 0;
    int32_t SupplyCap = 0;
    bool bSupplyModelled = false;
};

// --- Alerts ----------------------------------------------------------------

enum class AlertType : uint8_t
{
    None = 0,
    LowPower,
    InsufficientFunds,
    BaseUnderAttack,
    UnitsUnderAttack,
    BuildingLost,
    UnitLost,
    ConstructionComplete,
    UnitReady,
    ResourcesDepleted,
};

enum class AlertSeverity : uint8_t
{
    Info = 0,
    Warning,
    Critical,
};

struct Alert
{
    AlertType Type = AlertType::None;
    AlertSeverity Severity = AlertSeverity::Info;
    TickIndex FirstTick = 0;
    TickIndex LastTick = 0;
    int32_t RepeatCount = 1;      // identical events merge instead of flooding the feed
    Vec2 Location;
    bool bHasLocation = false;
    ContentId Content;            // what was built / lost, when applicable
};

// --- Selection -------------------------------------------------------------

enum class SelectionKind : uint8_t
{
    Empty = 0,
    SingleUnit,
    SingleBuilding,
    MultipleUnits,
    Mixed,          // units and buildings together
};

// One row of the multi-selection grid: units of the same type are grouped, exactly
// as the reference HUD shows them (icon plus a count badge).
struct SelectionGroup
{
    ContentId Content;
    int32_t Count = 0;
    int32_t HealthCurrent = 0;
    int32_t HealthMax = 0;
    EntityId Representative;
};

struct SelectionState
{
    SelectionKind Kind = SelectionKind::Empty;
    int32_t TotalCount = 0;

    // Populated for SingleUnit / SingleBuilding, and for the representative of a
    // multi-selection, which is what drives the portrait and the detail card.
    EntityId Primary;
    ContentId PrimaryContent;
    std::string PrimaryDisplayNameKey;
    int32_t PrimaryHealthCurrent = 0;
    int32_t PrimaryHealthMax = 0;
    bool bPrimaryIsOwned = false;

    // ADR-0013. Present for an owned building so the detail card can show the power
    // priority and offer the cycle control, and so a repair toggle can show whether it
    // is currently running. Without these the mechanics exist but are invisible: the
    // player has no way to see or change either one.
    bool bPrimaryIsBuilding = false;
    PowerPriority PrimaryPowerPriority = PowerPriority::Production;
    // Whether this building's priority band is offline at the owner's current power
    // tier, so the card can grey it rather than making the player work out the table.
    bool bPrimaryPowerOffline = false;
    bool bPrimaryIsRepairing = false;
    // A damaged, complete, owned building is the only thing repair can be started on.
    bool bPrimaryCanRepair = false;

    std::vector<SelectionGroup> Groups;

    // Set when the selection contains at least one producer, which is what makes the
    // production panel show that building's queue rather than the default.
    EntityId ProductionSource;
};

// --- Production ------------------------------------------------------------

enum class BuildBlockReason : uint8_t
{
    None = 0,             // buildable right now
    MissingPrerequisite,
    InsufficientCredits,
    NoProducer,
    QueueFull,
    MatchOver,
};

struct BuildOption
{
    ContentId Content;
    std::string DisplayNameKey;
    ProductionCategory Category = ProductionCategory::Infantry;
    int32_t Cost = 0;
    int32_t BuildTimeTicks = 0;
    int32_t PowerDelta = 0;             // +provided or -drained
    std::string PrerequisiteKey;
    bool bAvailable = false;
    BuildBlockReason BlockReason = BuildBlockReason::NoProducer;
    // The producer a click would actually go to, so the card can send a command
    // without the widget having to search the world for one.
    EntityId Producer;
};

struct QueueEntry
{
    ContentId Content;
    std::string DisplayNameKey;
    int32_t ProgressPercent = 0;
    int32_t RemainingTicks = 0;
    bool bPaused = false;
    bool bAwaitingPlacement = false;   // finished structure waiting for a click
    int32_t SlotIndex = 0;
    // ADR-0012. The whole point of the payment state is that the card can say *why*
    // an item stopped, so it is carried through verbatim rather than collapsed into
    // bPaused -- "you are out of money" and "you pressed pause" need different UI.
    FlowPaymentState PaymentState = FlowPaymentState::Queued;
    // How much of the price has been paid so far, for the funding bar.
    int32_t PaidCredits = 0;
    int32_t TotalCost = 0;
    bool bStarvedForCredits = false;
};

struct ProductionState
{
    EntityId Producer;
    std::vector<QueueEntry> Queue;
    std::vector<BuildOption> Options;
};

// --- Radar -----------------------------------------------------------------

struct RadarMarker
{
    EntityId Entity;
    Vec2 Position;
    PlayerId Owner = kInvalidPlayer;
    EntityKind Kind = EntityKind::Unit;
    bool bSelected = false;
};

struct RadarState
{
    int32_t MapWidthUnits = 0;
    int32_t MapHeightUnits = 0;
    std::vector<RadarMarker> Markers;

    // ADR-0013: the minimap itself goes dark from Moderate. The effect matrix lists
    // "Radar / minimap" as one row, and only half of it was implemented -- radar coverage
    // stopped, but the panel carried on showing every friendly unit as though nothing had
    // happened. When this is false the widget draws an offline panel instead of markers.
    //
    // Own forces are still filtered out along with everything else: the point of the row
    // is that the player loses the overview, not that they lose enemy contacts they were
    // never entitled to.
    bool bOnline = true;
    // Why it is dark, so the panel can say so rather than just being blank.
    bool bOfflineForPower = false;
};

// Letterboxes a map of the given size inside a panel of the given size, preserving the
// map's aspect ratio and centring it. Returns the offset and extent of the map rect in
// panel-local units.
//
// Lives here rather than in the widget because it is pure integer-free geometry with no
// Unreal types, so it can be tested headlessly -- and because both the painter and the
// click handler must use the identical mapping. A square panel drawing a 2:1 map stretched
// it vertically, so a marker halfway up the map appeared nowhere near the terrain it stood
// on, and a click came back with the wrong world position.
void ComputeMinimapRect(double PanelWidth, double PanelHeight,
                        double MapWidth, double MapHeight,
                        double& OutOffsetX, double& OutOffsetY,
                        double& OutWidth, double& OutHeight);

// --- Match -----------------------------------------------------------------

struct MatchState
{
    TickIndex Tick = 0;
    int32_t ElapsedSeconds = 0;
    MatchPhase Phase = MatchPhase::NotStarted;
    PlayerId Winner = kInvalidPlayer;
    bool bLocalPlayerDefeated = false;
};

// --- Snapshot --------------------------------------------------------------

struct HudSnapshot
{
    PlayerId LocalPlayer = 0;
    ResourceState Resources;
    SelectionState Selection;
    ProductionState Production;
    RadarState Radar;
    MatchState Match;
    std::vector<Alert> Alerts;
};

// Builds a snapshot for one tick.
//
// Stateful because two things need history: the credit delta, and alert merging --
// "base under attack" must not spam the feed once per damage event. Keep one builder
// per local player for the lifetime of the match.
class RA4PRESENTATION_API HudSnapshotBuilder
{
public:
    void Initialize(PlayerId InLocalPlayer);
    void Reset();

    // Call once per simulation tick, after SimWorld::Tick and before ClearEvents:
    // the alert feed is derived from that tick's events.
    void Build(const SimWorld& World, const std::vector<EntityId>& Selection, HudSnapshot& Out);

    // How long a merged alert stays in the feed before it is dropped.
    void SetAlertLifetimeTicks(int32_t Ticks) { AlertLifetimeTicks = Ticks; }
    // Identical alerts arriving within this window increment RepeatCount instead of
    // adding a row.
    void SetAlertMergeWindowTicks(int32_t Ticks) { AlertMergeWindowTicks = Ticks; }

private:
    void BuildResources(const SimWorld& World, ResourceState& Out);
    void BuildSelection(const SimWorld& World, const std::vector<EntityId>& Selection, SelectionState& Out) const;
    void BuildProduction(const SimWorld& World, const SelectionState& Selection, ProductionState& Out) const;
    void BuildRadar(const SimWorld& World, const std::vector<EntityId>& Selection, RadarState& Out) const;
    void BuildMatch(const SimWorld& World, MatchState& Out) const;
    void AccumulateAlerts(const SimWorld& World, const ResourceState& Resources);
    void PushAlert(const Alert& Incoming);

    PlayerId LocalPlayer = 0;
    int32_t PreviousCredits = 0;
    bool bHasPreviousCredits = false;
    std::vector<Alert> ActiveAlerts;

    int32_t AlertLifetimeTicks = 200;      // 10 s at 20 Hz
    int32_t AlertMergeWindowTicks = 60;    // 3 s
};

const char* RA4PRESENTATION_API ToString(AlertType Type);
const char* RA4PRESENTATION_API ToString(BuildBlockReason Reason);

} // namespace Presentation
} // namespace RA4
