// Copyright (c) Red Alert 4 project. In-Game Pause and Quit Menu.
#include "RA4PauseMenuWidget.h"

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

#define LOCTEXT_NAMESPACE "RA4PauseMenu"

namespace
{
const FLinearColor kBackdropColor(0.01f, 0.02f, 0.04f, 0.85f);
const FLinearColor kPanelBg(0.06f, 0.08f, 0.12f, 0.98f);
const FLinearColor kHeaderAccent(0.85f, 0.22f, 0.18f, 1.0f); // Red Alert Crimson
const FLinearColor kTextColor(0.92f, 0.94f, 0.96f, 1.0f);
const FLinearColor kSubtextColor(0.55f, 0.60f, 0.68f, 1.0f);
const FLinearColor kBtnResume(0.18f, 0.42f, 0.25f, 1.0f);     // Green
const FLinearColor kBtnStandard(0.12f, 0.16f, 0.22f, 1.0f);   // Slate Navy
const FLinearColor kBtnDanger(0.48f, 0.15f, 0.15f, 1.0f);     // Dark Red

UTextBlock* CreateStyledText(UWidgetTree* Tree, FName Name, int32 Size, const FLinearColor& Color, bool bBold)
{
    UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = Size;
    Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
    Text->SetFont(Font);
    Text->SetColorAndOpacity(FSlateColor(Color));
    return Text;
}

void ApplyButtonStyle(UButton* Button, const FLinearColor& Base)
{
    FButtonStyle Style = Button->GetStyle();
    Style.Normal.TintColor = FSlateColor(Base);
    Style.Hovered.TintColor = FSlateColor(Base * 1.35f);
    Style.Pressed.TintColor = FSlateColor(Base * 0.70f);
    Style.Disabled.TintColor = FSlateColor(Base * 0.40f);
    Button->SetStyle(Style);
}
} // namespace

TSharedRef<SWidget> URA4PauseMenuWidget::RebuildWidget()
{
    if (WidgetTree == nullptr || WidgetTree->RootWidget != nullptr)
    {
        return Super::RebuildWidget();
    }

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PauseMenuRoot"));

    // Dimmed Backdrop
    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
    Backdrop->SetBrushColor(kBackdropColor);
    if (UOverlaySlot* Slot = Root->AddChildToOverlay(Backdrop))
    {
        Slot->SetHorizontalAlignment(HAlign_Fill);
        Slot->SetVerticalAlignment(VAlign_Fill);
    }

    // Centered Panel
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
    Panel->SetBrushColor(kPanelBg);
    Panel->SetPadding(FMargin(36.0f, 28.0f));

    UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stack"));

    // Header Title
    UTextBlock* Title = CreateStyledText(WidgetTree, TEXT("PauseTitle"), 26, kHeaderAccent, true);
    Title->SetText(LOCTEXT("PauseTitle", "ПАУЗА // ТАКТИЧЕСКОЕ МЕНЮ"));
    Title->SetJustification(ETextJustify::Center);
    if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(Title))
    {
        Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
    }

    // Subtitle
    UTextBlock* Subtitle = CreateStyledText(WidgetTree, TEXT("PauseSubtitle"), 12, kSubtextColor, false);
    Subtitle->SetText(LOCTEXT("PauseSubtitle", "СИМУЛЯЦИЯ ПРИОСТАНОВЛЕНА"));
    Subtitle->SetJustification(ETextJustify::Center);
    if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(Subtitle))
    {
        Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
    }

    auto CreateMenuButton = [&](const FText& LabelText, const FLinearColor& Color) -> UButton*
    {
        UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
        ApplyButtonStyle(Btn, Color);

        UTextBlock* BtnText = CreateStyledText(WidgetTree, NAME_None, 14, kTextColor, true);
        BtnText->SetText(LabelText);
        BtnText->SetJustification(ETextJustify::Center);
        Btn->AddChild(BtnText);

        if (UButtonSlot* BSlot = Cast<UButtonSlot>(BtnText->Slot))
        {
            BSlot->SetPadding(FMargin(24.0f, 10.0f));
            BSlot->SetHorizontalAlignment(HAlign_Center);
            BSlot->SetVerticalAlignment(VAlign_Center);
        }

        if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(Btn))
        {
            Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
        }
        return Btn;
    };

    // 1. Resume
    UButton* ResumeBtn = CreateMenuButton(LOCTEXT("ResumeBtn", "► ПРОДОЛЖИТЬ ИГРУ"), kBtnResume);
    ResumeBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleResumeClicked);

    // 2. Restart
    UButton* RestartBtn = CreateMenuButton(LOCTEXT("RestartBtn", "↻ ПЕРЕЗАПУСТИТЬ МАТЧ"), kBtnStandard);
    RestartBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleRestartClicked);

    // 3. Settings
    UButton* SettingsBtn = CreateMenuButton(LOCTEXT("SettingsBtn", "⚙ НАСТРОЙКИ"), kBtnStandard);
    SettingsBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleSettingsClicked);

    // 4. Quit to Menu
    UButton* QuitMenuBtn = CreateMenuButton(LOCTEXT("QuitMenuBtn", "⎋ ВЫЙТИ В ГЛАВНОЕ МЕНЮ"), kBtnStandard);
    QuitMenuBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleQuitToMenuClicked);

    // 5. Quit Game
    UButton* QuitGameBtn = CreateMenuButton(LOCTEXT("QuitGameBtn", "✕ ВЫЙТИ НА РАБОЧИЙ СТОЛ"), kBtnDanger);
    QuitGameBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleQuitToDesktopClicked);

    USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Box"));
    Box->SetWidthOverride(380.0f);
    Box->AddChild(Panel);
    Panel->AddChild(Stack);

    if (UOverlaySlot* Slot = Root->AddChildToOverlay(Box))
    {
        Slot->SetHorizontalAlignment(HAlign_Center);
        Slot->SetVerticalAlignment(VAlign_Center);
    }

    WidgetTree->RootWidget = Root;
    return Super::RebuildWidget();
}

void URA4PauseMenuWidget::HandleResumeClicked()
{
    OnResumeRequested.Broadcast();
}

void URA4PauseMenuWidget::HandleRestartClicked()
{
    OnRestartRequested.Broadcast();
}

void URA4PauseMenuWidget::HandleSettingsClicked()
{
    OnSettingsRequested.Broadcast();
}

void URA4PauseMenuWidget::HandleQuitToMenuClicked()
{
    OnQuitToMenuRequested.Broadcast();
}

void URA4PauseMenuWidget::HandleQuitToDesktopClicked()
{
    OnQuitToDesktopRequested.Broadcast();
}

#undef LOCTEXT_NAMESPACE
