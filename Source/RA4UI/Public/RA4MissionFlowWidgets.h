// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "RA4CampaignViewModel.h"
#include "RA4ScreenRootWidget.h"
#include "RA4MissionFlowWidgets.generated.h"

class UProgressBar;
class URA4MissionMapScreenWidget;

UCLASS()
class RA4UI_API URA4MissionNodeButton : public UButton
{
    GENERATED_BODY()

public:
    void InitializeMissionNode(URA4MissionMapScreenWidget* InOwner, FName InMissionId);

private:
    UFUNCTION()
    void HandleMissionClicked();

    UPROPERTY(Transient)
    TObjectPtr<URA4MissionMapScreenWidget> MissionOwner;

    FName MissionId;
};

/** Interactive strategic mission map matching reference 8. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4MissionMapScreenWidget : public URA4ScreenRootWidget
{
    GENERATED_BODY()

public:
    URA4MissionMapScreenWidget(const FObjectInitializer& ObjectInitializer);

    const TArray<TObjectPtr<UButton>>& GetMissionButtons() const { return MissionButtons; }
    void SelectMissionById(FName MissionId);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    UFUNCTION()
    void StartSelectedMission();

    UFUNCTION()
    void GoBack();

    void RefreshMissionDetails();

    UPROPERTY(Transient)
    TObjectPtr<URA4CampaignViewModel> CampaignViewModel;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UButton>> MissionButtons;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> MissionTitleText;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> MissionObjectiveText;
};

/** Full operation briefing screen matching reference 9. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4BriefingScreenWidget : public URA4ScreenRootWidget
{
    GENERATED_BODY()

public:
    URA4BriefingScreenWidget(const FObjectInitializer& ObjectInitializer);
    UButton* GetContinueButton() const { return ContinueButton; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    UFUNCTION()
    void ContinueToComms();

    UFUNCTION()
    void GoBack();

    UPROPERTY(Transient)
    TObjectPtr<UButton> ContinueButton;
};

/** Secure split-screen command call matching reference 10. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4VideoCommsScreenWidget : public URA4ScreenRootWidget
{
    GENERATED_BODY()

public:
    URA4VideoCommsScreenWidget(const FObjectInitializer& ObjectInitializer);
    UButton* GetEndSessionButton() const { return EndSessionButton; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    UFUNCTION()
    void EndSession();

    UPROPERTY(Transient)
    TObjectPtr<UButton> EndSessionButton;
};

/** Native loading screen for references 12 and 19. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4LoadingScreenWidget : public URA4ScreenRootWidget
{
    GENERATED_BODY()

public:
    URA4LoadingScreenWidget(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = "RA4|Loading")
    void SetLoadingProgress(float InProgress);

    UFUNCTION(BlueprintCallable, Category = "RA4|Loading")
    void SetLoadingVariant(ERA4UIScreenVariant InVariant);

    float GetLoadingProgress() const { return LoadingProgress; }
    ERA4UIScreenVariant GetLoadingVariant() const { return LoadingVariant; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void RefreshProgressVisuals();

    UPROPERTY(EditDefaultsOnly, Category = "RA4|Loading")
    ERA4UIScreenVariant LoadingVariant = ERA4UIScreenVariant::Default;

    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> LoadingProgressBar;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> LoadingPercentText;

    float LoadingProgress = 0.0f;
};
