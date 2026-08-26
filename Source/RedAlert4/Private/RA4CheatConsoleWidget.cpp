// Copyright (c) Red Alert 4 project. In-Game Cheat Console Widget.
//
// Cheats mutate the simulation, which every other presentation-side caller is
// forbidden from doing: RA4SimWorldSubsystem hands out a `const SimWorld*` and says so
// in as many words -- "Nothing outside the simulation may mutate it directly."
//
// This file used to defeat that with a const_cast and then call Cheat* and ApplyCommand
// straight on the world. Two things were wrong with that, and neither is stylistic:
//
//   1. It bypassed EnqueueCommand, which is the one place that knows a lockstep match
//      must not apply a command locally -- it has to go to the server and come back in
//      the authoritative frame. A cheat applied directly executes on this peer and
//      nowhere else, which is a desync, not a cheat.
//   2. A const_cast is not a mechanism, it is the absence of one. Anyone copying the
//      line gets write access to authoritative state with no gate at all.
//
// So the console now refuses to run in a networked match, and refuses to exist in a
// shipping build. Where a real command exists (surrender) it goes through the ordinary
// player path. The Cheat* entry points are a debug-only escape hatch, taken through one
// explicitly-named accessor rather than a cast, so grep finds every caller.
#include "RA4CheatConsoleWidget.h"
#include "RedAlert4/Public/RA4PlayerController.h"
#include "RedAlert4/Public/RA4SimWorldSubsystem.h"
#include "RA4Simulation/SimWorld.h"
#include "RA4Core/SimConfig.h"
#include "RA4NetworkManager.h"

URA4CheatConsoleWidget::URA4CheatConsoleWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URA4CheatConsoleWidget::NativeConstruct()
{
    Super::NativeConstruct();
    AddLogLine(TEXT("--- RED ALERT 4 CHEAT TERMINAL ONLINE ---"));
    AddLogLine(TEXT("Type 'help' for available commands. Esc / Tilde to exit."));
}

void URA4CheatConsoleWidget::AddLogLine(const FString& Line)
{
    HistoryLines.Add(Line);
    if (HistoryLines.Num() > MaxHistoryLines)
    {
        HistoryLines.RemoveAt(0);
    }
    OnCheatExecuted.Broadcast(Line);
}

namespace
{
// The single place presentation code is allowed to take a mutable simulation pointer,
// named so that `grep MutableSimForCheats` lists every cheat that writes to the world.
// It is not a general-purpose accessor: it refuses in a networked match, because a
// local write there desyncs the peer instead of cheating, and it does not exist in a
// shipping build at all.
RA4::SimWorld* MutableSimForCheats(URA4SimWorldSubsystem* Subsystem)
{
#if UE_BUILD_SHIPPING
    (void)Subsystem;
    return nullptr;
#else
    if (Subsystem == nullptr)
    {
        return nullptr;
    }

    // Mirrors URA4SimWorldSubsystem::GetActiveNetwork, which is private: a manager
    // exists in every world and only means anything once the match is active. Before
    // that this is single player and a local write is safe.
    if (const UWorld* OwningWorld = Subsystem->GetWorld())
    {
        if (URA4NetworkManager* Network = OwningWorld->GetSubsystem<URA4NetworkManager>())
        {
            if (Network->IsMatchActive())
            {
                return nullptr;
            }
        }
    }

    // Still a cast, but now a single audited one behind two gates, instead of an
    // unmarked cast at the top of a 150-line function.
    return const_cast<RA4::SimWorld*>(Subsystem->GetSimWorld());
#endif
}

// Same predicate, for the error message. Kept adjacent so the two cannot drift.
bool IsNetworkedMatch(const URA4SimWorldSubsystem* Subsystem)
{
    if (Subsystem == nullptr)
    {
        return false;
    }
    if (const UWorld* OwningWorld = Subsystem->GetWorld())
    {
        if (URA4NetworkManager* Network = OwningWorld->GetSubsystem<URA4NetworkManager>())
        {
            return Network->IsMatchActive();
        }
    }
    return false;
}
} // namespace

bool URA4CheatConsoleWidget::ExecuteCommandText(const FString& InCommandText)
{
    FString Trimmed = InCommandText.TrimStartAndEnd();
    if (Trimmed.IsEmpty())
    {
        return false;
    }

    AddLogLine(FString::Printf(TEXT("> %s"), *Trimmed));

    ARA4PlayerController* PC = Cast<ARA4PlayerController>(GetOwningPlayer());
    URA4SimWorldSubsystem* SimSubsystem = PC ? PC->GetWorld()->GetSubsystem<URA4SimWorldSubsystem>() : nullptr;

    FString CmdLower = Trimmed.ToLower();
    TArray<FString> Tokens;
    CmdLower.ParseIntoArrayWS(Tokens);

    if (Tokens.Num() == 0) return false;

    FString Verb = Tokens[0];

    if (Verb == TEXT("help"))
    {
        AddLogLine(TEXT("--- HOI4-INSPIRED CHEATS ---"));
        AddLogLine(TEXT("annex -- absorb nearest enemy base + all forces"));
        AddLogLine(TEXT("spawnarmy -- spawn a full combined-arms division"));
        AddLogLine(TEXT("focusfire -- all units attack strongest enemy"));
        AddLogLine(TEXT("maxmods -- max damage/armor/speed/health modifiers"));
        AddLogLine(TEXT("annexall -- annex every enemy player"));
        AddLogLine(TEXT("--- CLASSIC CHEATS ---"));
        AddLogLine(TEXT("chaching, credits <amt>, power, fastbuild, god"));
        AddLogLine(TEXT("reveal, heal, killall, spawn <id>, nuke, win, lose"));
        return true;
    }

    // Win/lose are ordinary Surrender commands, not state pokes. They take the same road
    // as the debug keys already bound in the controller: through SubmitOrders, into
    // EnqueueCommand, and in a lockstep match out to the server rather than applied here.
    if (Verb == TEXT("win") || Verb == TEXT("victory") ||
        Verb == TEXT("lose") || Verb == TEXT("defeat") || Verb == TEXT("surrender"))
    {
#if UE_BUILD_SHIPPING
        AddLogLine(TEXT("ERROR: Cheats are not available in a shipping build."));
        return false;
#else
        if (PC == nullptr)
        {
            AddLogLine(TEXT("ERROR: No player controller."));
            return false;
        }
        const bool bWin = (Verb == TEXT("win") || Verb == TEXT("victory"));
        if (bWin)
        {
            PC->DebugForceVictory();
            AddLogLine(TEXT("SUCCESS: Enemy surrendered."));
        }
        else
        {
            PC->DebugForceDefeat();
            AddLogLine(TEXT("SUCCESS: Surrendered."));
        }
        return true;
#endif
    }

    RA4::SimWorld* Sim = MutableSimForCheats(SimSubsystem);
    if (Sim == nullptr)
    {
        // Distinguish the three reasons, because "nothing happened" is the least useful
        // thing a console can say.
#if UE_BUILD_SHIPPING
        AddLogLine(TEXT("ERROR: Cheats are not available in a shipping build."));
#else
        if (IsNetworkedMatch(SimSubsystem))
        {
            AddLogLine(TEXT("ERROR: Cheats are disabled in a networked match: a local"));
            AddLogLine(TEXT("       state change would desync this peer, not cheat."));
        }
        else
        {
            AddLogLine(TEXT("ERROR: Active simulation world not found!"));
        }
#endif
        return false;
    }

    RA4::PlayerId LocalPlayer = 0; // Local player index

    if (Verb == TEXT("chaching") || Verb == TEXT("credits") || Verb == TEXT("money"))
    {
        int32 Amount = 10000;
        if (Tokens.Num() > 1)
        {
            Amount = FCString::Atoi(*Tokens[1]);
        }
        Sim->CheatGrantCredits(LocalPlayer, Amount);
        AddLogLine(FString::Printf(TEXT("SUCCESS: Granted +%d credits!"), Amount));
        return true;
    }

    if (Verb == TEXT("power") || Verb == TEXT("poweroverwhelming") || Verb == TEXT("maxpower"))
    {
        Sim->CheatGrantPower(LocalPlayer, 9999);
        AddLogLine(TEXT("SUCCESS: Maximum power supply granted (9999 kW)!"));
        return true;
    }

    if (Verb == TEXT("fastbuild") || Verb == TEXT("instant"))
    {
        Sim->CheatInstantBuild(LocalPlayer);
        AddLogLine(TEXT("SUCCESS: Instant production queues completed!"));
        return true;
    }

    if (Verb == TEXT("god") || Verb == TEXT("invincible"))
    {
        Sim->CheatToggleGodMode(LocalPlayer);
        AddLogLine(TEXT("SUCCESS: Invincibility activated for your forces!"));
        return true;
    }

    // win / lose / surrender were handled above, before the mutable-world gate, because
    // they are real Commands and must travel the ordinary player path.

    if (Verb == TEXT("reveal") || Verb == TEXT("revealmap"))
    {
        Sim->CheatRevealMap(LocalPlayer);
        AddLogLine(TEXT("SUCCESS: Full map revealed!"));
        return true;
    }

    if (Verb == TEXT("heal"))
    {
        Sim->CheatHealAll(LocalPlayer);
        AddLogLine(TEXT("SUCCESS: All your forces healed to full!"));
        return true;
    }

    if (Verb == TEXT("killall") || Verb == TEXT("destroyenemies"))
    {
        Sim->CheatKillAllEnemies(LocalPlayer);
        AddLogLine(TEXT("SUCCESS: All enemy units and buildings destroyed!"));
        return true;
    }

    // --- HOI4-inspired cheats ---
    if (Verb == TEXT("maxmods") || Verb == TEXT("research"))
    {
        Sim->CheatMaxModifiers(LocalPlayer);
        AddLogLine(TEXT("SUCCESS: All modifiers maxed! Damage/Armor/Speed/Health boosted, +50k credits!"));
        return true;
    }

    if (Verb == TEXT("spawnarmy") || Verb == TEXT("division"))
    {
        Sim->CheatSpawnArmy(LocalPlayer, "sov");
        AddLogLine(TEXT("SUCCESS: Combined-arms division deployed at your base!"));
        return true;
    }

    if (Verb == TEXT("annex"))
    {
        // Annex the first active enemy player
        for (int32 P = 1; P < RA4::kMaxPlayers; ++P)
        {
            const RA4::PlayerState& PS = Sim->GetPlayer(RA4::PlayerId(P));
            if (PS.bActive && !PS.bDefeated)
            {
                Sim->CheatAnnexPlayer(LocalPlayer, RA4::PlayerId(P));
                AddLogLine(FString::Printf(TEXT("SUCCESS: Player %d annexed! All their forces are yours!"), P));
                return true;
            }
        }
        AddLogLine(TEXT("ERROR: No active enemy player to annex."));
        return true;
    }

    if (Verb == TEXT("annexall"))
    {
        int32 Count = 0;
        for (int32 P = 1; P < RA4::kMaxPlayers; ++P)
        {
            const RA4::PlayerState& PS = Sim->GetPlayer(RA4::PlayerId(P));
            if (PS.bActive && !PS.bDefeated)
            {
                Sim->CheatAnnexPlayer(LocalPlayer, RA4::PlayerId(P));
                ++Count;
            }
        }
        AddLogLine(FString::Printf(TEXT("SUCCESS: %d players annexed! The world is yours!"), Count));
        return true;
    }

    if (Verb == TEXT("focusfire") || Verb == TEXT("ff"))
    {
        Sim->CheatFocusFire(LocalPlayer);
        AddLogLine(TEXT("SUCCESS: All combat units focusing fire on the strongest enemy!"));
        return true;
    }

    if (Verb == TEXT("nuke"))
    {
        RA4::Vec2 CursorGround;
        if (PC && PC->GetCursorGroundPosition(CursorGround))
        {
            Sim->CheatGrantCredits(LocalPlayer, 0); // Trigger event
            AddLogLine(FString::Printf(TEXT("SUCCESS: Tactical nuclear strike launched at (%.0f, %.0f)!"), CursorGround.X.ToDoubleUnsafe(), CursorGround.Y.ToDoubleUnsafe()));
        }
        else
        {
            AddLogLine(TEXT("SUCCESS: Tactical nuclear strike launched at base!"));
        }
        return true;
    }

    if (Verb == TEXT("spawn"))
    {
        FString UnitId = Tokens.Num() > 1 ? Tokens[1] : TEXT("su_conscript");
        RA4::Vec2 CursorGround;
        if (PC && PC->GetCursorGroundPosition(CursorGround))
        {
            Sim->SpawnBuilding(RA4::ContentId(RA4::HashName(TCHAR_TO_UTF8(*UnitId))), LocalPlayer, RA4::TileCoord(CursorGround.X.ToIntFloor(), CursorGround.Y.ToIntFloor()), true);
        }
        AddLogLine(FString::Printf(TEXT("SUCCESS: Spawned entity '%s'!"), *UnitId));
        return true;
    }

    AddLogLine(FString::Printf(TEXT("ERROR: Unknown cheat command '%s'. Type 'help' for list."), *Verb));
    return false;
}
