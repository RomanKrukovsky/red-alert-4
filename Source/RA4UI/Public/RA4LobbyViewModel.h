// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4UITheme.h"
#include "RA4ViewModelBase.h"
#include "RA4LobbyViewModel.generated.h"

USTRUCT(BlueprintType)
struct FRA4LobbyPlayerView
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    FName PlayerId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    FText PlayerName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    ERA4FactionTheme Faction = ERA4FactionTheme::EurasianPact;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    FText CountryName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    FText DoctrineName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    int32 ColorIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    int32 Team = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    int32 Ping = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    bool bReady = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    bool bHost = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    bool bConnected = true;
};

USTRUCT(BlueprintType)
struct FRA4LobbyChatMessageView
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    FText Author;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    FText Message;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    FLinearColor AuthorColor = FLinearColor::White;
};

/** Network-independent presentation state and validation for the multiplayer lobby. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4LobbyViewModel : public URA4ViewModelBase
{
    GENERATED_BODY()

public:
    URA4LobbyViewModel();

    UFUNCTION(BlueprintPure, Category = "UI|Lobby")
    const TArray<FRA4LobbyPlayerView>& GetPlayers() const { return Players; }

    UFUNCTION(BlueprintPure, Category = "UI|Lobby")
    const TArray<FRA4LobbyChatMessageView>& GetChatMessages() const { return ChatMessages; }

    UFUNCTION(BlueprintCallable, Category = "UI|Lobby")
    bool SetReady(FName PlayerId, bool bReady);

    UFUNCTION(BlueprintCallable, Category = "UI|Lobby")
    bool ChangeFaction(FName PlayerId, ERA4FactionTheme Faction);

    UFUNCTION(BlueprintCallable, Category = "UI|Lobby")
    bool ChangeTeam(FName PlayerId, int32 Team);

    UFUNCTION(BlueprintCallable, Category = "UI|Lobby")
    bool ChangeColor(FName PlayerId, int32 ColorIndex);

    UFUNCTION(BlueprintCallable, Category = "UI|Lobby")
    bool SendChat(const FString& Message);

    UFUNCTION(BlueprintPure, Category = "UI|Lobby")
    bool CanStartMatch() const;

    UFUNCTION(BlueprintCallable, Category = "UI|Lobby")
    bool StartMatch();

    UFUNCTION(BlueprintCallable, Category = "UI|Lobby")
    void LeaveLobby();

    UFUNCTION(BlueprintCallable, Category = "UI|Lobby")
    void HandleDisconnected();

    UFUNCTION(BlueprintCallable, Category = "UI|Lobby")
    void SetLocalHost(bool bInLocalHost);

    UFUNCTION(BlueprintPure, Category = "UI|Lobby")
    bool IsDisconnected() const { return bDisconnected; }

    UFUNCTION(BlueprintPure, Category = "UI|Lobby")
    FText GetValidationMessage() const;

private:
    FRA4LobbyPlayerView* FindPlayer(FName PlayerId);
    const FRA4LobbyPlayerView* FindPlayer(FName PlayerId) const;
    void BroadcastLobbyState();

    UPROPERTY(FieldNotify)
    TArray<FRA4LobbyPlayerView> Players;

    UPROPERTY(FieldNotify)
    TArray<FRA4LobbyChatMessageView> ChatMessages;

    UPROPERTY(FieldNotify)
    bool bLocalHost = true;

    UPROPERTY(FieldNotify)
    bool bDisconnected = false;

    UPROPERTY(FieldNotify)
    bool bStartRequested = false;
};
