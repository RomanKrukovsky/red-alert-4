// Copyright (c) Red Alert 4 project. Remappable keyboard bindings, engine-free.
//
// The control scheme used to live as a wall of literal BindKey calls in the player
// controller, which made every key a recompile and made remapping impossible without
// shipping a new binary. That is the wrong shape for an RTS: players arrive with
// muscle memory from a decade of other games and the first thing a serious one does
// is rebind. It is also the wrong shape for this project's data rule -- changing what
// A does is content, not code.
//
// So the mapping lives here as a table: action -> chord. The engine side does two
// things with it and nothing else. It asks DistinctKeys() what physical keys to
// listen to, and on every press it asks Resolve() which action that was. Nothing in
// the controller names a key any more.
//
// Two chords per action, primary and alternate, because the arrow keys and WASD both
// have to pan and a single-slot table forces a choice no player agrees with.
//
// This layer is deliberately ignorant of Unreal: a chord holds the key's *name* as a
// string, which happens to be what FKey round-trips through. That keeps the whole
// thing testable without an engine, which is where the rules that actually bite --
// modifier precedence, conflict detection, unknown-action handling -- get proven.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4Input/ControlScheme.h"

#ifndef RA4INPUT_API
#define RA4INPUT_API
#endif

namespace RA4
{
namespace Input
{

// Everything a player can rebind. Mouse buttons are absent on purpose: which button
// selects and which orders is ControlScheme's decision, not a per-key one, because
// the two buttons have to swap together or the scheme becomes incoherent.
enum class GameAction : uint8_t
{
    None = 0,

    CameraPanUp,
    CameraPanDown,
    CameraPanLeft,
    CameraPanRight,
    CameraZoomIn,
    CameraZoomOut,
    // Held, not tapped: polled every frame rather than dispatched on press.
    CameraRotate,
    CameraFastPan,
    CameraCenterOnSelection,

    AttackMove,
    Stop,
    Guard,
    HoldPosition,
    CancelAction,
    ToggleDirectControl,
    ToggleCheatConsole,

    ControlGroup1,
    ControlGroup2,
    ControlGroup3,
    ControlGroup4,
    ControlGroup5,
    ControlGroup6,
    ControlGroup7,
    ControlGroup8,
    ControlGroup9,
    ControlGroup0,

    Count,
};

constexpr int32_t kGameActionCount = static_cast<int32_t>(GameAction::Count);

// A key plus the modifiers that must be held with it. An empty Key means "not bound",
// which is a legitimate state: a player is allowed to clear a binding entirely.
struct KeyChord
{
    // The engine-side name of the key, e.g. "A", "Up", "SpaceBar", "MouseScrollUp".
    // Matched case-insensitively so a config file written by hand still works.
    std::string Key;
    bool bCtrl = false;
    bool bShift = false;
    bool bAlt = false;

    bool IsBound() const { return !Key.empty(); }
    bool ModifiersMatch(bool bInCtrl, bool bInShift, bool bInAlt) const
    {
        return bCtrl == bInCtrl && bShift == bInShift && bAlt == bInAlt;
    }
};

RA4INPUT_API bool KeyNamesEqual(const std::string& A, const std::string& B);

struct ActionBinding
{
    KeyChord Primary;
    KeyChord Alternate;
};

// Why an override was rejected. Callers log this: a config line that silently does
// nothing is how a player loses an evening to a binding that "should" work.
enum class BindingParseResult : uint8_t
{
    Applied = 0,
    UnknownAction,
    MalformedChord,
};

class RA4INPUT_API KeyBindingTable
{
public:
    // Starts on the project default scheme rather than empty, so a missing or
    // truncated config file yields a playable game instead of a dead keyboard.
    KeyBindingTable();

    void LoadDefaults(ControlScheme Scheme);

    void Bind(GameAction Action, const KeyChord& Chord);
    void BindAlternate(GameAction Action, const KeyChord& Chord);
    void Unbind(GameAction Action);

    const ActionBinding& Get(GameAction Action) const;

    // Applies one config entry, e.g. ("AttackMove", "Ctrl+A"). An action may appear
    // twice; the second occurrence fills the alternate slot, which is how a config
    // file expresses "arrows or WASD" without inventing syntax for it.
    BindingParseResult ApplyOverride(const std::string& ActionName, const std::string& ChordText);

    // Which action a press of KeyName with these modifiers means, or None.
    //
    // Precedence matters and is the whole reason this is a function rather than a
    // map lookup. An exact modifier match always wins, so binding Ctrl+A to one
    // thing does not shadow plain A. Only if nothing matches exactly does a
    // modifier-less binding of the same key answer -- which is what makes Ctrl+1
    // reach ControlGroup1, letting the controller read Ctrl itself and decide
    // between assign and recall without ten more enumerators.
    GameAction Resolve(const std::string& KeyName, bool bCtrl, bool bShift, bool bAlt) const;

    // Every physical key named by any binding, de-duplicated. The engine layer binds
    // exactly these and nothing else.
    std::vector<std::string> DistinctKeys() const;

    // Actions whose primary chord collides with an earlier action's. Reported, not
    // repaired: which of two colliding bindings is the mistake is the player's call.
    std::vector<GameAction> FindConflicts() const;

private:
    ActionBinding Bindings[kGameActionCount];

    // Which actions the config has already spoken about. The first override for an
    // action replaces its default outright; a second one fills the alternate slot.
    // Without this the defaults and the overrides would coexist, and a player who
    // rebound attack-move to Q would find it still firing on A as well.
    bool bOverridden[kGameActionCount] = {};
};

RA4INPUT_API const char* ToString(GameAction Action);
RA4INPUT_API GameAction ParseAction(const std::string& Name);

// Accepts "A", "Ctrl+A", "Ctrl+Shift+A", "Alt + A", and the empty string (unbound).
// Modifier order is irrelevant; whitespace is ignored.
RA4INPUT_API bool ParseChord(const std::string& Text, KeyChord& OutChord);
RA4INPUT_API std::string ChordToString(const KeyChord& Chord);

// True when this action is polled every frame rather than dispatched on key press.
// Panning has to be smooth and pressure-sensitive, so it cannot ride on press events.
RA4INPUT_API bool IsHeldAction(GameAction Action);

// The control group this action drives, or -1. Group 0 is the tenth group and sits
// on the Zero key, matching the on-screen numbering.
RA4INPUT_API int32_t ControlGroupIndexOf(GameAction Action);

} // namespace Input
} // namespace RA4
