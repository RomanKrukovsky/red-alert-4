// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RA4CampaignSelectWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UTextBlock;
class UVerticalBox;
class URA4CampaignViewModel;

/** Reference-driven campaign selection screen with four real interactive faction cards. */
UCLASS(Blueprintable)
class RA4UI_API URA4CampaignSelectWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UFUNCTION()
    void SelectUSSR();

    UFUNCTION()
    void SelectAlliance();

    UFUNCTION()
    void SelectEasternCoalition();

    UFUNCTION()
    void SelectChronolegion();

    UFUNCTION()
    void ContinueCampaign();

    UFUNCTION()
    void OpenMainMenu();

    UFUNCTION()
    void OpenMultiplayer();

    UFUNCTION()
    void OpenChallenges();

    UFUNCTION()
    void OpenBarracks();

    UFUNCTION()
    void OpenSettings();

    void BuildLayout();
    void SelectFaction(int32 FactionIndex);
    void NavigateToScreen(int32 ScreenIndex);
    void AnimateEntrance();

    UPROPERTY(Transient)
    TObjectPtr<URA4CampaignViewModel> CampaignViewModel;

    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> MainCanvas;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UBorder>> CardFrames;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> FactionNameText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> FactionMottoText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> FactionDescriptionText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CampaignProgressText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ContinueLabelText;

    int32 SelectedFactionIndex = 0;
    FTimerHandle EntranceTimer;
    float EntranceElapsed = 0.0f;
};
