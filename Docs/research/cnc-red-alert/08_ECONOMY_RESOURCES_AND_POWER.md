# Red Alert Economy, Resources, and Power Systems

*Research based on original EA C&C Red Alert source code at `/Users/romanmolodyko/cnc-red-alert-original/CODE/`*

---

## 1. Resource Types

### 1.1 Ore (Gold) and Gems

Red Alert features two harvestable resource types:

| Resource | Overlay Types | Value Per Bail | Notes |
|----------|---------------|----------------|-------|
| **Ore (Gold)** | `OVERLAY_GOLD1`–`OVERLAY_GOLD4` | 35 credits | Standard resource, randomly selects visual variant |
| **Gems** | `OVERLAY_GEMS1`–`OVERLAY_GEMS4` | 440 credits | High-value resource (GemValue × 4 = 110 × 4) |

**Source**: `CELL.CPP:2035-2056` — `Tiberium_Adjust()` determines value based on overlay type.

### 1.2 Resource Cell Mechanics

- Tiberium/ore grows on cells with `LAND_TIBERIUM` terrain type
- Cell value calculated by `Tiberium_Adjust(pregame)`:
  - Counts adjacent tiberium cells (8 directions)
  - Uses lookup tables `_adj[]` (ore) and `_adjgem[]` (gems) for density multiplier
  - Returns `(OverlayData + 1) * value` where `OverlayData` is 0–2 based on neighbor count
- **Source**: `CELL.CPP:2020-2081`

### 1.3 Tiberium Growth and Spread

| Function | Trigger | Description |
|----------|---------|-------------|
| `Grow_Tiberium()` | Periodic | Increases overlay stage (1→2→3→4) if cell can grow |
| `Spread_Tiberium(forced)` | Periodic | Spreads to adjacent clear cells based on `Rule.GrowthRate` |
| `Can_Tiberium_Grow()` | Check | Requires `LAND_TIBERIUM` ground type and no overlay/building |
| `Can_Tiberium_Spread()` | Check | Adjacent cell must be clear and on tiberium-suitable terrain |

**Source**: `CELL.CPP:2775-2830`

---

## 2. Harvesting System

### 2.1 Harvester Unit

- **Unit Type**: `UNIT_HARVESTER` (`UDATA.CPP:313-940`)
- **Flag**: `IsToHarvest = true` on UnitTypeClass
- **Capacity**: `BailCount = 28` bails per full load (`RULES.H:742`, `RULES.CPP:237`)
- **Animation**: Uses `Harvester_Load_List[9]` and `Harvester_Dump_List[22]` frame sequences
- **Source**: `UDATA.CPP:60-62`, `UNIT.CPP:1943-1966`

### 2.2 Harvest Cycle

1. **Loading** (at tiberium field):
   - `Harvesting()` called when stationary on tiberium cell (`UNIT.CPP:2253-2280`)
   - Each bail loads `Rule.GoldValue` or `Rule.GemValue*4` credits
   - Visual stages track via `Harvester_Load_List`

2. **Return to Refinery**:
   - Harvester seeks nearest refinery (`STRUCT_REFINERY`)
   - Docks via `Exit_Object()` / docking logic

3. **Unloading** (at refinery):
   - `BuildingClass::Mission_Harvest()` state machine (`BUILDING.CPP:3735-3796`)
   - States: `INITIAL` → `WAIT_FOR_DOCK` → `MIDDLE` (offload bails) → `WAIT_FOR_UNDOCK`
   - Each bail: `techno->Offload_Tiberium_Bail()` → `House->Harvested(bail)`
   - Credits added via `HouseClass::Harvested()` → `Tiberium += value`, `HarvestedCredits += value`

**Source**: `BUILDING.CPP:3735-3796`, `HOUSE.CPP:1813-1820`

### 2.3 Refinery Limits

| Parameter | Default | Config Location |
|-----------|---------|-----------------|
| `RefineryLimit` | 4 | `RULES.H:191`, `RULES.CPP:118` |
| `RefineryRatio` | 0.16 (16%) | `RULES.H:197`, `RULES.CPP:119` |
| `IQHarvester` | 3 | `RULES.H:375`, `RULES.CPP:152` |

Computer AI respects `RefineryLimit` and `RefineryRatio` when building.

---

## 3. Credit Storage and Economy

### 3.1 House Credit Tracking (`HOUSE.H:403-422`, `HOUSE.CPP:1813-1925`)

```cpp
long Tiberium;           // Current stored credits (displayed as "credits")
long Credits;            // Same as Tiberium (legacy naming)
long Capacity;           // Maximum storage capacity (silos + refineries)
unsigned HarvestedCredits; // Total credits harvested this game
int StolenBuildingsCredits; // Credits from captured buildings
```

### 3.2 Credit Operations

| Function | File | Description |
|----------|------|-------------|
| `Harvested(tiberium)` | `HOUSE.CPP:1813` | Adds to `Tiberium`, `HarvestedCredits`, `Capacity` |
| `Spend_Money(money)` | `HOUSE.CPP:1890` | Deducts from `Credits`, adds to `CreditsSpent` |
| `Refund_Money(money)` | `HOUSE.CPP:1921` | Adds back to `Credits` |
| `Available_Money()` | `HOUSE.H:706` | Returns `Credits` (for UI/affordability) |
| `Adjust_Capacity(adjust)` | `HOUSE.H:713` | Modifies storage capacity (silos built/destroyed) |

### 3.3 Initial Credits

- **Single Player**: Set in scenario INI per house (`HouseStaticClass::InitialCredits`, `HOUSE.H:97`)
- **Multiplayer**: `Session.Options.Credits` (default 3000, `RULES.H:408` `MPDefaultMoney`)
- Carryover between missions: `ScenarioClass::CarryOverPercent`, `CarryOverCap` (`SCENARIO.H:169-185`)

### 3.4 Credit Storage Capacity

- Each refinery/silo adds to `Capacity`
- When `Tiberium >= Capacity`: `IsMaxedOut` flag set (player notified once)
- `Tiberium_Fraction()` = `Tiberium / Capacity` for UI bar
- **Source**: `HOUSE.H:201-207`, `HOUSE.CPP:201-207`, `HOUSE.H:420-422`

---

## 4. Power System

### 4.1 Power Accounting (`HOUSE.H:467-468`)

```cpp
int Power;    // Total power output from power plants
int Drain;    // Total power consumption from buildings
```

### 4.2 Power Adjustment

| Function | File | Description |
|----------|------|-------------|
| `Adjust_Power(adjust)` | `HOUSE.H:710`, `HOUSE.CPP:1980` | Adds to `Power` (positive for plants, negative for removal) |
| `Adjust_Drain(adjust)` | `HOUSE.H:711`, `HOUSE.CPP:1990` | Adds to `Drain` (buildings consume power) |
| `Power_Fraction()` | `HOUSE.H:714`, `HOUSE.CPP:2000` | Returns `fixed(Power, Drain)` bounded to 1.0 |

### 4.3 Power Ratio Effects

| Ratio Range | Effect |
|-------------|--------|
| `Power >= Drain` (100%+) | Normal operation |
| `Power < Drain` (<100%) | Build speed reduced proportionally |
| `Power < Drain/2` (<50%) | Power bar flashes red; severe build penalty |
| `Power == 0` | Base decay: `DamageTime` timer deals damage to buildings |

**Source**: `POWER.CPP:212-219` (color coding), `HOUSE.CPP:826` (DamageTime), `RULES.H:666` `DamageDelay`

### 4.4 Power Bar UI (`POWER.H/CPP`)

- Visual bar shows power (green/yellow) vs drain (red marker)
- Animates smoothly via `PowerHeight`/`DrainHeight` with bounce effect
- Flashes when `FlashTimer > 0` (triggered by low power warnings)
- Help text: "Power Output Low" when `Power_Fraction() < 1` and `Power > 0`

**Source**: `POWER.CPP:166-248`, `POWER.CPP:437-455`

### 4.5 AI Power Management

| Function | File | Purpose |
|----------|------|---------|
| `AI_Raise_Power()` | `HOUSE.CPP:1537` | Sells buildings to reduce drain |
| `AI_Lower_Power()` | `HOUSE.CPP:1545` | Builds power plants when surplus |
| `Check_Build_Power()` | `HOUSE.H:725` | Urgency check for power construction |

---

## 5. Building Power Values

Buildings define `Power` (output) or `Drain` (consumption) in their type data:
- Power plants: positive `Power` (e.g., +200)
- Radar, tech centers, defenses: negative `Drain` (e.g., -50)
- Adjusted via `HouseClass::Adjust_Power()` / `Adjust_Drain()` when built/sold/destroyed

---

## 6. Configuration (RULES.INI)

### 6.1 Economy Settings (`RULES.H:742-744`, `RULES.CPP:237-239, 466, 477-478`)

```ini
[General]
BailCount=28           ; Bails per full harvester load
GoldValue=35           ; Credits per ore bail
GemValue=110           ; Credits per gem bail (×4 in code)
OreTruckRate=2         ; Dump speed (ticks per bail?)
OreExplosive=false     ; Harvester explodes with cargo?
```

### 6.2 Power Settings (`RULES.H:665-667`)

```ini
[General]
DamageDelay=2          ; Minutes between low-power damage ticks
```

### 6.3 AI Economy (`RULES.H:118-119, 197-198, 375`, `RULES.CPP:738-739, 948`)

```ini
[AI]
RefineryLimit=4
RefineryRatio=.16
IQHarvester=3
```

---

## 7. Key Source Files Summary

| System | Header | Implementation | Key Functions |
|--------|--------|----------------|---------------|
| House Economy | `HOUSE.H:403-422` | `HOUSE.CPP:1813-1925` | `Harvested()`, `Spend_Money()`, `Available_Money()`, `Power_Fraction()` |
| Power UI | `POWER.H:49-95` | `POWER.CPP:166-350` | `Draw_It()`, `AI()`, `Power_Height()` |
| Harvesting | `BUILDING.H:326` | `BUILDING.CPP:3735-3796` | `Mission_Harvest()` state machine |
| Tiberium | `CELL.H:275-284` | `CELL.CPP:2775-2830` | `Grow_Tiberium()`, `Spread_Tiberium()`, `Tiberium_Adjust()` |
| Rules/Config | `RULES.H:742-744` | `RULES.CPP:464-478` | `BailCount`, `GoldValue`, `GemValue`, `OreDumpRate` |

---

## 8. Evidence Index

| Concept | File | Line Range | Confidence |
|---------|------|------------|------------|
| Resource types (ore/gems) | `CELL.CPP` | 2035-2056 | High |
| Bail system | `RULES.H` | 742-744 | High |
| Harvester load/unload | `BUILDING.CPP` | 3735-3796 | High |
| Credit storage | `HOUSE.H` | 420-422 | High |
| Power accounting | `HOUSE.H` | 467-468 | High |
| Power fraction | `HOUSE.CPP` | 2000-2010 | High |
| Low power damage | `HOUSE.H` | 826 | Medium |
| Power bar UI | `POWER.CPP` | 166-248 | High |
| AI power management | `HOUSE.CPP` | 1537-1545 | High |
| Initial credits | `HOUSE.CPP` | 4123-4127 | High |
| Tiberium growth | `CELL.CPP` | 2775-2830 | High |

---

*Document generated from source code analysis. Line numbers may vary slightly between versions.*