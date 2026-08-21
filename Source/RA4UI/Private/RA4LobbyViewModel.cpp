// Copyright (c) Red Alert 4 project.

#include "RA4LobbyViewModel.h"

#define LOCTEXT_NAMESPACE "RA4LobbyViewModel"

namespace
{
FRA4LobbyPlayerView MakeLobbyPlayer(
    const TCHAR* Id,
    const FText& Name,
    const ERA4FactionTheme Faction,
    const FText& Country,
    const FText& Doctrine,
    const int32 Color,
    const int32 Team,
    const int32 Ping,
    const bool bHost = false)
{
    FRA4LobbyPlayerView Player;
    Player.PlayerId = FName(Id);
    Player.PlayerName = Name;
    Player.Faction = Faction;
    Player.CountryName = Country;
    Player.DoctrineName = Doctrine;
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
        MakeLobbyPlayer(TEXT("sokolov_1945"), LOCTEXT("Sokolov", "SOKOLOV_1945"), ERA4FactionTheme::EurasianPact, LOCTEXT("C_RU", "РОССИЯ"), LOCTEXT("D_RU", "ТЯЖЁЛЫЙ ПРОРЫВ"), 0, 1, 32, true),
        MakeLobbyPlayer(TEXT("allied_command"), LOCTEXT("AlliedCommand", "Allied_Command"), ERA4FactionTheme::AtlanticAlliance, LOCTEXT("C_US", "США"), LOCTEXT("D_US", "СЕТЕВАЯ ВОЙНА"), 1, 1, 41),
        MakeLobbyPlayer(TEXT("jade_tiger"), LOCTEXT("JadeTiger", "JadeTiger"), ERA4FactionTheme::EasternCoalition, LOCTEXT("C_CN", "КИТАЙ"), LOCTEXT("D_CN", "ДРОНОВАЯ ВОЙНА"), 2, 2, 37),
        MakeLobbyPlayer(TEXT("shin_kaze"), LOCTEXT("ShinKaze", "ShinKaze"), ERA4FactionTheme::PacificPact, LOCTEXT("C_JP", "ЯПОНИЯ"), LOCTEXT("D_JP", "ОБОРОНА ОСТРОВОВ"), 3, 2, 45),
        MakeLobbyPlayer(TEXT("desert_signal"), LOCTEXT("DesertSignal", "DesertSignal"), ERA4FactionTheme::Independent, LOCTEXT("C_IR", "ИРАН"), LOCTEXT("D_IR", "РАКЕТНЫЕ ВОЙСКА"), 4, 3, 29),
        MakeLobbyPlayer(TEXT("amazonas"), LOCTEXT("Amazonas", "Amazonas"), ERA4FactionTheme::Independent, LOCTEXT("C_BR", "БРАЗИЛИЯ"), LOCTEXT("D_BR", "РЕЧНАЯ МОБИЛЬНОСТЬ"), 5, 3, 52),
        MakeLobbyPlayer(TEXT("nord_wolf"), LOCTEXT("NordWolf", "NordWolf"), ERA4FactionTheme::AtlanticAlliance, LOCTEXT("C_DE", "ГЕРМАНИЯ"), LOCTEXT("D_DE", "ВОЗДУШНОЕ ГОСПОДСТВО"), 6, 4, 34),
        MakeLobbyPlayer(TEXT("southern_cross"), LOCTEXT("SouthernCross", "SouthernCross"), ERA4FactionTheme::PacificPact, LOCTEXT("C_AU", "АВСТРАЛИЯ"), LOCTEXT("D_AU", "ЭКСПЕДИЦИОННЫЕ СИЛЫ"), 7, 4, 47)
    };

    ChatMessages = {
        MakeChat(LOCTEXT("ChatSokolov", "SOKOLOV_1945"), LOCTEXT("ChatSokolovMessage", "Всем удачи. Победа будет за нами!"), FLinearColor(0.75f, 0.35f, 0.95f, 1.0f)),
        MakeChat(LOCTEXT("ChatAllied", "Allied_Command"), LOCTEXT("ChatAlliedMessage", "Готовы к синхронной операции."), FLinearColor(0.35f, 0.70f, 0.98f, 1.0f)),
        MakeChat(LOCTEXT("ChatJade", "JadeTiger"), LOCTEXT("ChatJadeMessage", "Дальновидность определяет победу!"), FLinearColor(0.88f, 0.72f, 0.22f, 1.0f)),
        MakeChat(LOCTEXT("ChatShin", "ShinKaze"), LOCTEXT("ChatShinMessage", "Связь установлена."), FLinearColor(0.20f, 0.80f, 0.90f, 1.0f)),
        MakeChat(LOCTEXT("ChatDesert", "DesertSignal"), LOCTEXT("ChatDesertMessage", "Пески не прощают ошибок."), FLinearColor(0.78f, 0.52f, 0.18f, 1.0f)),
        MakeChat(LOCTEXT("ChatQuantum", "QuantumEcho"), LOCTEXT("ChatQuantumMessage", "Кибер-превосходство — ключ к победе."), FLinearColor(0.40f, 0.85f, 0.95f, 1.0f)),
        MakeChat(LOCTEXT("ChatNord", "NordWolf"), LOCTEXT("ChatNordMessage", "В небе решается исход войны."), FLinearColor(0.45f, 0.75f, 1.0f, 1.0f)),
        MakeChat(LOCTEXT("ChatIron", "IronHawk"), LOCTEXT("ChatIronMessage", "Сталь и скорость!"), FLinearColor(0.95f, 0.80f, 0.40f, 1.0f))
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
