# Game Design Document (GDD) (`GAME_DESIGN_DOCUMENT.md`)

**Document Version**: 2.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  
**Engine**: Deterministic 60Hz C++ Simulation Core + Unreal Engine 5 Presentation  

---

## 1. Product Pillars & Player Fantasy

### Product Pillars
1. **Deterministic Lockstep Precision**: 60Hz lockstep engine providing zero input latency feel and 100% reproducible replays.
2. **Asymmetric Industrial Warfare**: Four distinct factions featuring unique economic engines, construction mechanisms, and superweapons.
3. **Physics-Driven Battlefield Destruction**: Interactive terrain elevation, cover mechanics, bridge destruction, and dynamic structural collapse.
4. **Accessible Macro with Uncapped Micro Ceiling**: Streamlined base building and resource queues combined with deep micro control (formations, focus fire, ability micro, direct possession).

### Player Fantasy
The player steps into the role of a **Supreme Commander** directing mass industrial war machines, tactical strike forces, and superweapons from an orbital battlefield interface.

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
