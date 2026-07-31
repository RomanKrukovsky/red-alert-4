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
};

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
