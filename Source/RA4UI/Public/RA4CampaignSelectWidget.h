// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RA4FactionData.h"
#include "RA4CampaignSelectWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UHorizontalBox;
class UProgressBar;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;
class URA4CampaignViewModel;

UENUM(BlueprintType)
enum class ERA4CampaignSelectStep : uint8
{
    BlocSelection = 0,
    CountrySelection = 1,
    DoctrineSelection = 2
};

/**
 * Modern Scarlet Horizon 3-step selection flow:
 * Step 1: Bloc or Category (5 options: Eurasian Pact, Atlantic Alliance, Eastern Coalition, Pacific Pact, Independent Powers)
 * Step 2: Country selection (with ratings, specialization, and flag accents)
 * Step 3: Doctrine selection (with unit swaps, tactical traits, and combat philosophy)
 */
UCLASS(Blueprintable)
class RA4UI_API URA4CampaignSelectWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION()
    void OnBloc0();
    UFUNCTION()
    void OnBloc1();
    UFUNCTION()
    void OnBloc2();
    UFUNCTION()
    void OnBloc3();
    UFUNCTION()
    void OnBloc4();

    UFUNCTION()
    void OnCountry0();
    UFUNCTION()
    void OnCountry1();
    UFUNCTION()
    void OnCountry2();
    UFUNCTION()
    void OnCountry3();
    UFUNCTION()
    void OnCountry4();
    UFUNCTION()
    void OnCountry5();

    UFUNCTION()
    void OnDoctrine0();
    UFUNCTION()
    void OnDoctrine1();
    UFUNCTION()
    void OnDoctrine2();

    void OnBlocCardClicked(int32 BlocIndex);
    void OnCountryCardClicked(int32 CountryIndex);
    void OnDoctrineCardClicked(int32 DoctrineIndex);

    UFUNCTION()
    void GotoBlocStep();

    UFUNCTION()
    void GotoCountryStep();

    UFUNCTION()
    void GotoDoctrineStep();

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
    void RefreshBreadcrumbs();
    void RefreshBlocCards();
    void RefreshCountryCards();
    void RefreshDoctrineCards();
    void RefreshDossierPanel();
    void AnimateEntrance();

    UPROPERTY(Transient)
    TObjectPtr<URA4CampaignViewModel> CampaignViewModel;

    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> MainCanvas;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BreadcrumbBlocBtn;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BreadcrumbCountryBtn;

    UPROPERTY(Transient)
    TObjectPtr<UButton> BreadcrumbDoctrineBtn;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> BreadcrumbBlocText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> BreadcrumbCountryText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> BreadcrumbDoctrineText;

    /**
     * The direction step is a mosaic, not a row: one hero plate and four unequal
     * plates at fixed reference positions, so it needs a canvas rather than a box.
     */
    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanel> BlocCardsContainer;

    UPROPERTY(Transient)
    TObjectPtr<UWidget> DossierFrameWidget;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> CountryCardsContainer;

    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> DoctrineCardsContainer;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UBorder>> BlocCardFrames;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UBorder>> CountryCardFrames;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UBorder>> DoctrineCardFrames;

    // Dossier fields
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DossierHeaderTag;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DossierTitleText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DossierSubtitleText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DossierDescriptionText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DossierSpecializationText;

    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> FirepowerBar;

    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> ArmorBar;

    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> MobilityBar;

    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> TechBar;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ContinueLabelText;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ContinueButton;

    ERA4CampaignSelectStep CurrentStep = ERA4CampaignSelectStep::BlocSelection;
    int32 SelectedBlocIndex = 0;
    int32 SelectedCountryIndex = 0;
    int32 SelectedDoctrineIndex = 0;

    FTimerHandle EntranceTimer;
    float EntranceElapsed = 0.0f;
};
