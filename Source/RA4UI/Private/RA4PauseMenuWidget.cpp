// Copyright (c) Red Alert 4 project. In-Game Pause and Quit Menu with Tactical Soundtrack Player.
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
    Panel->SetPadding(FMargin(32.0f, 24.0f));

    UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stack"));

    // Header Title
    UTextBlock* Title = CreateStyledText(WidgetTree, TEXT("PauseTitle"), 24, kHeaderAccent, true);
    Title->SetText(LOCTEXT("PauseTitle", "ПАУЗА // ТАКТИЧЕСКОЕ МЕНЮ"));
    Title->SetJustification(ETextJustify::Center);
    Title->SetShadowOffset(FVector2D(2.0f, 2.0f));
    Title->SetShadowColorAndOpacity(FLinearColor::Black);
    Stack->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));

    // Subtitle
    UTextBlock* Subtitle = CreateStyledText(WidgetTree, TEXT("PauseSubtitle"), 12, kSubtextColor, false);
    Subtitle->SetText(LOCTEXT("PauseSubtitle", "СИМУЛЯЦИЯ ПРИОСТАНОВЛЕНА"));
    Subtitle->SetJustification(ETextJustify::Center);
    Stack->AddChildToVerticalBox(Subtitle)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));

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
    Stack->AddChildToVerticalBox(ResumeBtn)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

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
        FMargin(20.0f, 8.0f));
    RestartBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleRestartClicked);
    Stack->AddChildToVerticalBox(RestartBtn)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

    // =========================================================================
    // MUSIC PLAYER SECTION (ТАКТИЧЕСКИЙ ПРОИГРЫВАТЕЛЬ)
    // =========================================================================
    UBorder* MusicPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MusicPanel"));
    MusicPanel->SetBrushColor(kFrameBg);
    MusicPanel->SetPadding(FMargin(16.0f, 12.0f));

    UVerticalBox* MusicStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MusicStack"));

    // Music Header
    UTextBlock* MusicTitle = CreateStyledText(WidgetTree, TEXT("MusicTitle"), 13, kMusicHeaderAccent, true);
    MusicTitle->SetText(LOCTEXT("MusicTitle", "♪ САУНДТРЕК БОЯ // ТАКТИЧЕСКИЙ ЭФИР"));
    MusicTitle->SetJustification(ETextJustify::Center);
    MusicStack->AddChildToVerticalBox(MusicTitle)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    // Current Track Display Banner
    UBorder* TrackDisplayBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TrackDisplayBox"));
    TrackDisplayBox->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.05f, 0.95f));
    TrackDisplayBox->SetPadding(FMargin(12.0f, 8.0f));

    CurrentTrackDisplay = CreateStyledText(WidgetTree, TEXT("CurrentTrackDisplay"), 13, kTrackDisplayColor, true);
    CurrentTrackDisplay->SetText(FText::FromString(TEXT("▶ 01. Steel Horizon Pact")));
    CurrentTrackDisplay->SetJustification(ETextJustify::Center);
    CurrentTrackDisplay->SetAutoWrapText(false);
    TrackDisplayBox->AddChild(CurrentTrackDisplay);
    MusicStack->AddChildToVerticalBox(TrackDisplayBox)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    // Track Selector Dropdown
    TrackSelectorCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("TrackSelectorCombo"));
    TrackSelectorCombo->OnSelectionChanged.AddDynamic(this, &URA4PauseMenuWidget::HandleTrackSelectionChanged);
    MusicStack->AddChildToVerticalBox(TrackSelectorCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    // Playback Control Buttons Row (Prev / Pause / Next)
    UHorizontalBox* ControlRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ControlRow"));

    UButton* PrevBtn = CreateCustomButton(
        WidgetTree,
        TEXT("PrevBtn"),
        LOCTEXT("PrevBtn", "◄◄ ПРЕД"),
        kBtnMusicNormal,
        kBtnMusicHover,
        kBtnMusicPressed,
        kTextColor,
        13,
        FMargin(8.0f, 6.0f));
    PrevBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandlePrevTrackClicked);
    UHorizontalBoxSlot* PrevSlot = ControlRow->AddChildToHorizontalBox(PrevBtn);
    PrevSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    PrevSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));

    UTextBlock* OutMusicLabel = nullptr;
    PlayPauseButton = CreateCustomButton(
        WidgetTree,
        TEXT("PlayPauseBtn"),
        LOCTEXT("PauseLabel", "⏸ ПАУЗА"),
        kBtnMusicNormal,
        kBtnMusicHover,
        kBtnMusicPressed,
        kTextColor,
        13,
        FMargin(8.0f, 6.0f),
        &OutMusicLabel);
    PlayPauseLabel = OutMusicLabel;
    PlayPauseButton->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleToggleMusicClicked);
    UHorizontalBoxSlot* PlaySlot = ControlRow->AddChildToHorizontalBox(PlayPauseButton);
    PlaySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    PlaySlot->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 0.0f));

    UButton* NextBtn = CreateCustomButton(
        WidgetTree,
        TEXT("NextBtn"),
        LOCTEXT("NextBtn", "СЛЕД ►►"),
        kBtnMusicNormal,
        kBtnMusicHover,
        kBtnMusicPressed,
        kTextColor,
        13,
        FMargin(8.0f, 6.0f));
    NextBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleNextTrackClicked);
    UHorizontalBoxSlot* NextSlot = ControlRow->AddChildToHorizontalBox(NextBtn);
    NextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    NextSlot->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));

    MusicStack->AddChildToVerticalBox(ControlRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    // Volume Control Row ([-] [ГРОМКОСТЬ: 35%] [+])
    UHorizontalBox* VolRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("VolRow"));

    UButton* VolDownBtn = CreateCustomButton(
        WidgetTree,
        TEXT("VolDownBtn"),
        LOCTEXT("VolDown", "🔉 -"),
        kBtnMusicNormal,
        kBtnMusicHover,
        kBtnMusicPressed,
        kTextColor,
        12,
        FMargin(8.0f, 4.0f));
    VolDownBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleVolumeDownClicked);
    UHorizontalBoxSlot* VDSlot = VolRow->AddChildToHorizontalBox(VolDownBtn);
    VDSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    VDSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));

    VolumeTextDisplay = CreateStyledText(WidgetTree, TEXT("VolumeTextDisplay"), 12, kSubtextColor, true);
    VolumeTextDisplay->SetText(FText::FromString(TEXT("ГРОМКОСТЬ: 35%")));
    VolumeTextDisplay->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* VTSlot = VolRow->AddChildToHorizontalBox(VolumeTextDisplay);
    VTSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    VTSlot->SetHorizontalAlignment(HAlign_Center);
    VTSlot->SetVerticalAlignment(VAlign_Center);

    UButton* VolUpBtn = CreateCustomButton(
        WidgetTree,
        TEXT("VolUpBtn"),
        LOCTEXT("VolUp", "🔊 +"),
        kBtnMusicNormal,
        kBtnMusicHover,
        kBtnMusicPressed,
        kTextColor,
        12,
        FMargin(8.0f, 4.0f));
    VolUpBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleVolumeUpClicked);
    UHorizontalBoxSlot* VUSlot = VolRow->AddChildToHorizontalBox(VolUpBtn);
    VUSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    VUSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));

    MusicStack->AddChildToVerticalBox(VolRow);

    MusicPanel->AddChild(MusicStack);
    Stack->AddChildToVerticalBox(MusicPanel)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

    // =========================================================================
    // SYSTEM / NAVIGATION BUTTONS
    // =========================================================================

    // Settings
    UButton* SettingsBtn = CreateCustomButton(
        WidgetTree,
        TEXT("SettingsBtn"),
        LOCTEXT("SettingsBtn", "⚙ НАСТРОЙКИ"),
        kBtnStandardNormal,
        kBtnStandardHover,
        kBtnStandardPressed,
        kTextColor,
        14,
        FMargin(20.0f, 8.0f));
    SettingsBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleSettingsClicked);
    Stack->AddChildToVerticalBox(SettingsBtn)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    // Quit to Main Menu
    UButton* QuitMenuBtn = CreateCustomButton(
        WidgetTree,
        TEXT("QuitMenuBtn"),
        LOCTEXT("QuitMenuBtn", "⎋ В ГЛАВНОЕ МЕНЮ"),
        kBtnStandardNormal,
        kBtnStandardHover,
        kBtnStandardPressed,
        kTextColor,
        14,
        FMargin(20.0f, 8.0f));
    QuitMenuBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleQuitToMenuClicked);
    Stack->AddChildToVerticalBox(QuitMenuBtn)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    // Quit to Desktop
    UButton* QuitGameBtn = CreateCustomButton(
        WidgetTree,
        TEXT("QuitGameBtn"),
        LOCTEXT("QuitGameBtn", "✕ ВЫЙТИ НА РАБОЧИЙ СТОЛ"),
        kBtnDangerNormal,
        kBtnDangerHover,
        kBtnDangerPressed,
        FLinearColor::White,
        14,
        FMargin(20.0f, 8.0f));
    QuitGameBtn->OnClicked.AddDynamic(this, &URA4PauseMenuWidget::HandleQuitToDesktopClicked);
    Stack->AddChildToVerticalBox(QuitGameBtn);

    USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Box"));
    Box->SetWidthOverride(460.0f);
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
        const FString StatusIcon = bIsPlaying ? TEXT("▶") : TEXT("⏸");
        const FString Display = FString::Printf(TEXT("%s %02d. %s"), *StatusIcon, InCurrentIndex + 1, *InTrackTitle);
        CurrentTrackDisplay->SetText(FText::FromString(Display));
    }

    if (PlayPauseLabel != nullptr)
    {
        PlayPauseLabel->SetText(bIsPlaying ? LOCTEXT("PauseLabel", "⏸ ПАУЗА") : LOCTEXT("PlayLabel", "▶ ПЛЕЙ"));
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
        VolumeTextDisplay->SetText(FText::FromString(FString::Printf(TEXT("ГРОМКОСТЬ: %d%%"), Pct)));
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

#undef LOCTEXT_NAMESPACE
