// Copyright (c) Red Alert 4 project.
#pragma once

#include "CoreMinimal.h"
#include "RA4HUDWidget.h"

#include "RA4MatchResultOverlayWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE(FRA4OnMatchResultAction);

UCLASS()
class RA4UI_API URA4MatchResultOverlayWidget : public URA4MatchResultWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;

    void Configure(bool bLocalPlayerWon, bool bCanReturnToMenu);

    FRA4OnMatchResultAction OnRetryRequested;
    FRA4OnMatchResultAction OnExitRequested;

private:
    UFUNCTION()
    void HandleRetryClicked();

    UFUNCTION()
    void HandleExitClicked();

    void RefreshTexts();

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HeadingText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> BodyText;

    UPROPERTY(Transient)
    TObjectPtr<UButton> RetryButton;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> RetryButtonText;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ExitButton;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ExitButtonText;

    bool bVictory = false;
    bool bHasMainMenuLevel = false;
};
