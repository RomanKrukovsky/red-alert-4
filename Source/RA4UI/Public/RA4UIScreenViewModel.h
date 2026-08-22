// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4UITheme.h"
#include "RA4ViewModelBase.h"
#include "RA4UIScreenViewModel.generated.h"

UENUM(BlueprintType)
enum class ERA4UIScreenId : uint8
{
    Splash,
    MainMenu,
    CampaignSelect,
    EurasianCampaign,
    AtlanticCampaign,
    EasternCampaign,
    PacificCampaign,
    IndependentCampaign,
    MissionMap,
    Briefing,
    VideoComms,
    Loading,
    EurasianHud,
    AtlanticHud,
    EasternHud,
    PacificHud,
    IndependentHud,
    Pause,
    Victory,
    MultiplayerLobby,
    Encyclopedia,
    TechTree,
    Mods,
    Settings,

    // Legacy aliases for content authored before the Scarlet Horizon rename.
    SovietCampaign = EurasianCampaign,
    AlliesCampaign = AtlanticCampaign,
    SovietHud = EurasianHud,
    AlliesHud = AtlanticHud
};

USTRUCT(BlueprintType)
struct FRA4ProductionQueueItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Production")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Production")
    int32 Cost = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Production", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Progress = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Production")
    int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct FRA4LobbyPlayerSlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    FText PlayerName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    ERA4FactionTheme Faction = ERA4FactionTheme::EurasianPact;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
    bool bReady = false;
};

/**
 * Presentation-only state shared by all UMG screens. Game code updates it via
 * an adapter; widgets never read the deterministic simulation directly.
 */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4UIScreenViewModel : public URA4ViewModelBase
{
    GENERATED_BODY()

public:
    URA4UIScreenViewModel();

    UFUNCTION(BlueprintCallable, Category = "UI|Navigation")
    void SetActiveScreen(ERA4UIScreenId InScreen);

    UFUNCTION(BlueprintPure, Category = "UI|Navigation")
    ERA4UIScreenId GetActiveScreen() const;

    UFUNCTION(BlueprintCallable, Category = "UI|Campaign")
    void SetSelectedFaction(ERA4FactionTheme InFaction);

    UFUNCTION(BlueprintPure, Category = "UI|Campaign")
    ERA4FactionTheme GetSelectedFaction() const;

    UFUNCTION(BlueprintCallable, Category = "UI|Campaign")
    void SetSelectedMission(int32 InMissionIndex);

    UFUNCTION(BlueprintPure, Category = "UI|Campaign")
    int32 GetSelectedMission() const;

    UFUNCTION(BlueprintCallable, Category = "UI|Loading")
    void SetLoadingProgress(float InProgress);

    UFUNCTION(BlueprintPure, Category = "UI|Loading")
    float GetLoadingProgress() const;

    UFUNCTION(BlueprintCallable, Category = "UI|Modal")
    void ShowModal(const FText& InTitle, const FText& InBody);

    UFUNCTION(BlueprintCallable, Category = "UI|Modal")
    void CloseModal();

    UFUNCTION(BlueprintPure, Category = "UI|Modal")
    bool IsModalVisible() const;

    UFUNCTION(BlueprintPure, Category = "UI|Modal")
    FText GetModalTitle() const;

    UFUNCTION(BlueprintPure, Category = "UI|Modal")
    FText GetModalBody() const;

    UFUNCTION(BlueprintCallable, Category = "UI|Production")
    void SetProductionQueue(const TArray<FRA4ProductionQueueItem>& InQueue);

    UFUNCTION(BlueprintPure, Category = "UI|Production")
    const TArray<FRA4ProductionQueueItem>& GetProductionQueue() const;

    UFUNCTION(BlueprintCallable, Category = "UI|Lobby")
    void SetLobbySlots(const TArray<FRA4LobbyPlayerSlot>& InSlots);

    UFUNCTION(BlueprintPure, Category = "UI|Lobby")
    const TArray<FRA4LobbyPlayerSlot>& GetLobbySlots() const;

private:
    UPROPERTY(FieldNotify, Setter, Getter)
    ERA4UIScreenId ActiveScreen = ERA4UIScreenId::Splash;

    UPROPERTY(FieldNotify, Setter, Getter)
    ERA4FactionTheme SelectedFaction = ERA4FactionTheme::EurasianPact;

    UPROPERTY(FieldNotify, Setter, Getter)
    int32 SelectedMission = 0;

    UPROPERTY(FieldNotify, Setter, Getter)
    float LoadingProgress = 0.0f;

    UPROPERTY(FieldNotify)
    bool bModalVisible = false;

    UPROPERTY(FieldNotify)
    FText ModalTitle;

    UPROPERTY(FieldNotify)
    FText ModalBody;

    UPROPERTY(FieldNotify)
    TArray<FRA4ProductionQueueItem> ProductionQueue;

    UPROPERTY(FieldNotify)
    TArray<FRA4LobbyPlayerSlot> LobbySlots;
};
