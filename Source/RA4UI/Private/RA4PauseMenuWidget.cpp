// Copyright (c) Red Alert 4 project. In-Game Pause and Tactical Settings Menu.
#include "RA4PauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/ComboBoxString.h"
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
const FLinearColor kBackdropColor(0.01f, 0.02f, 0.04f, 0.88f);
const FLinearColor kPanelBg(0.055f, 0.07f, 0.10f, 0.98f);
const FLinearColor kFrameBg(0.035f, 0.045f, 0.065f, 0.95f);
const FLinearColor kHeaderAccent(0.90f, 0.15f, 0.12f, 1.0f); // Red Alert Crimson
const FLinearColor kMusicHeaderAccent(0.95f, 0.72f, 0.20f, 1.0f); // Gold Amber Accent
const FLinearColor kTextColor(0.92f, 0.94f, 0.96f, 1.0f);
const FLinearColor kSubtextColor(0.55f, 0.60f, 0.68f, 1.0f);
const FLinearColor kTrackDisplayColor(0.20f, 0.85f, 0.95f, 1.0f); // Tactical Cyan

const FLinearColor kBtnResumeNormal(0.12f, 0.40f, 0.20f, 1.0f);
const FLinearColor kBtnResumeHover(0.18f, 0.58f, 0.28f, 1.0f);
const FLinearColor kBtnResumePressed(0.08f, 0.28f, 0.14f, 1.0f);

const FLinearColor kBtnStandardNormal(0.10f, 0.14f, 0.20f, 1.0f);
const FLinearColor kBtnStandardHover(0.18f, 0.25f, 0.35f, 1.0f);
const FLinearColor kBtnStandardPressed(0.06f, 0.09f, 0.13f, 1.0f);

const FLinearColor kBtnDangerNormal(0.50f, 0.10f, 0.10f, 1.0f);
const FLinearColor kBtnDangerHover(0.72f, 0.16f, 0.16f, 1.0f);
const FLinearColor kBtnDangerPressed(0.32f, 0.06f, 0.06f, 1.0f);

const FLinearColor kBtnMusicNormal(0.14f, 0.18f, 0.24f, 1.0f);
const FLinearColor kBtnMusicHover(0.25f, 0.32f, 0.42f, 1.0f);
const FLinearColor kBtnMusicPressed(0.08f, 0.10f, 0.14f, 1.0f);

const FLinearColor kBtnActiveTabNormal(0.75f, 0.18f, 0.14f, 1.0f);
const FLinearColor kBtnActiveTabHover(0.85f, 0.24f, 0.18f, 1.0f);
const FLinearColor kBtnInactiveTabNormal(0.12f, 0.15f, 0.20f, 1.0f);
const FLinearColor kBtnInactiveTabHover(0.18f, 0.22f, 0.30f, 1.0f);

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

UButton* CreateCustomButton(
    UWidgetTree* Tree,
    FName Name,
    const FText& LabelText,
    const FLinearColor& Normal,
    const FLinearColor& Hover,
    const FLinearColor& Pressed,
    const FLinearColor& TextColor = kTextColor,
    int32 FontSize = 14,
    FMargin Padding = FMargin(16.0f, 8.0f),
    UTextBlock** OutLabel = nullptr)
{
    UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);

    FButtonStyle Style;
    Style.SetNormal(FSlateColorBrush(Normal));
    Style.SetHovered(FSlateColorBrush(Hover));
    Style.SetPressed(FSlateColorBrush(Pressed));
    Style.SetDisabled(FSlateColorBrush(FLinearColor(0.08f, 0.08f, 0.10f, 0.45f)));
    Style.NormalPadding = FMargin(0.0f);
    Style.PressedPadding = FMargin(1.0f, 1.0f, 0.0f, 0.0f);
    Btn->SetStyle(Style);

    UTextBlock* BtnText = CreateStyledText(Tree, NAME_None, FontSize, TextColor, true);
    BtnText->SetText(LabelText);
    BtnText->SetJustification(ETextJustify::Center);
    Btn->AddChild(BtnText);

    if (UButtonSlot* BSlot = Cast<UButtonSlot>(BtnText->Slot))
    {
        BSlot->SetPadding(Padding);
        BSlot->SetHorizontalAlignment(HAlign_Center);
        BSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (OutLabel != nullptr)
    {
        *OutLabel = BtnText;
    }
    return Btn;
}

void SetButtonActiveStyle(UButton* Btn, bool bActive)
{
    if (Btn == nullptr) return;
    FButtonStyle Style = Btn->GetStyle();
    if (bActive)
    {
        Style.SetNormal(FSlateColorBrush(kBtnActiveTabNormal));
        Style.SetHovered(FSlateColorBrush(kBtnActiveTabHover));
        Style.SetPressed(FSlateColorBrush(kBtnActiveTabNormal));
    }
    else
    {
        Style.SetNormal(FSlateColorBrush(kBtnInactiveTabNormal));
        Style.SetHovered(FSlateColorBrush(kBtnInactiveTabHover));
        Style.SetPressed(FSlateColorBrush(kBtnInactiveTabNormal));
    }
    Btn->SetStyle(Style);
}
} // namespace

TSharedRef<SWidget> URA4PauseMenuWidget::RebuildWidget()
{
    if (WidgetTree == nullptr || WidgetTree->RootWidget != nullptr)
    {
        return Super::RebuildWidget();
    }

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PauseRoot"));

    // Dimmed backdrop
    UBorder* DimBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DimBackdrop"));
    DimBackdrop->SetBrush(FSlateColorBrush(kBackdropColor));
    Root->AddChildToOverlay(DimBackdrop);

    // Main Container Box (Centered)
    USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MainSizeBox"));
    Box->SetWidthOverride(560.0f);

    UOverlay* CenterOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CenterOverlay"));
    Box->AddChild(CenterOverlay);

    // =========================================================================
    // 1. MAIN PAUSE MENU PANEL (Clean, Focused, C&C Style)
    // =========================================================================
    MainPausePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainPausePanel"));
    MainPausePanel->SetBrush(FSlateColorBrush(kPanelBg));
    MainPausePanel->SetPadding(FMargin(28.0f, 24.0f));

    UVerticalBox* MainStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainStack"));

    // Header Title
    UTextBlock* Title = CreateStyledText(WidgetTree, TEXT("PauseTitle"), 22, kHeaderAccent, true);
    Title->SetText(LOCTEXT("PauseTitle", "ПАУЗА // ТАКТИЧЕСКОЕ МЕНЮ"));
    Title->SetJustification(ETextJustify::Center);
    Title->SetShadowOffset(FVector2D(2.0f, 2.0f));
    Title->SetShadowColorAndOpacity(FLinearColor::Black);
    MainStack->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));

    // Subtitle
    UTextBlock* Subtitle = CreateStyledText(WidgetTree, TEXT("PauseSubtitle"), 12, kSubtextColor, false);
    Subtitle->SetText(LOCTEXT("PauseSubtitle", "СИМУЛЯЦИЯ ПРИОСТАНОВЛЕНА"));
    Subtitle->SetJustification(ETextJustify::Center);
    MainStack->AddChildToVerticalBox(Subtitle)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));

    // 1. Resume Game
    UButton* ResumeBtn = CreateCustomButton(
        WidgetTree,
        TEXT("ResumeBtn"),
        LOCTEXT("ResumeBtn", "► ПРОДОЛЖИТЬ ИГРУ"),
        kBtnResumeNormal,
        kBtnResumeHover,
        kBtnResumePressed,
        FLinearColor::White,
        15,
        FMargin(20.0f, 10.0f));
    ResumeBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleResumeClicked);
    MainStack->AddChildToVerticalBox(ResumeBtn)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

    // 2. Restart Match
    UButton* RestartBtn = CreateCustomButton(
        WidgetTree,
        TEXT("RestartBtn"),
        LOCTEXT("RestartBtn", "↻ ПЕРЕЗАПУСТИТЬ МАТЧ"),
        kBtnStandardNormal,
        kBtnStandardHover,
        kBtnStandardPressed,
        kTextColor,
        14,
        FMargin(20.0f, 9.0f));
    RestartBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleRestartClicked);
    MainStack->AddChildToVerticalBox(RestartBtn)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

    // 3. Settings (Opens Tactical Settings Sub-Menu)
    UButton* SettingsBtn = CreateCustomButton(
        WidgetTree,
        TEXT("SettingsBtn"),
        LOCTEXT("SettingsBtn", "НАСТРОЙКИ"),
        kBtnStandardNormal,
        kBtnStandardHover,
        kBtnStandardPressed,
        kTextColor,
        14,
        FMargin(20.0f, 9.0f));
    SettingsBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleSettingsClicked);
    MainStack->AddChildToVerticalBox(SettingsBtn)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

    // 4. Quit to Main Menu
    UButton* QuitMenuBtn = CreateCustomButton(
        WidgetTree,
        TEXT("QuitMenuBtn"),
        LOCTEXT("QuitMenuBtn", "В ГЛАВНОЕ МЕНЮ"),
        kBtnStandardNormal,
        kBtnStandardHover,
        kBtnStandardPressed,
        kTextColor,
        14,
        FMargin(20.0f, 9.0f));
    QuitMenuBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleQuitToMenuClicked);
    MainStack->AddChildToVerticalBox(QuitMenuBtn)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

    // 5. Quit to Desktop
    UButton* QuitGameBtn = CreateCustomButton(
        WidgetTree,
        TEXT("QuitGameBtn"),
        LOCTEXT("QuitGameBtn", "ВЫЙТИ НА РАБОЧИЙ СТОЛ"),
        kBtnDangerNormal,
        kBtnDangerHover,
        kBtnDangerPressed,
        FLinearColor::White,
        14,
        FMargin(20.0f, 9.0f));
    QuitGameBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleQuitToDesktopClicked);
    MainStack->AddChildToVerticalBox(QuitGameBtn);

    MainPausePanel->AddChild(MainStack);
    CenterOverlay->AddChildToOverlay(MainPausePanel);

    // =========================================================================
    // 2. EXPANDED TACTICAL SETTINGS PANEL (Activated by clicking "НАСТРОЙКИ")
    // =========================================================================
    SettingsPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsPanel"));
    SettingsPanel->SetBrush(FSlateColorBrush(kPanelBg));
    SettingsPanel->SetPadding(FMargin(24.0f, 18.0f));
    SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);

    UVerticalBox* SettingsStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsStack"));

    // Settings Header
    UTextBlock* SetTitle = CreateStyledText(WidgetTree, TEXT("SetTitle"), 18, kMusicHeaderAccent, true);
    SetTitle->SetText(LOCTEXT("SetTitle", "ТАКТИЧЕСКИЙ КОМПЛЕКС НАСТРОЕК"));
    SetTitle->SetJustification(ETextJustify::Center);
    SettingsStack->AddChildToVerticalBox(SetTitle)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

    // Tab Switcher Bar
    UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TabRow"));

    TabGfxButton = CreateCustomButton(WidgetTree, TEXT("TabGfxBtn"), LOCTEXT("TabGfx", "1. ГРАФИКА / FPS"), kBtnActiveTabNormal, kBtnActiveTabHover, kBtnActiveTabNormal, FLinearColor::White, 12, FMargin(6.0f, 6.0f));
    TabGfxButton->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleTabGraphicsClicked);
    TabRow->AddChildToHorizontalBox(TabGfxButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    TabAudButton = CreateCustomButton(WidgetTree, TEXT("TabAudBtn"), LOCTEXT("TabAud", "2. ЗВУК И МУЗЫКА"), kBtnInactiveTabNormal, kBtnInactiveTabHover, kBtnInactiveTabNormal, kTextColor, 12, FMargin(6.0f, 6.0f));
    TabAudButton->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleTabAudioClicked);
    TabRow->AddChildToHorizontalBox(TabAudButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    TabGameButton = CreateCustomButton(WidgetTree, TEXT("TabGameBtn"), LOCTEXT("TabGame", "3. УПРАВЛЕНИЕ"), kBtnInactiveTabNormal, kBtnInactiveTabHover, kBtnInactiveTabNormal, kTextColor, 12, FMargin(6.0f, 6.0f));
    TabGameButton->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleTabGameplayClicked);
    TabRow->AddChildToHorizontalBox(TabGameButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    SettingsStack->AddChildToVerticalBox(TabRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));

    // -------------------------------------------------------------------------
    // TAB 1: GRAPHICS & PERFORMANCE
    // -------------------------------------------------------------------------
    GraphicsContentStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GfxStack"));

    // Quality Preset Row
    UTextBlock* QLabel = CreateStyledText(WidgetTree, TEXT("QLabel"), 12, kTextColor, true);
    QLabel->SetText(LOCTEXT("QLabel", "Качество графики (Render Quality):"));
    GraphicsContentStack->AddChildToVerticalBox(QLabel)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

    UHorizontalBox* QRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("QRow"));
    UButton* QLow = CreateCustomButton(WidgetTree, TEXT("QLow"), LOCTEXT("QLow", "НИЗК"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    QLow->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleQualityLowClicked);
    QRow->AddChildToHorizontalBox(QLow)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* QMed = CreateCustomButton(WidgetTree, TEXT("QMed"), LOCTEXT("QMed", "СРЕДН"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    QMed->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleQualityMedClicked);
    QRow->AddChildToHorizontalBox(QMed)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* QHigh = CreateCustomButton(WidgetTree, TEXT("QHigh"), LOCTEXT("QHigh", "ВЫСОК"), kBtnActiveTabNormal, kBtnActiveTabHover, kBtnActiveTabNormal, FLinearColor::White, 11, FMargin(4.0f, 4.0f));
    QHigh->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleQualityHighClicked);
    QRow->AddChildToHorizontalBox(QHigh)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* QEpic = CreateCustomButton(WidgetTree, TEXT("QEpic"), LOCTEXT("QEpic", "ЭПИК"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    QEpic->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleQualityEpicClicked);
    QRow->AddChildToHorizontalBox(QEpic)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    GraphicsContentStack->AddChildToVerticalBox(QRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    // FPS Cap Row
    UTextBlock* FpsLabel = CreateStyledText(WidgetTree, TEXT("FpsLabel"), 12, kTextColor, true);
    FpsLabel->SetText(LOCTEXT("FpsLabel", "Ограничение FPS (Frame Rate Cap):"));
    GraphicsContentStack->AddChildToVerticalBox(FpsLabel)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

    UHorizontalBox* FpsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("FpsRow"));
    UButton* Fps60 = CreateCustomButton(WidgetTree, TEXT("Fps60"), LOCTEXT("Fps60", "60 FPS"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    Fps60->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleFps60Clicked);
    FpsRow->AddChildToHorizontalBox(Fps60)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* Fps120 = CreateCustomButton(WidgetTree, TEXT("Fps120"), LOCTEXT("Fps120", "120 FPS"), kBtnActiveTabNormal, kBtnActiveTabHover, kBtnActiveTabNormal, FLinearColor::White, 11, FMargin(4.0f, 4.0f));
    Fps120->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleFps120Clicked);
    FpsRow->AddChildToHorizontalBox(Fps120)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* Fps144 = CreateCustomButton(WidgetTree, TEXT("Fps144"), LOCTEXT("Fps144", "144 FPS"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    Fps144->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleFps144Clicked);
    FpsRow->AddChildToHorizontalBox(Fps144)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* FpsMax = CreateCustomButton(WidgetTree, TEXT("FpsMax"), LOCTEXT("FpsMax", "MAX"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    FpsMax->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleFpsMaxClicked);
    FpsRow->AddChildToHorizontalBox(FpsMax)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    GraphicsContentStack->AddChildToVerticalBox(FpsRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    // Anti-Aliasing Row
    UTextBlock* AaLabel = CreateStyledText(WidgetTree, TEXT("AaLabel"), 12, kTextColor, true);
    AaLabel->SetText(LOCTEXT("AaLabel", "Сглаживание (Anti-Aliasing):"));
    GraphicsContentStack->AddChildToVerticalBox(AaLabel)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

    UHorizontalBox* AaRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AaRow"));
    UButton* AaFxaa = CreateCustomButton(WidgetTree, TEXT("AaFxaa"), LOCTEXT("AaFxaa", "FXAA"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    AaFxaa->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleAaFxaaClicked);
    AaRow->AddChildToHorizontalBox(AaFxaa)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* AaTaa = CreateCustomButton(WidgetTree, TEXT("AaTaa"), LOCTEXT("AaTaa", "TAA"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    AaTaa->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleAaTaaClicked);
    AaRow->AddChildToHorizontalBox(AaTaa)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* AaTsr = CreateCustomButton(WidgetTree, TEXT("AaTsr"), LOCTEXT("AaTsr", "TSR (SUPER RES)"), kBtnActiveTabNormal, kBtnActiveTabHover, kBtnActiveTabNormal, FLinearColor::White, 11, FMargin(4.0f, 4.0f));
    AaTsr->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleAaTsrClicked);
    AaRow->AddChildToHorizontalBox(AaTsr)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    GraphicsContentStack->AddChildToVerticalBox(AaRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    // Screen Shake Toggle
    UHorizontalBox* ShakeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ShakeRow"));
    UTextBlock* ShakeTitle = CreateStyledText(WidgetTree, TEXT("ShakeTitle"), 12, kTextColor, true);
    ShakeTitle->SetText(LOCTEXT("ShakeTitle", "Встряска камеры от взрывов:"));
    ShakeRow->AddChildToHorizontalBox(ShakeTitle)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UTextBlock* RawShakeLabel = nullptr;
    UButton* ShakeBtn = CreateCustomButton(WidgetTree, TEXT("ShakeBtn"), LOCTEXT("ShakeOn", "[ВКЛ]"), kBtnResumeNormal, kBtnResumeHover, kBtnResumePressed, FLinearColor::White, 11, FMargin(12.0f, 4.0f), &RawShakeLabel);
    ScreenShakeBtnLabel = RawShakeLabel;
    ShakeBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleScreenShakeToggled);
    ShakeRow->AddChildToHorizontalBox(ShakeBtn);

    GraphicsContentStack->AddChildToVerticalBox(ShakeRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    SettingsStack->AddChildToVerticalBox(GraphicsContentStack);

    // -------------------------------------------------------------------------
    // TAB 2: AUDIO & TACTICAL SOUNDTRACK PLAYER
    // -------------------------------------------------------------------------
    AudioContentStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AudioStack"));
    AudioContentStack->SetVisibility(ESlateVisibility::Collapsed);

    // Master Volume
    UHorizontalBox* MVolRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MVolRow"));
    UTextBlock* MVolTitle = CreateStyledText(WidgetTree, TEXT("MVolTitle"), 12, kTextColor, true);
    MVolTitle->SetText(LOCTEXT("MVolTitle", "Общая громкость:"));
    MVolRow->AddChildToHorizontalBox(MVolTitle)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* MVolDown = CreateCustomButton(WidgetTree, TEXT("MVolDown"), LOCTEXT("MVolDown", " - "), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 12, FMargin(8.0f, 3.0f));
    MVolDown->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleMasterVolDown);
    MVolRow->AddChildToHorizontalBox(MVolDown);

    MasterVolLabel = CreateStyledText(WidgetTree, TEXT("MasterVolLabel"), 12, kTrackDisplayColor, true);
    MasterVolLabel->SetText(FText::FromString(TEXT(" 85% ")));
    MVolRow->AddChildToHorizontalBox(MasterVolLabel);

    UButton* MVolUp = CreateCustomButton(WidgetTree, TEXT("MVolUp"), LOCTEXT("MVolUp", " + "), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 12, FMargin(8.0f, 3.0f));
    MVolUp->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleMasterVolUp);
    MVolRow->AddChildToHorizontalBox(MVolUp);

    AudioContentStack->AddChildToVerticalBox(MVolRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

    // SFX Volume
    UHorizontalBox* SfxRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SfxRow"));
    UTextBlock* SfxTitle = CreateStyledText(WidgetTree, TEXT("SfxTitle"), 12, kTextColor, true);
    SfxTitle->SetText(LOCTEXT("SfxTitle", "Эффекты и взрывы (SFX):"));
    SfxRow->AddChildToHorizontalBox(SfxTitle)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* SfxDown = CreateCustomButton(WidgetTree, TEXT("SfxDown"), LOCTEXT("SfxDown", " - "), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 12, FMargin(8.0f, 3.0f));
    SfxDown->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleSfxVolDown);
    SfxRow->AddChildToHorizontalBox(SfxDown);

    SfxVolLabel = CreateStyledText(WidgetTree, TEXT("SfxVolLabel"), 12, kTrackDisplayColor, true);
    SfxVolLabel->SetText(FText::FromString(TEXT(" 100% ")));
    SfxRow->AddChildToHorizontalBox(SfxVolLabel);

    UButton* SfxUp = CreateCustomButton(WidgetTree, TEXT("SfxUp"), LOCTEXT("SfxUp", " + "), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 12, FMargin(8.0f, 3.0f));
    SfxUp->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleSfxVolUp);
    SfxRow->AddChildToHorizontalBox(SfxUp);

    AudioContentStack->AddChildToVerticalBox(SfxRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

    // EVA Announcer Volume
    UHorizontalBox* EvaRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("EvaRow"));
    UTextBlock* EvaTitle = CreateStyledText(WidgetTree, TEXT("EvaTitle"), 12, kTextColor, true);
    EvaTitle->SetText(LOCTEXT("EvaTitle", "Голосовой ассистент EVA:"));
    EvaRow->AddChildToHorizontalBox(EvaTitle)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* EvaDown = CreateCustomButton(WidgetTree, TEXT("EvaDown"), LOCTEXT("EvaDown", " - "), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 12, FMargin(8.0f, 3.0f));
    EvaDown->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleEvaVolDown);
    EvaRow->AddChildToHorizontalBox(EvaDown);

    EvaVolLabel = CreateStyledText(WidgetTree, TEXT("EvaVolLabel"), 12, kTrackDisplayColor, true);
    EvaVolLabel->SetText(FText::FromString(TEXT(" 100% ")));
    EvaRow->AddChildToHorizontalBox(EvaVolLabel);

    UButton* EvaUp = CreateCustomButton(WidgetTree, TEXT("EvaUp"), LOCTEXT("EvaUp", " + "), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 12, FMargin(8.0f, 3.0f));
    EvaUp->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleEvaVolUp);
    EvaRow->AddChildToHorizontalBox(EvaUp);

    AudioContentStack->AddChildToVerticalBox(EvaRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

    // Unit Voices Toggle
    UHorizontalBox* UVRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("UVRow"));
    UTextBlock* UVTitle = CreateStyledText(WidgetTree, TEXT("UVTitle"), 12, kTextColor, true);
    UVTitle->SetText(LOCTEXT("UVTitle", "Радиопереговоры юнитов:"));
    UVRow->AddChildToHorizontalBox(UVTitle)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UTextBlock* RawUVLabel = nullptr;
    UButton* UVBtn = CreateCustomButton(WidgetTree, TEXT("UVBtn"), LOCTEXT("UVOn", "[ВКЛ]"), kBtnResumeNormal, kBtnResumeHover, kBtnResumePressed, FLinearColor::White, 11, FMargin(12.0f, 4.0f), &RawUVLabel);
    UnitVoiceBtnLabel = RawUVLabel;
    UVBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleUnitVoiceToggled);
    UVRow->AddChildToHorizontalBox(UVBtn);

    AudioContentStack->AddChildToVerticalBox(UVRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

    // Embedded Tactical Soundtrack Player inside Settings Audio Tab
    UBorder* MusicFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MusicFrame"));
    MusicFrame->SetBrush(FSlateColorBrush(kFrameBg));
    MusicFrame->SetPadding(FMargin(12.0f, 10.0f));

    UVerticalBox* MusicInnerStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MusicInnerStack"));

    UTextBlock* MusicHeader = CreateStyledText(WidgetTree, TEXT("MusicHeader"), 12, kMusicHeaderAccent, true);
    MusicHeader->SetText(LOCTEXT("MusicHeader", "ТАКТИЧЕСКИЙ АУДИОКОМПЛЕКС // САУНДТРЕК"));
    MusicHeader->SetJustification(ETextJustify::Center);
    MusicInnerStack->AddChildToVerticalBox(MusicHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

    CurrentTrackDisplay = CreateStyledText(WidgetTree, TEXT("CurrentTrackDisplay"), 13, kTrackDisplayColor, true);
    CurrentTrackDisplay->SetText(FText::FromString(TEXT("► 01. Steel Horizon Pact")));
    CurrentTrackDisplay->SetJustification(ETextJustify::Center);
    MusicInnerStack->AddChildToVerticalBox(CurrentTrackDisplay)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    TrackSelectorCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("TrackSelectorCombo"));
    TrackSelectorCombo->OnSelectionChanged.AddDynamic(this, &URA4PauseMenuWidget::HandleTrackSelectionChanged);
    MusicInnerStack->AddChildToVerticalBox(TrackSelectorCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    // Music Controls (Prev / Pause / Next)
    UHorizontalBox* ControlRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ControlRow"));

    UButton* PrevBtn = CreateCustomButton(WidgetTree, TEXT("PrevBtn"), LOCTEXT("PrevBtn", "<< ПРЕД"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 12, FMargin(6.0f, 5.0f));
    PrevBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandlePrevTrackClicked);
    UHorizontalBoxSlot* PrevSlot = ControlRow->AddChildToHorizontalBox(PrevBtn);
    PrevSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    PrevSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

    UTextBlock* OutMusicLabel = nullptr;
    PlayPauseButton = CreateCustomButton(WidgetTree, TEXT("PlayPauseBtn"), LOCTEXT("PauseLabel", "ПАУЗА"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 12, FMargin(6.0f, 5.0f), &OutMusicLabel);
    PlayPauseLabel = OutMusicLabel;
    PlayPauseButton->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleToggleMusicClicked);
    UHorizontalBoxSlot* PlaySlot = ControlRow->AddChildToHorizontalBox(PlayPauseButton);
    PlaySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    PlaySlot->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 0.0f));

    UButton* NextBtn = CreateCustomButton(WidgetTree, TEXT("NextBtn"), LOCTEXT("NextBtn", "СЛЕД >>"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 12, FMargin(6.0f, 5.0f));
    NextBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleNextTrackClicked);
    UHorizontalBoxSlot* NextSlot = ControlRow->AddChildToHorizontalBox(NextBtn);
    NextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    NextSlot->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));

    MusicInnerStack->AddChildToVerticalBox(ControlRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

    // Music Volume Row
    UHorizontalBox* VolRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("VolRow"));
    UButton* VolDownBtn = CreateCustomButton(WidgetTree, TEXT("VolDownBtn"), LOCTEXT("VolDown", " - "), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(6.0f, 3.0f));
    VolDownBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleVolumeDownClicked);
    VolRow->AddChildToHorizontalBox(VolDownBtn)->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));

    VolumeTextDisplay = CreateStyledText(WidgetTree, TEXT("VolumeTextDisplay"), 11, kSubtextColor, true);
    VolumeTextDisplay->SetText(FText::FromString(TEXT("МУЗЫКА: 35%")));
    VolumeTextDisplay->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* VTSlot = VolRow->AddChildToHorizontalBox(VolumeTextDisplay);
    VTSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    VTSlot->SetHorizontalAlignment(HAlign_Center);
    VTSlot->SetVerticalAlignment(VAlign_Center);

    UButton* VolUpBtn = CreateCustomButton(WidgetTree, TEXT("VolUpBtn"), LOCTEXT("VolUp", " + "), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(6.0f, 3.0f));
    VolUpBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleVolumeUpClicked);
    VolRow->AddChildToHorizontalBox(VolUpBtn)->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));

    MusicInnerStack->AddChildToVerticalBox(VolRow);
    MusicFrame->AddChild(MusicInnerStack);
    AudioContentStack->AddChildToVerticalBox(MusicFrame);

    SettingsStack->AddChildToVerticalBox(AudioContentStack);

    // -------------------------------------------------------------------------
    // TAB 3: GAMEPLAY & CONTROLS
    // -------------------------------------------------------------------------
    GameplayContentStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GameplayStack"));
    GameplayContentStack->SetVisibility(ESlateVisibility::Collapsed);

    // Control Scheme
    UTextBlock* CtrlLabel = CreateStyledText(WidgetTree, TEXT("CtrlLabel"), 12, kTextColor, true);
    CtrlLabel->SetText(LOCTEXT("CtrlLabel", "Схема управления (Control Scheme):"));
    GameplayContentStack->AddChildToVerticalBox(CtrlLabel)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

    UHorizontalBox* CtrlRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CtrlRow"));
    UButton* CtrlClassic = CreateCustomButton(WidgetTree, TEXT("CtrlClassic"), LOCTEXT("CtrlClassic", "КЛАССИКА C&C (ЛКМ)"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    CtrlClassic->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleControlClassicClicked);
    CtrlRow->AddChildToHorizontalBox(CtrlClassic)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* CtrlModern = CreateCustomButton(WidgetTree, TEXT("CtrlModern"), LOCTEXT("CtrlModern", "RTS СТАНДАРТ (ПКМ)"), kBtnActiveTabNormal, kBtnActiveTabHover, kBtnActiveTabNormal, FLinearColor::White, 11, FMargin(4.0f, 4.0f));
    CtrlModern->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleControlModernClicked);
    CtrlRow->AddChildToHorizontalBox(CtrlModern)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    GameplayContentStack->AddChildToVerticalBox(CtrlRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    // Pan Speed
    UTextBlock* PanLabel = CreateStyledText(WidgetTree, TEXT("PanLabel"), 12, kTextColor, true);
    PanLabel->SetText(LOCTEXT("PanLabel", "Скорость прокрутки карты (Pan Speed):"));
    GameplayContentStack->AddChildToVerticalBox(PanLabel)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

    UHorizontalBox* PanRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PanRow"));
    UButton* PanSlow = CreateCustomButton(WidgetTree, TEXT("PanSlow"), LOCTEXT("PanSlow", "1.0x"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    PanSlow->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandlePanSpeedSlow);
    PanRow->AddChildToHorizontalBox(PanSlow)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* PanNorm = CreateCustomButton(WidgetTree, TEXT("PanNorm"), LOCTEXT("PanNorm", "1.5x"), kBtnActiveTabNormal, kBtnActiveTabHover, kBtnActiveTabNormal, FLinearColor::White, 11, FMargin(4.0f, 4.0f));
    PanNorm->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandlePanSpeedNormal);
    PanRow->AddChildToHorizontalBox(PanNorm)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* PanFast = CreateCustomButton(WidgetTree, TEXT("PanFast"), LOCTEXT("PanFast", "2.0x"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    PanFast->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandlePanSpeedFast);
    PanRow->AddChildToHorizontalBox(PanFast)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* PanUltra = CreateCustomButton(WidgetTree, TEXT("PanUltra"), LOCTEXT("PanUltra", "3.0x"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    PanUltra->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandlePanSpeedUltra);
    PanRow->AddChildToHorizontalBox(PanUltra)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    GameplayContentStack->AddChildToVerticalBox(PanRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    // Direct Control FOV
    UTextBlock* FovLabel = CreateStyledText(WidgetTree, TEXT("FovLabel"), 12, kTextColor, true);
    FovLabel->SetText(LOCTEXT("FovLabel", "Угол обзора Direct Control FOV:"));
    GameplayContentStack->AddChildToVerticalBox(FovLabel)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

    UHorizontalBox* FovRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("FovRow"));
    UButton* Fov80 = CreateCustomButton(WidgetTree, TEXT("Fov80"), LOCTEXT("Fov80", "80"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    Fov80->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleDcFov80);
    FovRow->AddChildToHorizontalBox(Fov80)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* Fov90 = CreateCustomButton(WidgetTree, TEXT("Fov90"), LOCTEXT("Fov90", "90"), kBtnActiveTabNormal, kBtnActiveTabHover, kBtnActiveTabNormal, FLinearColor::White, 11, FMargin(4.0f, 4.0f));
    Fov90->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleDcFov90);
    FovRow->AddChildToHorizontalBox(Fov90)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* Fov100 = CreateCustomButton(WidgetTree, TEXT("Fov100"), LOCTEXT("Fov100", "100"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    Fov100->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleDcFov100);
    FovRow->AddChildToHorizontalBox(Fov100)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* Fov110 = CreateCustomButton(WidgetTree, TEXT("Fov110"), LOCTEXT("Fov110", "110"), kBtnMusicNormal, kBtnMusicHover, kBtnMusicPressed, kTextColor, 11, FMargin(4.0f, 4.0f));
    Fov110->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleDcFov110);
    FovRow->AddChildToHorizontalBox(Fov110)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    GameplayContentStack->AddChildToVerticalBox(FovRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    SettingsStack->AddChildToVerticalBox(GameplayContentStack);

    // --- Settings Action Footer (Back to Menu & Reset Defaults) ---
    UHorizontalBox* FooterRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("FooterRow"));

    UButton* BackBtn = CreateCustomButton(WidgetTree, TEXT("BackBtn"), LOCTEXT("BackBtn", "◄ НАЗАД В МЕНЮ"), kBtnResumeNormal, kBtnResumeHover, kBtnResumePressed, FLinearColor::White, 13, FMargin(16.0f, 8.0f));
    BackBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleSettingsBackClicked);
    FooterRow->AddChildToHorizontalBox(BackBtn)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* ResetBtn = CreateCustomButton(WidgetTree, TEXT("ResetBtn"), LOCTEXT("ResetBtn", "↺ ПО УМОЛЧАНИЮ"), kBtnStandardNormal, kBtnStandardHover, kBtnStandardPressed, kTextColor, 13, FMargin(16.0f, 8.0f));
    ResetBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleSettingsDefaultsClicked);
    FooterRow->AddChildToHorizontalBox(ResetBtn)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    SettingsStack->AddChildToVerticalBox(FooterRow)->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 0.0f));

    SettingsPanel->AddChild(SettingsStack);
    CenterOverlay->AddChildToOverlay(SettingsPanel);

    if (UOverlaySlot* Slot = Root->AddChildToOverlay(Box))
    {
        Slot->SetHorizontalAlignment(HAlign_Center);
        Slot->SetVerticalAlignment(VAlign_Center);
    }

    WidgetTree->RootWidget = Root;
    return Super::RebuildWidget();
}

void URA4PauseMenuWidget::ShowMainMenu()
{
    if (MainPausePanel != nullptr && SettingsPanel != nullptr)
    {
        MainPausePanel->SetVisibility(ESlateVisibility::Visible);
        SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void URA4PauseMenuWidget::ShowSettingsMenu()
{
    if (MainPausePanel != nullptr && SettingsPanel != nullptr)
    {
        MainPausePanel->SetVisibility(ESlateVisibility::Collapsed);
        SettingsPanel->SetVisibility(ESlateVisibility::Visible);
    }
}

void URA4PauseMenuWidget::SetTrackList(const TArray<FString>& InTrackNames, int32 InCurrentIndex)
{
    if (TrackSelectorCombo == nullptr)
    {
        return;
    }

    bIsInternalTrackSelection = true;
    TrackSelectorCombo->ClearOptions();
    for (int32 i = 0; i < InTrackNames.Num(); ++i)
    {
        const FString Entry = FString::Printf(TEXT("%02d. %s"), i + 1, *InTrackNames[i]);
        TrackSelectorCombo->AddOption(Entry);
    }

    if (InTrackNames.IsValidIndex(InCurrentIndex))
    {
        CurrentTrackIndex = InCurrentIndex;
        TrackSelectorCombo->SetSelectedIndex(InCurrentIndex);
    }
    bIsInternalTrackSelection = false;
}

void URA4PauseMenuWidget::SetCurrentTrack(int32 InCurrentIndex, const FString& InTrackTitle, bool bIsPlaying)
{
    CurrentTrackIndex = InCurrentIndex;
    bPlaying = bIsPlaying;

    if (CurrentTrackDisplay != nullptr)
    {
        const FString StatusIcon = bIsPlaying ? TEXT("►") : TEXT("II");
        const FString Display = FString::Printf(TEXT("%s %02d. %s"), *StatusIcon, InCurrentIndex + 1, *InTrackTitle);
        CurrentTrackDisplay->SetText(FText::FromString(Display));
    }

    if (PlayPauseLabel != nullptr)
    {
        PlayPauseLabel->SetText(bIsPlaying ? LOCTEXT("PauseLabel", "ПАУЗА") : LOCTEXT("PlayLabel", "ПЛЕЙ"));
    }

    if (TrackSelectorCombo != nullptr && !bIsInternalTrackSelection)
    {
        bIsInternalTrackSelection = true;
        if (InCurrentIndex >= 0 && InCurrentIndex < TrackSelectorCombo->GetOptionCount())
        {
            TrackSelectorCombo->SetSelectedIndex(InCurrentIndex);
        }
        bIsInternalTrackSelection = false;
    }
}

void URA4PauseMenuWidget::SetMusicVolume(float InVolume)
{
    CurrentVolume = FMath::Clamp(InVolume, 0.0f, 1.0f);
    if (VolumeTextDisplay != nullptr)
    {
        const int32 Pct = FMath::RoundToInt(CurrentVolume * 100.0f);
        VolumeTextDisplay->SetText(FText::FromString(FString::Printf(TEXT("МУЗЫКА: %d%%"), Pct)));
    }
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
    ShowSettingsMenu();
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

void URA4PauseMenuWidget::HandlePrevTrackClicked()
{
    OnPrevTrackRequested.Broadcast();
}

void URA4PauseMenuWidget::HandleNextTrackClicked()
{
    OnNextTrackRequested.Broadcast();
}

void URA4PauseMenuWidget::HandleToggleMusicClicked()
{
    OnToggleMusicPauseRequested.Broadcast();
}

void URA4PauseMenuWidget::HandleTrackSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (bIsInternalTrackSelection || TrackSelectorCombo == nullptr)
    {
        return;
    }

    const int32 SelectedIndex = TrackSelectorCombo->GetSelectedIndex();
    if (SelectedIndex >= 0)
    {
        OnTrackSelected.Broadcast(SelectedIndex);
    }
}

void URA4PauseMenuWidget::HandleVolumeDownClicked()
{
    OnVolumeChanged.Broadcast(-0.10f);
}

void URA4PauseMenuWidget::HandleVolumeUpClicked()
{
    OnVolumeChanged.Broadcast(0.10f);
}

void URA4PauseMenuWidget::HandleSettingsBackClicked()
{
    ShowMainMenu();
}

void URA4PauseMenuWidget::HandleSettingsDefaultsClicked()
{
    QualityPreset = 2;
    FpsCap = 120;
    AntiAliasingMethod = 2;
    bScreenShakeEnabled = true;
    MasterVolume = 0.85f;
    SfxVolume = 1.0f;
    EvaVolume = 1.0f;
    bUnitVoiceEnabled = true;
    ControlScheme = 1;
    CameraPanSpeed = 1.5f;
    bEdgeScrollEnabled = true;
    HealthBarMode = 1;
    DirectControlFov = 90.0f;

    UpdateSettingsVisuals();

    OnQualityPresetChanged.Broadcast(QualityPreset);
    OnFpsCapChanged.Broadcast(FpsCap);
    OnAntiAliasingChanged.Broadcast(AntiAliasingMethod);
    OnScreenShakeChanged.Broadcast(bScreenShakeEnabled);
    OnMasterVolumeChanged.Broadcast(MasterVolume);
    OnSfxVolumeChanged.Broadcast(SfxVolume);
    OnEvaVolumeChanged.Broadcast(EvaVolume);
    OnUnitVoicesChanged.Broadcast(bUnitVoiceEnabled);
    OnControlSchemeChanged.Broadcast(ControlScheme);
    OnCameraSpeedChanged.Broadcast(CameraPanSpeed);
    OnEdgeScrollChanged.Broadcast(bEdgeScrollEnabled);
    OnHealthBarModeChanged.Broadcast(HealthBarMode);
    OnDirectControlFovChanged.Broadcast(DirectControlFov);
}

void URA4PauseMenuWidget::SetSettingsTab(ERA4PauseSettingsTab Tab)
{
    ActiveTab = Tab;
    if (GraphicsContentStack && AudioContentStack && GameplayContentStack)
    {
        GraphicsContentStack->SetVisibility(Tab == ERA4PauseSettingsTab::Graphics ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        AudioContentStack->SetVisibility(Tab == ERA4PauseSettingsTab::Audio ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        GameplayContentStack->SetVisibility(Tab == ERA4PauseSettingsTab::Gameplay ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    SetButtonActiveStyle(TabGfxButton, Tab == ERA4PauseSettingsTab::Graphics);
    SetButtonActiveStyle(TabAudButton, Tab == ERA4PauseSettingsTab::Audio);
    SetButtonActiveStyle(TabGameButton, Tab == ERA4PauseSettingsTab::Gameplay);
}

void URA4PauseMenuWidget::HandleTabGraphicsClicked()
{
    SetSettingsTab(ERA4PauseSettingsTab::Graphics);
}

void URA4PauseMenuWidget::HandleTabAudioClicked()
{
    SetSettingsTab(ERA4PauseSettingsTab::Audio);
}

void URA4PauseMenuWidget::HandleTabGameplayClicked()
{
    SetSettingsTab(ERA4PauseSettingsTab::Gameplay);
}

void URA4PauseMenuWidget::UpdateSettingsVisuals()
{
    if (MasterVolLabel) MasterVolLabel->SetText(FText::FromString(FString::Printf(TEXT(" %d%% "), FMath::RoundToInt(MasterVolume * 100.0f))));
    if (SfxVolLabel) SfxVolLabel->SetText(FText::FromString(FString::Printf(TEXT(" %d%% "), FMath::RoundToInt(SfxVolume * 100.0f))));
    if (EvaVolLabel) EvaVolLabel->SetText(FText::FromString(FString::Printf(TEXT(" %d%% "), FMath::RoundToInt(EvaVolume * 100.0f))));
    if (UnitVoiceBtnLabel) UnitVoiceBtnLabel->SetText(bUnitVoiceEnabled ? LOCTEXT("UVOn", "[ВКЛ]") : LOCTEXT("UVOff", "[ВЫКЛ]"));
    if (ScreenShakeBtnLabel) ScreenShakeBtnLabel->SetText(bScreenShakeEnabled ? LOCTEXT("ShakeOn", "[ВКЛ]") : LOCTEXT("ShakeOff", "[ВЫКЛ]"));
}

void URA4PauseMenuWidget::HandleQualityLowClicked() { QualityPreset = 0; OnQualityPresetChanged.Broadcast(0); }
void URA4PauseMenuWidget::HandleQualityMedClicked() { QualityPreset = 1; OnQualityPresetChanged.Broadcast(1); }
void URA4PauseMenuWidget::HandleQualityHighClicked() { QualityPreset = 2; OnQualityPresetChanged.Broadcast(2); }
void URA4PauseMenuWidget::HandleQualityEpicClicked() { QualityPreset = 3; OnQualityPresetChanged.Broadcast(3); }

void URA4PauseMenuWidget::HandleFps60Clicked() { FpsCap = 60; OnFpsCapChanged.Broadcast(60); }
void URA4PauseMenuWidget::HandleFps120Clicked() { FpsCap = 120; OnFpsCapChanged.Broadcast(120); }
void URA4PauseMenuWidget::HandleFps144Clicked() { FpsCap = 144; OnFpsCapChanged.Broadcast(144); }
void URA4PauseMenuWidget::HandleFpsMaxClicked() { FpsCap = 0; OnFpsCapChanged.Broadcast(0); }

void URA4PauseMenuWidget::HandleAaFxaaClicked() { AntiAliasingMethod = 0; OnAntiAliasingChanged.Broadcast(0); }
void URA4PauseMenuWidget::HandleAaTaaClicked() { AntiAliasingMethod = 1; OnAntiAliasingChanged.Broadcast(1); }
void URA4PauseMenuWidget::HandleAaTsrClicked() { AntiAliasingMethod = 2; OnAntiAliasingChanged.Broadcast(2); }

void URA4PauseMenuWidget::HandleScreenShakeToggled()
{
    bScreenShakeEnabled = !bScreenShakeEnabled;
    UpdateSettingsVisuals();
    OnScreenShakeChanged.Broadcast(bScreenShakeEnabled);
}

void URA4PauseMenuWidget::HandleMasterVolDown()
{
    MasterVolume = FMath::Clamp(MasterVolume - 0.10f, 0.0f, 1.0f);
    UpdateSettingsVisuals();
    OnMasterVolumeChanged.Broadcast(MasterVolume);
}

void URA4PauseMenuWidget::HandleMasterVolUp()
{
    MasterVolume = FMath::Clamp(MasterVolume + 0.10f, 0.0f, 1.0f);
    UpdateSettingsVisuals();
    OnMasterVolumeChanged.Broadcast(MasterVolume);
}

void URA4PauseMenuWidget::HandleSfxVolDown()
{
    SfxVolume = FMath::Clamp(SfxVolume - 0.10f, 0.0f, 1.0f);
    UpdateSettingsVisuals();
    OnSfxVolumeChanged.Broadcast(SfxVolume);
}

void URA4PauseMenuWidget::HandleSfxVolUp()
{
    SfxVolume = FMath::Clamp(SfxVolume + 0.10f, 0.0f, 1.0f);
    UpdateSettingsVisuals();
    OnSfxVolumeChanged.Broadcast(SfxVolume);
}

void URA4PauseMenuWidget::HandleEvaVolDown()
{
    EvaVolume = FMath::Clamp(EvaVolume - 0.10f, 0.0f, 1.0f);
    UpdateSettingsVisuals();
    OnEvaVolumeChanged.Broadcast(EvaVolume);
}

void URA4PauseMenuWidget::HandleEvaVolUp()
{
    EvaVolume = FMath::Clamp(EvaVolume + 0.10f, 0.0f, 1.0f);
    UpdateSettingsVisuals();
    OnEvaVolumeChanged.Broadcast(EvaVolume);
}

void URA4PauseMenuWidget::HandleUnitVoiceToggled()
{
    bUnitVoiceEnabled = !bUnitVoiceEnabled;
    UpdateSettingsVisuals();
    OnUnitVoicesChanged.Broadcast(bUnitVoiceEnabled);
}

void URA4PauseMenuWidget::HandleControlClassicClicked() { ControlScheme = 0; OnControlSchemeChanged.Broadcast(0); }
void URA4PauseMenuWidget::HandleControlModernClicked() { ControlScheme = 1; OnControlSchemeChanged.Broadcast(1); }

void URA4PauseMenuWidget::HandlePanSpeedSlow() { CameraPanSpeed = 1.0f; OnCameraSpeedChanged.Broadcast(1.0f); }
void URA4PauseMenuWidget::HandlePanSpeedNormal() { CameraPanSpeed = 1.5f; OnCameraSpeedChanged.Broadcast(1.5f); }
void URA4PauseMenuWidget::HandlePanSpeedFast() { CameraPanSpeed = 2.0f; OnCameraSpeedChanged.Broadcast(2.0f); }
void URA4PauseMenuWidget::HandlePanSpeedUltra() { CameraPanSpeed = 3.0f; OnCameraSpeedChanged.Broadcast(3.0f); }

void URA4PauseMenuWidget::HandleEdgeScrollToggled()
{
    bEdgeScrollEnabled = !bEdgeScrollEnabled;
    OnEdgeScrollChanged.Broadcast(bEdgeScrollEnabled);
}

void URA4PauseMenuWidget::HandleHealthBarModeToggle()
{
    HealthBarMode = (HealthBarMode + 1) % 3;
    OnHealthBarModeChanged.Broadcast(HealthBarMode);
}

void URA4PauseMenuWidget::HandleDcFov80() { DirectControlFov = 80.0f; OnDirectControlFovChanged.Broadcast(80.0f); }
void URA4PauseMenuWidget::HandleDcFov90() { DirectControlFov = 90.0f; OnDirectControlFovChanged.Broadcast(90.0f); }
void URA4PauseMenuWidget::HandleDcFov100() { DirectControlFov = 100.0f; OnDirectControlFovChanged.Broadcast(100.0f); }
void URA4PauseMenuWidget::HandleDcFov110() { DirectControlFov = 110.0f; OnDirectControlFovChanged.Broadcast(110.0f); }

#undef LOCTEXT_NAMESPACE


