// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "Components/ListView.h"
#include "RA4LobbyViewModel.h"
#include "RA4ScreenRootWidget.h"
#include "RA4LobbyScreenWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;

UCLASS()
class RA4UI_API URA4LobbyPlayerListView : public UListView
{
    GENERATED_BODY()

public:
    void ConfigureEntryWidgetClass(TSubclassOf<UUserWidget> InClass)
    {
        EntryWidgetClass = InClass;
    }
};

UCLASS()
class RA4UI_API URA4LobbyPlayerListItem : public UObject
{
    GENERATED_BODY()

public:
    void SetData(const FRA4LobbyPlayerView& InPlayer) { Player = InPlayer; }
    const FRA4LobbyPlayerView& GetData() const { return Player; }

private:
    UPROPERTY()
    FRA4LobbyPlayerView Player;
};

UCLASS()
class RA4UI_API URA4LobbyPlayerRowWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> IndexText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PlayerText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> FactionText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ColorText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TeamText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ReadyText;
};

/** Multiplayer lobby matching reference 17 with virtualized player rows. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4LobbyScreenWidget : public URA4ScreenRootWidget
{
    GENERATED_BODY()

public:
    URA4LobbyScreenWidget(const FObjectInitializer& ObjectInitializer);

    UListView* GetPlayerList() const { return PlayerList; }
    UButton* GetStartButton() const { return StartButton; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    UFUNCTION()
    void SendChat();

    UFUNCTION()
    void StartMatch();

    UFUNCTION()
    void LeaveLobby();

    void RefreshStartState();
    void PopulatePlayerList();

    UPROPERTY(Transient)
    TObjectPtr<URA4LobbyViewModel> LobbyViewModel;

    UPROPERTY(Transient)
    TObjectPtr<URA4LobbyPlayerListView> PlayerList;

    UPROPERTY(Transient)
    TArray<TObjectPtr<URA4LobbyPlayerListItem>> PlayerItems;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> ChatInput;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ReadyStatusText;

    UPROPERTY(Transient)
    TObjectPtr<UButton> StartButton;
};
