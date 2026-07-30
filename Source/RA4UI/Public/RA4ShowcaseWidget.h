// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RA4ShowcaseWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UProgressBar;
class UTextBlock;
class UVerticalBox;

/**
 * A runnable, data-free UI presentation used by the project entry map. It keeps
 * all interaction in UMG widgets and can later be visually overridden by its
 * Widget Blueprint child without connecting to simulation state.
 */
UCLASS(Blueprintable)
class RA4UI_API URA4ShowcaseWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Opens a presentation route; intended for Blueprint previews and UI QA. */
    UFUNCTION(BlueprintCallable, Category = "RA4|Screen")
    void ShowScreen(int32 InScreen);

    /** Sets the route before the widget is added to the viewport. */
    void SetInitialScreenForPresentation(int32 InScreen);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    /** Route assigned by each Widget Blueprint.  Kept presentation-only. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RA4|Screen", meta = (ClampMin = "0", ClampMax = "27"))
    int32 InitialScreenIndex = 0;

private:
    UFUNCTION()
    void OpenMainMenu();

    UFUNCTION()
    void OpenCampaign();

    UFUNCTION()
    void OpenHud();

    UFUNCTION()
    void OpenLobby();

    UFUNCTION()
    void OpenSettings();

    void BuildLayout();
    void BuildHudLayout();
    void SetScreen(int32 InScreen);
    UButton* AddNavigationButton(UVerticalBox* Parent, const FText& Label, FName WidgetName);
    UTextBlock* CreateText(const FText& Text, float FontSize, const FLinearColor& Color, FName WidgetName);

    UPROPERTY(Transient)
    TObjectPtr<UBorder> Background;

    UPROPERTY(Transient)
    TObjectPtr<UImage> Artwork;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> AccentPanel;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SubtitleText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ContentText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> ProgressBar;

    int32 ActiveScreen = 0;
    float PresentationTime = 0.0f;
};
