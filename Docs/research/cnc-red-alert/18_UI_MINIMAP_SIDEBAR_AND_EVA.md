# C&C Red Alert: UI, Sidebar, Minimap, Cursor, and EVA Systems

*Research document analyzing the original EA C&C Red Alert source code (2025 GPL release)*

---

## 1. Sidebar System (SIDEBAR.CPP / SIDEBAR.H)

### 1.1 Architecture Overview

The sidebar is the primary construction/production UI panel located on the right side of the screen (320-pixel width resolution). It is implemented as `SidebarClass` inheriting from `PowerClass` (which handles power bar display).

**File**: `CODE/SIDEBAR.CPP` (112 KB), `CODE/SIDEBAR.H` (14 KB)

### 1.2 Sidebar Structure

```
SidebarClass
├── Column[2] (StripClass) - Two vertical strips for buildable items
│   ├── Buildings column (COLUMN 0) - X=248, Y=93
│   └── Units column (COLUMN 1) - X=283, Y=93
├── Repair Button (ShapeButtonClass) - Toggles repair mode
├── Upgrade/Sell Button (ShapeButtonClass) - Toggles sell/upgrade mode  
├── Zoom Button (ShapeButtonClass) - Toggles radar zoom
├── Background Gadget (SBGadgetClass) - Captures sidebar clicks
└── Power Bar (inherited from PowerClass) - Top of sidebar
```

### 1.3 Sidebar Constants (SIDEBAR.H:54-107)

| Constant | Value | Description |
|----------|-------|-------------|
| `SIDE_X` | `320-80` (240) | Sidebar upper-left X |
| `SIDE_Y` | `7+70` (77) | Sidebar upper-left Y |
| `SIDE_WIDTH` | `SIDEBAR_WID` (80) | Full sidebar width |
| `SIDE_HEIGHT` | `200-(7+70)` (123) | Full sidebar height |
| `TOP_HEIGHT` | `13` | Top section height (repair/sell/zoom buttons) |
| `COLUMN_ONE_X` | `SIDE_X+8` (248) | First column X |
| `COLUMN_ONE_Y` | `SIDE_Y+TOP_HEIGHT` (90) | First column Y |
| `COLUMN_TWO_X` | `SIDE_X+8+((80-16)/2)+3` (283) | Second column X |
| `COLUMN_TWO_Y` | `7+70+13` (90) | Second column Y |
| `COLUMNS` | `2` | Number of side strips |

### 1.4 StripClass (Sidebar Strip) - SIDEBAR.H:141-353

Each column is a `StripClass` inheriting from `StageClass`:

**Visual Layout**:
- **4 visible slots** (`MAX_VISIBLE = 4`) at 24 pixels each (`OBJECT_HEIGHT = 24`)
- **Scroll up/down buttons** (`UpButton[2]`, `DownButton[2]`) - 16×12 pixels
- **Selection buttons** (`SelectButton[2][4]`) - Click to queue production
- **Build progress clock** - Animated translucent overlay on building item
- **Construction animation** - Frame-based clock using `ClockShapes` and `ClockTranslucentTable`

**Key Data Members** (SIDEBAR.H:219-351):
```cpp
int X, Y;                    // Strip upper-left position
int ID;                      // Strip identifier (0 or 1)
bool IsToRedraw;             // Redraw flag
bool IsBuilding;             // Construction in progress
bool IsScrollingDown;        // Scroll direction
bool IsScrolling;            // Currently scrolling
int Flasher;                 // Currently flashing slot index
int TopIndex;                // Topmost visible slot index
int Scroller;                // Queued scroll amount
int Slid;                    // Smooth scroll pixel offset (0-23)
int LastSlid;                // Previous frame Slid value
int BuildableCount;          // Number of items in strip
```

**Buildable Array** (MAX_BUILDABLES = 75):
```cpp
struct BuildType {
    int BuildableID;         // Object sub-type ID
    RTTIType BuildableType;  // Object type (building/unit/aircraft)
    int Factory;             // Linked factory manager ID
} Buildables[MAX_BUILDABLES];
```

### 1.5 Sidebar Activation & Flow

**Constructor** (SIDEBAR.CPP:145-174):
- Sets up clipping window `WINDOW_SIDEBAR` for proper scroll clipping
- Initializes two `StripClass` columns at calculated positions

**One_Time()** (SIDEBAR.CPP:222-259):
- Loads theater-specific sidebar shapes: `SIDE1NA.SHP`, `SIDE2NA.SHP`, `SIDE3NA.SHP` (NATO) or `SIDE1US.SHP` etc. (Soviet)
- Called once at game start

**Init_IO()** (SIDEBAR.CPP:303-359):
- Creates Repair, Upgrade, Zoom buttons with shapes from `REPAIR.SHP`, `SELL.SHP`, `MAP.SHP`
- Positions buttons at top of sidebar (Y=0x96/2 = 75)
- Registers buttons with input system via `Add_A_Button()`

**Activate(int control)** (SIDEBAR.CPP:965-1039):
- `control = -1`: Toggle (TAB key in DOS, always on in Win95)
- `control = 1`: Show sidebar (adjusts view dimensions, adds buttons to input system)
- `control = 0`: Hide sidebar (removes buttons, restores full view width)

**AI()** (SIDEBAR.CPP:821-911):
- Processes TAB key toggle (DOS only)
- Calls `Column[0].AI()` and `Column[1].AI()` for strip input
- Auto-enables Repair if player has buildings (`PlayerPtr->BScan`)
- Handles Repair/Upgrade/Zoom button clicks

**Draw_It()** (SIDEBAR.CPP:751-796):
- Draws 3-piece sidebar background (top/middle/bottom shapes)
- Renders Repair, Upgrade, Zoom buttons
- Calls `Column[0].Draw_It()` and `Column[1].Draw_It()`

### 1.6 StripClass AI & Interaction (SIDEBAR.CPP:1042+)

**StripClass::AI()** processes:
- Mouse clicks on scroll buttons → `Scroll()`
- Mouse clicks on select buttons → `SelectClass::Action()` → queues production
- Factory production progress updates → clock animation

**Scroll(bool up)** (SIDEBAR.CPP:709-733):
- Smooth pixel scrolling (`SCROLL_RATE = 8` DOS, `12` Win32)
- `Slid` accumulates until >= `OBJECT_HEIGHT` (24), then `TopIndex` increments
- Plays `VOC_SCOLD` sound if scroll not possible

**Factory_Link()** (SIDEBAR.CPP:473-479):
- Links factory to sidebar strip by object type
- Buildings → Column 0, Units → Column 1

**Which_Column(RTTIType)** (SIDEBAR.CPP:444-450):
```cpp
if (type == RTTI_BUILDINGTYPE || type == RTTI_BUILDING) return 0;
return 1;
```

---

## 2. Radar/Minimap System (RADAR.CPP / RADAR.H)

### 2.1 Architecture Overview

**File**: `CODE/RADAR.CPP` (108 KB), `CODE/RADAR.H` (8 KB)

`RadarClass` inherits from `DisplayClass` (which inherits from `CellClass` array for map data).

### 2.2 Radar Position & Dimensions (RADAR.CPP:151-183)

```cpp
RadWidth    = 80 * RESFACTOR;   // 80 pixels wide
RadHeight   = 70 * RESFACTOR;   // 70 pixels tall
RadX        = SeenBuff.Get_Width() - RadWidth;  // Right edge
RadY        = 7 * RESFACTOR;    // Top margin

// Win32 (high-res):
RadOffX = 6, RadOffY = 7
RadIWidth  = 146, RadIHeight = 130  // Internal render area

// DOS:
RadOffX = 4, RadOffY = 1
RadIWidth  = 72, RadIHeight = 69
```

### 2.3 Radar Activation States

**Radar_Activate(int control)** (RADAR.CPP:247-346):
| Control | Action |
|---------|--------|
| `-1` | Toggle (plays VOC_RADAR_ON/OFF) |
| `0` | Turn off (animates closing via RadarAnimFrame) |
| `1` | Turn on (animates opening) |
| `2` | Remove radar gadgets (sidebar hidden) |
| `3` | Add radar gadgets (sidebar visible) |
| `4` | Full removal (reset state) |

Animation uses `RadarAnim` shape with frames:
- `RADAR_ACTIVATED_FRAME = 22` (fully open)
- `MAX_RADAR_FRAMES = 41` (fully closed)

### 2.4 Radar Rendering Pipeline (RADAR.CPP:366-621)

**Draw_It(bool forced)** main flow:
1. **House-specific art loading**: `natoradr.shp`/`ussrradr.shp` + `nradrfrm.shp`/`uradrfrm.shp` + `PULSE.SHP`
2. **Player Names Mode** (`IsPlayerNames`): `Draw_Names()` only
3. **Spy Mode** (`IsHouseSpy`): `Draw_House_Info()` only
4. **Animating** (`IsRadarActivating`/`IsRadarDeactivating`/`IsRadarJammed`): `Radar_Anim()`
5. **Active Radar** (`IsRadarActive`):
   - **Partial update** (dirty pixels only): Processes `PixelStack[400]` 
   - **Full redraw**: Renders entire visible map to `HidPage`, then blits to `SeenBuff`

**Plot_Radar_Pixel(CELL cell)** (RADAR.CPP:1012-1122):
- Clips to radar viewport using `Cell_On_Radar()` and `In_Radar()`
- Calculates screen position: `x = RadX + RadOffX + BaseX + (cellX - RadarX) * ZoomFactor`
- **Jamming logic**: Checks `Jammed` bits per house
- **Terrain rendering**: 
  - Zoomed (ZoomFactor=3): Scales 24×24 template icons via `_TileStage.Scale()`
  - Unzoomed: Single pixel `Put_Pixel()` with `Cell_Color()`
- **Overlays**: `Render_Overlay()` for tiberium, etc.
- **Units/Infantry**: `Render_Infantry()` draws colored rectangles per house remap

### 2.5 Zoom System

**Zoom_Mode(CELL cell)** (RADAR.CPP:860-957):
- **Zoomed out** (`IsZoomed = true`): `ZoomFactor = 3`, shows ~62×62 cells
- **Zoomed in** (`IsZoomed = false`): `ZoomFactor = max(RadIWidth/MapCellWidth, RadIHeight/MapCellHeight)`, shows full map
- Centers on specified cell via `Set_Radar_Position()`
- Triggers `FullRedraw = true`

**Is_Zoomable()** (RADAR.CPP:976-985): Returns false if zoom factor would be 3 (no benefit)

### 2.6 Radar Input Handling

**RTacticalClass** (RADAR.H:165-172):
```cpp
class RTacticalClass : public GadgetClass {
    RTacticalClass() : GadgetClass(0,0,0,0, LEFTPRESS|LEFTRELEASE|LEFTHELD|LEFTUP|RIGHTPRESS, true) {}
    virtual int Action(unsigned flags, KeyNumType & key);
};
```
Captures all mouse input over radar area for:
- Click-to-center map
- Right-click context
- Drag-scrolling

**Click_In_Radar()** (RADAR.CPP:1175-1204): Converts screen coords to map cell coords accounting for zoom/base offsets.

---

## 3. Cursor System (MOUSE.CPP / MOUSE.H / DEFINES.H)

### 3.1 MouseType Enum (DEFINES.H:2529-2575)

41 distinct cursor states defined:

**Directional (8-way)**:
- `MOUSE_N`, `MOUSE_NE`, `MOUSE_E`, `MOUSE_SE`, `MOUSE_S`, `MOUSE_SW`, `MOUSE_W`, `MOUSE_NW`
- "No" variants (disabled): `MOUSE_NO_N` through `MOUSE_NO_NW`

**Action States**:
| Cursor | Description |
|--------|-------------|
| `MOUSE_NORMAL` | Default arrow |
| `MOUSE_NO_MOVE` | Invalid move target |
| `MOUSE_CAN_MOVE` | Valid move destination |
| `MOUSE_ENTER` | Enter transport/building |
| `MOUSE_DEPLOY` | Deploy MCV/structure |
| `MOUSE_CAN_SELECT` | Selectable unit |
| `MOUSE_CAN_ATTACK` | Valid attack target |
| `MOUSE_SELL_BACK` | Sell structure |
| `MOUSE_SELL_UNIT` | Sell unit |
| `MOUSE_REPAIR` | Repair mode active |
| `MOUSE_NO_REPAIR` | Invalid repair target |
| `MOUSE_NO_SELL_BACK` | Invalid sell target |
| `MOUSE_RADAR_CURSOR` | Over radar map |
| `MOUSE_NUCLEAR_BOMB` | Nuke targeting |
| `MOUSE_AIR_STRIKE` | Airstrike targeting |
| `MOUSE_DEMOLITIONS` | Demolition truck |
| `MOUSE_AREA_GUARD` | Area guard mode |
| `MOUSE_HEAL` | Medic heal |
| `MOUSE_DAMAGE` | Engineer sabotage |
| `MOUSE_GREPAIR` | Engineer repair |
| `MOUSE_STAY_ATTACK` | Force-attack |
| `MOUSE_NO_DEPLOY` | Cannot deploy here |
| `MOUSE_NO_ENTER` | Cannot enter |
| `MOUSE_NO_GREPAIR` | Cannot repair (engineer) |
| `MOUSE_CHRONO_SELECT` | Chronosphere source |
| `MOUSE_CHRONO_DEST` | Chronosphere destination |

### 3.2 Mouse Animation System (MOUSE.CPP)

**MouseStruct** (MOUSE.H:87-94):
```cpp
struct MouseStruct {
    int StartFrame;      // First animation frame
    int FrameCount;      // Number of frames
    int FrameRate;       // Ticks per frame (0 = static)
    int SmallFrame;      // Small version start frame (-1 = none)
    int X, Y;            // Hotspot offset
};
```

**MouseControl[MOUSE_COUNT]** (MOUSE.CPP:345-392): Defines animation for each cursor
- Directional cursors: 1 frame, 80ms rate, animated (8 directions)
- Action cursors: Various frame counts (2-24 frames), specific rates
- Small variants exist for most (hotspot at WD/2, HT/2)

**Animation Timer** (MOUSE.CPP:58):
```cpp
CDTimerClass<SystemTimerClass> Timer = 0;
```

**AI()** (MOUSE.CPP:253-271): Called per tick
- Advances frame when `Timer == 0`
- `Frame = (Frame + 1) % FrameCount`
- Updates cursor via `Set_Mouse_Cursor()` with current frame

### 3.3 Cursor Override API

**Set_Default_Mouse(MouseType, bool size)** (MOUSE.CPP:99-105):
- Sets `NormalMouseShape` and calls `Override_Mouse_Shape()`

**Override_Mouse_Shape(MouseType, bool wsmall)** (MOUSE.CPP:192-229):
- Handles small/large variants
- Only changes if different from current
- Resets animation timer: `Timer = control->FrameRate`

**Revert_Mouse_Shape()** (MOUSE.CPP:123-126): Restores `NormalMouseShape`

**Mouse_Small(bool)** (MOUSE.CPP:145-164): Toggles small variant for current cursor

### 3.4 Directional Cursor Logic (SCROLL.CPP:191-193)

```cpp
Override_Mouse_Shape((MouseType)(MOUSE_NO_N + control), false);  // Can't scroll
Override_Mouse_Shape((MouseType)(MOUSE_N + control), false);     // Can scroll
```
Where `control` = 0-7 for 8 directions → selects N/NE/E/SE/S/SW/W/NW variant

---

## 4. EVA Voice Notification System (AUDIO.CPP)

### 4.1 Architecture

**File**: `CODE/AUDIO.CPP` (40 KB), `CODE/AUDIO.H` (99 lines)

EVA (Electronic Video Agent) voice system is managed through:
- `Speak(VoxType voice)` - Queue voice line
- `Speak_AI()` - Process queue (called per tick)
- `Stop_Speaking()` - Cancel current + queue
- `Is_Speaking()` - Check if playing

### 4.2 Voice Lines (VOX_COUNT = 126)

**Speech Array** (AUDIO.CPP:475-600): Maps `VoxType` enum → filename (`.AUD` extension)

**Categories**:
| Range | Category | Examples |
|-------|----------|----------|
| `VOX_ACCOMPLISHED` | Mission complete | `MISNWON1.AUD` |
| `VOX_FAIL` | Mission failed | `MISNLST1.AUD` |
| `VOX_NO_FACTORY` | Can't build | `PROGRES1.AUD` |
| `VOX_CONSTRUCTION` | Building complete | `CONSCMP1.AUD` |
| `VOX_UNIT_READY` | Unit trained | `UNITRDY1.AUD` |
| `VOX_NEW_CONSTRUCT` | New build options | `NEWOPT1.AUD` |
| `VOX_DEPLOY` | Can't deploy | `NODEPLY1.AUD` |
| `VOX_STRUCTURE_DESTROYED` | Building lost | `STRCKIL1.AUD` |
| `VOX_INSUFFICIENT_POWER` | Low power | `NOPOWR1.AUD` |
| `VOX_NO_CASH` / `VOX_NEED_MO_MONEY` | No money | `NOFUNDS1.AUD` |
| `VOX_BASE_UNDER_ATTACK` | Base attack | `BASEATK1.AUD` |
| `VOX_UNABLE_TO_BUILD` | Build limit | `NOBUILD1.AUD` |
| `VOX_PRIMARY_SELECTED` | Primary building | `PRIBLDG1.AUD` |
| `VOX_LOW_POWER` | Power critical | `LOPOWER1.AUD` |
| `VOX_REINFORCEMENTS` | Reinforcements | `REINFOR1.AUD` |
| `VOX_CANCELED` | Order canceled | `CANCLD1.AUD` |
| `VOX_BUILDING` | "Building" | `ABLDGIN1.AUD` |
| `VOX_CONTROL_EXIT` | Battle control end | `BCT1.AUD` |
| `VOX_MADTANK_DEPLOYED` | MAD Tank ready | `TANK01.AUD` |
| `VOX_SOVIET_CAPTURED` | Building captured | (none) |
| `VOX_UNIT_LOST` | Unit destroyed | `UNITLST1.AUD` |
| `VOX_SELECT_TARGET` | Target needed | `SLCTTGT1.AUD` |
| `VOX_PREPARE` | Enemy approaching | `ENMYAPP1.AUD` |
| `VOX_NEED_MO_CAPACITY` | Need silos | `SILOND1.AUD` |
| `VOX_SUSPENDED` | Production paused | `ONHOLD1.AUD` |
| `VOX_REPAIRING` | Repairing | `REPAIR1.AUD` |
| `VOX_AIRCRAFT_LOST` | Aircraft lost | `AUNITL1.AUD` |
| `VOX_ALLIED_FORCES_APPROACHING` | Allied incoming | `AAPPRO1.AUD` |
| `VOX_ALLIED_APPROACHING` | Allied reinforcements | `AARRIVE1.AUD` |
| `VOX_BUILDING_INFILTRATED` | Spy in building | `BLDGINF1.AUD` |
| `VOX_CHRONO_CHARGING` | Chronosphere charging | `CHROCHR1.AUD` |
| `VOX_CHRONO_READY` | Chronosphere ready | `CHRORDY1.AUD` |
| `VOX_CHRONO_TEST` | Chrono test success | `CHROYES1.AUD` |
| `VOX_HQ_UNDER_ATTACK` | ConYard under attack | `CMDCNTR1.AUD` |
| `VOX_CENTER_DEACTIVATED` | Control center down | `CNTLDED1.AUD` |
| `VOX_CONVOY_APPROACHING` | Convoy incoming | `CONVYAP1.AUD` |
| `VOX_CONVOY_UNIT_LOST` | Convoy unit lost | `CONVLST1.AUD` |
| `VOX_EXPLOSIVE_PLACED` | Demo charge placed | `XPLOPLC1.AUD` |
| `VOX_MONEY_STOLEN` | Credits stolen | `CREDIT1.AUD` |
| `VOX_SHIP_LOST` | Naval unit lost | `NAVYLST1.AUD` |
| `VOX_SATALITE_LAUNCHED` | Spy satellite | `SATLNCH1.AUD` |
| `VOX_SONAR_AVAILABLE` | Sonar pulse ready | `PULSE1.AUD` |
| `VOX_SOVIET_FORCES_APPROACHING` | Soviet incoming | `SOVFAPP1.AUD` |
| `VOX_SOVIET_REINFORCEMENTS` | Soviet reinforcements | `SOVREIN1.AUD` |
| `VOX_TRAINING` | Training | `TRAIN1.AUD` |
| `VOX_ABOMB_READY` / `LAUNCH` / `PREPPING` | Nuclear | `AREADY1`/`ALAUNCH1`/`APREP1` |
| `VOX_ALLIES_N/S/E/W` | Directional allied | `AARRIVN1` etc. |
| `VOX_OBJECTIVE_1/2/3` | Objectives met | `1OBJMET1` etc. |
| `VOX_IRON_CHARGING` / `READY` | Iron Curtain | `IRONCHG1`/`IRONRDY1` |
| `VOX_RESCUED` | Kosygin rescued | `KOSYRES1.AUD` |
| `VOX_OBJECTIVE_NOT` | Objective failed | `OBJNMET1.AUD` |
| `VOX_SIGNAL_N/S/E/W` | Flare signals | `FLAREN1` etc. |
| `VOX_SPY_PLANE` | Spy plane | `SPYPLN1.AUD` |
| `VOX_FREED` | Tanya freed | `TANYAF1.AUD` |
| `VOX_UPGRADE_ARMOR/FIREPOWER/SPEED` | Tech upgrades | `ARMORUP1` etc. |
| `VOX_MISSION_TIMER` | Timer warning | `MTIMEIN1.AUD` |
| `VOX_UNIT_FULL` | Transport full | `UNITFUL1.AUD` |
| `VOX_UNIT_REPAIRED` | Unit repaired | `UNITREP1.AUD` |
| `VOX_TIME_40/30/20/10/5/4/3/2/1` | Countdown | `40MINR` etc. |
| `VOX_TIME_STOP` | Timer stopped | `TIMERNO1.AUD` |
| `VOX_UNIT_SOLD` | Unit sold | `UNITSLD1.AUD` |
| `VOX_TIMER_STARTED` | Timer started | `TIMERGO1.AUD` |
| `VOX_TARGET_RESCUED/FREED` | Rescue success | `TARGRES1`/`TARGFRE1` |
| `VOX_TANYA_RESCUED` | Tanya rescued | `TANYAR1.AUD` |
| `VOX_STRUCTURE_SOLD` | Building sold | `STRUSLD1.AUD` |
| `VOX_SOVIET_FORCES_FALLEN` | Soviet defeated | `SOVFORC1.AUD` |
| `VOX_SOVIET_SELECTED` | Soviet selected | `SOVEMP1.AUD` |
| `VOX_SOVIET_EMPIRE_FALLEN` | Soviet empire fallen | `SOVEFAL1.AUD` |
| `VOX_OPERATION_TERMINATED` | Op terminated | `OPTERM1.AUD` |
| `VOX_OBJECTIVE_REACHED/NOT_REACHED/MET` | Objective status | `OBJRCH1` etc. |
| `VOX_MERCENARY_RESCUED/FREED` | Mercenary | `MERCR1`/`MERCF1` |
| `VOX_KOSYGEN_FREED` | Kosygin freed | `KOSYFRE1.AUD` |
| `VOX_FLARE_DETECTED` | Flare seen | `FLARE1.AUD` |
| `VOX_COMMANDO_RESCUED/FREED` | Commando | `COMNDOR1`/`COMNDOF1` |
| `VOX_BUILDING_IN_PROGRESS` | Building placing | `BLDGPRG1.AUD` |
| `VOX_ATOM_PREPPING` | Nuke prepping | `ATPREP1.AUD` |
| `VOX_ALLIED_SELECTED` | Allied selected | `ASELECT1.AUD` |
| `VOX_ABOMB_PREPPING/LAUNCHED` | A-bomb | `APREP1`/`ATLNCH1` |
| `VOX_ALLIED_FORCES_FALLEN` | Allied defeated | `AFALLEN1.AUD` |
| `VOX_ABOMB_AVAILABLE` | Nuke available | `AAVAIL1.AUD` |
| `VOX_ALLIED_REINFORCEMENTS` | Allied reinf. | `AARRIVE1.AUD` |
| `VOX_MISSION_SAVED/LOADED` | Save/load | `SAVE1`/`LOAD1` |

### 4.3 Queue System (AUDIO.CPP:629-763)

```cpp
static VoxType CurrentVoice = VOX_NONE;
static VoxType SpeakQueue = VOX_NONE;
```

**Speak(VoxType)** (AUDIO.CPP:643-649):
- Only queues if: not quiet, volume > 0, samples loaded, not same as current/queued
- Sets `SpeakQueue = voice`

**Speak_AI()** (AUDIO.CPP:669-715):
- Called per game tick
- Checks `Is_Sample_Playing(SpeechBuffer[index])`
- **Double-buffering**: Two `SpeechBuffer[2]` slots, alternates `_index`
- **Cache system**: `SpeechRecord[2]` tracks which voice is in each buffer
- On cache hit: plays from existing buffer
- On cache miss: loads `.AUD` file via `CCFileClass` → `Play_Sample()`

**Priority**: EVA voices play at priority 254 (near maximum), volume = `Options.Volume * 256`

---

## 5. Message/Event Queue System (MSGLIST.CPP / MSGLIST.H)

### 5.1 Architecture

**File**: `CODE/MSGLIST.CPP` (57 KB), `CODE/MSGLIST.H` (5 KB)

`MessageListClass` manages:
- **Chat messages** (multiplayer)
- **EVA notifications** (single player)
- **System messages** (errors, warnings)
- **Editable input field** for chat

### 5.2 Data Structure

```cpp
#define MAX_MESSAGE_LENGTH  120
#define MAX_NUM_MESSAGES    14

TextLabelClass * MessageList;      // Linked list of messages
int MessageX, MessageY;            // Top-left position
int MaxMessages;                   // Limit (0 = unlimited)
int MaxChars;                      // Max chars per line
int Height;                        // Line height in pixels
```

**Message Buffers** (MSGLIST.H:208-209):
```cpp
char MessageBuffers[MAX_NUM_MESSAGES][MAX_MESSAGE_LENGTH + 30];
char BufferAvail[MAX_NUM_MESSAGES];
```

### 5.3 Key Functions

**Add_Message()** (MSGLIST.CPP:324-465):
- Prepends sender name: `"Name:Message"`
- Word-wraps to `Width` pixels using `String_Pixel_Width()`
- Auto-removes oldest if `MaxMessages` exceeded
- Plays `VOC_INCOMING_MESSAGE` sound
- Assigns timeout via `UserData1` (tick count)
- Returns `TextLabelClass*` for further manipulation

**Manage()** (MSGLIST.CPP:912-958):
- Called per tick
- Removes expired messages (timeout reached)
- Calls `Compute_Y()` to reposition remaining

**Input()** (MSGLIST.CPP:981-1137):
- Handles edit field for chat
- Returns codes:
  - `1` = Redraw needed
  - `2` = Full refresh
  - `3` = Send message (Enter)
  - `4` = Send overflow buffer

**Concat_Message()** (MSGLIST.CPP:560-670):
- Appends to existing message by sender+ID
- Used for multi-packet chat messages
- Trims from left if overflow (`Trim_Message()`)

### 5.4 Edit Field Features

**Add_Edit()** (MSGLIST.CPP:737-808):
- Creates `TextLabelClass` with focus
- Supports "To:" prefix for private messages
- Cursor character (`CursorChar`) blinking
- Overflow handling: trims left, stores excess in `OverflowBuf`

---

## 6. UI Gadget Hierarchy (GADGET.CPP / GADGET.H)

### 6.1 Class Hierarchy

```
LinkClass (base linked list)
└── GadgetClass
    ├── ControlClass (adds ID + peer notification)
    │   ├── ShapeButtonClass (image-based button)
    │   │   ├── TextButtonClass (text + button style)
    │   │   └── StaticButtonClass (non-interactive label)
    │   ├── GaugeClass (slider/progress bar)
    │   │   └── TriColorGaugeClass (R/Y/G zones)
    │   ├── ListClass (scrollable list box)
    │   │   └── IconListClass (icon + text rows)
    │   ├── SliderClass (scrollbar)
    │   ├── EditClass (text input)
    │   ├── CheckBoxClass
    │   └── RadioButtonClass
    └── ToggleClass (toggle button)
```

### 6.2 GadgetClass Core (GADGET.CPP:112-872)

**Constructor** (GADGET.CPP:112-118):
```cpp
GadgetClass(int x, int y, int w, int h, unsigned flags, int sticky)
```
- `Flags`: Input event mask (LEFTPRESS, LEFTHELD, LEFTRELEASE, RIGHTPRESS, KEYBOARD, etc.)
- `IsSticky`: Captures input while mouse held (standard button behavior)

**Static Members** (GADGET.H:68-87, GADGET.CPP:63-87):
- `StuckOn`: Currently captured sticky gadget
- `LastList`: Last processed gadget list (forces full redraw on change)
- `Focused`: Keyboard-focused gadget
- `ColorScheme`: Current `RemapControlType*` for drawing

**Input Processing** (GADGET.CPP:473-661):
```cpp
KeyNumType GadgetClass::Input(void)
```
1. Fetches keyboard input via `Keyboard->Check()/Get()`
2. Gets mouse position (`Get_Mouse_X/Y()` or queued click coords)
3. Builds event flags (LEFTPRESS, LEFTHELD, LEFTRELEASE, RIGHTPRESS, KEYBOARD)
4. **Sticky processing**: If `StuckOn`, only that gadget gets events
5. **Focus processing**: If `Focused` has keyboard events, routes to it
6. **List traversal**: Calls `Clicked_On()` for each gadget in chain
7. Returns processed key code (may be `KN_NONE` if consumed)

**Clicked_On()** (GADGET.CPP:196-220):
- Matches event flags against gadget's `Flags`
- Keyboard events always processed if `KEYBOARD` flag set
- Mouse events require cursor within gadget rect
- Calls virtual `Action(flags, key)`

### 6.3 Key Derived Classes

**ControlClass** (CONTROL.CPP:74-195):
- Adds `ID` (returned as `ID | KN_BUTTON` in `Action()`)
- Adds `Peer` pointer for cross-notification (`Peer_To_Peer()`)

**ShapeButtonClass** (not shown, used extensively):
- Uses `ShapeButtonClass` with `.SHP` frames for up/down states
- `IsSticky = true`, `IsToggleType` for toggle buttons
- `ReflectButtonState` syncs visual with logical state

**GaugeClass** (GAUGE.CPP:62-538):
- Horizontal/vertical slider with optional thumb
- `Set_Value()`, `Set_Maximum()`, `Pixel_To_Value()`, `Value_To_Pixel()`
- `TriColorGaugeClass` adds Red/Yellow/Green zones

**ListClass / IconListClass** (used in WOL_CHAT.CPP):
- Scrollable list with up/down buttons
- `IconListClass` supports tab stops, extra data pointers
- `MusicListClass` (SOUNDDLG.CPP) customizes entry drawing

---

## 7. Dialog & Text Rendering (DIALOG.CPP)

### 7.1 Dialog_Box() (DIALOG.CPP:74-166)

Draws 9-slice scaled dialog background using shapes:
- `DD-BKGND.SHP` (4 quarters for Win32, 1 for DOS)
- `DD-EDGE.SHP` (vertical side strips, tiled every 6px)
- `DD-LEFT.SHP`/`DD-RIGHT.SHP` (left/right borders)
- `DD-BOTM.SHP`/`DD-TOP.SHP` (bottom/top bars)
- `DD-CRNR.SHP` (4 corners)

### 7.2 Text Rendering Pipeline

**Simple_Text_Print()** (DIALOG.CPP:395-637): Core text renderer
- Supports multiple fonts: `FontPtr`, `Font3Ptr`, `Font6Ptr`, `Font8Ptr`, `GradFont6Ptr`, `VCRFontPtr`, `Metal12FontPtr`, `TypeFontPtr`, `ScoreFontPtr`, `MapFontPtr`, `FontLEDPtr`, `EditorFont`
- Gradient fonts (6pt Grad, VCR, Metal12, Type) use palette remapping
- Shadow modes: `TPF_NOSHADOW`, `TPF_DROPSHADOW`, `TPF_FULLSHADOW`, `TPF_LIGHTSHADOW`
- Alignment: `TPF_CENTER`, `TPF_RIGHT`
- Palette flags: `TPF_USE_GRAD_PAL`, `TPF_MEDIUM_COLOR`, `TPF_BRIGHT_COLOR`

**Fancy_Text_Print()** (DIALOG.CPP:665-750):
- Variadic formatting via `vsprintf()`
- Resolves text IDs via `Text_String(text_id)`
- Delegates to `Simple_Text_Print()`

**Conquer_Clip_Text_Print()** (DIALOG.CPP:782-857):
- Clips text at pixel width
- Supports `<TAB>` characters with tab stop array
- Used by list boxes

**Plain_Text_Print()** (DIALOG.CPP:881-944):
- Bypasses color scheme, uses raw RGB foreground
- Faster than `Fancy_Text_Print` for simple text

---

## 8. WOL Chat System (WOL_CHAT.CPP)

### 8.1 Dialog Layout (WOL_CHAT.CPP:73-230)

Full-screen dialog (320×200) with:
- **Chat List** (`IconListClass`): Main message area, 340×variable
- **Channel List** (`IconListClass`): Game/chat channels, 227×50 (expandable)
- **User List** (`IconListClass`): Players in channel, 227×variable (expandable)
- **Send Edit** (`EditClass`): Message input
- **Action Buttons**: Disconnect, Leave, Refresh, Squelch, Ban, Kick, Find Page, Options, Ladder, Help
- **Channel Actions**: Create Game, Join Game, Back
- **Expand Buttons**: Toggle channel/user list sizes
- **Rank Toggle**: RA/AM rank display (ShapeButtonClass)
- **Tooltips**: `ToolTipClass` system for all buttons

### 8.2 Channel Types (WOL_CHAT.CPP:1377-1566)

`ProcessChannelListSelection()` handles:
| Channel Type | Constant | Action |
|--------------|----------|--------|
| Official Chat | `CHANNELTYPE_OFFICIALCHAT` | `EnterLevel_OfficialChat()` |
| User Chat | `CHANNELTYPE_USERCHAT` | `EnterLevel_UserChat()` |
| Top Level | `CHANNELTYPE_TOP` | `EnterLevel_Top()` |
| Games List | `CHANNELTYPE_GAMES` | `EnterLevel_Games()` |
| Games of Type | `CHANNELTYPE_GAMESOFTYPE` | `EnterLevel_GamesOfType()` |
| Chat Channel | `CHANNELTYPE_CHATCHANNEL` | `EnterChannel()` |
| Game Channel | `CHANNELTYPE_GAMECHANNEL` | `EnterChannel(bGame=true)` → returns `rc=2` |
| Lobbies | `CHANNELTYPE_LOBBIES` | `EnterLevel_Lobbies()` |
| Lobby Channel | `CHANNELTYPE_LOBBYCHANNEL` | `EnterChannel(bGame=false)` |

---

## 9. Evidence Summary

| System | File | Key Functions | Line Range | Confidence |
|--------|------|---------------|------------|------------|
| Sidebar | SIDEBAR.CPP/.H | `SidebarClass::AI`, `Draw_It`, `Activate`, `StripClass::AI`, `Scroll`, `Factory_Link` | 145-1039 | ★★★★★ |
| Radar | RADAR.CPP/.H | `RadarClass::Draw_It`, `Plot_Radar_Pixel`, `Radar_Activate`, `Zoom_Mode`, `Radar_Cursor` | 103-1449 | ★★★★★ |
| Cursor | MOUSE.CPP/.H, DEFINES.H | `MouseClass::AI`, `Override_Mouse_Shape`, `MouseStruct`, `MouseControl[]` | 58-392, 2529-2575 | ★★★★★ |
| EVA Voice | AUDIO.CPP | `Speak`, `Speak_AI`, `Speech[]` array, `Stop_Speaking` | 475-763 | ★★★★★ |
| Message Queue | MSGLIST.CPP/.H | `Add_Message`, `Manage`, `Input`, `Concat_Message` | 324-1235 | ★★★★★ |
| Gadget Hierarchy | GADGET.CPP/.H | `GadgetClass::Input`, `Clicked_On`, `Action`, `Sticky_Process` | 112-872 | ★★★★★ |
| Gauges | GAUGE.CPP/.H | `GaugeClass::Draw_Me`, `Action`, `Set_Value`, `TriColorGaugeClass` | 62-538 | ★★★★★ |
| Dialogs/Text | DIALOG.CPP | `Dialog_Box`, `Simple_Text_Print`, `Fancy_Text_Print`, `Conquer_Clip_Text_Print` | 74-998 | ★★★★★ |
| WOL Chat | WOL_CHAT.CPP | `WOL_Chat_Dialog`, `ProcessChannelListSelection`, `EnterChannel` | 73-1581 | ★★★★★ |

---

## 10. Key Design Patterns Observed

1. **Sticky Gadget Capture**: `GadgetClass::StuckOn` implements Windows-style button capture
2. **Double-Buffered Speech**: `SpeechBuffer[2]` with `SpeechRecord[2]` cache for EVA
3. **Smooth Scroll Animation**: `Slid`/`LastSlid` pixel accumulation in sidebar strips
4. **Partial Radar Updates**: `PixelStack[400]` dirty-rect system for performance
5. **Theater-Specific Art**: Sidebar/radar shapes loaded per theater (NATO/Soviet) + house
6. **Resolution Factor**: `RESFACTOR` scales all coordinates for 640×400 vs 320×200
7. **Input Event Masking**: Each gadget declares `Flags` for events it handles
8. **Peer Notification**: `ControlClass::Make_Peer()` enables slider↔list coupling
9. **Font/Palette Abstraction**: `RemapControlType` + gradient font palettes for UI theming
10. **State-Driven Cursor**: `MouseClass` maintains `CurrentMouseShape` + `NormalMouseShape` with override stack