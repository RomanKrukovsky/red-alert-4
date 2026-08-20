# UI/UX Bible (`UI_UX_BIBLE.md`)

**Document Version**: 2.0
**Created**: 2026-08-05
**Updated**: 2026-08-20

---

## 1. Uncertainty Language (NORMATIVE — gates all intel UI)

The perceived world (ADR-0021/0026) shows the player *beliefs*, not facts. If the player cannot
instantly tell belief from fact, the system reads as a rendering bug (RISK-11) and excludes
players with atypical vision (RISK-19). This section is the contract every intel UI element must
satisfy. "MUST" items are acceptance criteria, testable in UI review.

### 1.1 The redundancy rule (accessibility-critical)

Confidence and staleness MUST each be encoded through **at least three independent channels**,
of which **at most one** may be colour- or opacity-based:

1. **Numeric**: confidence as a percentage, always available (see 1.3 for when it is visible).
2. **Shape/fill state**: icon fill degrades in discrete steps (full → hatched → outline-only).
   Shape reads under any colour-vision condition and any team-colour palette.
3. **Text badge**: "last seen" timestamp (`2:14`) rendered as text, not implied by fading.
4. *(Permitted fourth channel)* desaturation/transparency ramp — allowed as reinforcement,
   FORBIDDEN as the only cue.

**MUST NOT**: convey any intel state by hue alone, by opacity alone, or by hue+opacity together
without a shape/text channel. This is a hard review gate, not a preference.

### 1.2 The five confidence tiers (canonical vocabulary)

One shared vocabulary across HUD, minimap, tooltips and post-match report. These tiers map to
`PerceivedTrack.Confidence` ranges (exact thresholds are content-tunable; tier *semantics* are not):

| Tier | Name (RU/EN) | Icon fill | Position display | Count display | Timestamp badge |
|---|---|---|---|---|---|
| T5 | Наблюдается / LIVE | Solid + pulse tick on update | Exact point | Exact interval (min=max) | none (live) |
| T4 | Свежее / RECENT | Solid | Exact point | Interval, narrow | `0:0X` |
| T3 | Устаревает / AGEING | Hatched | Area ellipse (PositionErrorRadius) | Interval, widening | `0:XX` |
| T2 | Просрочено / STALE | Outline only | Wide area, dashed boundary | "~N?" | `X:XX` greyed |
| T1 | Слух / TRACE | Outline + "?" glyph | Region highlight only | none shown | `>5:00` |
| — | (below floor) | Removed from map | — | — | — |

Counts are ALWAYS `BelievedCountMin–BelievedCountMax` intervals (per ADR-0026); a single exact
number is displayed only when min equals max. Writing "24 tanks" when the data is 18–30 is a
defect.

### 1.3 Numeric confidence visibility

- **On selection / hover**: full readout, e.g. `Танки: 18–30 · достоверность 61% · виден 2:14 назад · источник: радар`.
- **Permanent numeric mode**: an accessibility setting displays the percentage on every intel icon
  permanently (RISK-19 requirement). Default off; MUST exist at v1, not "later".
- **Minimap**: tiers T1–T3 render with distinct minimap glyph shapes (not colour variants of one
  dot).

### 1.4 Source attribution and disagreement

- Every readout names its dominant source type when sources exist (`радар`, `визуально`,
  `донесение`, `вывод`). *(Depends on I-B3 — if source types are not restored to the track model,
  this line degrades to `N независимых источников` and this document must be amended.)*
- **Disagreement is surfaced, never averaged silently** (GDD §8): when `bContested` is set, the
  icon carries a split-diamond marker and the readout shows the conflict explicitly:
  `радар: колонна техники · визуально: пусто · 30%`.
- The honesty contract in one line for every UI writer: **the UI may show wrong data, but must
  never show wrong confidence.**

### 1.5 High-contrast intel mode

A dedicated setting (independent from the general high-contrast theme):
- tier fills become high-contrast patterns on a guaranteed-contrast backing plate (≥ 4.5:1 against
  any terrain);
- the desaturation ramp channel is disabled entirely (shape/text carry everything);
- error ellipses render as solid outlines instead of soft gradients.

### 1.6 Command feedback under ADR-0022 (order latency)

To keep propagation latency from reading as input lag (RISK-13):
- **Two-stage acknowledgement**, visually distinct: *order issued* (immediate local tick mark at
  cursor) vs *order received* (unit-side flash when delivery tick arrives). One-stage feedback is
  a defect.
- Group link status is a persistent chip on the unit plate and control-group icons:
  `СВЯЗЬ` / `ПОМЕХИ` (with current added delay in seconds) / `АВТОНОМНО` (with active doctrine
  name). Status must be visible BEFORE ordering, not discovered after.
- In `CommandNetwork = Classic` mode all of the above collapses to the classic single
  acknowledgement.

### 1.7 Teaching obligations

- The tutorial mission that introduces intel MUST make the player experience one *wrong-belief*
  event safely (a ghost building that turns out sold) before any competitive exposure.
- First 3 skirmish matches: one-line contextual hints on first encounter of each tier T3–T1.
- The post-match report reveals ground truth vs the player's belief timeline (ties into deferred
  idea 20; the report is the systemic answer to "the game lied to me").

### 1.8 What intel UI must NEVER do (hard prohibitions)

1. Never render a belief icon in the exact same visual state as a live-observed unit (T5 must be
   unambiguous).
2. Never silently merge contested sources into one confident readout.
3. Never use red/green as the only tier discriminator.
4. Never animate stale icons with behaviours implying liveness (idle animations, turret tracking).
5. Never show a phantom differently from a believed-real contact to the *owning observer* — the
   deceived player's UI must be internally consistent (the spectator view is where truth is
   marked, per ADR-0023).

## 2. HUD Layout & Sidebar

The HUD uses a fixed 1920×1080 design space inside an aspect-preserving safe frame. The world
remains visible and clickable in the centre. Only the actual objectives, minimap, selection,
production, queue and command rectangles block world input.

- Top: resources, power, command limit and match clock.
- Top-left: commander and objectives; critical alerts move to the centre only while active.
- Top-right: one batched Slate minimap. Unit contacts are draw elements, never child widgets.
- Right: faction production categories and build cards.
- Bottom: selected unit/building, queue and command grid.
- Faction changes are data and theme changes on one shared HUD shell. Duplicated faction HUD
  logic is forbidden.
- Simulation data reaches the HUD through event-driven snapshots. Per-frame polling is forbidden.

## 3. Menus, Lobby, Settings

All full-screen flows use CommonUI routing and the same focus/back contract. Layout is native UMG
and Slate; YAML may contain content but never defines widget geometry or interaction logic.

- Every menu has one obvious primary action and a persistent back route.
- Keyboard and gamepad focus must enter at the primary or last-used action.
- The lobby supports eight player rows, faction/team/colour, ready state, host-only launch,
  observers and chat. Rows are virtualized.
- Background art is presentation only. Buttons, text, progress, selection and validation remain
  real interactive widgets.
- The 24 canonical screen references are the visual acceptance set. Their mapping is validated by
  `Tools/Editor/ValidateRA4UI.py`.

## 4. Hotkey Philosophy

- A command has one canonical key and may expose a remappable alternative.
- Escape always closes the top modal, then navigates back, then opens pause in gameplay.
- HUD hotkeys operate gameplay even when the pointer is over the world; text input and open modals
  take priority.
- Tooltips show the current binding, never a hard-coded key name.
- Every mouse-only action needs a keyboard/gamepad path.

## 5. Localization & Text Rules

- Player-facing strings use `FText` and localization keys. Runtime UI must not assemble sentences
  from translated fragments.
- Russian is the visual-reference language; English parity is required before release.
- Labels may wrap to two lines. Critical numbers, costs, timers and hotkeys must never be clipped.
- Uppercase is reserved for headings, alerts and compact RTS controls, not long body copy.
- Minimum text contrast is 4.5:1; faction colour never replaces readable neutral text.
