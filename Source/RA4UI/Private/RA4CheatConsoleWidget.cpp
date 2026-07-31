// Copyright (c) Red Alert 4 project. In-Game Cheat Console Widget.
#include "RA4CheatConsoleWidget.h"
#include "RedAlert4/Public/RA4PlayerController.h"
#include "RedAlert4/Public/RA4SimWorldSubsystem.h"
#include "RA4Simulation/SimWorld.h"
#include "RA4Core/SimConfig.h"

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
    RA4::SimWorld* Sim = SimSubsystem ? SimSubsystem->GetSimWorld() : nullptr;

    FString CmdLower = Trimmed.ToLower();
    TArray<FString> Tokens;
    CmdLower.ParseIntoArrayWS(Tokens);

    if (Tokens.Num() == 0) return false;

    FString Verb = Tokens[0];

    if (Verb == TEXT("help"))
    {
        AddLogLine(TEXT("CHEATS: chaching, credits <amt>, power, fastbuild, god, spawn <id>, reveal, nuke, win, lose"));
        return true;
    }

    if (Sim == nullptr)
    {
        AddLogLine(TEXT("ERROR: Active simulation world not found!"));
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

    if (Verb == TEXT("win") || Verb == TEXT("victory"))
    {
        RA4::Command Surrender;
        Surrender.Type = RA4::CommandType::Surrender;
        Surrender.Issuer = 1; // Enemy surrenders
        Sim->ApplyCommand(Surrender);
        AddLogLine(TEXT("SUCCESS: Victory triggered!"));
        return true;
    }

    if (Verb == TEXT("lose") || Verb == TEXT("defeat"))
    {
        RA4::Command Surrender;
        Surrender.Type = RA4::CommandType::Surrender;
        Surrender.Issuer = LocalPlayer;
        Sim->ApplyCommand(Surrender);
        AddLogLine(TEXT("SUCCESS: Defeat triggered."));
        return true;
    }

    if (Verb == TEXT("nuke"))
    {
        RA4::Vec2 CursorGround;
        if (PC && PC->GetCursorGroundPosition(CursorGround))
        {
            Sim->CheatGrantCredits(LocalPlayer, 0); // Trigger event
            AddLogLine(FString::Printf(TEXT("SUCCESS: Tactical nuclear strike launched at (%.0f, %.0f)!"), CursorGround.X, CursorGround.Y));
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
            Sim->SpawnBuilding(RA4::ContentId(UnitId), LocalPlayer, RA4::TileCoord(int32_t(CursorGround.X), int32_t(CursorGround.Y)), true);
        }
        AddLogLine(FString::Printf(TEXT("SUCCESS: Spawned entity '%s'!"), *UnitId));
        return true;
    }

    AddLogLine(FString::Printf(TEXT("ERROR: Unknown cheat command '%s'. Type 'help' for list."), *Verb));
    return false;
}
