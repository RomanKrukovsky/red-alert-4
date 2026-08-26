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
class UEditableTextBox;

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
    void HandleAllianceNameChanged(const FText& Text);

    UFUNCTION()
    void HandleStartMatchClicked();

    UFUNCTION()
    void HandleBackClicked();

    void BuildLayout();
    void UpdateConflictValidation();
    void LaunchSkirmishMatch();

    // Selections
    int32 SelectedMapIndex = 0;
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
    TArray<TObjectPtr<UComboBoxString>> SlotStatusCombos;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UComboBoxString>> SlotFactionCombos;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UComboBoxString>> SlotTeamCombos;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UComboBoxString>> SlotSpotCombos;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UEditableTextBox>> AllianceNameEdits;

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
