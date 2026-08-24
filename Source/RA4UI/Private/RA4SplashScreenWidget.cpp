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

    // The remaster title plate is the canonical screen 01 reference: it bakes
    // the Scarlet Horizon wordmark and horizon line into the art itself, so
    // the live overlay only adds the continue prompt and the pre-release tag.
    if (UTexture2D* BackgroundTexture = LoadObject<UTexture2D>(
        nullptr,
        TEXT("/Game/RA4UI/Art/Remaster/T_SH_01_TitleScreen.T_SH_01_TitleScreen")))
    {
        GetBackgroundLayer()->SetBrushFromTexture(BackgroundTexture, false);
        GetBackgroundLayer()->SetColorAndOpacity(FLinearColor::White);
    }

    UBorder* Vignette = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("SplashVignette"));
    Vignette->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
    Vignette->SetVisibility(ESlateVisibility::HitTestInvisible);
    UOverlaySlot* VignetteSlot = GetContentLayer()->AddChildToOverlay(Vignette);
    VignetteSlot->SetHorizontalAlignment(HAlign_Fill);
    VignetteSlot->SetVerticalAlignment(VAlign_Fill);

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("SplashCanvas"));
    UOverlaySlot* CanvasSlot = GetContentLayer()->AddChildToOverlay(Canvas);
    CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
    CanvasSlot->SetVerticalAlignment(VAlign_Fill);

    UTextBlock* BuildLabel = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("SplashBuildLabel"));
    BuildLabel->SetText(LOCTEXT("SplashBuild", "ПРЕДВАРИТЕЛЬНАЯ ВЕРСИЯ"));
    BuildLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.60f, 0.64f, 0.69f, 1.0f)));
    BuildLabel->SetJustification(ETextJustify::Right);
    BuildLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UObject* BuildFont = LoadObject<UObject>(
        nullptr,
        TEXT("/Game/RA4UI/Fonts/RA4_RobotoCondensedRegular_Font.RA4_RobotoCondensedRegular_Font")))
    {
        FSlateFontInfo Info(BuildFont, 13);
        Info.LetterSpacing = 300;
        BuildLabel->SetFont(Info);
    }
    PlaceSplashWidget(Canvas, BuildLabel, FVector2D(1420.0f, 1018.0f), FVector2D(440.0f, 26.0f), 3);

    // Continue prompt: Scarlet Horizon rule + centre dot, then the prompt text,
    // matching the remaster title-screen reference composition.
    ContinuePrompt = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("ContinuePrompt"));
    ContinuePrompt->SetText(LOCTEXT("PressAnyKey", "НАЖМИТЕ ЛЮБУЮ КЛАВИШУ"));
    ContinuePrompt->SetColorAndOpacity(FSlateColor(FLinearColor(0.93f, 0.95f, 0.97f, 1.0f)));
    ContinuePrompt->SetJustification(ETextJustify::Center);
    ContinuePrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UObject* Font = LoadObject<UObject>(
        nullptr,
        TEXT("/Game/RA4UI/Fonts/RA4_RobotoCondensedSemiBold_Font.RA4_RobotoCondensedSemiBold_Font")))
    {
        FSlateFontInfo Info(Font, 22);
        Info.LetterSpacing = 100;
        ContinuePrompt->SetFont(Info);
    }
    PlaceSplashWidget(Canvas, ContinuePrompt, FVector2D(510.0f, 972.0f), FVector2D(900.0f, 42.0f), 4);

    UBorder* LeftRule = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("PromptLeftRule"));
    LeftRule->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
    LeftRule->SetVisibility(ESlateVisibility::HitTestInvisible);
    PlaceSplashWidget(Canvas, LeftRule, FVector2D(620.0f, 1018.0f), FVector2D(150.0f, 1.0f), 5);

    UBorder* RightRule = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("PromptRightRule"));
    RightRule->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
    RightRule->SetVisibility(ESlateVisibility::HitTestInvisible);
    PlaceSplashWidget(Canvas, RightRule, FVector2D(1150.0f, 1018.0f), FVector2D(150.0f, 1.0f), 5);
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
