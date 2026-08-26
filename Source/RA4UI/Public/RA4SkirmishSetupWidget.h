// Copyright (c) Red Alert 4 project. Skirmish Setup Menu Widget.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RA4SkirmishSetupWidget.generated.h"

class UCanvasPanel;
class UComboBoxString;
class UButton;
class UTextBlock;
class UBorder;

/**
 * Production Skirmish Setup Widget.
 * Handles map selection, player/AI factions, colors, start spots, difficulty,
 * starting credits, rules, and validates color/start position conflicts before starting.
 */
UCLASS(Blueprintable)
class RA4UI_API URA4SkirmishSetupWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    URA4SkirmishSetupWidget(const FObjectInitializer& ObjectInitializer);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void HandleOptionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void HandleStartMatchClicked();

    UFUNCTION()
    void HandleBackClicked();

    void BuildLayout();
    void UpdateConflictValidation();
    void LaunchSkirmishMatch();

    // Selections
    int32 SelectedMapIndex = 0;
    int32 PlayerFactionIndex = 0;   // 0: USSR, 1: Alliance, 2: Coalition, 3: Chrono
    int32 AIFactionIndex = 1;       // 0: USSR, 1: Alliance, 2: Coalition, 3: Chrono
    int32 PlayerColorIndex = 0;     // Red
    int32 AIColorIndex = 1;         // Blue
    int32 PlayerSpotIndex = 0;      // Spot 1
    int32 AISpotIndex = 1;          // Spot 2
    int32 DifficultyIndex = 1;      // Medium
    int32 CreditsIndex = 1;         // 10,000

    bool bFogOfWarEnabled = true;
    bool bSuperweaponsEnabled = true;

    // UI Widgets
    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> MainCanvas;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> MapCombo;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> PlayerFactionCombo;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> AIFactionCombo;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> PlayerColorCombo;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> AIColorCombo;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> PlayerSpotCombo;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> AISpotCombo;
    TObjectPtr<UComboBoxString> NumAICombo;
    TObjectPtr<UComboBoxString> TeamCombo;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> DifficultyCombo;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> CreditsCombo;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ValidationWarningText;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> ValidationBanner;

    UPROPERTY(Transient)
    TObjectPtr<UButton> StartButton;
};
