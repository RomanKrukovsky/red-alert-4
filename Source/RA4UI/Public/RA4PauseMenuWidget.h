// Copyright (c) Red Alert 4 project. In-Game Pause and Tactical Settings Menu.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RA4PauseMenuWidget.generated.h"

class UTextBlock;
class UButton;
class UComboBoxString;
class UBorder;
class UVerticalBox;
class UWidgetSwitcher;

DECLARE_MULTICAST_DELEGATE(FOnPauseMenuAction);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPauseMenuTrackSelected, int32 /* TrackIndex */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPauseMenuVolumeChanged, float /* DeltaVolume */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPauseMenuIntSettingChanged, int32 /* Value */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPauseMenuFloatSettingChanged, float /* Value */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPauseMenuBoolSettingChanged, bool /* Value */);

UENUM()
enum class ERA4PauseSettingsTab : uint8
{
    Graphics = 0,
    Audio = 1,
    Gameplay = 2
};

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

    // Music Player Callbacks
    FOnPauseMenuAction OnNextTrackRequested;
    FOnPauseMenuAction OnPrevTrackRequested;
    FOnPauseMenuAction OnToggleMusicPauseRequested;
    FOnPauseMenuTrackSelected OnTrackSelected;
    FOnPauseMenuVolumeChanged OnVolumeChanged;

    // Tactical Settings Callbacks
    FOnPauseMenuIntSettingChanged OnQualityPresetChanged;
    FOnPauseMenuIntSettingChanged OnFpsCapChanged;
    FOnPauseMenuIntSettingChanged OnAntiAliasingChanged;
    FOnPauseMenuBoolSettingChanged OnScreenShakeChanged;
    FOnPauseMenuFloatSettingChanged OnMasterVolumeChanged;
    FOnPauseMenuFloatSettingChanged OnSfxVolumeChanged;
    FOnPauseMenuFloatSettingChanged OnEvaVolumeChanged;
    FOnPauseMenuBoolSettingChanged OnUnitVoicesChanged;
    FOnPauseMenuIntSettingChanged OnControlSchemeChanged;
    FOnPauseMenuFloatSettingChanged OnCameraSpeedChanged;
    FOnPauseMenuBoolSettingChanged OnEdgeScrollChanged;
    FOnPauseMenuIntSettingChanged OnHealthBarModeChanged;
    FOnPauseMenuFloatSettingChanged OnDirectControlFovChanged;

    void SetTrackList(const TArray<FString>& InTrackNames, int32 InCurrentIndex = 0);
    void SetCurrentTrack(int32 InCurrentIndex, const FString& InTrackTitle, bool bIsPlaying = true);
    void SetMusicVolume(float InVolume);

    void ShowMainMenu();
    void ShowSettingsMenu();

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

    UFUNCTION()
    void HandlePrevTrackClicked();

    UFUNCTION()
    void HandleNextTrackClicked();

    UFUNCTION()
    void HandleToggleMusicClicked();

    UFUNCTION()
    void HandleTrackSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void HandleVolumeDownClicked();

    UFUNCTION()
    void HandleVolumeUpClicked();

    // Settings Navigation
    UFUNCTION()
    void HandleSettingsBackClicked();

    UFUNCTION()
    void HandleSettingsDefaultsClicked();

    UFUNCTION()
    void HandleTabGraphicsClicked();

    UFUNCTION()
    void HandleTabAudioClicked();

    UFUNCTION()
    void HandleTabGameplayClicked();

    // Settings adjustments
    void SetSettingsTab(ERA4PauseSettingsTab Tab);
    void UpdateSettingsVisuals();

    // Graphics handlers
    UFUNCTION() void HandleQualityLowClicked();
    UFUNCTION() void HandleQualityMedClicked();
    UFUNCTION() void HandleQualityHighClicked();
    UFUNCTION() void HandleQualityEpicClicked();

    UFUNCTION() void HandleFps60Clicked();
    UFUNCTION() void HandleFps120Clicked();
    UFUNCTION() void HandleFps144Clicked();
    UFUNCTION() void HandleFpsMaxClicked();

    UFUNCTION() void HandleAaFxaaClicked();
    UFUNCTION() void HandleAaTaaClicked();
    UFUNCTION() void HandleAaTsrClicked();

    UFUNCTION() void HandleScreenShakeToggled();

    // Audio handlers
    UFUNCTION() void HandleMasterVolDown();
    UFUNCTION() void HandleMasterVolUp();
    UFUNCTION() void HandleSfxVolDown();
    UFUNCTION() void HandleSfxVolUp();
    UFUNCTION() void HandleEvaVolDown();
    UFUNCTION() void HandleEvaVolUp();
    UFUNCTION() void HandleUnitVoiceToggled();

    // Gameplay handlers
    UFUNCTION() void HandleControlClassicClicked();
    UFUNCTION() void HandleControlModernClicked();
    UFUNCTION() void HandlePanSpeedSlow();
    UFUNCTION() void HandlePanSpeedNormal();
    UFUNCTION() void HandlePanSpeedFast();
    UFUNCTION() void HandlePanSpeedUltra();
    UFUNCTION() void HandleEdgeScrollToggled();
    UFUNCTION() void HandleHealthBarModeToggle();
    UFUNCTION() void HandleDcFov80();
    UFUNCTION() void HandleDcFov90();
    UFUNCTION() void HandleDcFov100();
    UFUNCTION() void HandleDcFov110();

    UPROPERTY(Transient)
    TObjectPtr<UBorder> MainPausePanel;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> SettingsPanel;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> GraphicsContentStack;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> AudioContentStack;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> GameplayContentStack;

    UPROPERTY(Transient)
    TObjectPtr<UButton> TabGfxButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> TabAudButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> TabGameButton;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CurrentTrackDisplay;

    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> TrackSelectorCombo;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> VolumeTextDisplay;

    UPROPERTY(Transient)
    TObjectPtr<UButton> PlayPauseButton;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PlayPauseLabel;

    // Setting Labels for live feedback
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MasterVolLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SfxVolLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> EvaVolLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> UnitVoiceBtnLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ScreenShakeBtnLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> EdgeScrollBtnLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HealthBarModeLabel;

    int32 CurrentTrackIndex = 0;
    float CurrentVolume = 0.35f;
    bool bPlaying = true;
    bool bIsInternalTrackSelection = false;

    // Active Setting Values
    ERA4PauseSettingsTab ActiveTab = ERA4PauseSettingsTab::Graphics;
    int32 QualityPreset = 2; // 0=Low, 1=Med, 2=High, 3=Epic
    int32 FpsCap = 120;
    int32 AntiAliasingMethod = 2; // 0=FXAA, 1=TAA, 2=TSR
    bool bScreenShakeEnabled = true;

    float MasterVolume = 0.85f;
    float SfxVolume = 1.0f;
    float EvaVolume = 1.0f;
    bool bUnitVoiceEnabled = true;

    int32 ControlScheme = 1; // 0=Classic C&C, 1=Modern RTS
    float CameraPanSpeed = 1.5f;
    bool bEdgeScrollEnabled = true;
    int32 HealthBarMode = 1; // 0=Always, 1=OnDamage, 2=SelectedOnly
    float DirectControlFov = 90.0f;
};

