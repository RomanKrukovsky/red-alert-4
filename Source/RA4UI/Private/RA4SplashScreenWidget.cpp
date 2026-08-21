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
        TEXT("/Game/RA4UI/Art/T_RA4_TitleBackdrop.T_RA4_TitleBackdrop")))
    {
        GetBackgroundLayer()->SetBrushFromTexture(BackgroundTexture, false);
        GetBackgroundLayer()->SetColorAndOpacity(FLinearColor::White);
    }

    UBorder* Vignette = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("SplashVignette"));
    Vignette->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.16f));
    Vignette->SetVisibility(ESlateVisibility::HitTestInvisible);
    UOverlaySlot* VignetteSlot = GetContentLayer()->AddChildToOverlay(Vignette);
    VignetteSlot->SetHorizontalAlignment(HAlign_Fill);
    VignetteSlot->SetVerticalAlignment(VAlign_Fill);

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("SplashCanvas"));
    UOverlaySlot* CanvasSlot = GetContentLayer()->AddChildToOverlay(Canvas);
    CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
    CanvasSlot->SetVerticalAlignment(VAlign_Fill);

    // Global Scarlet Horizon thin line
    UBorder* HorizonLine = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SplashHorizonLine"));
    HorizonLine->SetBrushColor(FLinearColor(0.95f, 0.12f, 0.16f, 1.0f));
    PlaceSplashWidget(Canvas, HorizonLine, FVector2D(0.0f, 0.0f), FVector2D(1920.0f, 3.0f), 10);

    LogoImage = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), TEXT("SplashLogo"));
    if (UTexture2D* LogoTexture = LoadObject<UTexture2D>(
        nullptr,
        TEXT("/Game/RA4UI/Art/T_RA4_Logo.T_RA4_Logo")))
    {
        LogoImage->SetBrushFromTexture(LogoTexture, false);
    }
    LogoImage->SetVisibility(ESlateVisibility::HitTestInvisible);
    PlaceSplashWidget(Canvas, LogoImage, FVector2D(566.0f, 168.0f), FVector2D(788.0f, 206.0f), 2);

    UTextBlock* Tagline = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("SplashTagline"));
    Tagline->SetText(LOCTEXT("SplashTagline", "· АЛЬТЕРНАТИВНАЯ СОВРЕМЕННОСТЬ ·"));
    Tagline->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.75f, 0.82f, 1.0f)));
    Tagline->SetJustification(ETextJustify::Center);
    Tagline->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UObject* TaglineFont = LoadObject<UObject>(
        nullptr,
        TEXT("/Game/RA4UI/Fonts/RA4_RobotoCondensedSemiBold_Font.RA4_RobotoCondensedSemiBold_Font")))
    {
        FSlateFontInfo Info(TaglineFont, 20);
        Info.LetterSpacing = 260;
        Tagline->SetFont(Info);
    }
    PlaceSplashWidget(Canvas, Tagline, FVector2D(566.0f, 372.0f), FVector2D(788.0f, 32.0f), 3);

    UTextBlock* BuildLabel = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("SplashBuildLabel"));
    BuildLabel->SetText(LOCTEXT("SplashBuild", "ПРЕДВАРИТЕЛЬНАЯ ВЕРСИЯ"));
    BuildLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.58f, 0.64f, 1.0f)));
    BuildLabel->SetJustification(ETextJustify::Right);
    BuildLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UObject* BuildFont = LoadObject<UObject>(
        nullptr,
        TEXT("/Game/RA4UI/Fonts/RA4_RobotoCondensedRegular_Font.RA4_RobotoCondensedRegular_Font")))
    {
        BuildLabel->SetFont(FSlateFontInfo(BuildFont, 15));
    }
    PlaceSplashWidget(Canvas, BuildLabel, FVector2D(1420.0f, 1016.0f), FVector2D(440.0f, 26.0f), 3);

    UBorder* PromptFrame = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("ContinuePromptFrame"));
    PromptFrame->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.22f));
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
        ContinuePrompt->SetFont(FSlateFontInfo(Font, 24));
    }
    PromptFrame->SetContent(ContinuePrompt);
    PlaceSplashWidget(Canvas, PromptFrame, FVector2D(645.0f, 938.0f), FVector2D(630.0f, 64.0f), 3);
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
