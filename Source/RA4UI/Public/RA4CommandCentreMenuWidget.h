// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RA4CommandCentreMenuWidget.generated.h"

class UCanvasPanel;
class UAudioComponent;
class UOverlay;

/**
 * Production presentation of the command-centre main menu.
 *
 * The class owns presentation and navigation only. No gameplay state is read
 * here, so the visual Blueprint child can be replaced without touching the
 * simulation layer.
 */
UCLASS(Blueprintable)
class RA4UI_API URA4CommandCentreMenuWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UFUNCTION()
    void OpenCampaign();

    UFUNCTION()
    void OpenMultiplayer();

    UFUNCTION()
    void OpenSkirmish();

    UFUNCTION()
    void OpenEditor();

    UFUNCTION()
    void OpenEncyclopedia();

    UFUNCTION()
    void OpenModifications();

    UFUNCTION()
    void OpenSettings();

    UFUNCTION()
    void RequestExit();

    UFUNCTION()
    void CancelExit();

    UFUNCTION()
    void ConfirmExit();

    void BuildLayout();
    void NavigateToScreen(int32 ScreenIndex);
    void AnimateEntrance();

    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> MainCanvas;

    UPROPERTY(Transient)
    TObjectPtr<UOverlay> ExitModal;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> MenuMusic;

    FTimerHandle EntranceTimer;
    float EntranceElapsed = 0.0f;
};
