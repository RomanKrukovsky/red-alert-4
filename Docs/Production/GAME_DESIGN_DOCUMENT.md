# Game Design Document (GDD) (`GAME_DESIGN_DOCUMENT.md`)

**Document Version**: 2.1  
**Project Title**: *Iron Resonance: Command of Tomorrow*  
**Engine**: Deterministic fixed-tick C++ Simulation Core (20 Hz sim tick, `kTicksPerSecond`) + Unreal Engine 5 Presentation rendering at 60+ FPS by interpolation  

---

## 1. Product Pillars & Player Fantasy

### Product Pillars
1. **Deterministic Lockstep Precision**: fixed-tick (20 Hz) lockstep engine providing zero input latency feel and 100% reproducible replays; presentation interpolates to 60+ FPS.
2. **Asymmetric Industrial Warfare**: Four distinct factions featuring unique economic engines, construction mechanisms, and superweapons.
3. **Physics-Driven Battlefield Destruction**: Interactive terrain elevation, cover mechanics, bridge destruction, and dynamic structural collapse — with persistent consequences (section 8).
4. **Accessible Macro with Uncapped Micro Ceiling**: Streamlined base building and resource queues combined with deep micro control (formations, focus fire, ability micro, direct possession).
5. **Perception Warfare**: the player commands their *belief* about the battlefield, not the battlefield itself. Intel is late, aged and falsifiable; orders travel through built infrastructure; the battlefield remembers. See sections 8–10 and ADR-0021..0026.

### Player Fantasy
The player steps into the role of a **Supreme Commander** directing mass industrial war machines, tactical strike forces, and superweapons from an orbital battlefield interface — commanding through an imperfect picture assembled from field reports, not an omniscient god-view.

---

## 2. Gameplay Loops & Match Pacing

### Core Gameplay Loop
```
  [ Harvest Aethelite ] ---> [ Expand Base & Power Grid ]
           ^                                |
           |                                v
  [ Destroy Enemy Base ] <--- [ Recruit & Micro Armies ]
```

### Match Pacing & Phases
1. **Early Phase (0 - 3 minutes)**: Base deployment, scout unit skirmishes, securing secondary Aethelite ore nodes.
2. **Mid Phase (3 - 10 minutes)**: Tech tier escalation, factory expansion, air/naval harassment, tactical operations.
3. **Late Phase (10 - 25 minutes)**: Mega-structure deployment, superweapon countdowns, heavy army clashes, victory condition resolution.

---

## 3. Economic System & Resource Management

### Primary Resource: Aethelite Substrate
- **Harvesting**: Harvesters collect Aethelite from ground crystal nodes and return cargo to Refineries.
- **Yield**: Standard node yields 100 credits per harvesting cycle. Harvester capacity = 500 credits.
- **Refinery Docking**: Queue system manages harvester docking without collision overlaps.

### Power Grid Management
- Structures consume or produce Power.
- **Power Surplus**: Production facilities operate at 100% speed.
- **Power Deficit**: Production queues slow down by 50%; defensive turrets deactivate; radar vision is disabled.

### Building Refund & Repair
- **Repair**: Costs 50% of original construction credit rate per second; restores health over time.
- **Sell**: Refunds 50% of structure build cost; spawns civilian/survival infantry units.

---

## 4. Weapons, Armor & Damage Matrix

The simulation uses an explicit per-mille multiplier matrix (`DamageMatrixDef`):

| Warhead Type | Light Infantry | Heavy Vehicle | Fortified Building | Air Unit |
| :--- | :--- | :--- | :--- | :--- |
| **Ballistic** | 100% (1000) | 50% (500) | 25% (250) | 10% (100) |
| **Fragmentation** | 150% (1500) | 25% (250) | 50% (500) | 20% (200) |
| **Armor Piercing** | 50% (500) | 145% (1450) | 75% (750) | 30% (300) |
| **Siege / Explosive** | 30% (300) | 80% (800) | 170% (1700) | 0% (0) |
| **Electric / Energy** | 120% (1200) | 100% (1000) | 100% (1000) | 75% (750) |
| **Anti-Air Missile** | 0% (0) | 10% (100) | 0% (0) | 150% (1500) |

---

## 5. Army Controls, Micro & Possession

- **Control Groups (0-9)**: Assign, append (Shift), and recall units.
- **Formations**: Wedge, Column, Line, and Box formations with speed matching.
- **Attack-Move & Force Fire**: `Q` key for attack-move; `Ctrl + Right-Click` for force fire; `Alt + Right-Click` for force move.
- **Direct Control Possession (`F` Key)**: Allows player to directly drive a selected unit with WASD keys and aim/fire with mouse cursor in third-person view.

---

## 6. Veterancy System

Units accrue veterancy experience by dealing damage:
1. **Recruit Rank (0 XP)**: Base unit stats.
2. **Veteran Rank (1x Cost in Kills)**: +10% Damage, +10% Health, visual veteran badge.
3. **Elite Rank (2x Cost in Kills)**: +25% Damage, +25% Speed, increased weapon range.
4. **Heroic Rank (5x Cost in Kills)**: Passive self-heal (10 HP/sec), red tracer fire, rank icon overlay.

---

## 7. Destructible Environment, Height & Cover

- **Height Advantage**: Elevated terrain grants +2 vision range and 15% damage bonus against lower elevation targets.
- **Cover Mechanics**: Infantry inside urban structures receive 75% damage reduction.
- **Destructible Bridges**: Bridges can be destroyed by Siege weapons, blocking ground movement until repaired by Engineer units.

---

## 8. Intel, Confidence & the Perceived World

**Design records**: ADR-0021 (Knowledge Map), ADR-0026 (Unreliable Intelligence Layer — implementation).

The player never sees the objective battlefield. Each commander (human or AI) owns a
**perceived world** built exclusively from reports produced by their own units, sensors and
allies. Every enemy contact carries a **confidence** value, and confidence decays with time.

### What the player sees

| Confidence | Presentation | Meaning |
| :--- | :--- | :--- |
| Live contact (max) | Solid icon, exact position, exact count | A unit of mine is observing it right now. |
| Recent (high) | Solid icon, timestamp badge | Last confirmed seconds ago; likely still accurate. |
| Ageing (medium) | Desaturated icon, position as a small area | Position drifting; count shown as an approximate range. |
| Stale (low) | Ghost outline, wide area, "last seen 2:14" | Probably wrong; may be an already-dead unit. |
| Expired | Removed from map | Below the confidence floor; the memory is discarded. |

Concretely: `24 tanks, confidence 61%`, not `24 tanks`. Buildings that were sold still show as
present until re-observed. A column that moved out of vision keeps its last known heading.

### Rules the player can rely on

- **The game never lies about confidence.** A displayed confidence value is honest even when the
  underlying report is wrong; if the player is deceived, it is because their scouting was thin,
  never because the UI cheated.
- **Sources are visible.** When two source types disagree ("radar sees a column, optics see nothing"),
  the disagreement is surfaced rather than silently resolved.
- **Everything is exposable.** Every false picture can be dispelled by observation of sufficient
  grade and duration.
- **The AI plays under the same rules** (structurally, not by convention — ADR-0021 K-invariants).

### Doctrine consequences

Scouting stops being an opening checkbox and becomes a continuous cost centre. Reconnaissance-in-force,
picket lines, standing patrols, and re-scouting before committing become genuinely optimal play
rather than flavour.

### Deception (ADR-0023)

Three shared-tech tools ship first, one per signature axis: **decoy structures** (false radar
signature), **phantom columns** (false visual contacts, low confidence ceiling), and
**signature masking** (reduced emissions at an energy cost). Every deception tool must have at
least two counters; a deception without counterplay is a design defect, not a feature.

---

## 9. Command Infrastructure & Order Delivery

**Design record**: ADR-0022.

Orders are not telepathic. An issued order travels from the player's HQ through a network of
**command nodes** — HQ, relay structures, command vehicles, satellite uplinks — before reaching
the receiving group.

- **Healthy network**: delivery within 4 sim ticks (200 ms). Imperceptible; feels like a normal RTS.
- **Degraded network**: measurable delay; the UI marks affected groups as `DEGRADED`.
- **Severed network**: groups become `AUTONOMOUS` and execute their **standing doctrine** —
  a short player-authored policy chosen per group (hold / withdraw to rally / continue last order /
  local discretion).

This makes relay towers and command vehicles legitimate military targets, and gives jamming a real
role beyond a debuff. The player builds a nervous system, not only an army.

**Competitive integrity**: skirmish and ladder settings include `CommandNetwork = Classic`, which sets
delivery latency to zero and reproduces conventional RTS behaviour bit-identically. The mode is
recorded in the replay header, so a competitive replay always states which ruleset produced it.

**Anti-frustration rules**: a newer order to a group supersedes older undelivered ones (no order pile-up);
link status is always visible before an order is issued, never only after it fails; and direct-control
possession (`F`, section 5) is exempt from propagation delay — a possessed unit is under the commander's
own hand. (Resolution of the open question recorded in ADR-0022.)

---

## 10. Battlefield Memory & Salvage

**Design record**: ADR-0024.

The battlefield accumulates the history of the fight and, in the campaign, carries it between missions.

- **Terrain state**: craters, burned ground, contamination, and rubble persist. Repeatedly used
  vehicle routes compact into **improvised roads** that grant a movement bonus — the army's own
  logistics habits reshape the map.
- **Wrecks**: destroyed units leave wreck entities rather than vanishing. A wreck can be salvaged
  for partial credits, stripped for technology, rigged with explosives, or used as hard cover. Wrecks
  are ordinary entities, so existing targeting, vision and command systems work on them unchanged.
- **Caps**: wrecks are capped per map (300 target / 500 hard) with deterministic despawn of the
  oldest and least valuable — battlefield memory must never become a performance or clarity hazard.
- **Campaign persistence**: end-of-mission terrain state serializes into a `TheaterState` blob.
  Returning to a theatre shows the player the consequences of their earlier decisions. Mission-critical
  routes are declared persistence-immune, so no accumulated damage can make a mission unwinnable, and a
  missing or corrupt blob always falls back to the pristine authored map.
- **Skirmish/multiplayer**: cross-match persistence is OFF; memory lasts within the match only.

---

## 11. Faction Identity Along the Information Axis

**Design record**: PERCEPTION_WARFARE_DIRECTION.md section 4. Faction asymmetry is expressed not only
as different unit rosters but as different *relationships to information*.

| Faction | Information doctrine | Strength | Vulnerability |
| :--- | :--- | :--- | :--- |
| Soviets | Analog resilience: redundant, low-tech command chains | Degrades gracefully when jammed; hard to blind | Coarsest intel fidelity; slower to react |
| Alliance | Sensor and network superiority | Best confidence fidelity and refresh rate | Catastrophic loss of capability when infrastructure falls |
| Eastern Coalition | Deception and information attack | Cheapest and deepest access to falsification tools | Weaker in a straight attritional exchange |
| Chrono Legion | Temporal debt economy (ADR-0019) | Borrows force from its own future | Debt repayment windows; paradox penalties |

