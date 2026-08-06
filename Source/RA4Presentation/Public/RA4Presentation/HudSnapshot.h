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

// The minimap drew markers onto an empty grid, so a player could see where their units
// were but nothing about where they were standing -- no coast, no cliff, no ore, and no
// record of which parts of the map they had actually explored. These two layers are the
// background underneath the markers.
//
// Terrain and shroud are separate because they answer different questions: what is there,
// and how much of it the player is entitled to know. Keeping them apart means the shroud
// can dim or black out a cell without the terrain layer having to encode fog states as
// extra terrain values.
enum class MinimapTerrain : uint8_t
{
    Unknown = 0,   // never explored: nothing may be drawn here
    Ground,
    Water,
    Cliff,
    Ore,
    Structure,     // a building footprint occupies the cell
};

enum class MinimapShroud : uint8_t
{
    NeverSeen = 0,  // black
    Remembered,     // explored but not currently watched: dimmed
    Visible,        // eyes or radar on it now: full brightness
};

// A large map must not turn into a per-cell upload the size of the map. Both axes are
// downsampled to at most this many cells, which is already finer than the panel's pixel
// count at any realistic panel size.
constexpr int32_t kMinimapMaxCellsPerAxis = 96;

struct MinimapBackground
{
    int32_t Width = 0;
    int32_t Height = 0;
    // Row-major, Width * Height entries each. Terrain holds MinimapTerrain values and
    // Shroud holds MinimapShroud values; stored as bytes so the UI can hand them to a
    // texture without a conversion pass.
    std::vector<uint8_t> Terrain;
    std::vector<uint8_t> Shroud;
};

// A transient marker on the minimap for something that just happened: a base being shelled,
// a structure finishing. The alert feed already carries these events with coordinates, but a
// line of text does not tell a player *where* to look, which is the whole reason a minimap
// exists during an attack.
//
// Derived from the alert list rather than being a second event channel, so a ping cannot
// appear for something the feed does not also report -- one source of truth for "what just
// happened", presented two ways.
enum class RadarPingKind : uint8_t
{
    Attack = 0,     // own base or forces under fire: the urgent one
    Loss,           // something of the player's was destroyed
    Construction,   // a building finished or a unit rolled out
};

struct RadarPing
{
    Vec2 Position;
    RadarPingKind Kind = RadarPingKind::Attack;
    // 0..1, counting down. The widget scales and fades by this, so a ping draws attention
    // when it is new and stops competing with live markers as it ages. Computed here so the
    // decay curve is testable and does not depend on the UI's frame rate.
    int32_t IntensityPercent = 100;
};

// How long a ping stays on the panel. Shorter than the alert feed's own lifetime: a text row
// is worth re-reading, a flashing dot stops being information and becomes noise.
constexpr int32_t kRadarPingLifetimeTicks = 60;   // 3 s at 20 Hz

// Maps an alert to a ping kind. Returns false for alerts that have no place on the map --
// "low power" and "insufficient funds" are conditions, not locations, and pinging the map for
// them would train the player to ignore pings.
bool RA4PRESENTATION_API RadarPingKindForAlert(AlertType Type, RadarPingKind& OutKind);

// Remaining intensity of a ping raised at RaisedTick, as of Now. Zero once expired.
int32_t RA4PRESENTATION_API RadarPingIntensityPercent(TickIndex RaisedTick, TickIndex Now,
                                                      int32_t LifetimeTicks);

struct RadarState
{
    int32_t MapWidthUnits = 0;
    int32_t MapHeightUnits = 0;
    std::vector<RadarMarker> Markers;

    // Background always describes the currently explored map, so a consumer that starts
    // mid-match gets a complete picture immediately rather than waiting for the next scout
    // to move. bBackgroundChanged says whether it differs from the previous snapshot, and
    // BackgroundRevision counts how many times it has changed: together they let the UI
    // skip re-uploading its texture on the ticks where nothing was newly explored, which
    // is most of them once the map is open.
    uint32_t BackgroundRevision = 0;
    bool bBackgroundChanged = false;
    MinimapBackground Background;

    // Newest first, so a widget that draws only the top few shows the most urgent.
    std::vector<RadarPing> Pings;

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
void RA4PRESENTATION_API ComputeMinimapRect(double PanelWidth, double PanelHeight,
                        double MapWidth, double MapHeight,
                        double& OutOffsetX, double& OutOffsetY,
                        double& OutWidth, double& OutHeight);

// The camera's view rectangle projected onto the minimap panel, as panel-local pixels.
//
// Takes the map rect from ComputeMinimapRect and the camera footprint in world units, and
// returns the outline corners clamped to the map. Lives here rather than in the widget so the
// clamping and the Y flip -- the simulation's Y grows northward, the panel's downward -- are
// covered by headless tests instead of being judged by eye.
//
// Returns false when there is nothing to draw: an unknown footprint (zero extent), or a
// degenerate map.
bool RA4PRESENTATION_API ComputeMinimapCameraFrame(
    double MapRectOffsetX, double MapRectOffsetY, double MapRectWidth, double MapRectHeight,
    double MapWorldWidth, double MapWorldHeight,
    double ViewCentreX, double ViewCentreY, double ViewExtentX, double ViewExtentY,
    double& OutLeft, double& OutTop, double& OutRight, double& OutBottom);

// Downsamples a map of TileWidth x TileHeight to at most kMinimapMaxCellsPerAxis per axis.
// Returns the cell counts and, through the two callbacks' worth of stride, how many tiles
// each cell covers. Exposed so the tests can assert the sampling without going through a
// whole SimWorld.
void RA4PRESENTATION_API ComputeMinimapCellGrid(int32_t TileWidth, int32_t TileHeight,
                            int32_t& OutCellsX, int32_t& OutCellsY,
                            int32_t& OutStrideX, int32_t& OutStrideY);

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

    // How often the minimap background is re-sampled. The terrain barely changes and the
    // explored area grows slowly, so re-scanning the whole map every tick would burn time
    // to produce an identical result. Set to 1 in tests that need it evaluated immediately.
    void SetMinimapRefreshIntervalTicks(int32_t Ticks)
    {
        MinimapRefreshIntervalTicks = Ticks > 0 ? Ticks : 1;
    }

private:
    void BuildResources(const SimWorld& World, ResourceState& Out);
    void BuildSelection(const SimWorld& World, const std::vector<EntityId>& Selection, SelectionState& Out) const;
    void BuildProduction(const SimWorld& World, const SelectionState& Selection, ProductionState& Out) const;
    void BuildRadar(const SimWorld& World, const std::vector<EntityId>& Selection, RadarState& Out) const;
    // Non-const: owns the cached background and the revision counter, since the whole
    // point is to avoid rebuilding an identical one every tick.
    void BuildMinimapBackground(const SimWorld& World, RadarState& Out);
    void BuildMatch(const SimWorld& World, MatchState& Out) const;
    void AccumulateAlerts(const SimWorld& World, const ResourceState& Resources);
    void PushAlert(const Alert& Incoming);

    PlayerId LocalPlayer = 0;
    int32_t PreviousCredits = 0;
    bool bHasPreviousCredits = false;
    std::vector<Alert> ActiveAlerts;

    int32_t AlertLifetimeTicks = 200;      // 10 s at 20 Hz
    int32_t AlertMergeWindowTicks = 60;    // 3 s

    // 0.5 s at 20 Hz: fast enough that newly explored ground appears while the scout is
    // still moving, slow enough to be 1/10th of the work of doing it every tick.
    int32_t MinimapRefreshIntervalTicks = 10;
    MinimapBackground CachedBackground;
    uint32_t BackgroundRevision = 0;
    bool bHasSampledBackground = false;
    TickIndex LastBackgroundTick = 0;
};

const char* RA4PRESENTATION_API ToString(AlertType Type);
const char* RA4PRESENTATION_API ToString(BuildBlockReason Reason);

} // namespace Presentation
} // namespace RA4
