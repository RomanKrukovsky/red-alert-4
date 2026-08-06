# Red Alert Building Placement and Construction Systems

*Research based on original EA C&C Red Alert source code at `<home>/cnc-red-alert-original/CODE/`*

---

## 1. Building Placement Validation

### 1.1 Core Placement Check: `CellClass::Is_Clear_To_Build()` (`CELL.H:239`, `CELL.CPP:453-513`)

The primary function determining if a building can be placed on a cell:

```cpp
bool CellClass::Is_Clear_To_Build(SpeedType loco = SPEED_TRACK) const
```

**Check sequence (stops at first failure):**

| # | Check | Condition | Blocks? |
|---|-------|-----------|---------|
| 1 | Scenario Init | `ScenarioInit == true` | **No** (always allowed during init) |
| 2 | Object Occupancy | `Cell_Object() != NULL` | Yes |
| 3 | Flag Object | `IsFlagged` (with `FIXIT_FLAG_CHECK`) | Yes |
| 4 | Wall/Flag Overlay | `Overlay != OVERLAY_NONE` AND (`IsWall` OR `Overlay == OVERLAY_FLAG_SPOT`) | Yes |
| 5 | Bib/Smudge | `Smudge != SMUDGE_NONE` AND `IsBib` | Yes |
| 6 | Bridge | `loco == SPEED_NONE` AND `Is_Bridge_Here()` | Yes |
| 7 | Terrain Buildable | `loco == SPEED_NONE` AND `!Ground[Land_Type()].Build` | Yes |
| 8 | Terrain Passable | `loco != SPEED_NONE` AND `Ground[Land_Type()].Cost[loco] == 0` | Yes |

### 1.2 Locomotion Types for Placement (`TYPE.H`)

| SpeedType | Used For | Placement Check |
|-----------|----------|-----------------|
| `SPEED_NONE` | **Buildings** | Full buildability + passability |
| `SPEED_TRACK` | Tracked vehicles | Terrain track cost > 0 |
| `SPEED_WHEEL` | Wheeled vehicles | Terrain wheel cost > 0 |
| `SPEED_FOOT` | Infantry | Terrain foot cost > 0 |
| `SPEED_HOVER` | Hovercraft | Terrain hover cost > 0 |
| `SPEED_FLOAT` | Naval units | Terrain float cost > 0 |
| `SPEED_AMPHIB` | Amphibious | Both land/water |

**Source**: `CELL.CPP:453-513`, `CELL.H:239`

### 1.3 Terrain Buildability (`CELL.CPP:503`, `TYPE.H` Ground data)

```cpp
return(::Ground[Land_Type()].Build);  // For SPEED_NONE (buildings)
```

- Each `LandType` has `Build` boolean in ground type table
- Water, cliffs, rough terrain = `Build = false`
- Clear ground, pavement = `Build = true`

### 1.4 Cell Occupancy Flags (`CELL.H:191-203`)

```cpp
struct {
    unsigned Center:1;    // Building center
    unsigned NW:1;        // Northwest corner
    unsigned NE:1;        // Northeast corner
    unsigned SW:1;        // Southwest corner
    unsigned SE:1;        // Southeast corner
    unsigned Vehicle:1;   // Vehicle passing through
    unsigned Monolith:1;  // Immovable obstacle
    unsigned Building:1;  // Building present (blocks movement)
} Occupy;
```

- **Building placement** sets `Building` bit + relevant corner bits
- **Overl
- **Vehicles** set `Vehicle` bit while moving through
- **Monolith** for indestructible map objects

---

## 2. Building Footprint and Bib System

### 2.1 Bib (Foundation) Creation (`BUILDING.CPP:3041-3057`)

On construction completion:

```cpp
CELL cell = Coord_Cell(Coord);
SmudgeType bib;
if (Class->Bib_And_Offset(bib, cell)) {
    SmudgeClass * smudge = new SmudgeClass(bib);
    smudge->Disown(cell);  // Remove any existing
    delete smudge;
    new SmudgeClass(bib, Cell_Coord(cell), 
        Class->IsBase ? House->Class->House : HOUSE_NONE);
}
```

**Key points:**
- `Bib_And_Offset()` returns bib type + cell offset from building center
- Creates `SmudgeClass` (persistent ground decal)
- Owned by building's house (or neutral for base buildings)
- Prevents other buildings on same cells

### 2.2 Building Dimensions

Buildings occupy multiple cells based on their `StructType` definition:
- **Construction Yard**: 3×3 (typically)
- **Power Plant**: 2×2
- **Refinery**: 3×2
- **Barracks/War Factory**: 3×3
- **Service Depot**: 2×3

Exact footprint defined in building type data (`BDATA.CPP`) via:
- `Foundation` size
- `Bib` offset/type
- `Occupy` bits

---

## 3. Construction Process

### 3.1 FactoryClass Production Pipeline (`FACTORY.H/CPP`)

```cpp
class FactoryClass : private StageClass {
    int Balance;           // Remaining cost
    int OriginalBalance;   // Full cost at start
    TechnoClass *Object;   // Object in limbo
    int SpecialItem;       // Non-object production
    HouseClass *House;     // Owner
    // From StageClass:
    //   Set_Stage(step) / Fetch_Stage()  -- 0 to STEP_COUNT (54)
    //   Set_Rate(ticks_per_step)
};
```

**Constants** (`FACTORY.H:92-93`):
- `STEP_COUNT = 54` — Fixed production steps

### 3.2 Production Lifecycle

| Phase | Function | Details |
|-------|----------|---------|
| **Setup** | `Set(type, house)` | Creates object in limbo; `Balance = Cost * CostBias` |
| **Start** | `Start()` | Checks funds; sets rate based on build time + power |
| **Tick** | `AI()` | Called per frame; pays `Cost_Per_Tick()` |
| **Complete** | `Has_Completed()` | `Fetch_Stage() == STEP_COUNT` |
| **Finalize** | `Completed()` | Clears factory; object ready for placement |

**Source**: `FACTORY.CPP:201-239`, `FACTORY.CPP:411-448`

### 3.3 Cost Payment System (`FACTORY.CPP:615-627`, `FACTORY.CPP:220-225`)

```cpp
int FactoryClass::Cost_Per_Tick() {
    int steps = STEP_COUNT - Fetch_Stage();
    return steps ? Balance / steps : Balance;  // Even distribution
}

// In AI():
int cost = Cost_Per_Tick();
cost = min(cost, Balance);
if (cost > House->Available_Money()) {
    Set_Stage(Fetch_Stage() - 1);  // Revert one step, wait for funds
} else {
    House->Spend_Money(cost);
    Balance -= cost;
}
```

- **Even payment**: Cost divided across remaining steps
- **Insufficient funds**: Reverts one stage, retries next tick
- **Completion**: Pays any remaining `Balance`

### 3.4 Build Speed Calculation (`FACTORY.CPP:430-443`)

```cpp
int time = Object->Time_To_Build();  // From object type data

// IQ slowdown for computer
if (!House->IsHuman && Rule.Diff[House->Difficulty].IsBuildSlowdown) {
    time = time * Inverse(fixed(House->IQ + Rule.MaxIQ, Rule.MaxIQ * 2));
}

// Power fraction effect (minimum 1/16 speed)
int rate = time / Bound(House->Power_Fraction(), fixed(1,16), fixed(1));
rate /= STEP_COUNT;
rate = Bound(rate, 1, 255);
Set_Rate(rate);
```

**Speed factors:**
| Factor | Effect |
|--------|--------|
| `Time_To_Build()` | Base build time (from type data) |
| `IQ` (computer) | Lower IQ = slower (max 2× at IQ=0) |
| `Power_Fraction()` | <100% power = slower (min 1/16) |
| `BuildSpeedBias` | Difficulty/handicap multiplier |
| `BuildDelay` | Difficulty delay offset |

---

## 4. Human vs Computer Production

### 4.1 Human Player: Single Active Queue Per Type (`HOUSE.H:474-478`)

```cpp
int AircraftFactory;   // Active factory index (-1 = none)
int InfantryFactory;
int UnitFactory;
int VesselFactory;
int BuildingFactory;
```

- **One factory active per category** at any time
- Sidebar selection calls `Begin_Production()` → `Set()` → `Start()`
- Placement mode for buildings (`Place_Object()`)

### 4.2 Computer Player: Parallel Factories (`HOUSE.H:456-460`)

```cpp
int AircraftFactories;  // Count of active factories
int InfantryFactories;
int UnitFactories;
int VesselFactories;
int BuildingFactories;
```

- Multiple factories produce simultaneously
- AI assigns production via `Begin_Production(RTTIType, id)`

### 4.3 Factory Mapping (`HOUSE.H:691-692`)

```cpp
FactoryClass * Fetch_Factory(RTTIType rtti) const;
void Set_Factory(RTTIType rtti, FactoryClass * factory);
```

- Maps object type → active `FactoryClass` instance
- Used for tracking production state per type

---

## 5. Building Placement Mode

### 5.1 Placement Flow (`HOUSE.H:656`, `HOUSE.CPP`)

```cpp
bool Place_Object(RTTIType type, CELL cell);      // Attempt placement
bool Manual_Place(BuildingClass * builder, BuildingClass * object);  // Visual mode
COORDINATE Find_Build_Location(BuildingClass * building) const;  // Auto-find
```

**Process:**
1. Player selects building from sidebar
2. `Place_Object()` validates cell via `Is_Clear_To_Build(SPEED_NONE)`
3. Enters placement mode: shows ghost building, green/red cursor
4. Player clicks valid cell → `Manual_Place()` finalizes
5. Building created at location, factory completes

### 5.2 Auto-Placement (Computer AI) (`HOUSE.H:664`, `HOUSE.CPP`)

```cpp
COORDINATE Find_Build_Location(BuildingClass * building) const;
BuildingClass * Find_Building(StructType type, ZoneType zone=ZONE_NONE) const;
CELL Zone_Cell(ZoneType zone) const;
CELL Random_Cell_In_Zone(ZoneType zone) const;
```

- AI finds location near base center, in appropriate zone
- Checks `Is_Clear_To_Build()` before placing

### 5.3 Zone-Based Placement (`HOUSE.H:511-517`)

```cpp
COORDINATE Center;    // Base center
int Radius;           // Average building distance
struct {
    int AirDefense;
    int ArmorDefense;
    int InfantryDefense;
} ZoneInfo[ZONE_COUNT];  // ZONE_NORTH, SOUTH, EAST, WEST, CORE
```

- Base divided into 5 zones (N/S/E/W/Core)
- Defense structures placed in appropriate zones
- `Which_Zone(coord/cell/object)` determines zone membership

---

## 6. Construction Yard Special Logic

### 6.1 Primary Factory (`BUILDING.CPP`, `BDATA.CPP`)

- **Construction Yard** (`STRUCT_CONYARD`) = primary building factory
- Has `IsBase = true` flag
- Creates **bib with house ownership** (not neutral)
- Undeployable → MCV (`Rule.IsMCVDeploy` in `RULES.H:543`)

### 6.2 MCV Deployment (`RULES.H:543`, `BUILDING.CPP`)

```cpp
unsigned IsMCVDeploy:1;  // If true, Construction Yard can undeploy to MCV
```

- Only enabled if `Rule.IsMCVDeploy = true`
- Undeploying removes building factory capability

---

## 7. Repair and Sell During Construction

### 7.1 Repair System (`BUILDING.CPP:3817-3900`, `RULES.H:712-721`)

```cpp
// Repair rate per tick
int RepairStep;          // Strength points per tick
fixed RepairPercent;     // Cost fraction of full price
int URepairStep;         // Unit repair step
fixed URepairPercent;    // Unit repair cost fraction

// Computer repair logic
if (!IsHuman && Rule.Diff[Difficulty].IQRepairSell) {
    // Auto-repair damaged buildings
}
```

- Repair dock/Service Depot handles vehicle repair
- Building repair: `Mission_Repair()` state machine
- Cost = `RepairPercent` of full price per full repair

### 7.2 Selling Buildings (`TECHNO.CPP:5743-5762`, `BUILDING.CPP:3520`)

```cpp
int TechnoClass::Refund_Amount() const {
    int cost = Techno_Type_Class()->Raw_Cost() * House->CostBias;
    if (House->IsHuman) {
        cost = cost * Rule.RefundPercent;  // Typically 50%
    }
    return cost;  // Computer gets 100%
}
```

- **Human**: 50% refund (`Rule.RefundPercent`)
- **Computer**: 100% refund (no penalty)
- **Condition**: Computer only sells if `IsTickedOff` (enemy damaged it)

---

## 8. Power and Construction Interaction

### 8.1 Low Power Effects on Construction

| Power Level | Build Speed | Visual |
|-------------|-------------|--------|
| ≥100% | Normal | Green power bar |
| 50-99% | Proportional slowdown | Yellow power bar |
| <50% | Severe slowdown (min 1/16) | Red flashing bar |
| 0% | Base decay damage | Critical warning |

**Implementation** (`FACTORY.CPP:434`):
```cpp
int rate = time / Bound(House->Power_Fraction(), fixed(1,16), fixed(1));
```

### 8.2 Power Bar UI (`POWER.H/CPP`)

- `PowerClass` inherits from `RadarClass`
- Renders vertical bar in sidebar
- Green = power output, Red marker = drain threshold
- Flashes when `FlashTimer > 0` (low power warning)
- Help text: "Power Output Low" when `Power_Fraction() < 1`

---

## 9. Multiplayer and Scenario Overrides

### 9.1 Starting Base (`SCENARIO.H:191`, `SCENARIO.CPP:2616`)

```cpp
int Percent;  // Computer base build percentage at start
// ...
housep->Control.TechLevel = _build_tech[BuildLevel];
housep->Init_Data(color, house, Session.Options.Credits);
```

### 9.2 Multiplayer Settings (`RULES.H:408-416`)

```cpp
int MPDefaultMoney;     // 3000
int MPMaxMoney;         // Cap
bool IsMPBasesOn;       // Start with Construction Yard
bool IsMPTiberiumGrow;  // Resource regrowth
bool IsMPCrates;        // Bonus crates enabled
```

### 9.3 Carryover Between Missions (`SCENARIO.H:169-185`)

```cpp
fixed CarryOverPercent;  // % of credits carried to next mission
int CarryOverMoney;      // Actual carried amount
int CarryOverCap;        // Maximum carryover
bool IsToCarryOver;      // Enable carryover
bool IsToInherit;        // Inherit units from previous mission
```

---

## 10. Key Source Files Reference

| System | Header | Implementation | Key Functions |
|--------|--------|----------------|---------------|
| Cell placement check | `CELL.H:239` | `CELL.CPP:453-513` | `Is_Clear_To_Build()` |
| Cell occupancy | `CELL.H:191-203` | `CELL.CPP` | `Occupy` bitfield |
| Building bib | `BUILDING.CPP:3041-3057` | — | `Bib_And_Offset()`, `SmudgeClass` |
| Factory production | `FACTORY.H` | `FACTORY.CPP` | `Set()`, `Start()`, `AI()`, `Cost_Per_Tick()` |
| Human queues | `HOUSE.H:474-478` | `HOUSE.CPP` | `BuildingFactory`, etc. |
| Computer queues | `HOUSE.H:456-460` | `HOUSE.CPP` | `UnitFactories`, etc. |
| Auto-placement | `HOUSE.H:664-665` | `HOUSE.CPP` | `Find_Build_Location()` |
| Zone system | `HOUSE.H:511-517` | `HOUSE.CPP` | `ZoneInfo`, `Which_Zone()` |
| Build speed | `FACTORY.CPP:430-443` | — | `Power_Fraction()`, `IQ` |
| Repair | `BUILDING.CPP:3817` | `RULES.H:712-721` | `RepairStep`, `RepairPercent` |
| Selling | `TECHNO.H:284` | `TECHNO.CPP:5743-5762` | `Refund_Amount()` |
| Power UI | `POWER.H` | `POWER.CPP:166-248` | `Draw_It()`, `AI()` |

---

## 11. Evidence Index

| Concept | File | Line Range | Confidence |
|---------|------|------------|------------|
| Is_Clear_To_Build | `CELL.CPP` | 453-513 | High |
| Cell occupancy bits | `CELL.H` | 191-203 | High |
| Bib creation | `BUILDING.CPP` | 3041-3057 | High |
| STEP_COUNT=54 | `FACTORY.H` | 92-93 | High |
| Cost_Per_Tick | `FACTORY.CPP` | 615-627 | High |
| Build speed formula | `FACTORY.CPP` | 430-443 | High |
| Human single queue | `HOUSE.H` | 474-478 | High |
| Computer multi-queue | `HOUSE.H` | 456-460 | High |
| Placement mode | `HOUSE.H:656` | `HOUSE.CPP` | High |
| Zone-based placement | `HOUSE.H:511-517` | `HOUSE.CPP` | High |
| Power fraction speed | `FACTORY.CPP` | 434 | High |
| Repair rates | `RULES.H` | 712-721 | High |
| Sell refund | `TECHNO.CPP` | 5743-5762 | High |
| MCV deploy | `RULES.H` | 543 | High |
| Power bar UI | `POWER.CPP` | 166-248 | High |

---

*Document generated from source code analysis. Line numbers may vary slightly between versions.*