// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4UIScreenViewModel.h"
#include "RA4ViewModelBase.h"
#include "RA4CampaignViewModel.generated.h"

UENUM(BlueprintType)
enum class ERA4CampaignDifficulty : uint8
{
    Recruit,
    Normal,
    Veteran
};

UENUM(BlueprintType)
enum class ERA4CampaignFlowStage : uint8
{
    FactionSelect,
    CampaignOverview,
    MissionMap,
    Briefing,
    VideoComms,
    Loading
};

USTRUCT(BlueprintType)
struct FRA4FactionCardView
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    FName ContentId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    ERA4FactionTheme Theme = ERA4FactionTheme::EurasianPact;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    FText Motto;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    FText CommanderName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    int32 CompletedMissions = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    int32 TotalMissions = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Progress = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    bool bLocked = false;
};

USTRUCT(BlueprintType)
struct FRA4MissionNodeView
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    FName ContentId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    ERA4FactionTheme Theme = ERA4FactionTheme::EurasianPact;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    FText Location;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    FText Objective;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    int32 MissionNumber = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign", meta = (ClampMin = "0", ClampMax = "3"))
    int32 Stars = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    bool bCompleted = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campaign")
    bool bLocked = false;
};

/** Presentation state for faction selection, campaign chapters and mission launch flow. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4CampaignViewModel : public URA4ViewModelBase
{
    GENERATED_BODY()

public:
    URA4CampaignViewModel();

    UFUNCTION(BlueprintPure, Category = "UI|Campaign")
    const TArray<FRA4FactionCardView>& GetFactionCards() const { return FactionCards; }

    UFUNCTION(BlueprintPure, Category = "UI|Campaign")
    const TArray<FRA4MissionNodeView>& GetMissionNodes() const { return MissionNodes; }

    UFUNCTION(BlueprintPure, Category = "UI|Campaign")
    ERA4FactionTheme GetSelectedFaction() const { return SelectedFaction; }

    UFUNCTION(BlueprintPure, Category = "UI|Campaign")
    FName GetSelectedMissionId() const { return SelectedMissionId; }

    UFUNCTION(BlueprintPure, Category = "UI|Campaign")
    ERA4CampaignDifficulty GetDifficulty() const { return Difficulty; }

    UFUNCTION(BlueprintPure, Category = "UI|Campaign")
    ERA4CampaignFlowStage GetFlowStage() const { return FlowStage; }

    UFUNCTION(BlueprintCallable, Category = "UI|Campaign")
    bool SelectFaction(ERA4FactionTheme InFaction);

    UFUNCTION(BlueprintCallable, Category = "UI|Campaign")
    bool SelectMission(FName MissionId);

    UFUNCTION(BlueprintCallable, Category = "UI|Campaign")
    bool SetCampaignProgress(ERA4FactionTheme Faction, int32 CompletedMissions, int32 TotalMissions);

    UFUNCTION(BlueprintCallable, Category = "UI|Campaign")
    void SetDifficulty(ERA4CampaignDifficulty InDifficulty);

    UFUNCTION(BlueprintCallable, Category = "UI|Campaign")
    bool StartMission();

    UFUNCTION(BlueprintCallable, Category = "UI|Campaign")
    void SkipBriefing();

    const FRA4FactionCardView* FindFaction(ERA4FactionTheme Faction) const;
    const FRA4MissionNodeView* FindSelectedMission() const;
    ERA4UIScreenId GetSelectedCampaignScreen() const;

private:
    void RefreshMissionNodes();

    UPROPERTY(FieldNotify)
    TArray<FRA4FactionCardView> FactionCards;

    UPROPERTY()
    TArray<FRA4MissionNodeView> AllMissionNodes;

    UPROPERTY(FieldNotify)
    TArray<FRA4MissionNodeView> MissionNodes;

    UPROPERTY(FieldNotify)
    ERA4FactionTheme SelectedFaction = ERA4FactionTheme::EurasianPact;

    UPROPERTY(FieldNotify)
    FName SelectedMissionId;

    UPROPERTY(FieldNotify)
    ERA4CampaignDifficulty Difficulty = ERA4CampaignDifficulty::Veteran;

    UPROPERTY(FieldNotify)
    ERA4CampaignFlowStage FlowStage = ERA4CampaignFlowStage::FactionSelect;
};
