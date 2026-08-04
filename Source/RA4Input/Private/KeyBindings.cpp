// Copyright (c) Red Alert 4 project. Remappable keyboard bindings, engine-free.
#include "RA4Input/KeyBindings.h"

#include <algorithm>
#include <cctype>

namespace RA4
{
namespace Input
{

namespace
{

char LowerAscii(char C)
{
    return static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
}

std::string Trim(const std::string& Text)
{
    size_t First = 0;
    while (First < Text.size() && std::isspace(static_cast<unsigned char>(Text[First])) != 0)
    {
        ++First;
    }
    size_t Last = Text.size();
    while (Last > First && std::isspace(static_cast<unsigned char>(Text[Last - 1])) != 0)
    {
        --Last;
    }
    return Text.substr(First, Last - First);
}

// The action names below double as the config file's keys, so they are part of the
// project's surface: renaming one silently breaks every config that used it.
struct ActionName
{
    GameAction Action;
    const char* Name;
};

const ActionName kActionNames[] = {
    {GameAction::CameraPanUp, "CameraPanUp"},
    {GameAction::CameraPanDown, "CameraPanDown"},
    {GameAction::CameraPanLeft, "CameraPanLeft"},
    {GameAction::CameraPanRight, "CameraPanRight"},
    {GameAction::CameraZoomIn, "CameraZoomIn"},
    {GameAction::CameraZoomOut, "CameraZoomOut"},
    {GameAction::CameraRotate, "CameraRotate"},
    {GameAction::CameraFastPan, "CameraFastPan"},
    {GameAction::CameraCenterOnSelection, "CameraCenterOnSelection"},
    {GameAction::AttackMove, "AttackMove"},
    {GameAction::Stop, "Stop"},
    {GameAction::Guard, "Guard"},
    {GameAction::HoldPosition, "HoldPosition"},
    {GameAction::CancelAction, "CancelAction"},
    {GameAction::ToggleDirectControl, "ToggleDirectControl"},
    {GameAction::ToggleCheatConsole, "ToggleCheatConsole"},
    {GameAction::ControlGroup1, "ControlGroup1"},
    {GameAction::ControlGroup2, "ControlGroup2"},
    {GameAction::ControlGroup3, "ControlGroup3"},
    {GameAction::ControlGroup4, "ControlGroup4"},
    {GameAction::ControlGroup5, "ControlGroup5"},
    {GameAction::ControlGroup6, "ControlGroup6"},
    {GameAction::ControlGroup7, "ControlGroup7"},
    {GameAction::ControlGroup8, "ControlGroup8"},
    {GameAction::ControlGroup9, "ControlGroup9"},
    {GameAction::ControlGroup0, "ControlGroup0"},
};

KeyChord Chord(const char* Key)
{
    KeyChord Out;
    Out.Key = Key;
    return Out;
}

KeyChord CtrlChord(const char* Key)
{
    KeyChord Out;
    Out.Key = Key;
    Out.bCtrl = true;
    return Out;
}

} // namespace

bool KeyNamesEqual(const std::string& A, const std::string& B)
{
    if (A.size() != B.size())
    {
        return false;
    }
    for (size_t I = 0; I < A.size(); ++I)
    {
        if (LowerAscii(A[I]) != LowerAscii(B[I]))
        {
            return false;
        }
    }
    return true;
}

const char* ToString(GameAction Action)
{
    for (const ActionName& Entry : kActionNames)
    {
        if (Entry.Action == Action)
        {
            return Entry.Name;
        }
    }
    return "None";
}

GameAction ParseAction(const std::string& Name)
{
    const std::string Trimmed = Trim(Name);
    for (const ActionName& Entry : kActionNames)
    {
        if (KeyNamesEqual(Trimmed, Entry.Name))
        {
            return Entry.Action;
        }
    }
    return GameAction::None;
}

bool ParseChord(const std::string& Text, KeyChord& OutChord)
{
    OutChord = KeyChord();

    const std::string Trimmed = Trim(Text);
    if (Trimmed.empty())
    {
        // Explicitly clearing a binding is legal, and has to be distinguishable from
        // a typo: an empty value unbinds, a garbage value is rejected and logged.
        return true;
    }

    size_t Cursor = 0;
    while (Cursor < Trimmed.size())
    {
        const size_t Plus = Trimmed.find('+', Cursor);
        const std::string Token = Trim(Trimmed.substr(Cursor, Plus == std::string::npos ? std::string::npos
                                                                                        : Plus - Cursor));
        if (Token.empty())
        {
            return false;
        }

        const bool bLastToken = Plus == std::string::npos;
        if (KeyNamesEqual(Token, "Ctrl") || KeyNamesEqual(Token, "Control"))
        {
            OutChord.bCtrl = true;
        }
        else if (KeyNamesEqual(Token, "Shift"))
        {
            OutChord.bShift = true;
        }
        else if (KeyNamesEqual(Token, "Alt"))
        {
            OutChord.bAlt = true;
        }
        else if (bLastToken)
        {
            OutChord.Key = Token;
        }
        else
        {
            // A non-modifier in a non-final position means two keys were chorded,
            // which this layer does not support and must not silently truncate.
            return false;
        }

        if (bLastToken)
        {
            break;
        }
        Cursor = Plus + 1;
    }

    // "Ctrl+" and friends parse into modifiers with no key, which is not a binding.
    return OutChord.IsBound();
}

std::string ChordToString(const KeyChord& Chord)
{
    if (!Chord.IsBound())
    {
        return std::string();
    }
    std::string Out;
    if (Chord.bCtrl)
    {
        Out += "Ctrl+";
    }
    if (Chord.bShift)
    {
        Out += "Shift+";
    }
    if (Chord.bAlt)
    {
        Out += "Alt+";
    }
    Out += Chord.Key;
    return Out;
}

bool IsHeldAction(GameAction Action)
{
    switch (Action)
    {
    case GameAction::CameraPanUp:
    case GameAction::CameraPanDown:
    case GameAction::CameraPanLeft:
    case GameAction::CameraPanRight:
    case GameAction::CameraRotate:
    case GameAction::CameraFastPan:
        return true;
    default:
        return false;
    }
}

int32_t ControlGroupIndexOf(GameAction Action)
{
    // Slot order follows the key row, not the label: One is slot 0 and Zero is slot
    // 9, which is the indexing SelectionModel and the HUD already agree on.
    switch (Action)
    {
    case GameAction::ControlGroup1:
        return 0;
    case GameAction::ControlGroup2:
        return 1;
    case GameAction::ControlGroup3:
        return 2;
    case GameAction::ControlGroup4:
        return 3;
    case GameAction::ControlGroup5:
        return 4;
    case GameAction::ControlGroup6:
        return 5;
    case GameAction::ControlGroup7:
        return 6;
    case GameAction::ControlGroup8:
        return 7;
    case GameAction::ControlGroup9:
        return 8;
    case GameAction::ControlGroup0:
        return 9;
    default:
        return -1;
    }
}

KeyBindingTable::KeyBindingTable()
{
    LoadDefaults(ControlScheme::ClassicRA);
}

void KeyBindingTable::LoadDefaults(ControlScheme Scheme)
{
    for (ActionBinding& Binding : Bindings)
    {
        Binding = ActionBinding();
    }

    Bindings[int32_t(GameAction::CameraPanUp)] = {Chord("Up"), Chord("W")};
    Bindings[int32_t(GameAction::CameraPanDown)] = {Chord("Down"), Chord("S")};
    Bindings[int32_t(GameAction::CameraPanRight)] = {Chord("Right"), Chord("D")};
    Bindings[int32_t(GameAction::CameraZoomIn)] = {Chord("MouseScrollUp"), KeyChord()};
    Bindings[int32_t(GameAction::CameraZoomOut)] = {Chord("MouseScrollDown"), KeyChord()};
    Bindings[int32_t(GameAction::CameraRotate)] = {Chord("SpaceBar"), KeyChord()};
    Bindings[int32_t(GameAction::CameraFastPan)] = {Chord("LeftShift"), Chord("RightShift")};
    Bindings[int32_t(GameAction::CameraCenterOnSelection)] = {Chord("Home"), KeyChord()};

    Bindings[int32_t(GameAction::Stop)] = {Chord("X"), KeyChord()};
    Bindings[int32_t(GameAction::Guard)] = {Chord("G"), KeyChord()};
    Bindings[int32_t(GameAction::HoldPosition)] = {Chord("H"), KeyChord()};
    Bindings[int32_t(GameAction::CancelAction)] = {Chord("Escape"), KeyChord()};
    Bindings[int32_t(GameAction::ToggleDirectControl)] = {Chord("F"), KeyChord()};
    Bindings[int32_t(GameAction::ToggleCheatConsole)] = {Chord("Tilde"), KeyChord()};

    Bindings[int32_t(GameAction::ControlGroup1)] = {Chord("One"), KeyChord()};
    Bindings[int32_t(GameAction::ControlGroup2)] = {Chord("Two"), KeyChord()};
    Bindings[int32_t(GameAction::ControlGroup3)] = {Chord("Three"), KeyChord()};
    Bindings[int32_t(GameAction::ControlGroup4)] = {Chord("Four"), KeyChord()};
    Bindings[int32_t(GameAction::ControlGroup5)] = {Chord("Five"), KeyChord()};
    Bindings[int32_t(GameAction::ControlGroup6)] = {Chord("Six"), KeyChord()};
    Bindings[int32_t(GameAction::ControlGroup7)] = {Chord("Seven"), KeyChord()};
    Bindings[int32_t(GameAction::ControlGroup8)] = {Chord("Eight"), KeyChord()};
    Bindings[int32_t(GameAction::ControlGroup9)] = {Chord("Nine"), KeyChord()};
    Bindings[int32_t(GameAction::ControlGroup0)] = {Chord("Zero"), KeyChord()};

    // The one genuine disagreement between the schemes. Letter hotkeys and WASD
    // panning both want A, and no amount of design resolves that -- one of them has
    // to move. Classic keeps A for attack-move, which is the gesture a Red Alert
    // player reaches for without looking, and pans left with the arrow key, the
    // screen edge or the middle-mouse drag. Modern gives A back to the camera so
    // WASD is whole, and puts attack-move on Ctrl+A.
    if (Scheme == ControlScheme::Modern)
    {
        Bindings[int32_t(GameAction::CameraPanLeft)] = {Chord("Left"), Chord("A")};
        Bindings[int32_t(GameAction::AttackMove)] = {CtrlChord("A"), KeyChord()};
    }
    else
    {
        Bindings[int32_t(GameAction::CameraPanLeft)] = {Chord("Left"), KeyChord()};
        Bindings[int32_t(GameAction::AttackMove)] = {Chord("A"), KeyChord()};
    }
}

void KeyBindingTable::Bind(GameAction Action, const KeyChord& InChord)
{
    if (Action == GameAction::None || Action >= GameAction::Count)
    {
        return;
    }
    Bindings[int32_t(Action)].Primary = InChord;
}

void KeyBindingTable::BindAlternate(GameAction Action, const KeyChord& InChord)
{
    if (Action == GameAction::None || Action >= GameAction::Count)
    {
        return;
    }
    Bindings[int32_t(Action)].Alternate = InChord;
}

void KeyBindingTable::Unbind(GameAction Action)
{
    if (Action == GameAction::None || Action >= GameAction::Count)
    {
        return;
    }
    Bindings[int32_t(Action)] = ActionBinding();
}

const ActionBinding& KeyBindingTable::Get(GameAction Action) const
{
    static const ActionBinding Empty;
    if (Action == GameAction::None || Action >= GameAction::Count)
    {
        return Empty;
    }
    return Bindings[int32_t(Action)];
}

BindingParseResult KeyBindingTable::ApplyOverride(const std::string& ActionName, const std::string& ChordText)
{
    const GameAction Action = ParseAction(ActionName);
    if (Action == GameAction::None)
    {
        return BindingParseResult::UnknownAction;
    }

    KeyChord Parsed;
    if (!ParseChord(ChordText, Parsed))
    {
        return BindingParseResult::MalformedChord;
    }

    // The first override for an action replaces the whole default, alternate
    // included. Otherwise a config that rebinds AttackMove to Q would leave the
    // default alternate live and the player would still be firing on A.
    ActionBinding& Binding = Bindings[int32_t(Action)];
    if (!bOverridden[int32_t(Action)])
    {
        bOverridden[int32_t(Action)] = true;
        Binding.Primary = Parsed;
        Binding.Alternate = KeyChord();
    }
    else
    {
        Binding.Alternate = Parsed;
    }
    return BindingParseResult::Applied;
}

GameAction KeyBindingTable::Resolve(const std::string& KeyName, bool bCtrl, bool bShift, bool bAlt) const
{
    GameAction LooseMatch = GameAction::None;

    for (int32_t Index = 0; Index < kGameActionCount; ++Index)
    {
        const ActionBinding& Binding = Bindings[Index];
        const KeyChord* Candidates[2] = {&Binding.Primary, &Binding.Alternate};
        for (const KeyChord* Candidate : Candidates)
        {
            if (!Candidate->IsBound() || !KeyNamesEqual(Candidate->Key, KeyName))
            {
                continue;
            }
            if (Candidate->ModifiersMatch(bCtrl, bShift, bAlt))
            {
                return static_cast<GameAction>(Index);
            }
            if (!Candidate->bCtrl && !Candidate->bShift && !Candidate->bAlt &&
                LooseMatch == GameAction::None)
            {
                LooseMatch = static_cast<GameAction>(Index);
            }
        }
    }

    return LooseMatch;
}

std::vector<std::string> KeyBindingTable::DistinctKeys() const
{
    std::vector<std::string> Out;
    const auto Append = [&Out](const KeyChord& Candidate)
    {
        if (!Candidate.IsBound())
        {
            return;
        }
        for (const std::string& Existing : Out)
        {
            if (KeyNamesEqual(Existing, Candidate.Key))
            {
                return;
            }
        }
        Out.push_back(Candidate.Key);
    };

    for (const ActionBinding& Binding : Bindings)
    {
        Append(Binding.Primary);
        Append(Binding.Alternate);
    }
    return Out;
}

std::vector<GameAction> KeyBindingTable::FindConflicts() const
{
    std::vector<GameAction> Out;
    for (int32_t Index = 1; Index < kGameActionCount; ++Index)
    {
        const KeyChord& Chord = Bindings[Index].Primary;
        if (!Chord.IsBound())
        {
            continue;
        }
        for (int32_t Other = 1; Other < Index; ++Other)
        {
            const KeyChord& Earlier = Bindings[Other].Primary;
            if (!Earlier.IsBound() || !KeyNamesEqual(Earlier.Key, Chord.Key))
            {
                continue;
            }
            if (Earlier.ModifiersMatch(Chord.bCtrl, Chord.bShift, Chord.bAlt))
            {
                Out.push_back(static_cast<GameAction>(Index));
                break;
            }
        }
    }
    return Out;
}

} // namespace Input
} // namespace RA4
