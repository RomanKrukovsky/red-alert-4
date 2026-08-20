// Copyright (c) Red Alert 4 project.

#include "RA4SplashScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Input/Events.h"
#include "Input/Reply.h"

#define LOCTEXT_NAMESPACE "RA4SplashScreenWidget"

namespace
{
void PlaceSplashWidget(
    UCanvasPanel* Canvas,
    UWidget* Widget,
    const FVector2D Position,
    const FVector2D Size,
    const int32 ZOrder)
{
    UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
    Slot->SetPosition(Position);
    Slot->SetSize(Size);
    Slot->SetZOrder(ZOrder);
}
}

URA4SplashScreenWidget::URA4SplashScreenWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetScreenIdentity(ERA4UIScreenId::Splash);
    SetIsFocusable(true);
}

TSharedRef<SWidget> URA4SplashScreenWidget::RebuildWidget()
{
    const TSharedRef<SWidget> RootWidget = Super::RebuildWidget();
    if (!WidgetTree || !GetContentLayer())
    {
        return RootWidget;
    }

    if (UTexture2D* BackgroundTexture = LoadObject<UTexture2D>(
        nullptr,
        TEXT("/Game/RA4UI/Art/T_RA4_USSR_MainMenuBackground.T_RA4_USSR_MainMenuBackground")))
    {
        GetBackgroundLayer()->SetBrushFromTexture(BackgroundTexture, false);
        GetBackgroundLayer()->SetColorAndOpacity(FLinearColor(0.58f, 0.64f, 0.78f, 1.0f));
    }

    UBorder* Vignette = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("SplashVignette"));
    Vignette->SetBrushColor(FLinearColor(0.0f, 0.01f, 0.04f, 0.34f));
    Vignette->SetVisibility(ESlateVisibility::HitTestInvisible);
    UOverlaySlot* VignetteSlot = GetContentLayer()->AddChildToOverlay(Vignette);
    VignetteSlot->SetHorizontalAlignment(HAlign_Fill);
    VignetteSlot->SetVerticalAlignment(VAlign_Fill);

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("SplashCanvas"));
    UOverlaySlot* CanvasSlot = GetContentLayer()->AddChildToOverlay(Canvas);
    CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
    CanvasSlot->SetVerticalAlignment(VAlign_Fill);

    LogoImage = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), TEXT("SplashLogo"));
    if (UTexture2D* LogoTexture = LoadObject<UTexture2D>(
        nullptr,
        TEXT("/Game/RA4UI/Art/T_RA4_Logo.T_RA4_Logo")))
    {
        LogoImage->SetBrushFromTexture(LogoTexture, false);
    }
    LogoImage->SetVisibility(ESlateVisibility::HitTestInvisible);
    PlaceSplashWidget(Canvas, LogoImage, FVector2D(430.0f, 180.0f), FVector2D(1060.0f, 354.0f), 2);

    UBorder* PromptFrame = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("ContinuePromptFrame"));
    PromptFrame->SetBrushColor(FLinearColor(0.01f, 0.012f, 0.022f, 0.72f));
    PromptFrame->SetPadding(FMargin(42.0f, 14.0f));
    PromptFrame->SetVisibility(ESlateVisibility::HitTestInvisible);

    ContinuePrompt = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("ContinuePrompt"));
    ContinuePrompt->SetText(LOCTEXT("PressAnyKey", "НАЖМИТЕ ЛЮБУЮ КЛАВИШУ"));
    ContinuePrompt->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.88f, 0.92f, 1.0f)));
    ContinuePrompt->SetJustification(ETextJustify::Center);
    if (UObject* Font = LoadObject<UObject>(
        nullptr,
        TEXT("/Game/RA4UI/Fonts/RA4_RobotoCondensedRegular_Font.RA4_RobotoCondensedRegular_Font")))
    {
        ContinuePrompt->SetFont(FSlateFontInfo(Font, 30));
    }
    PromptFrame->SetContent(ContinuePrompt);
    PlaceSplashWidget(Canvas, PromptFrame, FVector2D(645.0f, 920.0f), FVector2D(630.0f, 74.0f), 3);
    return RootWidget;
}

void URA4SplashScreenWidget::NativeTick(
    const FGeometry& MyGeometry,
    const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    PromptTime += InDeltaTime;
    if (ContinuePrompt)
    {
        const float Opacity = 0.62f + (0.38f * FMath::Sin(PromptTime * 2.4f) + 0.38f) * 0.5f;
        ContinuePrompt->SetRenderOpacity(Opacity);
    }
}

FReply URA4SplashScreenWidget::NativeOnKeyDown(
    const FGeometry& InGeometry,
    const FKeyEvent& InKeyEvent)
{
    return ContinueToMainMenu() ? FReply::Handled() : Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply URA4SplashScreenWidget::NativeOnMouseButtonUp(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    return ContinueToMainMenu()
        ? FReply::Handled()
        : Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

bool URA4SplashScreenWidget::ContinueToMainMenu()
{
    if (bContinueRequested)
    {
        return false;
    }

    bContinueRequested = true;
    NavigateToScreen(ERA4UIScreenId::MainMenu);
    return true;
}

#undef LOCTEXT_NAMESPACE
