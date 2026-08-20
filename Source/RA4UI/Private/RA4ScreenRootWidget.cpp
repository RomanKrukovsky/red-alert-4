// Copyright (c) Red Alert 4 project.

#include "RA4ScreenRootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SafeZone.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "RA4UIScreenData.h"

#define LOCTEXT_NAMESPACE "RA4ScreenRootWidget"

namespace
{
constexpr FLinearColor NeutralBackground(0.004f, 0.007f, 0.012f, 1.0f);
}

TSharedRef<SWidget> URA4ScreenRootWidget::RebuildWidget()
{
    if (!WidgetTree)
    {
        return Super::RebuildWidget();
    }

    SafeZone = WidgetTree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), TEXT("SafeZone"));
    WidgetTree->RootWidget = SafeZone;

    UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(
        UOverlay::StaticClass(), TEXT("ScreenOverlay"));
    SafeZone->SetContent(RootOverlay);

    BackgroundLayer = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), TEXT("BackgroundLayer"));
    UOverlaySlot* BackgroundSlot = RootOverlay->AddChildToOverlay(BackgroundLayer);
    BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
    BackgroundSlot->SetVerticalAlignment(VAlign_Fill);

    UScaleBox* ReferenceScale = WidgetTree->ConstructWidget<UScaleBox>(
        UScaleBox::StaticClass(), TEXT("ReferenceScale"));
    ReferenceScale->SetStretch(EStretch::ScaleToFit);
    ReferenceScale->SetStretchDirection(EStretchDirection::Both);
    UOverlaySlot* ScaleSlot = RootOverlay->AddChildToOverlay(ReferenceScale);
    ScaleSlot->SetHorizontalAlignment(HAlign_Fill);
    ScaleSlot->SetVerticalAlignment(VAlign_Fill);

    ReferenceFrame = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("ReferenceFrame"));
    ReferenceFrame->SetWidthOverride(1920.0f);
    ReferenceFrame->SetHeightOverride(1080.0f);
    ReferenceScale->SetContent(ReferenceFrame);

    UOverlay* ReferenceLayers = WidgetTree->ConstructWidget<UOverlay>(
        UOverlay::StaticClass(), TEXT("ReferenceLayers"));
    ReferenceFrame->SetContent(ReferenceLayers);

    ChromeLayer = WidgetTree->ConstructWidget<UOverlay>(
        UOverlay::StaticClass(), TEXT("ChromeLayer"));
    UOverlaySlot* ChromeSlot = ReferenceLayers->AddChildToOverlay(ChromeLayer);
    ChromeSlot->SetHorizontalAlignment(HAlign_Fill);
    ChromeSlot->SetVerticalAlignment(VAlign_Fill);

    ContentLayer = WidgetTree->ConstructWidget<UOverlay>(
        UOverlay::StaticClass(), TEXT("ContentLayer"));
    UOverlaySlot* ContentSlot = ReferenceLayers->AddChildToOverlay(ContentLayer);
    ContentSlot->SetHorizontalAlignment(HAlign_Fill);
    ContentSlot->SetVerticalAlignment(VAlign_Fill);

    ApplyNeutralBackground();
    ValidationError = LOCTEXT("MissingScreenData", "Screen data is not assigned.");
    return Super::RebuildWidget();
}

void URA4ScreenRootWidget::ApplyScreenData(const URA4UIScreenData* ScreenData)
{
    if (!BackgroundLayer)
    {
        return;
    }

    if (!ScreenData)
    {
        ApplyNeutralBackground();
        ValidationError = LOCTEXT("MissingScreenData", "Screen data is not assigned.");
        return;
    }

    UTexture2D* BackgroundTexture = ScreenData->BackgroundArt.LoadSynchronous();
    if (!BackgroundTexture)
    {
        ApplyNeutralBackground();
        ValidationError = LOCTEXT("MissingBackgroundArt", "Screen background art is not assigned.");
        return;
    }

    BackgroundLayer->SetBrushFromTexture(BackgroundTexture, false);
    BackgroundLayer->SetColorAndOpacity(FLinearColor::White);
    ValidationError = FText::GetEmpty();
}

void URA4ScreenRootWidget::ApplyNeutralBackground()
{
    if (BackgroundLayer)
    {
        BackgroundLayer->SetBrushFromTexture(nullptr);
        BackgroundLayer->SetColorAndOpacity(NeutralBackground);
    }
}

#undef LOCTEXT_NAMESPACE
