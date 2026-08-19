// Copyright (c) Red Alert 4 project. In-Game Pause and Quit Menu.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RA4PauseMenuWidget.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnPauseMenuAction);

UCLASS()
class RA4UI_API URA4PauseMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    FOnPauseMenuAction OnResumeRequested;
    FOnPauseMenuAction OnRestartRequested;
    FOnPauseMenuAction OnSettingsRequested;
    FOnPauseMenuAction OnQuitToMenuRequested;
    FOnPauseMenuAction OnQuitToDesktopRequested;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    UFUNCTION()
    void HandleResumeClicked();

    UFUNCTION()
    void HandleRestartClicked();

    UFUNCTION()
    void HandleSettingsClicked();

    UFUNCTION()
    void HandleQuitToMenuClicked();

    UFUNCTION()
    void HandleQuitToDesktopClicked();
};
