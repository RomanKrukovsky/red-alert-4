// Copyright (c) Red Alert 4 project.
#include "RA4MatchResultOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
const FLinearColor kBackdrop(0.02f, 0.03f, 0.05f, 0.78f);
const FLinearColor kPanel(0.07f, 0.08f, 0.11f, 0.96f);
const FLinearColor kVictory(0.74f, 0.86f, 0.47f, 1.0f);
const FLinearColor kDefeat(0.93f, 0.42f, 0.37f, 1.0f);
const FLinearColor kText(0.88f, 0.90f, 0.94f, 1.0f);
const FLinearColor kButtonPrimary(0.21f, 0.43f, 0.28f, 1.0f);
const FLinearColor kButtonSecondary(0.18f, 0.20f, 0.24f, 1.0f);

UTextBlock* MakeLabel(UWidgetTree* Tree, FName Name, int32 Size, const FLinearColor& Color, bool bBold)
{
    UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = Size;
    Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
    Text->SetFont(Font);
    Text->SetColorAndOpacity(FSlateColor(Color));
    return Text;
}

void StyleButton(UButton* Button, const FLinearColor& Base)
{
    FButtonStyle Style = Button->GetStyle();
    Style.Normal.TintColor = FSlateColor(Base);
    Style.Hovered.TintColor = FSlateColor(Base * 1.20f);
    Style.Pressed.TintColor = FSlateColor(Base * 0.75f);
    Style.Disabled.TintColor = FSlateColor(Base * 0.50f);
    Button->SetStyle(Style);
}
} // namespace

TSharedRef<SWidget> URA4MatchResultOverlayWidget::RebuildWidget()
{
    if (WidgetTree == nullptr || WidgetTree->RootWidget != nullptr)
    {
        return Super::RebuildWidget();
    }

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MatchResultRoot"));

    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
    Backdrop->SetBrushColor(kBackdrop);
    if (UOverlaySlot* Slot = Root->AddChildToOverlay(Backdrop))
    {
        Slot->SetHorizontalAlignment(HAlign_Fill);
        Slot->SetVerticalAlignment(VAlign_Fill);
    }

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
    Panel->SetBrushColor(kPanel);
    Panel->SetPadding(FMargin(28.0f, 24.0f));

    UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stack"));

    HeadingText = MakeLabel(WidgetTree, TEXT("Heading"), 30, kVictory, true);
    HeadingText->SetJustification(ETextJustify::Center);
    if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(HeadingText))
    {
        Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
    }

    BodyText = MakeLabel(WidgetTree, TEXT("Body"), 15, kText, false);
    BodyText->SetJustification(ETextJustify::Center);
    BodyText->SetAutoWrapText(true);
    if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(BodyText))
    {
        Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
    }

    UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),
                                                                          TEXT("Buttons"));

    RetryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RetryButton"));
    StyleButton(RetryButton, kButtonPrimary);
    RetryButton->OnClicked.AddDynamic(this, &URA4MatchResultOverlayWidget::HandleRetryClicked);
    RetryButtonText = MakeLabel(WidgetTree, TEXT("RetryText"), 15, kText, true);
    RetryButtonText->SetJustification(ETextJustify::Center);
    RetryButton->AddChild(RetryButtonText);
    if (UButtonSlot* Slot = Cast<UButtonSlot>(RetryButtonText->Slot))
    {
        Slot->SetPadding(FMargin(18.0f, 10.0f));
        Slot->SetHorizontalAlignment(HAlign_Center);
        Slot->SetVerticalAlignment(VAlign_Center);
    }
    if (UHorizontalBoxSlot* Slot = Buttons->AddChildToHorizontalBox(RetryButton))
    {
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Slot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
    }

    ExitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ExitButton"));
    StyleButton(ExitButton, kButtonSecondary);
    ExitButton->OnClicked.AddDynamic(this, &URA4MatchResultOverlayWidget::HandleExitClicked);
    ExitButtonText = MakeLabel(WidgetTree, TEXT("ExitText"), 15, kText, true);
    ExitButtonText->SetJustification(ETextJustify::Center);
    ExitButton->AddChild(ExitButtonText);
    if (UButtonSlot* Slot = Cast<UButtonSlot>(ExitButtonText->Slot))
    {
        Slot->SetPadding(FMargin(18.0f, 10.0f));
        Slot->SetHorizontalAlignment(HAlign_Center);
        Slot->SetVerticalAlignment(VAlign_Center);
    }
    if (UHorizontalBoxSlot* Slot = Buttons->AddChildToHorizontalBox(ExitButton))
    {
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Slot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
    }

    Stack->AddChildToVerticalBox(Buttons);
    Panel->AddChild(Stack);

    USizeBox* PanelWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelWidth"));
    PanelWidth->SetWidthOverride(520.0f);
    PanelWidth->AddChild(Panel);
    if (UOverlaySlot* Slot = Root->AddChildToOverlay(PanelWidth))
    {
        Slot->SetHorizontalAlignment(HAlign_Center);
        Slot->SetVerticalAlignment(VAlign_Center);
        Slot->SetPadding(FMargin(24.0f));
    }

    WidgetTree->RootWidget = Root;
    RefreshTexts();
    return Super::RebuildWidget();
}

void URA4MatchResultOverlayWidget::Configure(bool bLocalPlayerWon, bool bCanReturnToMenu)
{
    bVictory = bLocalPlayerWon;
    bHasMainMenuLevel = bCanReturnToMenu;
    RefreshTexts();
}

void URA4MatchResultOverlayWidget::HandleRetryClicked()
{
    OnRetryRequested.Broadcast();
}

void URA4MatchResultOverlayWidget::HandleExitClicked()
{
    OnExitRequested.Broadcast();
}

void URA4MatchResultOverlayWidget::RefreshTexts()
{
    if (HeadingText != nullptr)
    {
        HeadingText->SetText(bVictory ? NSLOCTEXT("RA4", "MatchResultVictory", "ПОБЕДА")
                                      : NSLOCTEXT("RA4", "MatchResultDefeat", "ПОРАЖЕНИЕ"));
        HeadingText->SetColorAndOpacity(FSlateColor(bVictory ? kVictory : kDefeat));
    }

    if (BodyText != nullptr)
    {
        BodyText->SetText(
            bVictory
                ? NSLOCTEXT("RA4", "MatchResultVictoryBody",
                            "Операция завершена успешно. Можно сразу начать матч заново или безопасно выйти.")
                : NSLOCTEXT("RA4", "MatchResultDefeatBody",
                            "База потеряна. Попробуйте еще раз или завершите текущую демо-сессию."));
    }

    if (RetryButtonText != nullptr)
    {
        RetryButtonText->SetText(NSLOCTEXT("RA4", "MatchResultRetry", "ПОВТОРИТЬ"));
    }

    if (ExitButtonText != nullptr)
    {
        ExitButtonText->SetText(
            bHasMainMenuLevel ? NSLOCTEXT("RA4", "MatchResultExitToMenu", "ВЫЙТИ В МЕНЮ")
                              : NSLOCTEXT("RA4", "MatchResultExit", "ВЫЙТИ"));
    }
}
