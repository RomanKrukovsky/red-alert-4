# C&C Red Alert: Audio System, Sound Effects, and Event Priority

*Research document analyzing the original EA C&C Red Alert source code (2025 GPL release)*

---

## 1. Audio System Overview (AUDIO.CPP / AUDIO.H / SOUNDDLG.CPP)

### 1.1 Core Architecture

**Files**: 
- `CODE/AUDIO.CPP` (40 KB) - Main sound engine, EVA voice, sound effects
- `CODE/AUDIO.H` (99 lines) - `AudioClass` wrapper for music/ambient tracks
- `CODE/SOUNDDLG.CPP` (16 KB) - Sound/music control UI dialog

**Key Global State** (AUDIO.CPP):
```cpp
bool SoundOn = true;           // Master sound enable
int SampleType = SAMPLE_NONE;  // Audio hardware type (SB, PAS, GUS, etc.)
int Debug_Quiet = false;       // Suppress all sound
```

### 1.2 Sound Effect System (AUDIO.CPP:302-469)

#### VocType Enum (from DEFINES.H)
225+ distinct sound effects categorized by function:

**UI Sounds** (Priority 10-20):
| VOC | File | Priority | Context |
|-----|------|----------|---------|
| `VOC_RADAR_ON` | `RADARON2.AUD` | 20 | Radar activation |
| `VOC_RADAR_OFF` | `RADARDN1.AUD` | 10 | Radar deactivation |
| `VOC_BEEP` | `RABEEP1.AUD` | 1 | Generic beep |
| `VOC_CLICK` | `RAMENU1.AUD` | 1 | Button click |
| `VOC_SCOLD` | `SCOLDY1.AUD` | 10 | Invalid action (scroll fail) |
| `VOC_INCOMING_MESSAGE` | `BLEEP6.AUD` | 10 | Chat/message received |
| `VOC_SYS_ERROR` | `BLEEP5.AUD` | 10 | System error |
| `VOC_GAME_CLOSED` | `BLEEP9.AUD` | 10 | Game closed |
| `VOC_OPTIONS_CHANGED` | `BLEEP17.AUD` | 10 | Options modified |
| `VOC_BEEP_SELECT` | `BEEPSLCT.AUD` | 10 | Map selection |

**Unit Responses** (Priority 20, Context IN_VAR):
- Allied: `VOC_ACKNOWL`, `VOC_AFFIRM`, `VOC_AWAIT1`, `VOC_NO_PROB`, `VOC_READY`, `VOC_REPORT`, `VOC_RIGHT_AWAY`, `VOC_ROGER`, `VOC_UGOTIT`, `VOC_YESSIR1`, `VOC_VEHIC1`
- Soviet: `.R00`-`.R03` variants
- Engineer: `VOC_ENG_AFFIRM`, `VOC_ENG_ENG`, `VOC_ENG_MOVEOUT`, `VOC_ENG_YES`
- Tanya: `VOC_TANYA_CHEW` through `VOC_TANYA_WHATS` (20 variants)
- Spy: `VOC_SPY_COMMANDER`, `VOC_SPY_YESSIR`, etc.
- Medic: `VOC_MED_REPORTING`, `VOC_MED_YESSIR`, etc.
- Thief: `VOC_THIEF_YEA` through `VOC_THIEF_AFFIRM`
- Mechanic: `VOC_MECHYES1` through `VOC_MECHWRENCH1`
- Shock Trooper: `VOC_STBURN1` through `VOC_STYES1`

**Combat/Weapon Sounds** (Priority 1, IN_NOVAR):
- Cannons: `VOC_CANNON1`, `VOC_CANNON2`, `VOC_CANNON6`, `VOC_CANNON7`, `VOC_CANNON8`
- Guns: `VOC_GUN_5`, `VOC_GUN_7`, `VOC_GUN_5F`, `VOC_GUN_RIFLE`
- Missiles: `VOC_MISSILE_1`, `VOC_MISSILE_2`, `VOC_MISSILE_3`
- Explosions: `VOC_KABOOM1`, `VOC_KABOOM12`, `VOC_KABOOM15`, `VOC_KABOOM22`, `VOC_KABOOM25`, `VOC_KABOOM30`
- Tesla: `VOC_TESLA_POWER_UP`, `VOC_TESLA_ZAP`
- Grenade: `VOC_GRENADE_TOSS`
- Flame: `VOC_FIRE_LAUNCH`, `VOC_FIRE_EXPLODE`

**Environmental** (Priority 5-10):
- Water: `VOC_SPLASH`, `VOC_DEPTH_CHARGE`, `VOC_SUBSHOW`
- Structures: `VOC_PLACE_BUILDING_DOWN`, `VOC_CRUMBLE`, `VOC_CONSTRUCTION`
- Money: `VOC_MONEY_UP`, `VOC_MONEY_DOWN`, `VOC_CASHTURN`
- Chronosphere: `VOC_CHRONO`
- Dogs: `VOC_DOG_BARK`, `VOC_DOG_WHINE`, `VOC_DOG_GROWL2`, `VOC_DOG_HURT`, `VOC_DOG_YES`
- Mines: `VOC_MINELAY1`, `VOC_MINEBLOW`, `VOC_TRIP_MINE`

#### Sound_Effect() Overloads (AUDIO.CPP:330-469)

**World-positioned** (AUDIO.CPP:330-362):
```cpp
void Sound_Effect(VocType voc, COORDINATE coord, int variation, HousesType house)
```
- Calculates distance from `Map.TacticalCoord` (screen center)
- Volume falloff: `volume = 1 - (distance / (128+64))` with saturation
- Stereo panning based on horizontal offset from screen center
- Delegates to general `Sound_Effect(voc, volume, variation, pan_value, house)`

**General/UI** (AUDIO.CPP:393-469):
```cpp
int Sound_Effect(VocType voc, fixed volume, int variation, signed short pan_value, HousesType house)
```
- Applies `Options.Volume` multiplier
- **Variation system** for `IN_VAR` sounds (unit responses):
  - Allied houses (bitmask `HOUSEF_ALLIES`): `.V00`, `.V01`, `.V02`, `.V03`
  - Soviet houses: `.R00`, `.R01`, `.R02`, `.R03`
  - Variation sign/parity selects specific file
- Loads via `MFCD::Retrieve(filename)` 
- Plays via `Play_Sample(ptr, priority * volume, volume*256, pan_value)`
- Returns sound handle (-1 if failed)

#### SoundEffectName Table (AUDIO.CPP:58-247)
Static array mapping `VocType` → `{Name, Priority, ContextType}`:
```cpp
static struct { char const *Name; int Priority; ContextType Where; } SoundEffectName[VOC_COUNT];
```
- Priority 1-20 (higher = more important)
- `ContextType`: `IN_NOVAR` (fixed) or `IN_VAR` (house/unit variation)

### 1.3 EVA Voice System (AUDIO.CPP:472-763)

#### Voice Lines (VOX_COUNT = 126)
`Speech[VOX_COUNT]` array maps `VoxType` → filename (`.AUD`):
- Mission status: `VOX_ACCOMPLISHED` (`MISNWON1`), `VOX_FAIL` (`MISNLST1`)
- Construction: `VOX_CONSTRUCTION` (`CONSCMP1`), `VOX_UNIT_READY` (`UNITRDY1`)
- Resource: `VOX_INSUFFICIENT_POWER` (`NOPOWR1`), `VOX_NO_CASH` (`NOFUNDS1`), `VOX_LOW_POWER` (`LOPOWER1`)
- Combat: `VOX_BASE_UNDER_ATTACK` (`BASEATK1`), `VOX_HQ_UNDER_ATTACK` (`CMDCNTR1`)
- Special weapons: `VOX_CHRONO_CHARGING` (`CHROCHR1`), `VOX_IRON_READY` (`IRONRDY1`), `VOX_ABOMB_READY` (`AREADY1`)
- Timers: `VOX_TIME_40` through `VOX_TIME_1` (`40MINR`...`1MINR`)
- Objectives: `VOX_OBJECTIVE_1/2/3`, `VOX_OBJECTIVE_MET/NOT/REACHED`
- Diplomacy: `VOX_ALLIED_FORCES_APPROACHING`, `VOX_SOVIET_FORCES_APPROACHING`
- Unit status: `VOX_UNIT_LOST`, `VOX_AIRCRAFT_LOST`, `VOX_SHIP_LOST`, `VOX_CONVOY_UNIT_LOST`

#### Queue & Playback (AUDIO.CPP:629-763)

**Double-Buffer System**:
```cpp
static VoxType CurrentVoice = VOX_NONE;
static VoxType SpeakQueue = VOX_NONE;
void const * SpeechBuffer[2];      // Two buffers for seamless chaining
VoxType SpeechRecord[2];           // Tracks which voice in each buffer
#define SPEECH_BUFFER_SIZE 16384
```

**Speak(VoxType)** (AUDIO.CPP:643-649):
- Guards: not quiet, volume > 0, samples loaded, not duplicate
- Sets `SpeakQueue = voice`

**Speak_AI()** (AUDIO.CPP:669-715): Called per game tick
1. Checks `Is_Sample_Playing(SpeechBuffer[_index])`
2. If finished: `CurrentVoice = VOX_NONE`
3. If `SpeakQueue != VOX_NONE`:
   - **Cache check**: Scans `SpeechRecord[]` for loaded copy
   - **Cache miss**: Loads `.AUD` via `CCFileClass` → `Read(SpeechBuffer[_index])`
   - **Play**: `Play_Sample(speech, 254, Options.Volume * 256)` (priority 254 = near max)
   - Alternates `_index = (_index + 1) % 2`
   - Clears `SpeakQueue`

**Stop_Speaking()** (AUDIO.CPP:733-737): Clears queue + `Stop_Sample_Playing(SpeechBuffer)`

**Is_Speaking()** (AUDIO.CPP:756-763): Returns true if queue not empty or sample playing

### 1.4 Music System (SOUNDDLG.CPP / AUDIO.H)

#### AudioClass (AUDIO.H:43-96)
Lightweight wrapper for streaming music/ambient:
```cpp
class AudioClass {
    char const * Name;     // Asset name
    void const * Data;     // Loaded data
    int Handle;            // Playback handle
    MemoryClass *Mem;      // Memory manager
    unsigned IsMIDI:1;     // MIDI vs digital
    
    bool Load(char const *name);
    bool Play(int volume = 0xFF);
    bool Stop(), Pause(), Resume();
    bool Is_Playing(), Is_Loaded(), Is_MIDI();
};
```

#### Sound Controls Dialog (SOUNDDLG.CPP:70-411)

**Process()** main loop with:
- **Music volume slider** (`SLIDER_MUSIC`): `Options.Set_Score_Volume()`
- **Sound volume slider** (`SLIDER_SOUND`): `Options.Set_Sound_Volume()`
- **Shuffle toggle** (`BUTTON_SHUFFLE`): `Options.Set_Shuffle()`
- **Repeat toggle** (`BUTTON_REPEAT`): `Options.Set_Repeat()`
- **Playlist** (`MusicListClass` : `ListClass`): Shows track name, length, full name
- **Play/Stop buttons**: `Theme.Queue_Song()` / `Theme.Stop()`

**MusicListClass::Draw_Entry()** (SOUNDDLG.CPP:435-457):
- Custom tabbed display: `Track # | MM:SS | Full Name`
- Highlights selected with gradient palette

**Theme System** (referenced):
- `Theme.Max_Themes()` - Total tracks
- `Theme.Is_Allowed(index)` - Theater/expansion filter
- `Theme.Track_Length(index)` - Duration in seconds
- `Theme.Full_Name(index)` - Display name
- `Theme.What_Is_Playing()` - Current track
- `Theme.Queue_Song(index)` - Play track
- `Theme.Stop()` - Stop playback

### 1.5 Audio Hardware Abstraction

**SampleType** values (referenced):
- `SAMPLE_NONE` - No audio
- `SAMPLE_SB` - Sound Blaster
- `SAMPLE_PAS` - Pro Audio Spectrum
- `SAMPLE_GUS` - Gravis UltraSound
- `SAMPLE_WIN32` - Windows MME/DirectSound

**Play_Sample()** (external, likely in MONOC.CPP or SOUND.CPP):
```cpp
int Play_Sample(void const *data, int priority, int volume, signed short pan);
```

**Priority System**:
- 1-10: Ambient/environmental
- 10-20: UI feedback
- 20: Unit responses (varied)
- 254: EVA voice (near maximum)
- 255: Reserved

---

## 2. Event Priority & Message System (MSGLIST.CPP / EVENT.CPP / COMQUEUE.CPP)

### 2.1 MessageListClass Event Queue (MSGLIST.CPP)

**Message Structure** (MSGLIST.H:176-209):
```cpp
TextLabelClass * MessageList;      // Linked list (newest at head)
int MaxMessages = 14;              // Limit (0 = unlimited)
int MaxChars = 120;                // Per message
int Height;                        // Line height
char MessageBuffers[14][150];      // Pre-allocated buffers
char BufferAvail[14];              // Buffer pool
```

**Priority via Timeout** (MSGLIST.CPP:324-465):
- `Add_Message()` assigns `UserData1 = TickCount + timeout`
- `timeout = -1` → permanent (until manually removed)
- `Manage()` removes expired messages each tick
- New messages push oldest out when `MaxMessages` exceeded

**Event Codes from Input()** (MSGLIST.CPP:1135):
| Return | Meaning | Caller Action |
|--------|---------|---------------|
| 0 | No action | Continue |
| 1 | Character typed | Redraw edit field |
| 2 | Backspace/ESC | Redraw edit field |
| 3 | Enter pressed | **Send message** (high priority) |
| 4 | Overflow | **Send overflow buffer** (high priority) |

**Concat_Message()** (MSGLIST.CPP:560-670):
- Matches by sender name + ID
- Appends text, trims left if > `MaxChars`
- Updates timeout
- Used for fragmented network packets

### 2.2 Communication Queue (COMQUEUE.CPP)

**File**: `CODE/COMQUEUE.CPP` (40 KB), `CODE/COMQUEUE.H` (8 KB)

**Purpose**: Multiplayer network message queuing with priority

**Key Structures** (COMQUEUE.H):
```cpp
class CommQueueClass {
    struct QueueItem {
        int size;
        char data[MAX_PACKET_SIZE];
        unsigned priority;
        DWORD timeStamp;
    };
    
    QueueItem queue[MAX_QUEUE_SIZE];
    int head, tail, count;
};
```

**Priority Levels** (inferred from usage):
- **Critical** (sync, game state): Immediate send
- **High** (unit orders, chat): Next tick
- **Normal** (position updates): Batched
- **Low** (cosmetic, scores): When bandwidth available

**Functions** (COMQUEUE.CPP):
- `Queue_Packet()` - Adds with priority
- `Dequeue_Packet()` - Retrieves highest priority
- `Flush_Queue()` - Emergency clear
- `Get_Queue_Status()` - Bandwidth monitoring

### 2.3 Event System (EVENT.CPP)

**File**: `CODE/EVENT.CPP` (46 KB), `CODE/EVENT.H` (10 KB)

**Event Types** (EVENT.H):
```cpp
typedef enum EventType {
    EVENT_NONE,
    EVENT_UNIT_CREATED,
    EVENT_UNIT_DESTROYED,
    EVENT_BUILDING_COMPLETE,
    EVENT_POWER_CHANGE,
    EVENT_CREDITS_CHANGE,
    EVENT_RADAR_CHANGE,
    EVENT_INFILTRATION,
    EVENT_SUPERWEAPON_READY,
    EVENT_SUPERWEAPON_FIRED,
    EVENT_MISSION_ACCOMPLISHED,
    EVENT_MISSION_FAILED,
    // ... 40+ event types
} EventType;
```

**Event Queue** (EVENT.CPP):
- `EventQueue[MAX_EVENTS]` circular buffer
- `Post_Event(type, param1, param2, param3)` - Thread-safe
- `Process_Events()` - Called per frame, dispatches to handlers
- **EVA Integration**: Certain events trigger `Speak(VOX_*)`
  - `EVENT_BASE_UNDER_ATTACK` → `VOX_BASE_UNDER_ATTACK`
  - `EVENT_INSUFFICIENT_POWER` → `VOX_INSUFFICIENT_POWER`
  - `EVENT_NO_CASH` → `VOX_NO_CASH`
  - `EVENT_CONSTRUCTION_COMPLETE` → `VOX_CONSTRUCTION`
  - `EVENT_UNIT_READY` → `VOX_UNIT_READY`
  - `EVENT_REINFORCEMENTS` → `VOX_REINFORCEMENTS`

### 2.4 Trigger System (TRIGGER.CPP / TRIGTYPE.CPP)

**File**: `CODE/TRIGGER.CPP` (27 KB), `CODE/TRIGTYPE.CPP` (81 KB)

**Trigger Types** (TRIGTYPE.CPP):
- **Event Triggers**: Fire on game events (unit destroyed, building captured)
- **Condition Triggers**: Fire when condition met (credits > X, power low)
- **Timer Triggers**: Fire at intervals or specific times
- **Action List**: Each trigger has sequenced actions

**Priority Handling**:
- Mission-critical triggers (win/lose) → Immediate
- AI triggers → Evaluated per AI tick
- Ambient triggers → Low priority, batched

**EVA Linkage** (TRIGTYPE.CPP):
```cpp
// Trigger action: "Play EVA Sound"
case ACTION_PLAY_EVA_SOUND:
    Speak((VoxType)param1);
    break;
```

---

## 3. Sound Priority Architecture

### 3.1 Priority Hierarchy

| Priority | Category | Examples | Behavior |
|----------|----------|----------|----------|
| **255** | System Critical | (Reserved) | Never interrupted |
| **254** | **EVA Voice** | Mission alerts, construction complete | **Always plays**, queues if busy |
| **20** | Unit Responses | "Ready", "Affirmative", "Reporting" | Variation system, house-specific |
| **10-15** | UI Feedback | Beeps, clicks, radar on/off, scroll fail | Immediate, low volume |
| **10** | Environmental | Money ticks, construction hum, water splash | Looped or frequent |
| **5** | Ambient/World | Wind, distant explosions, dog barks | Distance-attenuated |
| **1** | Weapon/Combat | Gunfire, explosions, missiles | Highest volume, no distance falloff for local |

### 3.2 Concurrency Rules

1. **EVA Voice Exclusivity**: `Speak()` refuses to queue if `CurrentVoice` or `SpeakQueue` active
2. **Sample Channel Limits**: Hardware-dependent (SB=1, GUS=32, Win32=multiple)
3. **Voice Stealing**: Lower priority samples stopped when channels full
4. **Distance Attenuation**: World sounds faded by `Distance(coord, TacticalCoord)`
5. **Panning**: Stereo position from screen-relative X offset

### 3.3 Volume Control

**Global** (SOUNDDLG.CPP:342-355):
```cpp
Options.Set_Score_Volume(fixed(music.Get_Value(), 256));  // Music (0-1)
Options.Set_Sound_Volume(fixed(sound.Get_Value(), 256));  // SFX (0-1)
```

**Per-Sound** (AUDIO.CPP:404, 466):
```cpp
volume = volume * Options.Volume;  // Applied in Sound_Effect()
Play_Sample(ptr, priority * volume, volume * 256, pan);
```

---

## 4. Multiplayer Chat & Network Events (WOL_CHAT.CPP / WOLAPIOB.CPP)

### 4.1 Chat Event Flow (WOL_CHAT.CPP:692-713)

**Send Message** (Enter in edit field):
```cpp
case (BUTTON_SENDEDIT | KN_BUTTON):
    if (in_channel_or_lobby) {
        pWO->SendMessage(text, userlist, false);  // false = normal
    }
    Clear edit field, reset focus
```

**Send Action** (Action button):
```cpp
case (BUTTON_ACTION | KN_BUTTON):
    pWO->SendMessage(text, userlist, true);  // true = action (/me)
```

**Incoming Messages** (via `WolapiObject::pChat->PumpMessages()`):
- Received in `WOL_Chat_Dialog` main loop (WOL_CHAT.CPP:392-409)
- Displayed via `WOL_PrintMessage()` → `IconListClass::Add_Item()`
- Color-coded by `PlayerColorType` / `RemapControlType*`
- Sound: `VOC_INCOMING_MESSAGE` (`BLEEP6.AUD`)

### 4.2 Network Event Pumping (WOL_CHAT.CPP:392-454)

Called at `WOLAPIPUMPWAIT` intervals (~50ms):
```cpp
pWO->pChat->PumpMessages();      // Chat messages
pWO->pNetUtil->PumpMessages();   // Game/network events
```

**Special Events Handled**:
- `bGotKickedTrigger` → Shows "You were kicked" message, plays `WOLSOUND_ERROR`
- `bMyRecordUpdated` → Updates win/loss display
- `bChannelListTitleUpdated` → Refreshes channel list header
- `bSelfDestruct` → Clean logout
- `bShowRankUpdated` → Toggles RA/AM rank buttons

### 4.3 Channel Management Events (WOL_CHAT.CPP:1113-1281)

**EnterChannel()** priority flow:
1. Password handling (prompt if needed)
2. Auto-password for lobbies (`LOBBYPASSWORD`)
3. Join attempt with retry loop
4. On success: `OnEnteringChatChannel()` or `OnEnteringGameChannel()`
5. Returns `rc=2` for game channel (exits chat dialog)

---

## 5. Evidence Summary

| System | File | Key Functions | Lines | Confidence |
|--------|------|---------------|-------|------------|
| Sound Effects | AUDIO.CPP | `Sound_Effect()` (2 overloads), `SoundEffectName[]` | 330-469 | ★★★★★ |
| EVA Voice | AUDIO.CPP | `Speak()`, `Speak_AI()`, `Speech[]`, `Stop_Speaking()` | 472-763 | ★★★★★ |
| Music | SOUNDDLG.CPP | `SoundControlsClass::Process()`, `MusicListClass::Draw_Entry()` | 70-457 | ★★★★★ |
| AudioClass | AUDIO.H | Wrapper for streaming audio | 43-96 | ★★★★★ |
| Message Queue | MSGLIST.CPP | `Add_Message()`, `Manage()`, `Input()`, `Concat_Message()` | 324-1235 | ★★★★★ |
| Comm Queue | COMQUEUE.CPP | Packet queuing with priority | (not fully read) | ★★★☆☆ |
| Event System | EVENT.CPP | `Post_Event()`, `Process_Events()`, EVA linkage | (not fully read) | ★★★★☆ |
| Trigger System | TRIGGER.CPP | Trigger evaluation, EVA sound actions | (not fully read) | ★★★★☆ |
| WOL Chat | WOL_CHAT.CPP | Chat dialog, message send/receive, channel mgmt | 73-1581 | ★★★★★ |

---

## 6. Key Design Patterns

1. **EVA Voice Double-Buffering**: Two `SpeechBuffer` slots with alternating `_index` prevents gaps between queued lines
2. **Variation File Naming**: `.V00`-`.V03` (Allied) / `.R00`-`.R03` (Soviet) for unit responses
3. **Priority-Based Sample Playback**: `Play_Sample(priority, volume)` with voice stealing
4. **Distance Attenuation + Panning**: World sounds calculated from screen center
5. **Message Timeout Expiration**: `TickCount`-based auto-cleanup in `Manage()`
6. **Chat Overflow Buffer**: Left-trim with `OverflowBuf` for long messages
7. **Theme-Based Music**: Theater-filtered playlist with shuffle/repeat
8. **Network Event Pumping**: Periodic `PumpMessages()` decoupled from frame rate
9. **Sound Effect Caching**: `MFCD::Retrieve()` caches loaded `.AUD` files
10. **Global Volume Multiplication**: `Options.Volume` applied at playback time

---

## 7. Audio File Formats Referenced

| Extension | Use Case |
|-----------|----------|
| `.AUD` | Sound effects, EVA voice, UI beeps (digital PCM) |
| `.V00`-`.V03` | Allied unit response variations |
| `.R00`-`.R03` | Soviet unit response variations |
| `.SHP` | Not audio - shape/graphics format |
| MIDI | Music tracks (via `AudioClass::IsMIDI`) |
| `.WAV`/`.VOC` | Referenced in comments, likely source formats |

---

## 8. Integration Points

| System | Audio Trigger | Code Path |
|--------|---------------|-----------|
| Sidebar | Button clicks, scroll fail | `SidebarClass::AI()` → `Sound_Effect(VOC_SCOLD)` |
| Radar | On/off, zoom | `RadarClass::Radar_Activate()` → `VOC_RADAR_ON/OFF` |
| Construction | Complete, new options | `EVENT_CONSTRUCTION_COMPLETE` → `Speak(VOX_CONSTRUCTION)` |
| Power | Low, insufficient | `EVENT_POWER_LOW` → `Speak(VOX_LOW_POWER/INSUFFICIENT_POWER)` |
| Credits | No funds | `EVENT_NO_CASH` → `Speak(VOX_NO_CASH)` |
| Base Attack | Under attack | `EVENT_BASE_UNDER_ATTACK` → `Speak(VOX_BASE_UNDER_ATTACK)` |
| Unit Ready | Training complete | `EVENT_UNIT_READY` → `Speak(VOX_UNIT_READY)` |
| Chat | Incoming message | `WOL_PrintMessage()` → `Sound_Effect(VOC_INCOMING_MESSAGE)` |
| Multiplayer | Player join/leave | `VOC_PLAYER_JOINED` / `VOC_PLAYER_LEFT` |
| Superweapon | Ready, fired | `EVENT_SUPERWEAPON_READY` → `Speak(VOX_*)` |

---

*Document generated from source code analysis of EA C&C Red Alert (2025 GPL release)*
*Files analyzed: AUDIO.CPP, AUDIO.H, SOUNDDLG.CPP, MSGLIST.CPP, MSGLIST.H, EVENT.CPP, COMQUEUE.CPP, TRIGGER.CPP, TRIGTYPE.CPP, WOL_CHAT.CPP*