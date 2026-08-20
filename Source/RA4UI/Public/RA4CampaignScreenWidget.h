// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4CampaignViewModel.h"
#include "RA4ScreenRootWidget.h"
#include "RA4CampaignScreenWidget.generated.h"

class UButton;

/** Shared native faction campaign screen for references 4-7, 11 and 18. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4CampaignScreenWidget : public URA4ScreenRootWidget
{
    GENERATED_BODY()

public:
    URA4CampaignScreenWidget(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = "RA4|Campaign")
    void ConfigureCampaign(
        ERA4FactionTheme InFaction,
        ERA4UIScreenVariant InVariant = ERA4UIScreenVariant::Default);

    ERA4FactionTheme GetFactionTheme() const { return FactionTheme; }
    ERA4UIScreenVariant GetScreenVariant() const { return CampaignVariant; }
    const TArray<TObjectPtr<UButton>>& GetActionButtons() const { return ActionButtons; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RA4|Campaign")
    ERA4FactionTheme FactionTheme = ERA4FactionTheme::USSR;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RA4|Campaign")
    ERA4UIScreenVariant CampaignVariant = ERA4UIScreenVariant::Default;

private:
    UFUNCTION()
    void StartNewCampaign();

    UFUNCTION()
    void ContinueCampaign();

    UFUNCTION()
    void OpenChapterMap();

    UFUNCTION()
    void GoBack();

    void OpenMissionMap();

    UPROPERTY(Transient)
    TObjectPtr<URA4CampaignViewModel> CampaignViewModel;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UButton>> ActionButtons;
};
