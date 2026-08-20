// Copyright (c) Red Alert 4 project.

#include "RA4LobbyViewModel.h"

#define LOCTEXT_NAMESPACE "RA4LobbyViewModel"

namespace
{
FRA4LobbyPlayerView MakeLobbyPlayer(
    const TCHAR* Id,
    const FText& Name,
    const ERA4FactionTheme Faction,
    const int32 Color,
    const int32 Team,
    const int32 Ping,
    const bool bHost = false)
{
    FRA4LobbyPlayerView Player;
    Player.PlayerId = FName(Id);
    Player.PlayerName = Name;
    Player.Faction = Faction;
    Player.ColorIndex = Color;
    Player.Team = Team;
    Player.Ping = Ping;
    Player.bReady = true;
    Player.bHost = bHost;
    return Player;
}

FRA4LobbyChatMessageView MakeChat(
    const FText& Author,
    const FText& Message,
    const FLinearColor& Color)
{
    FRA4LobbyChatMessageView Chat;
    Chat.Author = Author;
    Chat.Message = Message;
    Chat.AuthorColor = Color;
    return Chat;
}
} // namespace

URA4LobbyViewModel::URA4LobbyViewModel()
{
    Players = {
        MakeLobbyPlayer(TEXT("sokolov_1945"), LOCTEXT("Sokolov", "SOKOLOV_1945"), ERA4FactionTheme::USSR, 0, 1, 32, true),
        MakeLobbyPlayer(TEXT("allied_command"), LOCTEXT("AlliedCommand", "Allied_Command"), ERA4FactionTheme::Allies, 1, 1, 41),
        MakeLobbyPlayer(TEXT("dragon_warlord"), LOCTEXT("DragonWarlord", "Dragon_Warlord"), ERA4FactionTheme::EasternCoalition, 2, 2, 37),
        MakeLobbyPlayer(TEXT("chrono_legionnaire"), LOCTEXT("ChronoLegionnaire", "ChronoLegionnaire"), ERA4FactionTheme::Chronolegion, 3, 2, 45),
        MakeLobbyPlayer(TEXT("red_october"), LOCTEXT("RedOctober", "RedOctober"), ERA4FactionTheme::USSR, 4, 3, 29),
        MakeLobbyPlayer(TEXT("sky_eagle"), LOCTEXT("SkyEagle", "SkyEagle"), ERA4FactionTheme::Allies, 5, 3, 52),
        MakeLobbyPlayer(TEXT("jade_tiger"), LOCTEXT("JadeTiger", "JadeTiger"), ERA4FactionTheme::EasternCoalition, 6, 4, 34),
        MakeLobbyPlayer(TEXT("time_walker"), LOCTEXT("TimeWalker", "TimeWalker"), ERA4FactionTheme::Chronolegion, 7, 4, 47)
    };

    ChatMessages = {
        MakeChat(LOCTEXT("ChatSokolov", "SOKOLOV_1945"), LOCTEXT("ChatSokolovMessage", "Всем удачи. За Родину!"), FLinearColor(0.95f, 0.18f, 0.20f, 1.0f)),
        MakeChat(LOCTEXT("ChatAllied", "Allied_Command"), LOCTEXT("ChatAlliedMessage", "For freedom!"), FLinearColor(0.22f, 0.58f, 1.0f, 1.0f)),
        MakeChat(LOCTEXT("ChatDragon", "Dragon_Warlord"), LOCTEXT("ChatDragonMessage", "Честь и традиции."), FLinearColor(0.42f, 0.86f, 0.26f, 1.0f)),
        MakeChat(LOCTEXT("ChatChrono", "ChronoLegionnaire"), LOCTEXT("ChatChronoMessage", "Время на нашей стороне."), FLinearColor(0.70f, 0.32f, 1.0f, 1.0f)),
        MakeChat(LOCTEXT("ChatSky", "SkyEagle"), LOCTEXT("ChatSkyMessage", "Ready when you are."), FLinearColor(0.36f, 0.74f, 1.0f, 1.0f)),
        MakeChat(LOCTEXT("ChatJade", "JadeTiger"), LOCTEXT("ChatJadeMessage", "Пусть дракон ведёт нас к победе."), FLinearColor(0.58f, 0.88f, 0.28f, 1.0f)),
        MakeChat(LOCTEXT("ChatTime", "TimeWalker"), LOCTEXT("ChatTimeMessage", "История перепишется."), FLinearColor(0.82f, 0.42f, 1.0f, 1.0f))
    };
}

bool URA4LobbyViewModel::SetReady(const FName PlayerId, const bool bReady)
{
    FRA4LobbyPlayerView* Player = FindPlayer(PlayerId);
    if (!Player || !Player->bConnected)
    {
        return false;
    }
    Player->bReady = bReady;
    BroadcastLobbyState();
    return true;
}

bool URA4LobbyViewModel::ChangeFaction(const FName PlayerId, const ERA4FactionTheme Faction)
{
    FRA4LobbyPlayerView* Player = FindPlayer(PlayerId);
    if (!Player || !Player->bConnected)
    {
        return false;
    }
    Player->Faction = Faction;
    BroadcastLobbyState();
    return true;
}

bool URA4LobbyViewModel::ChangeTeam(const FName PlayerId, const int32 Team)
{
    if (Team < 1 || Team > 4)
    {
        return false;
    }
    FRA4LobbyPlayerView* Player = FindPlayer(PlayerId);
    if (!Player || !Player->bConnected)
    {
        return false;
    }
    Player->Team = Team;
    BroadcastLobbyState();
    return true;
}

bool URA4LobbyViewModel::ChangeColor(const FName PlayerId, const int32 ColorIndex)
{
    if (ColorIndex < 0 || ColorIndex > 7)
    {
        return false;
    }
    const bool bColorTaken = Players.ContainsByPredicate(
        [PlayerId, ColorIndex](const FRA4LobbyPlayerView& Player)
        {
            return Player.PlayerId != PlayerId && Player.bConnected && Player.ColorIndex == ColorIndex;
        });
    if (bColorTaken)
    {
        return false;
    }
    FRA4LobbyPlayerView* Player = FindPlayer(PlayerId);
    if (!Player || !Player->bConnected)
    {
        return false;
    }
    Player->ColorIndex = ColorIndex;
    BroadcastLobbyState();
    return true;
}

bool URA4LobbyViewModel::SendChat(const FString& Message)
{
    FString Trimmed = Message;
    Trimmed.TrimStartAndEndInline();
    if (bDisconnected || Trimmed.IsEmpty())
    {
        return false;
    }

    ChatMessages.Add(MakeChat(
        LOCTEXT("LocalChatAuthor", "SOKOLOV_1945"),
        FText::FromString(Trimmed.Left(180)),
        FLinearColor(0.95f, 0.18f, 0.20f, 1.0f)));
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ChatMessages);
    return true;
}

bool URA4LobbyViewModel::CanStartMatch() const
{
    if (!bLocalHost || bDisconnected || Players.Num() != 8)
    {
        return false;
    }

    TSet<int32> UsedColors;
    for (const FRA4LobbyPlayerView& Player : Players)
    {
        if (!Player.bConnected || !Player.bReady || Player.Team < 1 || Player.Team > 4)
        {
            return false;
        }
        if (UsedColors.Contains(Player.ColorIndex))
        {
            return false;
        }
        UsedColors.Add(Player.ColorIndex);
    }
    return true;
}

bool URA4LobbyViewModel::StartMatch()
{
    if (!CanStartMatch())
    {
        return false;
    }
    bStartRequested = true;
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bStartRequested);
    return true;
}

void URA4LobbyViewModel::LeaveLobby()
{
    HandleDisconnected();
}

void URA4LobbyViewModel::HandleDisconnected()
{
    bDisconnected = true;
    for (FRA4LobbyPlayerView& Player : Players)
    {
        Player.bConnected = false;
        Player.bReady = false;
    }
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bDisconnected);
    BroadcastLobbyState();
}

void URA4LobbyViewModel::SetLocalHost(const bool bInLocalHost)
{
    if (bLocalHost != bInLocalHost)
    {
        bLocalHost = bInLocalHost;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bLocalHost);
    }
}

FText URA4LobbyViewModel::GetValidationMessage() const
{
    if (bDisconnected)
    {
        return LOCTEXT("Disconnected", "СОЕДИНЕНИЕ С ЛОББИ ПОТЕРЯНО");
    }
    if (!bLocalHost)
    {
        return LOCTEXT("WaitingForHost", "ОЖИДАНИЕ ЗАПУСКА ХОСТОМ");
    }
    int32 ReadyPlayers = 0;
    for (const FRA4LobbyPlayerView& Player : Players)
    {
        ReadyPlayers += Player.bConnected && Player.bReady ? 1 : 0;
    }
    return FText::Format(
        LOCTEXT("ReadyCount", "{0}/8 ИГРОКОВ ГОТОВЫ"), ReadyPlayers);
}

FRA4LobbyPlayerView* URA4LobbyViewModel::FindPlayer(const FName PlayerId)
{
    return Players.FindByPredicate(
        [PlayerId](const FRA4LobbyPlayerView& Player)
        {
            return Player.PlayerId == PlayerId;
        });
}

const FRA4LobbyPlayerView* URA4LobbyViewModel::FindPlayer(const FName PlayerId) const
{
    return Players.FindByPredicate(
        [PlayerId](const FRA4LobbyPlayerView& Player)
        {
            return Player.PlayerId == PlayerId;
        });
}

void URA4LobbyViewModel::BroadcastLobbyState()
{
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Players);
}

#undef LOCTEXT_NAMESPACE
