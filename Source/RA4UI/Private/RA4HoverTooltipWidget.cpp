// Copyright (c) Red Alert 4 project.
#include "RA4HoverTooltipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
const FLinearColor kBackground(0.04f, 0.05f, 0.06f, 0.92f);
const FLinearColor kTitle(0.92f, 0.94f, 0.97f);
const FLinearColor kSubtitle(0.58f, 0.63f, 0.70f);
} // namespace

TSharedRef<SWidget> URA4HoverTooltipWidget::RebuildWidget()
{
    if (WidgetTree == nullptr || WidgetTree->RootWidget != nullptr)
    {
        return Super::RebuildWidget();
    }

    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TipColumn"));

    auto MakeLabel = [&](FName Name, const FLinearColor& Colour, int32 Size, bool bBold) -> UTextBlock*
    {
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
        FSlateFontInfo Font = Text->GetFont();
        Font.Size = Size;
        Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
        Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(Colour));
        return Text;
    };

    TitleText = MakeLabel(TEXT("TipTitle"), kTitle, 12, true);
    Column->AddChildToVerticalBox(TitleText);

    SubtitleText = MakeLabel(TEXT("TipSubtitle"), kSubtitle, 10, false);
    if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(SubtitleText))
    {
        Slot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
    }

    UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TipFrame"));
    Frame->SetBrushColor(kBackground);
    Frame->SetPadding(FMargin(10.0f, 6.0f));
    Frame->AddChild(Column);
    // The tooltip follows the cursor; it must never eat the click that would select
    // or order the very thing it is describing.
    Frame->SetVisibility(ESlateVisibility::HitTestInvisible);

    WidgetTree->RootWidget = Frame;
    return Super::RebuildWidget();
}

void URA4HoverTooltipWidget::SetContent(const FText& Title, const FText& Subtitle)
{
    if (TitleText != nullptr)
    {
        TitleText->SetText(Title);
    }
    if (SubtitleText != nullptr)
    {
        SubtitleText->SetText(Subtitle);
        SubtitleText->SetVisibility(Subtitle.IsEmpty() ? ESlateVisibility::Collapsed
                                                       : ESlateVisibility::HitTestInvisible);
    }
}
