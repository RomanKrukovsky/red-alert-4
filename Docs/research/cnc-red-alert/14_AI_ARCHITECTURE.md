# Command & Conquer: Red Alert - AI Architecture Documentation

**Source Code Path:** `/Users/romanmolodyko/cnc-red-alert-original/CODE/`
**Analysis Date:** 2025

---

## Overview

The Red Alert AI system is a **hybrid expert system** combining strategic resource management, base building, tactical team coordination, and a trigger-based mission system. The AI runs as a periodic "expert system" per computer-controlled house (player), evaluating urgency-weighted strategies and executing actions through team-based tactical units.

---

## Core AI Components

### 1. HouseClass - Strategic AI Controller

**File:** `CODE/HOUSE.H` (915 lines), `CODE/HOUSE.CPP` (7,765 lines)

The `HouseClass` is the central AI controller for each player (human or computer). Each house maintains:

#### Key AI State Variables (HOUSE.H:111-912)

```cpp
// Difficulty & IQ System
DiffType Difficulty;           // Current difficulty level
int IQ;                        // AI intelligence level (0=human, 1-5=computer)
HouseStaticClass Control;      // Scenario-static AI parameters
int Control.IQ;                // Base IQ from scenario
int Control.TechLevel;         // Buildable tech level

// Base State
StateType State;               // STATE_BUILDUP, STATE_BROKE, STATE_ATTACKED, STATE_ENDGAME
bool IsBaseBuilding;           // Auto base building enabled
bool IsAlerted;                // Base alerted - creates attack teams
bool IsStarted;                // Production enabled
COORDINATE Center;             // Base center coordinate
int Radius;                    // Average building distance from center
struct { int AirDefense, ArmorDefense, InfantryDefense; } ZoneInfo[ZONE_COUNT];

// Production Tracking
StructType BuildStructure;     // Pending building to construct
UnitType BuildUnit;            // Pending unit to construct
InfantryType BuildInfantry;    // Pending infantry to construct
AircraftType BuildAircraft;    // Pending aircraft to construct
VesselType BuildVessel;        // Pending vessel to construct

// Economy
long Tiberium, Credits, Capacity;
unsigned CreditsSpent, HarvestedCredits;

// Timers (CDTimerClass<FrameTimerClass>)
CDTimerClass RepairTimer;      // Repair action delay
CDTimerClass AlertTime;        // Auto-attack team creation delay
CDTimerClass BorrowedTime;     // Delayed trigger events
CDTimerClass DamageTime;       // Low power building damage
CDTimerClass TeamTime;         // Team creation interval
CDTimerClass TriggerTime;      // Trigger processing rate
CDTimerClass AITimer;          // Expert AI processing interval
CDTimerClass Attack;           // All-out attack timer

// Enemy & Diplomacy
HousesType Enemy;              // Primary target enemy
unsigned Allies;               // Allied houses bitmask
bool IsParanoid;               // Prevents human alliances
```

#### Main AI Loop - `HouseClass::AI()` (HOUSE.CPP:923-1363)

Called once per house per game tick. Processing order:

1. **Base Building Activation** (932-936): Enables base building if `IQ >= Rule.IQProduction`
2. **Win/Loss Conditions** (941-960): Checks `IsToWin`, `IsToLose`, `IsToDie` flags
3. **Power Management** (976-987): Low power damage to buildings
4. **Auto-Attack Teams** (984-1002): Creates attack teams when `AlertTime` expires
5. **Flag Defense** (1009-1046): Protects flag home cell
6. **Periodic Team Creation** (1052-1060): Creates teams when `TeamTime` expires
7. **Low Power Damage** (1066-1087): Damages buildings when `Power_Fraction() < 1`
8. **Player Announcements** (1101-1137): VOX alerts for low money/power
9. **Trigger Processing** (1159-1248): Runs triggers when `TriggerTime` expires
10. **Expert AI** (1313-1315): Calls `Expert_AI()` when `AITimer` expires
11. **Endgame Fire Sale** (1317-1320): Sells all buildings in `STATE_ENDGAME`
12. **Production AI** (1322-1326): Calls `AI_Building()`, `AI_Unit()`, `AI_Vessel()`, `AI_Infantry()`, `AI_Aircraft()`

#### Expert AI System - `HouseClass::Expert_AI()` (HOUSE.CPP:4581-4874)

The core expert system, called periodically (returns delay: `TICKS_PER_SECOND*5 + Random_Pick(1, TICKS_PER_SECOND/2)`).

**Processing Flow:**
1. **Enemy Selection** (4602-4729): Picks closest active enemy with base, factors in distance, kills, base size, last attacker
2. **Max Unit Limits Adjustment** (4706-4728): Scales `MaxUnit`, `MaxBuilding`, etc. to match average enemy
3. **State Transitions** (4737-4757): 
   - `STATE_ENDGAME` → Fire sale + hunt
   - `STATE_BUILDUP` → `STATE_BROKE` if money < 25
   - `STATE_BROKE` → `STATE_BUILDUP` if money >= 25
   - `STATE_ATTACKED` → `STATE_BUILDUP` after 1 minute
   - Attacked recently → `STATE_ATTACKED`
4. **Strategy Urgency Evaluation** (4764-4812): Computes urgency for 11 strategies:
   - `STRATEGY_BUILD_POWER` - Check_Build_Power()
   - `STRATEGY_BUILD_DEFENSE` - Check_Build_Defense()
   - `STRATEGY_BUILD_INCOME` - Check_Build_Income()
   - `STRATEGY_FIRE_SALE` - Check_Fire_Sale()
   - `STRATEGY_BUILD_ENGINEER` - Check_Build_Engineer()
   - `STRATEGY_BUILD_OFFENSE` - Check_Build_Offense()
   - `STRATEGY_RAISE_MONEY` - Check_Raise_Money()
   - `STRATEGY_RAISE_POWER` - Check_Raise_Power()
   - `STRATEGY_LOWER_POWER` - Check_Lower_Power()
   - `STRATEGY_ATTACK` - Check_Attack()
5. **Action Execution** (4820-4871): Processes strategies from `URGENCY_CRITICAL` down to `URGENCY_LOW`, stops at first successful action per urgency level

#### Urgency Levels
```cpp
UrgencyType: URGENCY_NONE, URGENCY_LOW, URGENCY_MEDIUM, URGENCY_HIGH, URGENCY_CRITICAL
```

#### Strategy Check Functions (HOUSE.CPP:4877-5047)

| Function | Returns | Logic |
|----------|---------|-------|
| `Check_Build_Power()` | Urgency | `Power_Fraction() < 1` → LOW; `< 3/4` → MEDIUM; under attack + chronosphere → HIGH |
| `Check_Build_Defense()` | Urgency | Always `URGENCY_NONE` (stub) |
| `Check_Build_Offense()` | Urgency | Always `URGENCY_NONE` (stub) |
| `Check_Build_Income()` | Urgency | Always `URGENCY_NONE` (stub) |
| `Check_Fire_Sale()` | Urgency | `CRITICAL` if no factories and has buildings |
| `Check_Build_Engineer()` | Urgency | Always `URGENCY_NONE` (stub) |
| `Check_Raise_Money()` | Urgency | LOW if money < 100; MEDIUM if money < 2000 & can't make money |
| `Check_Lower_Power()` | Urgency | LOW if `Power > Drain + 300` |
| `Check_Raise_Power()` | Urgency | MEDIUM if `Power_Fraction() < PowerEmergencyFraction` & `Power < Drain - 400`; HIGH if attacked |
| `Check_Attack()` | Urgency | CRITICAL if `Frame > 1min` & `Attack==0`; LOW if `STATE_ATTACKED` |

#### Strategy Action Functions (HOUSE.CPP:5049-5407)

| Function | Behavior |
|----------|----------|
| `AI_Attack()` | Sends 75% of armed units on `MISSION_HUNT`, 20% shuffle guards |
| `AI_Build_Power()` | Stub - returns false |
| `AI_Build_Defense()` | Stub - returns false |
| `AI_Build_Offense()` | Stub - returns false |
| `AI_Build_Income()` | Stub - returns false |
| `AI_Fire_Sale()` | Sells all buildings, sends all units to hunt |
| `AI_Build_Engineer()` | Stub - returns false |
| `AI_Raise_Money()` | Sells buildings in priority order (Chronosphere → Tesla) |
| `AI_Raise_Power()` | Sells power-draining buildings (Chronosphere → Tesla) |
| `AI_Lower_Power()` | Sells one power plant |

---

### 2. Production AI

#### Building AI - `AI_Building()` (HOUSE.CPP:5422-5761)

**Priority-based build queue using `BuildChoiceClass` heap:**

1. **Power Plants** (5470-5484): If `Power <= Drain + PowerSurplus` → Advanced Power (MEDIUM/LOW) → Power Plant
2. **Refineries** (5489-5498): Ratio-based `Round_Up(RefineryRatio * CurBuildings)` up to `RefineryLimit`
3. **Barracks** (5504-5521): Ratio-based, includes Tent fallback
4. **Kennel** (5527-5535): One max, requires income
5. **Gap Generator** (5540-5549): One max, requires full power + income
6. **War Factory** (5555-5564): Ratio-based, requires 2000+ credits or income
7. **Base Defense** (5569-5596): Pillbox/Flame Turret/Turret mix, ratio-based
8. **Air Defense** (5601-5652): Detects air threat → Radar (HIGH) → SAM (HIGH/MEDIUM) → AA Gun
9. **Tesla Coil** (5657-5666): Ratio-based, requires full power
10. **Tech Center** (5671-5688): One max (Allied/Soviet), requires full power
11. **Helipad** (5694-5707): Ratio-based, urgency based on enemy aircraft count
12. **Airstrip** (5712-5726): Ratio-based, urgency based on enemy aircraft

**Selection:** Picks highest urgency from `BuildChoice` heap (5747-5757)

#### Unit AI - `AI_Unit()` (HOUSE.CPP:5778-5905)

**Two modes:**
- **Normal (GAME_NORMAL):** Team-demand driven (5805-5873)
  - Counts units needed by active teams ×2 + prebuilt teams
  - Subtracts existing recruitable units
  - Picks most needed affordable unit
- **Base Building:** Weighted random by weapon presence (5877-5902)
  - Units with weapons = weight 20, others = 1

#### Infantry AI - `AI_Infantry()` (HOUSE.CPP:6031-6210)

Similar team-demand system with special handling:
- Engineer/Renovator/Tanya limited to 1-5 max
- Weighted random selection by assigned value (6134-6207)
- E1=3, E2=5, E3=2, E4=5, Engineer=1-count, Tanya=1-count

#### Aircraft AI - `AI_Aircraft()` (HOUSE.CPP:6227-6265)

Priority order (requires helipad/airstrip capacity):
1. Longbow (Allied helipad)
2. Hind (Soviet helipad)
3. Mig (Airstrip)
4. Yak (Airstrip)
Requires `IQ >= Rule.IQAircraft`

#### Vessel AI - `AI_Vessel()` (HOUSE.CPP:5909-6012)

Team-demand driven, similar to unit AI.

---

### 3. Tactical Team AI

**Files:** `CODE/TEAM.H` (269 lines), `CODE/TEAM.CPP` (3,076 lines), `CODE/TEAMTYPE.H` (280 lines), `CODE/TEAMTYPE.CPP` (1,879 lines)

#### Team States (TEAM.H:70-141)

```cpp
bool IsForcedActive;      // Force active regardless of strength
bool IsHasBeen;           // Ever reached full strength
bool IsFullStrength;      // At desired member count
bool IsUnderStrength;     // Below 1/3 (reinforcable) or not yet started
bool IsReforming;         // Regrouping after losses
bool IsAltered;           // Composition changed, needs recount
bool IsMoving;            // Executing mission (past buildup)
bool IsNextMission;       // Advance to next mission
bool IsLeaveMap;          // Team leaving map
bool Suspended;           // Low priority suspension
bool IsActive;            // Team object alive
FormationType Formation;  // FORMATION_NONE, etc.
```

#### Team Mission Types (TEAMTYPE.H:44-66)

```cpp
enum TeamMissionType {
    TMISSION_ATTACK,          // Attack quarry type
    TMISSION_ATT_WAYPT,       // Attack waypoint
    TMISSION_FORMATION,       // Change formation
    TMISSION_MOVE,            // Move to waypoint
    TMISSION_MOVECELL,        // Move to cell
    TMISSION_GUARD,           // Guard area
    TMISSION_LOOP,            // Jump to mission list start
    TMISSION_ATTACKTARCOM,    // Attack specific target
    TMISSION_UNLOAD,          // Unload transport
    TMISSION_DEPLOY,          // Deploy mobile building
    TMISSION_HOUND_DOG,       // Follow friendlies
    TMISSION_DO,              // Guard/sticky/area guard
    TMISSION_SET_GLOBAL,      // Set global variable
    TMISSION_INVULNERABLE,    // Temporary invulnerability
    TMISSION_LOAD,            // Load onto transport
    TMISSION_SPY,             // Spy on building
    TMISSION_PATROL           // Patrol with engagement
};
```

#### Team AI Loop - `TeamClass::AI()` (TEAM.CPP:470-870)

**Phases:**
1. **Suspension Check** (484-489): Skip if suspended
2. **Altered State Recalc** (495-572): Recalculates `IsUnderStrength`/`IsFullStrength`
   - `IsFullStrength = (Total == desired)`
   - Reinforcable: `IsUnderStrength = (Total <= desired/3)`
   - Non-reinforcable: `IsUnderStrength = !IsHasBeen`
   - Transition `UnderStrength → !UnderStrength` triggers `IsReforming`
3. **Activation** (627-652): When `IsFullStrength` or `IsForcedActive`:
   - Sets `IsMoving`, `IsHasBeen`, clears `IsUnderStrength`
   - Infantry gesture (50% chance)
   - Marks all members `IsInitiated`
4. **Center Calculation** (658-660): `Calc_Center()` for zone/member tracking
5. **Recruitment** (666-673): Recruits missing members if under strength & reinforcable
6. **Empty Team Cleanup** (679-697): Deletes team if no members & `IsHasBeen`
7. **Mission Processing** (704-869):
   - Advances mission index on `IsNextMission`
   - Sets timeout from mission data
   - Dispatches to mission handlers

#### Mission Handlers (TEAM.CPP:777-847)

| Mission | Handler | Behavior |
|---------|---------|----------|
| `TMISSION_PATROL` | `TMission_Patrol()` | Patrols between waypoints |
| `TMISSION_FORMATION` | `TMission_Formation()` | Changes team formation |
| `TMISSION_ATTACK`/`ATTACKTARCOM` | `TMission_Attack()` | Coordinates attack on target |
| `TMISSION_LOAD` | `TMission_Load()` | Loads onto transport |
| `TMISSION_DEPLOY` | `TMission_Deploy()` | Deploys mobile building |
| `TMISSION_UNLOAD` | `TMission_Unload()` | Unloads passengers |
| `TMISSION_MOVE`/`MOVECELL` | `Coordinate_Move()` | Moves to waypoint/cell |
| `TMISSION_INVULNERABLE` | `TMission_Invulnerable()` | Temporary invulnerability |
| `TMISSION_GUARD` | `Coordinate_Regroup()` | Regroups at location |
| `TMISSION_DO` | `Coordinate_Do()` | Guard/sticky/area guard |
| `TMISSION_SET_GLOBAL` | `TMission_Set_Global()` | Sets global variable |
| `TMISSION_ATT_WAYPT` | `Coordinate_Attack()` | Attacks at waypoint |
| `TMISSION_SPY` | `TMission_Spy()` | Spy on building |
| `TMISSION_HOUND_DOG` | `TMission_Follow()` | Follows friendly units |
| `TMISSION_LOOP` | `TMission_Loop()` | Jumps to mission list index |

#### Team Coordination Functions

- **`Coordinate_Attack()`** (TEAM.CPP): Assigns targets, manages engagement
- **`Coordinate_Move()`** (TEAM.CPP): Formation movement to waypoint
- **`Coordinate_Regroup()`** (TEAM.CPP): Regroups at safe location (prefers repair bay)
- **`Coordinate_Do()`** (TEAM.CPP): Guard/area guard/sticky behavior
- **`Calc_Center()`** (TEAM.CPP): Computes team geometric center & closest member

#### Team Member Management

- **`Add()`** (TEAM.CPP:891-936): Recruits unit, steals from lower priority teams
- **`Remove()`** (TEAM.CPP:1053-1158): Unlinks unit, selects new captain if needed
- **`Can_Add()`** (TEAM.CPP:961-1029): Validates recruit (alive, same house, not in radio contact, recruitable mission, priority check, aircraft ammo check)
- **`Recruit()`** (TEAM.CPP:1161-1400): Scans for matching unit types, prefers closest

#### TeamTypeClass - Team Templates (TEAMTYPE.H:112-277)

```cpp
// Team composition
int ClassCount;                          // Number of unit types (max 5)
TeamMemberClass Members[MAX_TEAM_CLASSCOUNT];  // Type + Quantity

// Mission script
int MissionCount;                        // Number of missions (max 20)
TeamMissionClass MissionList[MAX_TEAM_MISSIONS];

// Behavior flags
bool IsAutocreate;       // Created automatically by AI when alerted
bool IsPrebuilt;         // Build members before team activation
bool IsReinforcable;     // Accepts replacements for losses
bool IsSuicide;          // No retreat, fight to death
bool IsRoundAbout;       // Avoids high-threat paths
bool IsTransient;        // Auto-delete when no teams of this type exist

// Limits
unsigned char InitNum;   // Initial teams at scenario start
unsigned char MaxAllowed; // Max concurrent teams
unsigned char Fear;      // Threat avoidance level (0-255)

// Ownership & Triggers
HousesType House;                    // Owning house
CCPtr<TriggerTypeClass> Trigger;     // Trigger assigned to each member
WAYPOINT Origin;                     // Reinforcement entry waypoint
```

#### Dynamic Team Suggestion - `Suggested_New_Team()` (TEAMTYPE.CPP:419-497)

Selects team type based on available units:
1. Filters by house, `MaxAllowed`, autocreate vs alerted state
2. Randomly picks from valid choices (original scoring commented out)

---

### 4. Trigger System

**Files:** `CODE/TRIGGER.H` (121 lines), `CODE/TRIGGER.CPP` (497 lines), `CODE/TRIGTYPE.H` (152 lines), `CODE/TRIGTYPE.CPP` (1,435+ lines)

#### Trigger Architecture

```cpp
// TriggerTypeClass (template)
class TriggerTypeClass {
    PersistantType IsPersistant;  // VOLATILE, SEMIPERSISTANT, PERSISTANT
    HousesType House;             // Owner house
    TEventClass Event1, Event2;   // Primary/secondary events
    MultiStyleType EventControl;  // ONLY, AND, OR, LINKED
    TActionClass Action1, Action2; // Primary/secondary actions
    MultiStyleType ActionControl; // ONLY, AND, OR, LINKED
};

// TriggerClass (instance)
class TriggerClass {
    CCPtr<TriggerTypeClass> Class;
    TDEventClass Event1, Event2;  // Event state tracking
    int AttachCount;               // Reference count
    CELL Cell;                     // Fixed cell location (for cell-only triggers)
};
```

#### Event Types (from TEVENT.H - referenced in TRIGTYPE.CPP)

Key event types used by AI:
- `TEVENT_DESTROYED` - Object destroyed
- `TEVENT_ENTERED` - Unit entered cell/waypoint
- `TEVENT_DISCOVERED` - House discovered
- `TEVENT_BASE_DISCOVERED` - Base spotted
- `TEVENT_SPIED` - Radar spied
- `TEVENT_THIEVED` - Building captured
- `TEVENT_ATTACKED` - Base attacked
- `TEVENT_GLOBAL_SET`/`GLOBAL_CLEAR` - Global variable flags
- `TEVENT_TIMED` - Timer elapsed
- `TEVENT_LEAVES_MAP` - Team leaves map
- `TEVENT_INFILTRATED` - Spy entered building

#### Action Types (from TACTION.H - referenced in TRIGTYPE.CPP)

Key AI actions:
- `TACTION_CREATE_TEAM` - Spawns team type
- `TACTION_SET_GLOBAL` - Sets global flag
- `TACTION_CLEAR_GLOBAL` - Clears global flag
- `TACTION_ALLOW_WIN` - Permits victory
- `TACTION_PLAY_THEME` - Plays music theme
- `TACTION_PLAY_MOVIE` - Plays video
- `TACTION_PLAY_SOUND` - Plays sound
- `TACTION_SPY_MISSION` - Spy plane reveal
- `TACTION_PARA_BOMB` - Paratrooper bomb
- `TACTION_PARA_INFANTRY` - Paratrooper drop
- `TACTION_NUKE` - Nuclear strike
- `TACTION_IRON_CURTAIN` - Iron curtain
- `TACTION_CHRONOSPHERE` - Chronosphere

#### Persistence Modes

| Mode | Behavior |
|------|----------|
| `VOLATILE` (0) | Destroys self immediately after firing, detaches from all objects |
| `SEMIPERSISTANT` (1) | Tracks attachment count; fires only after ALL attached objects trigger it |
| `PERSISTANT` (2) | Never auto-destroys |

#### Multi-Event/Action Logic

```cpp
enum MultiStyleType {
    MULTI_ONLY,   // Only primary event/action
    MULTI_AND,    // Both event1 AND event2 must occur
    MULTI_OR,     // Either event1 OR event2 triggers
    MULTI_LINKED  // Event1→Action1, Event2→Action2 paired
};
```

#### Trigger Processing - `TriggerClass::Spring()` (TRIGGER.CPP)

Called from `HouseClass::AI()` (HOUSE.CPP:1243-1248) for each house trigger:
1. Evaluates Event1 (and Event2 based on `EventControl`)
2. If triggered, executes Action1 (and Action2 based on `ActionControl`)
3. Handles persistence: Volatile triggers delete after firing

---

### 5. Base Building System

**Files:** `CODE/BASE.H`, `CODE/BASE.CPP` (548 lines)

#### BaseClass - Predefined Base Layouts

Used for scenario-defined AI base construction:

```cpp
class BaseNodeClass {
    StructType Type;   // Building type
    CELL Cell;         // Map cell coordinate
};

class BaseClass {
    HouseClass *House;           // Owner
    DynamicVectorClass<BaseNodeClass> Nodes;  // Build queue
    
    BaseNodeClass* Next_Buildable();  // Returns next unbuilt node
    bool Is_Built(StructType type);   // Checks if type exists in base
    // INI save/load for scenario persistence
};
```

#### Integration with AI

In `HouseClass::AI_Building()` (HOUSE.CPP:5428-5433):
```cpp
if (Session.Type == GAME_NORMAL && Base.House == Class->House) {
    BaseNodeClass * node = Base.Next_Buildable();
    if (node) BuildStructure = node->Type;
}
```
- Scenario bases override free-form AI building
- `Next_Buildable()` returns first unbuilt node in sequence

---

### 6. AI Difficulty & IQ System

**Configuration:** `CODE/RULES.CPP` (lines 920-958)

#### IQ Levels (0-5, default MaxIQ=5)

| IQ Level | Name | Capabilities Unlocked |
|----------|------|----------------------|
| 0 | Human | Player-controlled |
| 1 | Basic | Base building, simple production |
| 2 | Guard Area | `Rule.IQGuardArea=4` - Guard area missions |
| 3 | Repair/Sell | `Rule.IQRepairSell=3` - Auto-repair, sell buildings |
| 4 | Aircraft | `Rule.IQAircraft=4` - Builds aircraft |
| 4 | Super Weapons | `Rule.IQSuperWeapons=4` - Uses superweapons |
| 5 | Production | `Rule.IQProduction=5` - Auto base building |
| 5 | Harvester | `Rule.IQHarvester=3` - Replaces harvesters |
| 5 | Crush | `Rule.IQCrush=2` - Infantry crushing |
| 5 | Scatter | `Rule.IQScatter=3` - Scatter under fire |
| 5 | Content Scan | `Rule.IQContentScan=4` - Transport scanning |
| 5 | Sell Back | `Rule.IQSellBack=2` - Sells damaged buildings |

#### Difficulty Handicaps (RULES.CPP:278-307, HOUSE.CPP:278-307)

Applied in `Assign_Handicap()`:

| Handicap | Easy | Normal | Hard |
|----------|------|--------|------|
| FirepowerBias | 0.5 | 1.0 | 1.5 |
| GroundspeedBias | 0.5 | 1.0 | 1.5 |
| AirspeedBias | 0.5 | 1.0 | 1.5 |
| ArmorBias | 0.5 | 1.0 | 1.5 |
| ROFBias (Rate of Fire) | 0.5 | 1.0 | 1.5 |
| CostBias | 2.0 | 1.0 | 0.5 |
| BuildSpeedBias | 0.5 | 1.0 | 1.5 |
| RepairDelay | Longer | Normal | Shorter |
| BuildDelay | Longer | Normal | Shorter |

**Multiplayer:** Uses `ActLike` house type biases × difficulty biases × `GameSpeedBias`

---

### 7. AI Timing Constants (RULES.CPP)

| Constant | Default | Description |
|----------|---------|-------------|
| `AttackDelay` | 5 min | Initial attack timer |
| `AutocreateTime` | 5 min | Alert team creation interval |
| `TeamDelay` | 0.6 min | Standard team creation interval |
| `DamageDelay` | (from INI) | Low power damage interval |
| `SpeakDelay` | (from INI) | VOX announcement spacing |
| `BaseSizeAdd` | 3 | Extra buildings vs largest human base |

---

### 8. Build Ratios (RULES.CPP:101-120, used in AI_Building)

| Ratio | Default | Formula |
|-------|---------|---------|
| `RefineryRatio` | 0.16 | `Round_Up(Ratio * CurBuildings)` |
| `BarracksRatio` | 0.16 | `Round_Up(Ratio * CurBuildings)` |
| `WarRatio` | 0.10 | `Round_Up(Ratio * CurBuildings)` |
| `DefenseRatio` | 0.50 | `Round_Up(Ratio * CurBuildings)` |
| `AARatio` | 0.14 | `Round_Up(Ratio * CurBuildings)` |
| `TeslaRatio` | 0.16 | `Round_Up(Ratio * CurBuildings)` |
| `HelipadRatio` | 0.12 | `Round_Up(Ratio * CurBuildings)` |
| `AirstripRatio` | 0.12 | `Round_Up(Ratio * CurBuildings)` |
| `RefineryLimit` | (from INI) | Hard cap |
| `BarracksLimit` | (from INI) | Hard cap |
| `WarLimit` | (from INI) | Hard cap |
| `DefenseLimit` | (from INI) | Hard cap |
| `AALimit` | (from INI) | Hard cap |
| `TeslaLimit` | (from INI) | Hard cap |

---

### 9. Base Defense Zones

**Zone Types** (HOUSE.H:513-517, used in HOUSE.CPP:4364-4566):

```cpp
enum ZoneType { ZONE_NORTH, ZONE_CORE, ZONE_SOUTH, ZONE_EAST, ZONE_WEST, ZONE_NONE, ZONE_COUNT=5 };
```

**ZoneInfo Tracking** (HOUSE.H:513-517, HOUSE.CPP:4462-4566):
```cpp
struct { int AirDefense, ArmorDefense, InfantryDefense; } ZoneInfo[ZONE_COUNT];
```

**Zone Assignment** (HOUSE.CPP:4549-4554):
- Each building contributes its `Anti_Air()`, `Anti_Armor()`, `Anti_Infantry()` to its zone
- `Which_Zone(COORDINATE)` determines zone by coordinate

**AI_Base_Defense** (HOUSE.CPP:5332-5407):
- Calculates average defense across zones
- Core zone weighted for defense priority
- Currently returns 0 (stub implementation)

**Last Attack Tracking** (HOUSE.H:523-527):
```cpp
int LATime;           // Frame of last attack
RTTIType LAType;      // Attacker type
ZoneType LAZone;      // Zone attacked
HousesType LAEnemy;   // Attacker house
```

---

### 10. AI Production Timing

**Factory Speed Adjustment** (FACTORY.CPP:426-431):
```cpp
// Computer IQ affects production speed
time = time * Inverse(fixed(House->IQ + Rule.MaxIQ, Rule.MaxIQ * 2));
// IQ=1: 1*(1+5)/(5*2) = 0.6x speed (faster)
// IQ=5: 1*(5+5)/(5*2) = 1.0x speed (normal)
```

**Production Queue** (HOUSE.H:617-621):
```cpp
StructType BuildStructure;
UnitType BuildUnit;
InfantryType BuildInfantry;
AircraftType BuildAircraft;
VesselType BuildVessel;
```
Only one item per category queued; cleared by `Production_Begun()` when factory starts.

---

### 11. Super Weapon AI

**Handler:** `HouseClass::Super_Weapon_Handler()` (HOUSE.CPP:1381-1411)

**Auto-Fire Logic** (HOUSE.CPP:1423-1742):
- **GPS Satellite:** Auto-fires when ready if `IQ >= Rule.IQSuperWeapons` (HOUSE.CPP:1442-1456)
- **Nuclear Missile:** Computer auto-fires when ready (HOUSE.CPP:1651-1653)
- **Chronosphere:** Enabled with Tech Center, `IQ >= Rule.IQSuperWeapons` (HOUSE.CPP:1535-1550)
- **Iron Curtain:** Enabled for USSR/Ukraine, `IQ >= Rule.IQSuperWeapons` (HOUSE.CPP:1576-1593)
- **Spy Plane:** Enabled with Airfield, `TechLevel >= SpyPlaneTechLevel` (HOUSE.CPP:1693-1699)
- **Para Bombs/Infantry:** Enabled with Airfield, auto-fires when ready (HOUSE.CPP:1709-1741)

**Power Management:** Suspends repeating superweapons when `Power_Fraction() < 1` (HOUSE.CPP:1406-1408)

---

### 12. Strategic AI Behaviors

#### Enemy Selection (Expert_AI:4607-4729)
Priority formula:
```
value = (2 * MAP_CELL_W - Distance(Center, EnemyCenter)) * 2
      + BuildingsKilled[Us] * 5
      + UnitsKilled[Us]
      + (EnemyCurUnits - OurCurUnits)
      + (EnemyCurBuildings - OurCurBuildings)
      + (EnemyCurInfantry - OurCurInfantry) / 4
      + (if LastAttacker) 100
```
Ignores enemies without active base (`!IsStarted`)

#### Base Center Calculation (HOUSE.CPP:4406-4425)
```cpp
// Recalculates Center as average of all building positions
// Radius = average distance from Center
```

#### Threat Assessment (HOUSE.CPP:4462-4566)
- `Adjust_Threat(region, threat)` - Modifies regional threat
- `Where_To_Go(FootClass*)` - Finds safe cell in zone for unit
- `Zone_Cell(ZoneType)` - Returns center cell of zone

---

### 13. AI Cheat/Handicap Systems

1. **Computer Omniscience:** `Can_Build()` returns `true` for computer in normal game (HOUSE.CPP:824)
2. **Prerequisite Tracking:** Computer uses `OldBScan` (completed buildings) vs player's `ActiveBScan` (HOUSE.CPP:847-849)
3. **Resource Cheating:** Computer sells buildings for emergency money/power (AI_Raise_Money, AI_Raise_Power)
4. **Max Unit Limits:** Dynamically scaled to match enemy averages (Expert_AI:4706-4728)
5. **Factory Speed:** Higher IQ = faster production (FACTORY.CPP:431)
6. **Paranoid Mode:** Computers ally against human if human allies with computer (HOUSE.CPP:2123-2129, 3577)

---

## File Reference Summary

| File | Lines | Purpose |
|------|-------|---------|
| `HOUSE.H` | 915 | HouseClass definition, AI state, enums |
| `HOUSE.CPP` | 7,765 | Expert AI, production AI, economy, triggers |
| `TEAM.H` | 269 | TeamClass definition, states, missions |
| `TEAM.CPP` | 3,076 | Team AI, coordination, recruitment |
| `TEAMTYPE.H` | 280 | TeamTypeClass template definition |
| `TEAMTYPE.CPP` | 1,879 | Team type management, suggestion |
| `TRIGGER.H` | 121 | TriggerClass instance definition |
| `TRIGGER.CPP` | ~500 | Trigger processing |
| `TRIGTYPE.H` | 152 | TriggerTypeClass template definition |
| `TRIGTYPE.CPP` | 1,435+ | Trigger editor, event/action enums |
| `BASE.H/CPP` | ~550 | Predefined base layouts |
| `RULES.CPP` | ~1,000 | AI constants, IQ config, difficulty |
| `SCENARIO.CPP` | 3,488 | Scenario setup, AI player init |
| `FACTORY.CPP` | ~2,000 | Production speed, IQ scaling |

---

## Key AI Flow Summary

```
Game Tick
  └── HouseClass::AI() [per house]
       ├── Base building activation (IQ check)
       ├── Win/Loss/Blowup checks
       ├── Power damage
       ├── Auto-attack teams (AlertTime)
       ├── Flag defense
       ├── Periodic teams (TeamTime)
       ├── Low power damage (DamageTime)
       ├── Player VOX alerts
       ├── Trigger processing (TriggerTime)
       ├── Expert AI (AITimer) → Expert_AI()
       │    ├── Enemy selection
       │    ├── Max unit scaling
       │    ├── State transitions
       │    ├── Strategy urgency evaluation (11 strategies)
       │    └── Action execution (highest urgency first)
       ├── Endgame fire sale
       └── Production AI
            ├── AI_Building() → priority heap
            ├── AI_Unit() → team demand / weighted random
            ├── AI_Vessel() → team demand
            ├── AI_Infantry() → team demand + weighted
            └── AI_Aircraft() → priority list (IQ gated)

Team Tick (per active team)
  └── TeamClass::AI()
       ├── Suspension check
       ├── Strength recalculation
       ├── Activation (gesture, initiate)
       ├── Center calculation
       ├── Recruitment
       ├── Empty team cleanup
       └── Mission processing → Coordinate_* handlers
```

---

## Evidence References

| Feature | File | Line Range |
|---------|------|------------|
| Expert_AI main loop | HOUSE.CPP | 4581-4874 |
| Strategy urgency checks | HOUSE.CPP | 4877-5047 |
| Strategy actions | HOUSE.CPP | 5049-5407 |
| AI_Building priority heap | HOUSE.CPP | 5422-5761 |
| AI_Unit team demand | HOUSE.CPP | 5778-5905 |
| AI_Infantry weighted | HOUSE.CPP | 6031-6210 |
| AI_Aircraft priority | HOUSE.CPP | 6227-6265 |
| Team AI main loop | TEAM.CPP | 470-870 |
| Team mission handlers | TEAM.CPP | 777-847 |
| Coordinate_Attack | TEAM.CPP | ~2000+ |
| Coordinate_Move | TEAM.CPP | ~2000+ |
| Coordinate_Regroup | TEAM.CPP | ~2000+ |
| Team recruitment | TEAM.CPP | 1161-1400 |
| TeamType suggestion | TEAMTYPE.CPP | 419-497 |
| Trigger Spring() | TRIGGER.CPP | (Spring method) |
| TriggerType events/actions | TRIGTYPE.CPP | 356-387, 1300+ |
| IQ config | RULES.CPP | 920-958 |
| Difficulty handicaps | HOUSE.CPP | 278-307 |
| Factory IQ scaling | FACTORY.CPP | 426-431 |
| Base building zones | HOUSE.CPP | 4364-4566 |
| Super weapon AI | HOUSE.CPP | 1381-1742 |

---

*Document generated from source code analysis of the original EA/Westwood C&C: Red Alert codebase.*