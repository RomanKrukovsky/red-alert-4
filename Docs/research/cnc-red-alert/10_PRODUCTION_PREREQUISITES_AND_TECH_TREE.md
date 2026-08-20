# Red Alert Production, Prerequisites, and Tech Tree Systems

*Research based on original EA C&C Red Alert source code at `<home>/cnc-red-alert-original/CODE/`*

---

## 1. Production Queue Architecture

### 1.1 FactoryClass: Core Production Engine (`FACTORY.H`, `FACTORY.CPP`)

```cpp
class FactoryClass : private StageClass {
    int Balance;           // Remaining cost to pay
    int OriginalBalance;   // Full cost at production start
    TechnoClass *Object;   // Object in limbo during production
    int SpecialItem;       // Non-object production (superweapons)
    HouseClass *House;     // Owner house
    // Inherited from StageClass:
    //   Fetch_Stage() / Set_Stage(step)  -- 0 to STEP_COUNT (54)
    //   Fetch_Rate() / Set_Rate(ticks)   -- production speed
};
```

**Key constant** (`FACTORY.H:92-93`):
- `STEP_COUNT = 54` — All production divided into 54 discrete steps

### 1.2 Production Lifecycle

| Phase | Function | Description |
|-------|----------|-------------|
| **Setup** | `Set(type, house)` | Creates object in limbo; `Balance = Cost * CostBias` |
| **Start** | `Start()` | Validates funds; sets rate from `Time_To_Build()` + power |
| **Tick** | `AI()` | Called per frame; pays `Cost_Per_Tick()` |
| **Complete** | `Has_Completed()` | Returns true when `Fetch_Stage() == STEP_COUNT` |
| **Finalize** | `Completed()` | Clears factory; object ready for placement |

**Source**: `FACTORY.CPP:201-239`, `FACTORY.CPP:411-448`

### 1.3 Cost Payment System (`FACTORY.CPP:615-627`)

```cpp
int FactoryClass::Cost_Per_Tick() {
    int steps = STEP_COUNT - Fetch_Stage();
    return steps ? Balance / steps : Balance;  // Even distribution
}

// In AI():
int cost = Cost_Per_Tick();
cost = min(cost, Balance);
if (cost > House->Available_Money()) {
    Set_Stage(Fetch_Stage() - 1);  // Step back, wait for funds
} else {
    House->Spend_Money(cost);
    Balance -= cost;
}

// On completion:
House->Spend_Money(Balance);  // Pay any remainder
Balance = 0;
```

- **Exact payment**: Balance divided across remaining steps ensures precise total
- **Insufficient funds**: Reverts one stage, retries next tick
- **No partial credit loss**: Money only spent when stage advances

---

## 2. Build Speed Determination

### 2.1 Rate Calculation (`FACTORY.CPP:430-443`)

```cpp
int time = Object->Time_To_Build();  // From object type definition

// Computer IQ slowdown
if (!House->IsHuman && Rule.Diff[House->Difficulty].IsBuildSlowdown) {
    time = time * Inverse(fixed(House->IQ + Rule.MaxIQ, Rule.MaxIQ * 2));
}

// Power fraction effect (bounded 1/16 to 1.0)
int rate = time / Bound(House->Power_Fraction(), fixed(1,16), fixed(1));
rate /= STEP_COUNT;
rate = Bound(rate, 1, 255);
Set_Rate(rate);
```

### 2.2 Power Fraction (`HOUSE.H:714`, `HOUSE.CPP:2001-2010`)

```cpp
fixed HouseClass::Power_Fraction(void) const {
    if (Drain == 0) return fixed(1);  // No power consumers = full speed
    return fixed(Power, Drain);        // Ratio, bounded to 1/16 minimum
}
```

| Power/Drain | Speed Modifier |
|-------------|----------------|
| ≥ 1.0 (surplus) | 100% |
| 0.75 | 75% |
| 0.5 | 50% |
| 0.25 | 25% |
| ≤ 0.0625 (1/16) | 6.25% (minimum) |
| 0 | 0% (halts) |

### 2.3 Additional Speed Modifiers

| Modifier | Source | Effect |
|----------|--------|--------|
| `BuildSpeedBias` | `HOUSE.H:136` | Difficulty/handicap multiplier |
| `BuildDelay` | `HOUSE.H:138` | Difficulty delay offset |
| `IQ` (computer) | `RULES.H:325` | Lower IQ = slower (up to 2× at IQ=0) |
| `GameSpeedBias` | `RULES.H:271` | Global speed multiplier |

---

## 3. Human vs Computer Production Model

### 3.1 Human: Single Active Queue Per Type (`HOUSE.H:474-478`)

```cpp
int AircraftFactory;   // Active factory index (-1 = none)
int InfantryFactory;
int UnitFactory;
int VesselFactory;
int BuildingFactory;
```

- **One factory active per category** at any time
- Sidebar selection → `Begin_Production()` → `Set()` → `Start()`
- Buildings enter placement mode after completion

### 3.2 Computer: Parallel Factories (`HOUSE.H:456-460`)

```cpp
int AircraftFactories;  // Count of active factories
int InfantryFactories;
int UnitFactories;
int VesselFactories;
int BuildingFactories;
```

- Each production building gets its own `FactoryClass` instance
- Multiple factories produce simultaneously
- AI assigns via `Begin_Production(RTTIType, id)`

### 3.3 Factory Mapping (`HOUSE.H:691-692`)

```cpp
FactoryClass * Fetch_Factory(RTTIType rtti) const;
void Set_Factory(RTTIType rtti, FactoryClass * factory);
```

- Maps object type (RTTI) → active `FactoryClass`
- Used to track/find factory for a given production type

---

## 4. Prerequisite System

### 4.1 Bitmask Prerequisites (`TYPE.H:460`, `HOUSE.CPP:851-877`)

Each `TechnoTypeClass` has a `Prerequisite` bitmask:

```cpp
long Prerequisite;  // STRUCTF_* flags required to build
```

### 4.2 Building Flag Providers (`STRUCTF_*`)

| Flag | Provided By | Enables |
|------|-------------|---------|
| `STRUCTF_POWER` | Power Plant, Adv. Power Plant | Basic structures |
| `STRUCTF_ADVANCED_POWER` | Adv. Power Plant | High-tech structures |
| `STRUCTF_REFINERY` | Ore Refinery | Harvesters, War Factory |
| `STRUCTF_BARRACKS` | Barracks | Infantry, Medic, Engineer |
| `STRUCTF_WAR` | War Factory | Vehicles, Tanks |
| `STRUCTF_TECH` | Tech Center | Superweapons, advanced units |
| `STRUCTF_RADAR` | Radar Dome | Spy Plane, GPS, Parabombs |
| `STRUCTF_AIRSTRIP` | Airstrip | Yak, Mig, Transport |
| `STRUCTF_HELIPAD` | Helipad | Longbow, Hind |
| `STRUCTF_SOVIET_TECH` | Soviet Tech Center | Soviet high-tech |
| `STRUCTF_ADVANCED_TECH` | Allied Tech Center | Allied high-tech |

### 4.3 Prerequisite Resolution (`HOUSE.CPP:851-877`)

```cpp
bool HouseClass::Can_Build(ObjectTypeClass const * type, HousesType house) const {
    // 1. Tech level -1 = never buildable
    if (((TechnoTypeClass const *)type)->Level == -1) return false;
    
    // 2. Computer can always build (single player)
    if (!IsHuman && Session.Type == GAME_NORMAL) return true;
    
    // 3. Ownability check (house bitmask)
    if (((1L << house) & type->Get_Ownable()) == 0) return false;
    
    // 4. Active building scan
    long flags = IsHuman ? ActiveBScan : OldBScan;
    
    // 5. Advanced power implies basic power
    if (flags & STRUCTF_ADVANCED_POWER) flags |= STRUCTF_POWER;
    
    // 6. Tech centers unified in multiplayer
    if (Session.Type != GAME_NORMAL) {
        if ((flags & (STRUCTF_SOVIET_TECH|STRUCTF_ADVANCED_TECH)) != 0)
            flags |= STRUCTF_SOVIET_TECH|STRUCTF_ADVANCED_TECH;
    }
    
    // 7. Check all prerequisites met AND tech level sufficient
    int pre = ((TechnoTypeClass const *)type)->Prerequisite;
    int level = Control.TechLevel;
    
    return ((pre & flags) == pre && type->Level <= level);
}
```

### 4.4 Special Prerequisite Rules

| Rule | Condition | Effect |
|------|-----------|--------|
| Advanced Power → Power | Always | `ADVANCED_POWER` sets `POWER` bit |
| Tech Centers Unified | Multiplayer only | Soviet + Allied = both flags |
| Ownability | Per-object `Get_Ownable()` | House bitmask check |
| Tech Level Cap | Per-object `Level` | Must have `Control.TechLevel >= Level` |

---

## 5. Tech Tree and Tech Levels

### 5.1 House Tech Level (`HOUSE.H:74`, `HOUSE.H:303-311`, `SCENARIO.CPP:2616`)

```cpp
// In HouseStaticClass (scenario-defined):
int TechLevel;  // Default 1

// In HouseClass (current, can change):
HouseStaticClass Control;  // Contains TechLevel
```

**Scenario initialization** (`SCENARIO.CPP:2616`):
```cpp
housep->Control.TechLevel = _build_tech[BuildLevel];
```

### 5.2 Object Tech Levels

Each object type has a `Level` field (`TYPE.H` via `TechnoTypeClass`):

```cpp
int Level;  // -1 = never, 0 = always, 1+ = requires tech level
```

**Typical progression** (from AI logic `HOUSE.CPP:5790, 5925, 6141, 6236-6257`):

| Level | Examples |
|-------|----------|
| 1 | Power Plant, Refinery, Barracks, Harvester |
| 2 | Radar Dome, Helipad, Airstrip |
| 3 | War Factory, Service Depot |
| 4 | Tech Center, Adv. Power Plant, Tesla Coil, Medium Tank |
| 5 | Chronosphere, Iron Curtain, Nuke Silo, Mammoth Tank |
| 6+ | Superweapons, advanced aircraft |

### 5.3 Superweapon Tech Requirements (`RULES.H:242, 307, 313, 319, 873`, `HOUSE.CPP:1465-1735`)

| Superweapon | Tech Requirement | Building Required |
|-------------|------------------|-------------------|
| GPS Satellite | `GPSTechLevel` (default 0) | Tech Center |
| Chronosphere | `ChronoTechLevel` (default 1) | Chronosphere |
| Iron Curtain | `TechLevel >= IronCurtain.Level` | Iron Curtain |
| Nuclear Missile | `TechLevel >= Nuke.Level` | Missile Silo |
| Parabombs | `ParaBombTechLevel` (10) | Airstrip |
| Parainfantry | `ParaInfantryTechLevel` (10) | Airstrip |
| Spy Plane | `SpyPlaneTechLevel` (10) | Airstrip + Radar |

**AI checks** (`HOUSE.CPP`):
```cpp
// GPS
Control.TechLevel >= Rule.GPSTechLevel && (ActiveBScan & STRUCTF_TECH)

// Spy Plane
(ActiveBScan & STRUCTF_AIRSTRIP) && Control.TechLevel >= Rule.SpyPlaneTechLevel

// Parabombs
(ActiveBScan & STRUCTF_AIRSTRIP) && Control.TechLevel >= Rule.ParaBombTechLevel
```

---

## 6. Production Limits and AI Logic

### 6.1 AI Build Limits (`RULES.H:109-175`, `RULES.CPP:118-120, 738-739`)

| Limit | Default | Purpose |
|-------|---------|---------|
| `RefineryLimit` | 4 | Max refineries |
| `RefineryRatio` | 0.16 | % of base as refineries |
| `WarLimit` | 2 | Max War Factories |
| `WarRatio` | 0.10 | % as war factories |
| `BarracksLimit` | 2 | Max Barracks |
| `BarracksRatio` | 0.16 | % as barracks |
| `AirstripLimit` | 5 | Max Airstrips |
| `AirstripRatio` | 0.12 | % as airstrips |
| `HelipadLimit` | 5 | Max Helipads |
| `HelipadRatio` | 0.12 | % as helipads |
| `DefenseLimit` | 40 | Max defensive structures |
| `TeslaLimit` | 10 | Max Tesla Coils |
| `AALimit` | 10 | Max AA guns |
| `BaseSizeAdd` | 3 | Computer base = largest human + 3 |

### 6.2 AI Urgency System (`HOUSE.H:725-733`, `HOUSE.CPP:852-863`)

```cpp
struct BuildChoiceClass {
    UrgencyType Urgency;
    StructType Structure;
};
static TFixedIHeapClass<BuildChoiceClass> BuildChoice;  // Priority heap

// Urgency check functions:
Check_Build_Power()      // Need more power
Check_Build_Income()     // Need more money (refineries)
Check_Build_Defense()    // Base under attack
Check_Build_Offense()    // Build attack forces
Check_Build_Engineer()   // Need engineer for capture
Check_Raise_Money()      // Emergency funds low
Check_Raise_Power()      // Emergency power low
Check_Lower_Power()      // Excess power (sell plants)
Check_Fire_Sale()        // Sell everything (defeat imminent)
```

### 6.3 IQ-Based Automation (`RULES.H:324-370`, `HOUSE.CPP`)

| IQ Level | Feature |
|----------|---------|
| `IQSuperWeapons` (4) | Auto-fire superweapons |
| `IQProduction` (5) | Computer controls production |
| `IQGuardArea` (4) | Units start in guard-area mode |
| `IQRepairSell` (3) | Auto-repair/sell damaged buildings |
| `IQCrush` (2) | Auto-crush infantry with vehicles |
| `IQScatter` (3) | Units scatter from threats |
| `IQContentScan` (4) | Scan transports for best target |
| `IQAircraft` | Auto-replace lost aircraft |
| `IQHarvester` (3) | Auto-replace lost harvesters |
| `IQSellBack` | Sell damaged buildings |

---

## 7. Reinforcements and Special Production

### 7.1 Reinforcement System (`REINF.CPP:372-530`)

```cpp
bool Do_Reinforcements(TeamTypeClass const * teamtype) {
    // Creates team from teamtype definition
    // Places at map edge based on House.Control.Edge
    // Default mission: ATTACK_WAYPOINT
}
```

### 7.2 Air Reinforcements (`REINF.CPP:639-749`)

```cpp
int Create_Air_Reinforcement(HouseClass * house, AircraftType air, int number,
    MissionType mission, TARGET tarcom, TARGET navcom, InfantryType passenger) {
    // Spawns aircraft at map edge, assigns mission/target
    // Used for: parabombs, paratroopers, airstrikes
}
```

### 7.3 Special Reinforcements (`REINF.CPP:559-609`)

```cpp
bool Create_Special_Reinforcement(HouseClass * house, TechnoTypeClass const * type,
    TechnoTypeClass const * another, TeamMissionType mission, int argument) {
    // Ad-hoc: replacement harvester, airfield-ordered units
}
```

---

## 8. Selling and Capture Economics

### 8.1 Selling (`TECHNO.CPP:5743-5762`, `BUILDING.CPP:3520-3572`)

```cpp
int TechnoClass::Refund_Amount() const {
    int cost = Techno_Type_Class()->Raw_Cost() * House->CostBias;
    if (House->IsHuman) {
        cost = cost * Rule.RefundPercent;  // Typically 50%
    }
    return cost;  // Computer gets 100%
}
```

- **Human**: 50% refund (`Rule.RefundPercent` from `RULES.H:806`)
- **Computer**: 100% refund (no penalty)
- **Condition**: Computer only sells if `IsTickedOff` (enemy damaged it)

### 8.2 Building Capture (`BUILDING.CPP:3060`)

```cpp
House->Stole(Refund_Amount());  // Credits granted to new owner
```

- Capturing grants `Refund_Amount()` credits
- Previous owner retains spent credits (no deduction)

---

## 9. Multiplayer and Scenario Overrides

### 9.1 Initial Setup (`SCENARIO.CPP:2616`, `SCENARIO.H:191`)

```cpp
housep->Control.TechLevel = _build_tech[BuildLevel];
housep->Init_Data(color, house, Session.Options.Credits);
```

- `BuildLevel` (0-5) → tech level via `_build_tech[]`
- `Session.Options.Credits` = starting money (MP default 3000)

### 9.2 Multiplayer Settings (`RULES.H:408-416`)

```cpp
int MPDefaultMoney;     // 3000
int MPMaxMoney;         // Cap
bool IsMPBasesOn;       // Start with Construction Yard
bool IsMPTiberiumGrow;  // Resource regrowth
bool IsMPCrates;        // Bonus crates
```

---

## 10. Key Source Files Reference

| System | Header | Implementation | Key Structures |
|--------|--------|----------------|----------------|
| Factory production | `FACTORY.H` | `FACTORY.CPP` | `FactoryClass`, `STEP_COUNT=54` |
| House economy | `HOUSE.H` | `HOUSE.CPP` | `HouseClass`, `Control.TechLevel` |
| Prerequisites | `TYPE.H:460` | `HOUSE.CPP:851-877` | `Prerequisite` bitmask, `STRUCTF_*` |
| Tech levels | `HOUSE.H:74` | `SCENARIO.CPP:2616` | `Control.TechLevel`, object `Level` |
| Superweapons | `RULES.H:242,307,313,319,873` | `HOUSE.CPP:1465-1735` | `GPSTechLevel`, `ChronoTechLevel` |
| AI build limits | `RULES.H:109-175` | `RULES.CPP:118-120,738-739` | `*Limit`, `*Ratio` |
| Build speed | `FACTORY.CPP:430-443` | `HOUSE.CPP:2001` | `Power_Fraction()`, `BuildSpeedBias` |
| Selling | `TECHNO.H:284` | `TECHNO.CPP:5743-5762` | `Refund_Amount()`, `RefundPercent` |
| Reinforcements | — | `REINF.CPP:372-749` | `Do_Reinforcements()`, `Create_Air_Reinforcement()` |

---

## 11. Evidence Index

| Concept | File | Line Range | Confidence |
|---------|------|------------|------------|
| Factory STEP_COUNT=54 | `FACTORY.H` | 92-93 | High |
| Cost_Per_Tick | `FACTORY.CPP` | 615-627 | High |
| Build speed formula | `FACTORY.CPP` | 430-443 | High |
| Power_Fraction | `HOUSE.CPP` | 2001-2010 | High |
| Human single queue | `HOUSE.H` | 474-478 | High |
| Computer multi-queue | `HOUSE.H` | 456-460 | High |
| Prerequisite bitmask | `HOUSE.CPP` | 851-877 | High |
| Tech level check | `HOUSE.CPP` | 865-876 | High |
| Refund percentage | `TECHNO.CPP` | 5757-5760 | High |
| AI build limits | `RULES.H` | 109-175 | High |
| IQ production | `RULES.H` | 324-370 | High |
| Reinforcements | `REINF.CPP` | 372-530 | High |
| Air reinforcements | `REINF.CPP` | 639-749 | High |
| Scenario tech level | `SCENARIO.CPP` | 2616 | High |

---

*Document generated from source code analysis. Line numbers may vary slightly between versions.*