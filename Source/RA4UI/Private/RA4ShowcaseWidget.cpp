// Copyright (c) Red Alert 4 project.

#include "RA4ShowcaseWidget.h"

#include "RA4FactionData.h"
#include "RA4MainMenuScreenWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/Slider.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateColorBrush.h"
#include "Misc/Paths.h"
#include "Styling/SlateTypes.h"

#define LOCTEXT_NAMESPACE "RA4ShowcaseWidget"

namespace
{
const FLinearColor PanelBlack(0.005f, 0.008f, 0.013f, 0.58f);
const FLinearColor PanelDark(0.012f, 0.018f, 0.028f, 0.80f);
const FLinearColor TextWhite(0.87f, 0.91f, 0.95f, 1.0f);
// Bloc accents come from FRA4FactionDataRegistry so the showcase can never drift
// away from the palette the live selection and HUD screens use.
const FLinearColor EurasianPlum = FRA4FactionDataRegistry::GetBlocAccentColor(ERA4FactionTheme::EurasianPact);
const FLinearColor AtlanticCobalt = FRA4FactionDataRegistry::GetBlocAccentColor(ERA4FactionTheme::AtlanticAlliance);
const FLinearColor EasternJade = FRA4FactionDataRegistry::GetBlocAccentColor(ERA4FactionTheme::EasternCoalition);
const FLinearColor PacificTurquoise = FRA4FactionDataRegistry::GetBlocAccentColor(ERA4FactionTheme::PacificPact);
const FLinearColor IndependentAmber = FRA4FactionDataRegistry::GetBlocAccentColor(ERA4FactionTheme::Independent);
// Scarlet is the shared horizon and alarm accent only, never a bloc colour.
const FLinearColor ScarletAlert = FRA4FactionDataRegistry::GetHorizonScarletColor();

bool IsHudScreen(const int32 ScreenIndex)
{
    return ScreenIndex == 2 || (ScreenIndex >= 14 && ScreenIndex <= 16) || (ScreenIndex >= 23 && ScreenIndex <= 27);
}

void PlaceOnReferenceCanvas(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D Position,
                            const FVector2D Size, const int32 ZOrder = 0)
{
    UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
    Slot->SetPosition(Position);
    Slot->SetSize(Size);
    Slot->SetAnchors(FAnchors(0.0f, 0.0f));
    Slot->SetAlignment(FVector2D::ZeroVector);
    Slot->SetZOrder(ZOrder);
}
}

void URA4ShowcaseWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetScreen(InitialScreenIndex);
}

void URA4ShowcaseWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    PresentationTime += InDeltaTime;
    if (Artwork)
    {
        const float BreathingOpacity = 0.82f + FMath::Sin(PresentationTime * 0.45f) * 0.045f;
        Artwork->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, BreathingOpacity));
    }
    if (ProgressBar)
    {
        ProgressBar->SetRenderOpacity(0.82f + FMath::Sin(PresentationTime * 2.2f) * 0.18f);
    }
}

void URA4ShowcaseWidget::ShowScreen(const int32 InScreen)
{
    SetScreen(FMath::Clamp(InScreen, 0, 27));
}

void URA4ShowcaseWidget::SetInitialScreenForPresentation(const int32 InScreen)
{
    InitialScreenIndex = FMath::Clamp(InScreen, 0, 27);
    ActiveScreen = InitialScreenIndex;
}

TSharedRef<SWidget> URA4ShowcaseWidget::RebuildWidget()
{
    if (WidgetTree)
    {
        ActiveScreen = InitialScreenIndex;
        BuildLayout();
    }

    return Super::RebuildWidget();
}

UTextBlock* URA4ShowcaseWidget::CreateText(const FText& Text, const float FontSize, const FLinearColor& Color,
                                            const FName WidgetName)
{
    UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), WidgetName);
    TextBlock->SetText(Text);
    TextBlock->SetColorAndOpacity(FSlateColor(Color));
    FSlateFontInfo Font(
        FPaths::ProjectContentDir() / TEXT("RA4UI/Fonts/RA4_RobotoCondensedSemiBold.ttf"), FontSize);
    Font.LetterSpacing = 55;
    TextBlock->SetFont(Font);
    TextBlock->SetAutoWrapText(true);
    return TextBlock;
}

UButton* URA4ShowcaseWidget::AddNavigationButton(UVerticalBox* Parent, const FText& Label, const FName WidgetName)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), WidgetName);
    FButtonStyle Style;
    Style.SetNormal(FSlateColorBrush(FLinearColor(0.015f, 0.022f, 0.032f, 0.86f)));
    Style.SetHovered(FSlateColorBrush(FLinearColor(0.42f, 0.035f, 0.045f, 0.94f)));
    Style.SetPressed(FSlateColorBrush(FLinearColor(0.92f, 0.09f, 0.09f, 0.98f)));
    Style.SetDisabled(FSlateColorBrush(FLinearColor(0.01f, 0.012f, 0.018f, 0.46f)));
    Button->SetStyle(Style);
    Button->SetColorAndOpacity(TextWhite);
    Button->AddChild(CreateText(Label, 18.0f, TextWhite, FName(WidgetName.ToString() + TEXT("_Label"))));

    UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Button);
    Slot->SetPadding(FMargin(0.0f, 4.0f));
    return Button;
}

void URA4ShowcaseWidget::BuildLayout()
{
    if (IsHudScreen(InitialScreenIndex))
    {
        BuildHudLayout();
        return;
    }

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ScreenRoot"));
    WidgetTree->RootWidget = Root;

    Artwork = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ScreenArtwork"));
    if (UTexture2D* CommandCenterTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/RA4UI/Art/T_RA4_USSR_CommandCenter.T_RA4_USSR_CommandCenter")))
    {
        Artwork->SetBrushFromTexture(CommandCenterTexture, false);
    }
    Artwork->SetColorAndOpacity(FLinearColor(0.62f, 0.62f, 0.62f, 1.0f));
    UOverlaySlot* ArtworkSlot = Root->AddChildToOverlay(Artwork);
    ArtworkSlot->SetHorizontalAlignment(HAlign_Fill);
    ArtworkSlot->SetVerticalAlignment(VAlign_Fill);

    Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CinematicShade"));
    Background->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.42f));
    UOverlaySlot* ShadeSlot = Root->AddChildToOverlay(Background);
    ShadeSlot->SetHorizontalAlignment(HAlign_Fill);
    ShadeSlot->SetVerticalAlignment(VAlign_Fill);

    UScaleBox* ResponsiveScale = WidgetTree->ConstructWidget<UScaleBox>(
        UScaleBox::StaticClass(), TEXT("ResponsiveScale"));
    ResponsiveScale->SetStretch(EStretch::ScaleToFit);
    ResponsiveScale->SetStretchDirection(EStretchDirection::Both);
    UOverlaySlot* ScaleSlot = Root->AddChildToOverlay(ResponsiveScale);
    ScaleSlot->SetHorizontalAlignment(HAlign_Fill);
    ScaleSlot->SetVerticalAlignment(VAlign_Fill);

    USizeBox* ReferenceFrame = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("ReferenceFrame"));
    ReferenceFrame->SetWidthOverride(1920.0f);
    ReferenceFrame->SetHeightOverride(1080.0f);
    ResponsiveScale->SetContent(ReferenceFrame);

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("ScreenCanvas"));
    ReferenceFrame->SetContent(Canvas);

    const auto FramePanel = [this](UWidget* Content, const FName Name, const FMargin Padding) -> UBorder*
    {
        UBorder* Metal = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), FName(Name.ToString() + TEXT("_Metal")));
        Metal->SetBrushColor(FLinearColor(0.17f, 0.18f, 0.19f, 0.98f));
        Metal->SetPadding(FMargin(2.0f));
        UBorder* AccentEdge = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), FName(Name.ToString() + TEXT("_Accent")));
        AccentEdge->SetBrushColor(FLinearColor(0.38f, 0.012f, 0.018f, 1.0f));
        AccentEdge->SetPadding(FMargin(2.0f));
        Metal->SetContent(AccentEdge);
        UBorder* Interior = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
        Interior->SetBrushColor(FLinearColor(0.006f, 0.007f, 0.009f, 0.92f));
        Interior->SetPadding(Padding);
        AccentEdge->SetContent(Interior);
        Interior->SetContent(Content);
        return Metal;
    };

    UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("HeaderRow"));
    UButton* BackButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("BackButton"));
    FButtonStyle BackStyle;
    BackStyle.SetNormal(FSlateColorBrush(FLinearColor(0.025f, 0.018f, 0.019f, 0.96f)));
    BackStyle.SetHovered(FSlateColorBrush(FLinearColor(0.22f, 0.012f, 0.018f, 1.0f)));
    BackStyle.SetPressed(FSlateColorBrush(FLinearColor(0.55f, 0.02f, 0.025f, 1.0f)));
    BackButton->SetStyle(BackStyle);
    BackButton->AddChild(CreateText(LOCTEXT("Back", "‹  BACK"), 20.0f, TextWhite, TEXT("BackLabel")));
    BackButton->OnClicked.AddDynamic(this, &URA4ShowcaseWidget::OpenMainMenu);
    UHorizontalBoxSlot* BackSlot = HeaderRow->AddChildToHorizontalBox(BackButton);
    BackSlot->SetPadding(FMargin(12.0f, 12.0f, 26.0f, 12.0f));
    BackSlot->SetVerticalAlignment(VAlign_Center);

    UVerticalBox* BrandBlock = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("BrandBlock"));
    TitleText = CreateText(LOCTEXT("Title", "SCARLET HORIZON"), 36.0f, EurasianPlum, TEXT("TitleText"));
    BrandBlock->AddChildToVerticalBox(TitleText);
    UTextBlock* CommandLink = CreateText(
        LOCTEXT("CommandLink", "ЗАЩИЩЁННАЯ СЕТЬ КОМАНДОВАНИЯ  //  КАНАЛ 04"), 11.0f,
        FLinearColor(0.48f, 0.46f, 0.45f, 1.0f), TEXT("CommandLink"));
    BrandBlock->AddChildToVerticalBox(CommandLink);
    UHorizontalBoxSlot* BrandSlot = HeaderRow->AddChildToHorizontalBox(BrandBlock);
    BrandSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BrandSlot->SetVerticalAlignment(VAlign_Center);

    UTextBlock* SecurityStatus = CreateText(
        LOCTEXT("SecurityStatus", "СВЯЗЬ: СТАБИЛЬНАЯ\nШИФРОВАНИЕ: АКТИВНО"), 12.0f,
        FLinearColor(0.70f, 0.68f, 0.65f, 1.0f), TEXT("SecurityStatus"));
    SecurityStatus->SetJustification(ETextJustify::Right);
    HeaderRow->AddChildToHorizontalBox(SecurityStatus)->SetPadding(FMargin(20.0f, 18.0f));
    PlaceOnReferenceCanvas(
        Canvas, FramePanel(HeaderRow, TEXT("ScreenHeader"), FMargin(6.0f)),
        FVector2D(32.0f, 26.0f), FVector2D(1856.0f, 112.0f), 3);

    UVerticalBox* Rail = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("FactionRail"));
    UTextBlock* RailMark = CreateText(LOCTEXT("RailMark", "SH"), 28.0f, EurasianPlum, TEXT("RailMark"));
    RailMark->SetJustification(ETextJustify::Center);
    Rail->AddChildToVerticalBox(RailMark)->SetPadding(FMargin(0.0f, 20.0f));
    UTextBlock* RailCode = CreateText(
        LOCTEXT("RailCode", "04\n\n17\n\n26\n\n49"), 14.0f,
        FLinearColor(0.38f, 0.36f, 0.34f, 1.0f), TEXT("RailCode"));
    RailCode->SetJustification(ETextJustify::Center);
    Rail->AddChildToVerticalBox(RailCode);
    PlaceOnReferenceCanvas(
        Canvas, FramePanel(Rail, TEXT("FactionRailFrame"), FMargin(4.0f)),
        FVector2D(32.0f, 164.0f), FVector2D(112.0f, 786.0f), 3);

    UVerticalBox* ContentColumn = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ContentColumn"));
    SubtitleText = CreateText(FText::GetEmpty(), 36.0f, TextWhite, TEXT("SubtitleText"));
    ContentColumn->AddChildToVerticalBox(SubtitleText)->SetPadding(FMargin(34.0f, 28.0f, 34.0f, 12.0f));
    UTextBlock* SectionRule = CreateText(
        LOCTEXT("SectionRule", "================================================"), 12.0f,
        EurasianPlum, TEXT("SectionRule"));
    ContentColumn->AddChildToVerticalBox(SectionRule)->SetPadding(FMargin(34.0f, 0.0f, 34.0f, 20.0f));
    ContentText = CreateText(FText::GetEmpty(), 20.0f, TextWhite, TEXT("ContentText"));
    ContentText->SetLineHeightPercentage(1.12f);
    UVerticalBoxSlot* ContentTextSlot = ContentColumn->AddChildToVerticalBox(ContentText);
    ContentTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ContentTextSlot->SetPadding(FMargin(34.0f, 10.0f, 34.0f, 20.0f));

    if (InitialScreenIndex == 4)
    {
        UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(), TEXT("SettingsTabs"));
        const FText TabLabels[] = {
            LOCTEXT("VideoTab", "ИЗОБРАЖЕНИЕ"),
            LOCTEXT("AudioTab", "ЗВУК"),
            LOCTEXT("ControlsTab", "УПРАВЛЕНИЕ"),
            LOCTEXT("GameTab", "ИГРА")
        };
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(TabLabels); ++Index)
        {
            UBorder* Tab = WidgetTree->ConstructWidget<UBorder>(
                UBorder::StaticClass(), FName(*FString::Printf(TEXT("SettingsTab_%d"), Index)));
            Tab->SetBrushColor(Index == 0
                ? FLinearColor(0.36f, 0.012f, 0.018f, 0.96f)
                : FLinearColor(0.025f, 0.022f, 0.024f, 0.92f));
            UTextBlock* TabText = CreateText(
                TabLabels[Index], 14.0f, TextWhite,
                FName(*FString::Printf(TEXT("SettingsTabText_%d"), Index)));
            TabText->SetJustification(ETextJustify::Center);
            Tab->SetContent(TabText);
            UHorizontalBoxSlot* TabSlot = Tabs->AddChildToHorizontalBox(Tab);
            TabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            TabSlot->SetPadding(FMargin(3.0f));
        }
        ContentColumn->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(30.0f, 0.0f, 30.0f, 14.0f));

        const auto AddSlider = [this, ContentColumn](
            const FText& Label, const float Value, const FName Name)
        {
            UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
                UHorizontalBox::StaticClass(), FName(Name.ToString() + TEXT("_Row")));
            UTextBlock* RowLabel = CreateText(
                Label, 15.0f, TextWhite, FName(Name.ToString() + TEXT("_Label")));
            UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(RowLabel);
            LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            LabelSlot->SetVerticalAlignment(VAlign_Center);
            USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), Name);
            Slider->SetValue(Value);
            Slider->SetSliderBarColor(FLinearColor(0.20f, 0.18f, 0.18f, 1.0f));
            Slider->SetSliderHandleColor(EurasianPlum);
            USizeBox* SliderSize = WidgetTree->ConstructWidget<USizeBox>(
                USizeBox::StaticClass(), FName(Name.ToString() + TEXT("_Size")));
            SliderSize->SetWidthOverride(420.0f);
            SliderSize->SetContent(Slider);
            UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(SliderSize);
            SliderSlot->SetVerticalAlignment(VAlign_Center);
            SliderSlot->SetPadding(FMargin(20.0f, 0.0f, 0.0f, 0.0f));
            ContentColumn->AddChildToVerticalBox(Row)->SetPadding(FMargin(34.0f, 7.0f));
        };
        AddSlider(LOCTEXT("Brightness", "ЯРКОСТЬ  54%"), 0.54f, TEXT("BrightnessSlider"));
        AddSlider(LOCTEXT("InterfaceScale", "МАСШТАБ ИНТЕРФЕЙСА  100%"), 0.50f, TEXT("InterfaceScaleSlider"));
        AddSlider(LOCTEXT("MusicVolume", "МУЗЫКА  80%"), 0.80f, TEXT("MusicVolumeSlider"));
        AddSlider(LOCTEXT("EffectsVolume", "ЭФФЕКТЫ  90%"), 0.90f, TEXT("EffectsVolumeSlider"));
    }

    ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
        UProgressBar::StaticClass(), TEXT("ScreenProgress"));
    ProgressBar->SetPercent(0.72f);
    ProgressBar->SetFillColorAndOpacity(EurasianPlum);
    ContentColumn->AddChildToVerticalBox(ProgressBar)->SetPadding(FMargin(34.0f, 12.0f, 34.0f, 28.0f));
    AccentPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AccentPanel"));
    AccentPanel->SetBrushColor(FLinearColor(0.10f, 0.004f, 0.006f, 0.90f));
    if (InitialScreenIndex == 4)
    {
        if (UTexture2D* PanelGradient = LoadObject<UTexture2D>(
            nullptr, TEXT("/Game/RA4UI/Art/T_RA4_PanelGradient_USSR.T_RA4_PanelGradient_USSR")))
        {
            FSlateBrush GradientBrush;
            GradientBrush.SetResourceObject(PanelGradient);
            GradientBrush.DrawAs = ESlateBrushDrawType::Image;
            GradientBrush.ImageSize = FVector2D(1024.0f, 1024.0f);
            AccentPanel->SetBrush(GradientBrush);
        }
    }
    AccentPanel->SetContent(ContentColumn);
    PlaceOnReferenceCanvas(
        Canvas, FramePanel(AccentPanel, TEXT("PrimaryPanel"), FMargin(2.0f)),
        FVector2D(164.0f, 164.0f), FVector2D(1156.0f, 786.0f), 3);

    UVerticalBox* StatusColumn = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("StatusColumn"));
    UTextBlock* StatusHeader = CreateText(
        LOCTEXT("StatusHeader", "ОПЕРАТИВНЫЕ ДАННЫЕ"), 16.0f, TextWhite, TEXT("StatusHeader"));
    StatusColumn->AddChildToVerticalBox(StatusHeader)->SetPadding(FMargin(24.0f, 24.0f, 24.0f, 12.0f));
    UTextBlock* StatusRule = CreateText(
        LOCTEXT("StatusRule", "================"), 11.0f, EurasianPlum, TEXT("StatusRule"));
    StatusColumn->AddChildToVerticalBox(StatusRule)->SetPadding(FMargin(24.0f, 0.0f, 24.0f, 18.0f));
    StatusText = CreateText(FText::GetEmpty(), 17.0f, TextWhite, TEXT("StatusText"));
    StatusText->SetLineHeightPercentage(1.15f);
    UVerticalBoxSlot* StatusTextSlot = StatusColumn->AddChildToVerticalBox(StatusText);
    StatusTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    StatusTextSlot->SetPadding(FMargin(24.0f));
    UTextBlock* Access = CreateText(
        LOCTEXT("Access", "ДОПУСК: АЛЬФА\nСЕАНС: ЗАШИФРОВАН"), 12.0f,
        FLinearColor(0.56f, 0.53f, 0.50f, 1.0f), TEXT("Access"));
    StatusColumn->AddChildToVerticalBox(Access)->SetPadding(FMargin(24.0f, 12.0f, 24.0f, 28.0f));
    PlaceOnReferenceCanvas(
        Canvas, FramePanel(StatusColumn, TEXT("StatusPanel"), FMargin(2.0f)),
        FVector2D(1340.0f, 164.0f), FVector2D(548.0f, 786.0f), 3);

    UHorizontalBox* FooterRow = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("FooterRow"));
    UTextBlock* Footer = CreateText(
        LOCTEXT("Footer", "СИСТЕМА ГОТОВА  ·  НАВИГАЦИЯ АКТИВНА  ·  РУССКИЙ"),
        13.0f, TextWhite, TEXT("FooterText"));
    FooterRow->AddChildToHorizontalBox(Footer)->SetPadding(FMargin(18.0f, 12.0f));
    UTextBlock* Build = CreateText(
        LOCTEXT("Build", "RA4 // BUILD 1.0.0"), 12.0f,
        FLinearColor(0.47f, 0.45f, 0.43f, 1.0f), TEXT("BuildText"));
    UHorizontalBoxSlot* BuildSlot = FooterRow->AddChildToHorizontalBox(Build);
    BuildSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BuildSlot->SetHorizontalAlignment(HAlign_Right);
    BuildSlot->SetPadding(FMargin(18.0f, 12.0f));
    PlaceOnReferenceCanvas(
        Canvas, FramePanel(FooterRow, TEXT("FooterPanel"), FMargin(2.0f)),
        FVector2D(32.0f, 974.0f), FVector2D(1856.0f, 68.0f), 3);
}

void URA4ShowcaseWidget::BuildHudLayout()
{
    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HudRoot"));
    WidgetTree->RootWidget = Root;

    Artwork = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HudArtwork"));
    if (UTexture2D* CommandCenterTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/RA4UI/Art/T_RA4_USSR_CommandCenter.T_RA4_USSR_CommandCenter")))
    {
        Artwork->SetBrushFromTexture(CommandCenterTexture, false);
    }
    Artwork->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.78f));
    Root->AddChildToOverlay(Artwork);

    Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HudShade"));
    Background->SetBrushColor(FLinearColor(0.002f, 0.005f, 0.009f, 0.22f));
    Root->AddChildToOverlay(Background);

    UVerticalBox* Frame = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HudFrame"));
    UOverlaySlot* FrameSlot = Root->AddChildToOverlay(Frame);
    FrameSlot->SetPadding(FMargin(26.0f));

    UBorder* ResourceStrip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResourceStrip"));
    ResourceStrip->SetBrushColor(FLinearColor(0.005f, 0.008f, 0.013f, 0.86f));
    UHorizontalBox* ResourceRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ResourceRow"));
    ResourceStrip->SetContent(ResourceRow);
    TitleText = CreateText(LOCTEXT("HudTitle", "ЕВРАЗИЙСКИЙ ПАКТ // ОПЕРАЦИЯ"), 25.0f, EurasianPlum, TEXT("HudTitleText"));
    ResourceRow->AddChildToHorizontalBox(TitleText)->SetPadding(FMargin(18.0f, 11.0f));
    UTextBlock* Credits = CreateText(LOCTEXT("HudCredits", "КРЕДИТЫ  23 450"), 18.0f, TextWhite, TEXT("HudCredits"));
    ResourceRow->AddChildToHorizontalBox(Credits)->SetPadding(FMargin(28.0f, 14.0f));
    UTextBlock* Power = CreateText(LOCTEXT("HudPower", "ЭНЕРГИЯ  780 / 920"), 18.0f, TextWhite, TEXT("HudPower"));
    ResourceRow->AddChildToHorizontalBox(Power)->SetPadding(FMargin(20.0f, 14.0f));
    UTextBlock* Clock = CreateText(LOCTEXT("HudClock", "00:18:42"), 18.0f, TextWhite, TEXT("HudClock"));
    UHorizontalBoxSlot* ClockSlot = ResourceRow->AddChildToHorizontalBox(Clock);
    ClockSlot->SetPadding(FMargin(20.0f, 14.0f));
    ClockSlot->SetHorizontalAlignment(HAlign_Right);
    ClockSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UButton* BackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("HudBackButton"));
    FButtonStyle BackStyle;
    BackStyle.SetNormal(FSlateColorBrush(FLinearColor(0.12f, 0.02f, 0.03f, 0.90f)));
    BackStyle.SetHovered(FSlateColorBrush(EurasianPlum));
    BackStyle.SetPressed(FSlateColorBrush(FLinearColor(0.50f, 0.02f, 0.03f, 1.0f)));
    BackButton->SetStyle(BackStyle);
    UTextBlock* BackLabel = CreateText(LOCTEXT("HudBack", "◄ В МЕНЮ"), 14.0f, TextWhite, TEXT("HudBackLabel"));
    BackButton->AddChild(BackLabel);
    BackButton->OnClicked.AddDynamic(this, &URA4ShowcaseWidget::OpenMainMenu);
    UHorizontalBoxSlot* BackSlot = ResourceRow->AddChildToHorizontalBox(BackButton);
    BackSlot->SetPadding(FMargin(16.0f, 6.0f));
    BackSlot->SetVerticalAlignment(VAlign_Center);

    Frame->AddChildToVerticalBox(ResourceStrip)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

    UHorizontalBox* TacticalSpace = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TacticalSpace"));
    Frame->AddChildToVerticalBox(TacticalSpace)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UVerticalBox* LeftTactical = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftTactical"));
    UHorizontalBoxSlot* LeftSlot = TacticalSpace->AddChildToHorizontalBox(LeftTactical);
    LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LeftSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));

    USizeBox* Spacer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TacticalSpacer"));
    LeftTactical->AddChildToVerticalBox(Spacer)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UBorder* ObjectiveCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ObjectiveCard"));
    ObjectiveCard->SetBrushColor(FLinearColor(0.007f, 0.012f, 0.020f, 0.88f));
    ContentText = CreateText(FText::GetEmpty(), 16.0f, TextWhite, TEXT("HudObjectiveText"));
    ObjectiveCard->SetContent(ContentText);
    LeftTactical->AddChildToVerticalBox(ObjectiveCard)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

    UBorder* Minimap = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Minimap"));
    Minimap->SetBrushColor(FLinearColor(0.008f, 0.032f, 0.027f, 0.88f));
    StatusText = CreateText(FText::GetEmpty(), 15.0f, TextWhite, TEXT("HudMinimapText"));
    Minimap->SetContent(StatusText);
    USizeBox* MinimapSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MinimapSize"));
    MinimapSize->SetWidthOverride(300.0f);
    MinimapSize->SetHeightOverride(176.0f);
    MinimapSize->SetContent(Minimap);
    LeftTactical->AddChildToVerticalBox(MinimapSize);

    UBorder* ProductionPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ProductionPanel"));
    ProductionPanel->SetBrushColor(FLinearColor(0.011f, 0.016f, 0.025f, 0.88f));
    AccentPanel = ProductionPanel;
    UVerticalBox* ProductionColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ProductionColumn"));
    ProductionPanel->SetContent(ProductionColumn);
    UTextBlock* ProductionHeader = CreateText(LOCTEXT("ProductionHeader", "ПРОИЗВОДСТВО"), 22.0f, TextWhite, TEXT("ProductionHeader"));
    ProductionColumn->AddChildToVerticalBox(ProductionHeader)->SetPadding(FMargin(18.0f, 16.0f, 18.0f, 8.0f));
    UTextBlock* QueueOne = CreateText(LOCTEXT("QueueOne", "ОБТ-92 «ГРАНИТ»\nТяжёлая броня  ·  очередь 01"), 16.0f, TextWhite, TEXT("QueueOne"));
    ProductionColumn->AddChildToVerticalBox(QueueOne)->SetPadding(FMargin(18.0f, 12.0f));
    ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ProductionProgress"));
    ProgressBar->SetPercent(0.72f);
    ProgressBar->SetFillColorAndOpacity(EurasianPlum);
    ProductionColumn->AddChildToVerticalBox(ProgressBar)->SetPadding(FMargin(18.0f, 0.0f, 18.0f, 18.0f));
    UTextBlock* QueueTwo = CreateText(LOCTEXT("QueueTwo", "ПРИЗЫВНИК ×5\nКазармы  ·  очередь 02\n\nМИГ «КУЗНЕЦ»\nАэродром  ·  ожидание"), 16.0f, TextWhite, TEXT("QueueTwo"));
    UVerticalBoxSlot* QueueTwoSlot = ProductionColumn->AddChildToVerticalBox(QueueTwo);
    QueueTwoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    QueueTwoSlot->SetPadding(FMargin(18.0f, 8.0f));
    UTextBlock* Intel = CreateText(LOCTEXT("Intel", "РАЗВЕДКА // ЮЖНЫЙ МОСТ\nКОНТАКТЫ: 12  ·  УГРОЗА: ВЫСОКАЯ"), 14.0f, TextWhite, TEXT("Intel"));
    ProductionColumn->AddChildToVerticalBox(Intel)->SetPadding(FMargin(18.0f, 8.0f, 18.0f, 16.0f));
    USizeBox* ProductionSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ProductionSize"));
    ProductionSize->SetWidthOverride(330.0f);
    ProductionSize->SetContent(ProductionPanel);
    TacticalSpace->AddChildToHorizontalBox(ProductionSize);

    UBorder* CommandStrip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CommandStrip"));
    CommandStrip->SetBrushColor(FLinearColor(0.005f, 0.008f, 0.013f, 0.88f));
    CommandStrip->SetContent(CreateText(LOCTEXT("HudCommands", "ОТРЯД: 3 × ТЕСЛА-ТАНК   ·   12 × ПРИЗЫВНИК   ·   ПРИКАЗ: УДЕРЖИВАТЬ ПОЗИЦИЮ   ·   СИЛЫ СПЕЦНАЗНАЧЕНИЯ: ГОТОВЫ"), 15.0f, TextWhite, TEXT("HudCommands")));
    Frame->AddChildToVerticalBox(CommandStrip)->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
}

void URA4ShowcaseWidget::SetScreen(const int32 InScreen)
{
    ActiveScreen = InScreen;
    UE_LOG(LogTemp, Display, TEXT("RA4 UI showcase selected screen %d."), ActiveScreen);

    FLinearColor Accent = EurasianPlum;
    FText Heading = LOCTEXT("MainHeading", "ГЛОБАЛЬНЫЙ КОМАНДНЫЙ ЦЕНТР");
    FText Body = LOCTEXT("MainBody", "Выберите направление операции. Кампания, сетевая игра, схватка, редактор и энциклопедия доступны из защищённой оперативной сети.");
    FText Status = LOCTEXT("MainStatus", "КОМАНДИР\nУровень 24\n\nСОСТОЯНИЕ СЕТИ\nПодключена\n\nТЕКУЩАЯ ОПЕРАЦИЯ\nБарьер «Тифон» — фаза 2/4");
    float Progress = 1.0f;

    switch (ActiveScreen)
    {
    case 1:
        Accent = EurasianPlum;
        Heading = LOCTEXT("CampaignHeading", "ВЫБОР: БЛОК ИЛИ КАТЕГОРИЯ");
        Body = LOCTEXT("CampaignBody", "Евразийский пакт — тяжёлая механизация, РЭБ и эшелонированная ПВО.\nАтлантический альянс — авиация, разведка и точные удары.\nВосточная коалиция — массовое производство, ракеты и дроны.\nТихоокеанский пакт — робототехника, ПВО и море.\nНезависимые державы — категория выбора самостоятельных стран, а не союз.");
        Status = LOCTEXT("CampaignStatus", "ПУТЬ ВЫБОРА\n01 Блок → 02 Страна → 03 Доктрина\n\nВЫБРАНО\nЕвразийский пакт\n\nДАЛЕЕ\nСтрана");
        Progress = 0.18f;
        break;
    case 2:
        Accent = EurasianPlum;
        Heading = LOCTEXT("HudHeading", "ОПЕРАЦИЯ «ТИХИЙ РЕЛЕЙ»");
        Body = LOCTEXT("HudBody", "БОЕВОЙ HUD · РОССИЯ · ТЯЖЁЛЫЙ ПРОРЫВ\n\nКРЕДИТЫ 23 450     ЭНЕРГИЯ 17 820     ЛИМИТ 186 / 200\n\nОЧЕРЕДЬ ПРОИЗВОДСТВА\nОБТ-92 «Гранит» — 72%\nМС-12 «Рубеж» ×5\n\nЗАДАЧА: подавить 3 узла связи и провести бронегруппу через перевал.");
        Status = LOCTEXT("HudStatus", "МИНИ-КАРТА\nГорный коридор\n\nВЫДЕЛЕНО\n3 × ОБТ-92 «Гранит»\n12 × МС-12 «Рубеж»\n\nТРЕВОГА\nНизкая");
        Progress = 0.72f;
        break;
    case 3:
        Accent = AtlanticCobalt;
        Heading = LOCTEXT("LobbyHeading", "ЛОББИ СЕТЕВОГО МАТЧА");
        Body = LOCTEXT("LobbyBody", "КАРТА: АРХИПЕЛАГ «ТИФОН»\nРЕЖИМ: СХВАТКА\n\nSOKOLOV_1945 — ЕВРАЗИЙСКИЙ ПАКТ — РОССИЯ — ТЯЖЁЛЫЙ ПРОРЫВ — ГОТОВ\nAllied_Command — АТЛАНТИЧЕСКИЙ АЛЬЯНС — США — СЕТЕВАЯ ВОЙНА — ГОТОВ\nJadeTiger — ВОСТОЧНАЯ КОАЛИЦИЯ — КИТАЙ — ДРОНОВАЯ ВОЙНА — ГОТОВ\nShinKaze — ТИХООКЕАНСКИЙ ПАКТ — ЯПОНИЯ — ОБОРОНА ОСТРОВОВ — ГОТОВ\nDesertSignal — НЕЗАВИСИМЫЕ ДЕРЖАВЫ — ИРАН — РАКЕТНЫЕ ВОЙСКА — ГОТОВ");
        Status = LOCTEXT("LobbyStatus", "ОБЩИЙ ЧАТ\n[20:49] Allied_Command: Готов.\n[20:49] JadeTiger: Проверяю связь.\n\nПИНГ\n34 мс\n\nГОТОВНОСТЬ\n8 / 8");
        Progress = 0.50f;
        break;
    case 4:
        Accent = EurasianPlum;
        Heading = LOCTEXT("SettingsHeading", "НАСТРОЙКИ СИСТЕМЫ");
        Body = LOCTEXT("SettingsBody", "ИЗОБРАЖЕНИЕ\nРазрешение: 1920×1080\nМасштаб интерфейса: автоматически\n\nЗВУК\nМузыка: 80%\nЭффекты: 90%\n\nУПРАВЛЕНИЕ\nEnhanced Input: активно");
        Status = LOCTEXT("SettingsStatus", "ПРОФИЛЬ\nКомандир\n\nЛОКАЛИЗАЦИЯ\nРусский\n\nВЕРСИЯ\n0.1.0");
        Progress = 0.86f;
        break;
    case 5:
        Accent = ScarletAlert;
        Heading = LOCTEXT("SplashHeading", "SCARLET HORIZON");
        Body = LOCTEXT("SplashBody", "АЛЬТЕРНАТИВНАЯ СОВРЕМЕННОСТЬ\n\nНажмите любую клавишу, чтобы войти в оперативную сеть.");
        Status = LOCTEXT("SplashStatus", "СТАТУС СИСТЕМ\nВсе контуры готовы\n\nСЕАНС\nЗашифрован");
        Progress = 0.08f;
        break;
    case 6:
        Accent = EurasianPlum;
        Heading = LOCTEXT("FactionHeading", "ЕВРАЗИЙСКИЙ ПАКТ → РОССИЯ");
        Body = LOCTEXT("FactionBody", "Опора Пакта и ударная сила на континенте. Масштабные бронетанковые объединения, развитые средства РЭБ и эшелонированная ПВО создают непреодолимый щит.\n\nДОКТРИНЫ\nРЭБ и ракетные войска · Тяжёлый прорыв · Арктическая группировка");
        Status = LOCTEXT("FactionStatus", "ВЫБРАНО\nРоссия\n\nДОКТРИНА\nТяжёлый прорыв\n\nСОЮЗНИКИ ПО БЛОКУ\nБеларусь · Казахстан");
        Progress = 0.25f;
        break;
    case 7:
        Accent = AtlanticCobalt;
        Heading = LOCTEXT("AtlanticCampaignHeading", "США: ДАЛЬНИЙ РУБЕЖ");
        Body = LOCTEXT("AtlanticCampaignBody", "На дальних подступах к нашим интересам формируется нестабильность. Мы прощупываем силу, поддерживаем союзников и обеспечиваем свободу манёвра в любой точке океана.");
        Status = LOCTEXT("AtlanticCampaignStatus", "КОМАНДИР\nЛейтенант-коммандер Маркус Рид\n\nТЕАТР\nСеверная Атлантика\n\nМИССИЙ ЗАВЕРШЕНО\n5 / 12");
        Progress = 0.42f;
        break;
    case 8:
        Accent = EasternJade;
        Heading = LOCTEXT("EasternCampaignHeading", "КИТАЙ: НЕФРИТОВАЯ СЕТЬ");
        Body = LOCTEXT("EasternCampaignBody", "Умные автозаводы, массовая бронетехника и гиперзвуковые ракетные рубежи связаны в единую сеть. Потеря узлов быстро снижает синхронизацию всей группировки.");
        Status = LOCTEXT("EasternCampaignStatus", "ГЛАВА 5\nРой над дельтой\n\nСЛОЖНОСТЬ\nВетеран\n\nМИССИЙ ЗАВЕРШЕНО\n8 / 12");
        Progress = 0.63f;
        break;
    case 9:
        Accent = PacificTurquoise;
        Heading = LOCTEXT("PacificCampaignHeading", "ЯПОНИЯ: ДУГА ШТОРМА");
        Body = LOCTEXT("PacificCampaignBody", "Островная оборона строится на лазерном перехвате, автономных боевых роботах и скоростной переброске между терминалами побережья.");
        Status = LOCTEXT("PacificCampaignStatus", "ГЛАВА 3\nПервая волна\n\nСЛОЖНОСТЬ\nНормально\n\nМИССИЙ ЗАВЕРШЕНО\n4 / 12");
        Progress = 0.37f;
        break;
    case 10:
        Accent = EurasianPlum;
        Heading = LOCTEXT("MissionMapHeading", "КАРТА ОПЕРАЦИЙ: ЕВРАЗИЙСКИЙ ПАКТ");
        Body = LOCTEXT("MissionMapBody", "01 — БАЗА «КАРПАТЫ»\nРазвернуть передовой узел снабжения.\n\n04 — ЖЕЛЕЗНАЯ ДУГА\nОбеспечить проход бронеколонны по железнодорожному коридору.\n\n07 — ТИХИЙ РЕЛЕЙ\nПодавить сеть наведения противника в горном коридоре.");
        Status = LOCTEXT("MissionMapStatus", "ВЫБРАННАЯ ОПЕРАЦИЯ\nТихий релей\n\nСЛОЖНОСТЬ\nВетеран\n\nНАГРАДА\nЭМП-7 «Громобой»");
        Progress = 0.33f;
        break;
    case 11:
        Accent = EurasianPlum;
        Heading = LOCTEXT("BriefingHeading", "БРИФИНГ: ТИХИЙ РЕЛЕЙ");
        Body = LOCTEXT("BriefingBody", "Горный коридор закрывается. Противник развернул сеть наведения на перевале. Подавите узлы связи, проведите бронегруппу до рассвета и сохраните мобильный комплекс РЭБ.");
        Status = LOCTEXT("BriefingStatus", "ОСНОВНЫЕ ЦЕЛИ\nПодавить 3 узла связи\nПровести бронегруппу через перевал\n\nДОПОЛНИТЕЛЬНАЯ\nСохранить комплекс РЭБ\n\nОЦЕНКА ПРОТИВНИКА\nВысокая");
        Progress = 0.44f;
        break;
    case 12:
        Accent = AtlanticCobalt;
        Heading = LOCTEXT("VideoHeading", "ЗАЩИЩЁННЫЙ КАНАЛ СВЯЗИ");
        Body = LOCTEXT("VideoBody", "МАРКУС РИД, АТЛАНТИЧЕСКИЙ АЛЬЯНС:\n«У нас девять минут до закрытия коридора. Решение нужно сейчас.»\n\nИРИНА ВОЛКОВА, ЕВРАЗИЙСКИЙ ПАКТ:\n«Бронегруппа выйдет на рубеж раньше. Держите воздух.»");
        Status = LOCTEXT("VideoStatus", "ШИФРОВАНИЕ\nКаскад-4\n\nЗАДЕРЖКА\n42 мс\n\nРИСК ПЕРЕХВАТА\nНизкий");
        Progress = 0.61f;
        break;
    case 13:
        Accent = EurasianPlum;
        Heading = LOCTEXT("LoadingHeading", "ЗАГРУЗКА МИССИИ");
        Body = LOCTEXT("LoadingBody", "Синхронизация театра боевых действий. Загрузка маршрутов снабжения, боевых групп и защищённых каналов управления.");
        Status = LOCTEXT("LoadingStatus", "ОПЕРАЦИЯ\nТихий релей\n\nСТРАНА\nРоссия\n\nСОСТОЯНИЕ\nСинхронизация театра");
        Progress = 0.72f;
        break;
    case 14:
        Accent = AtlanticCobalt;
        Heading = LOCTEXT("AtlanticHudHeading", "ОПЕРАЦИЯ: ЛИНИЯ ПРИБОЯ");
        Body = LOCTEXT("AtlanticHudBody", "БОЕВОЙ HUD · США · ЭКСПЕДИЦИОННЫЕ СИЛЫ\n\nКРЕДИТЫ 23 450     ЭНЕРГИЯ 17 820     ЛИМИТ 186 / 200\n\nОЧЕРЕДЬ ПРОИЗВОДСТВА\nОБТ «Bulwark» — 54%\nM6 «Sentinel» ×4\n\nЗАДАЧА: удержать пролив и защитить экспедиционную группу.");
        Status = LOCTEXT("AtlanticHudStatus", "МИНИ-КАРТА\nПрибрежный сектор\n\nВЫДЕЛЕНО\nАвианосец «Свобода»\n\nПАЛУБНАЯ АВИАЦИЯ\n78 / 78");
        Progress = 0.54f;
        break;
    case 15:
        Accent = EasternJade;
        Heading = LOCTEXT("EasternHudHeading", "ОПЕРАЦИЯ: РОЙ НАД ДЕЛЬТОЙ");
        Body = LOCTEXT("EasternHudBody", "БОЕВОЙ HUD · КИТАЙ · МАССОВАЯ МЕХАНИЗАЦИЯ\n\nКРЕДИТЫ 23 450     ЭНЕРГИЯ 17 820     ЛИМИТ 186 / 200\n\nОЧЕРЕДЬ ПРОИЗВОДСТВА\nОБТ «Цинлун» — 43%\nБоевой дрон ×3\n\nЗАДАЧА: развернуть производственный контур и выпустить 12 боевых машин.");
        Status = LOCTEXT("EasternHudStatus", "МИНИ-КАРТА\nПромышленная дельта\n\nВЫДЕЛЕНО\nУмный автозавод\n\nСИНХРОНИЗАЦИЯ\n91%");
        Progress = 0.43f;
        break;
    case 16:
        Accent = PacificTurquoise;
        Heading = LOCTEXT("PacificHudHeading", "ОПЕРАЦИЯ: ЧИСТЫЙ ГОРИЗОНТ");
        Body = LOCTEXT("PacificHudBody", "БОЕВОЙ HUD · ЯПОНИЯ · РОБОТИЗИРОВАННЫЕ СИЛЫ\n\nКРЕДИТЫ 23 450     ЭНЕРГИЯ 17 820     ЛИМИТ 184 / 200\n\nОЧЕРЕДЬ ПРОИЗВОДСТВА\nПерехватчик «Сирокко» — 68%\nШагоход «Камакири» ×2\n\nЗАДАЧА: перехватить ударную группу и защитить радарный узел.");
        Status = LOCTEXT("PacificHudStatus", "МИНИ-КАРТА\nОстровная гряда\n\nВЫДЕЛЕНО\nЭскадрилья «Сирокко» 6 / 6\n\nБОЕКОМПЛЕКТ\n100%");
        Progress = 0.68f;
        break;
    case 17:
        Accent = EurasianPlum;
        Heading = LOCTEXT("PauseHeading", "ПАУЗА");
        Body = LOCTEXT("PauseBody", "Операция приостановлена. Продолжите бой, откройте настройки или вернитесь в командный центр.");
        Status = LOCTEXT("PauseStatus", "ОПЕРАЦИЯ\nТихий релей\n\nВРЕМЯ БОЯ\n18:42\n\nСОХРАНЕНИЕ\nАвтоматическое");
        Progress = 0.50f;
        break;
    case 18:
        Accent = EurasianPlum;
        Heading = LOCTEXT("VictoryHeading", "ПОБЕДА");
        Body = LOCTEXT("VictoryBody", "Сеть наведения противника подавлена, бронегруппа прошла перевал. Район переходит под наш контроль.");
        Status = LOCTEXT("VictoryStatus", "РЕЗУЛЬТАТ\nПобеда · Россия\n\nВРЕМЯ\n24:18\n\nПОТЕРИ\nПриемлемые");
        Progress = 1.0f;
        break;
    case 19:
        Accent = AtlanticCobalt;
        Heading = LOCTEXT("EncyclopediaHeading", "ЭНЦИКЛОПЕДИЯ");
        Body = LOCTEXT("EncyclopediaBody", "ЭМП-7 «ГРОМОБОЙ»\nМашина РЭБ Евразийского пакта: подавляет наведение и связь противника.\n\nM6 «SENTINEL»\nБазовый стрелок общего атлантического арсенала.\n\nШАГОХОД «КАМАКИРИ»\nАвтономная роботизированная платформа Тихоокеанского пакта.");
        Status = LOCTEXT("EncyclopediaStatus", "КАТЕГОРИЯ\nБоевые единицы\n\nЗАПИСЕЙ\n128\n\nФИЛЬТР\nВсе страны");
        Progress = 0.40f;
        break;
    case 20:
        Accent = EasternJade;
        Heading = LOCTEXT("TechTreeHeading", "ТЕХНОЛОГИЧЕСКОЕ ДЕРЕВО");
        Body = LOCTEXT("TechTreeBody", "T1  ШТАБ → КАЗАРМА → ЗАВОД БРОНЕТЕХНИКИ\n\nT2  РАДАРНЫЙ УЗЕЛ → АЭРОДРОМ → ВОЕННО-МОРСКАЯ БАЗА\n\nT3  ЦЕНТР ПЕРСПЕКТИВНЫХ РАЗРАБОТОК → КОМПЛЕКС ЭМИ → РАКЕТНАЯ ШАХТА");
        Status = LOCTEXT("TechTreeStatus", "СТРАНА\nРоссия\n\nОТКРЫТО\n18 / 46\n\nУРОВЕНЬ\nT2");
        Progress = 0.39f;
        break;
    case 21:
        Accent = PacificTurquoise;
        Heading = LOCTEXT("ModsHeading", "МОДИФИКАЦИИ");
        Body = LOCTEXT("ModsBody", "РАСШИРЕННЫЕ КАРТЫ — включено\nНовые сценарии и таблицы баланса.\n\nТАКТИЧЕСКИЕ ПОРТРЕТЫ — включено\nДополнительные портреты командиров.\n\nEXPERIMENTAL / LEGACY — выключено\nАномальный темпоральный контент вне стандартного канона.");
        Status = LOCTEXT("ModsStatus", "АКТИВНЫХ МОДОВ\n2\n\nСОВМЕСТИМОСТЬ\nПроверена\n\nПЕРЕЗАПУСК\nНе требуется");
        Progress = 0.67f;
        break;
    case 22:
        Accent = IndependentAmber;
        Heading = LOCTEXT("IndependentDetailHeading", "ИРАН: ТЕНЬ НАД ХРЕБТОМ");
        Body = LOCTEXT("IndependentDetailBody", "Разрозненные мобильные группы превращают рельеф, маскировку и ложные сигналы в единую сеть сдерживания. Иран — самостоятельная держава и не входит ни в один военный блок.");
        Status = LOCTEXT("IndependentDetailStatus", "ДОКТРИНА\nБПЛА и асимметричная война\n\nОГРАНИЧЕНИЕ\nОграниченная тяжёлая броня\n\nГЛАВА 2\nПустой след");
        Progress = 0.21f;
        break;
    case 23:
        Accent = ScarletAlert;
        Heading = LOCTEXT("BattleHudHeading", "ТРЕВОГА: БРОНЕКОЛОННА");
        Body = LOCTEXT("BattleHudBody", "КРЕДИТЫ 23 450     ЭНЕРГИЯ 17 820     ЛИМИТ 186 / 200\n\nПРОИЗВОДСТВО\nТТП-11 «Воевода» — 89%\nЭМП-7 «Громобой» — 31%\n\nТАКТИЧЕСКОЕ СОБЫТИЕ\nПротивник атакует южный мост. Активируйте заградительный огонь.");
        Status = LOCTEXT("BattleHudStatus", "МИНИ-КАРТА\nЮг: контакт\n\nВЫДЕЛЕНО\n4 × ТТП-11 «Воевода»\n\nУГРОЗА\nВысокая");
        Progress = 0.89f;
        break;
    case 24:
        Accent = ScarletAlert;
        Heading = LOCTEXT("AlertHudHeading", "ПРЕДУПРЕЖДЕНИЕ: ВОЗДУШНАЯ УГРОЗА");
        Body = LOCTEXT("AlertHudBody", "Обнаружена ударная группа противника. Разверните ПВО, смените позиции мобильных комплексов и защитите энергоузел.");
        Status = LOCTEXT("AlertHudStatus", "КОНТАКТОВ\n12\n\nДО ПОДЛЁТА\n00:32\n\nПВО\n4 батареи");
        Progress = 0.32f;
        break;
    case 25:
        Accent = AtlanticCobalt;
        Heading = LOCTEXT("NavalHudHeading", "МОРСКАЯ ОПЕРАЦИЯ: ЛИНИЯ ПРИБОЯ");
        Body = LOCTEXT("NavalHudBody", "КРЕДИТЫ 23 450     ЭНЕРГИЯ 17 820     ЛИМИТ 186 / 200\n\nФЛОТ\nАвианосец «Свобода» — готов\nЭсминец ×3\n\nЗАДАЧА: провести экспедиционную группу через пролив и подавить береговые батареи.");
        Status = LOCTEXT("NavalHudStatus", "МОРСКАЯ КАРТА\nКвадрат D-4\n\nКОНВОЙ\n5 / 6 кораблей\n\nПОГОДА\nШторм");
        Progress = 0.58f;
        break;
    case 26:
        Accent = PacificTurquoise;
        Heading = LOCTEXT("AirHudHeading", "ВОЗДУШНАЯ ОПЕРАЦИЯ: ЧИСТЫЙ ГОРИЗОНТ");
        Body = LOCTEXT("AirHudBody", "КРЕДИТЫ 23 450     ЭНЕРГИЯ 17 820     ЛИМИТ 184 / 200\n\nЭСКАДРИЛЬЯ «СИРОККО»\nМногоцелевой перехватчик ×6\nБПЛА «Гаруда» ×2\n\nЗАДАЧА: перехватить ударную группу и не допустить прорыва к городу.");
        Status = LOCTEXT("AirHudStatus", "ВОЗДУШНАЯ КАРТА\nОстровная гряда\n\nТОПЛИВО\n76%\n\nКОНТАКТЫ\n3 эскадрильи");
        Progress = 0.76f;
        break;
    case 27:
        Accent = IndependentAmber;
        Heading = LOCTEXT("SaturationHudHeading", "САТУРАЦИОННЫЙ УДАР: ГОТОВНОСТЬ 94%");
        Body = LOCTEXT("SaturationHudBody", "МОБИЛЬНЫЙ УЗЕЛ «МИРАЖ»\nГотовность 100%\n\nВЫБЕРИТЕ КООРДИНАТУ УДАРА\nЗалп мобильных пусковых и дроновый рой накрывают район одновременно, перегружая активную защиту противника.");
        Status = LOCTEXT("SaturationHudStatus", "ДО ЗАЛПА\n00:18\n\nПУСКОВЫХ\n9\n\nПОСЛЕ УДАРА\nОбязательная смена позиции");
        Progress = 0.94f;
        break;
    default:
        break;
    }

    if (TitleText)
    {
        TitleText->SetColorAndOpacity(FSlateColor(Accent));
    }
    if (Artwork)
    {
        const TCHAR* ArtworkPath = TEXT("/Game/RA4UI/Art/T_RA4_USSR_CommandCenter.T_RA4_USSR_CommandCenter");
        if (ActiveScreen == 3 || ActiveScreen == 7 || ActiveScreen == 12 || ActiveScreen == 14 || ActiveScreen == 19 || ActiveScreen == 25 || ActiveScreen == 26)
        {
            ArtworkPath = TEXT("/Game/RA4UI/Art/T_RA4_Allies_ArcticFleet.T_RA4_Allies_ArcticFleet");
        }
        else if (ActiveScreen == 1 || ActiveScreen == 6 || ActiveScreen == 8 || ActiveScreen == 15 || ActiveScreen == 20 || ActiveScreen == 22)
        {
            ArtworkPath = TEXT("/Game/RA4UI/Art/T_RA4_Eastern_CommandFortress.T_RA4_Eastern_CommandFortress");
        }
        else if (ActiveScreen == 9 || ActiveScreen == 16 || ActiveScreen == 21 || ActiveScreen == 27)
        {
            ArtworkPath = TEXT("/Game/RA4UI/Art/T_RA4_Chrono_TemporalCitadel.T_RA4_Chrono_TemporalCitadel");
        }

        if (UTexture2D* ScreenArtwork = LoadObject<UTexture2D>(nullptr, ArtworkPath))
        {
            Artwork->SetBrushFromTexture(ScreenArtwork, false);
        }
    }
    if (AccentPanel)
    {
        AccentPanel->SetBrushColor(ActiveScreen == 4
            ? FLinearColor(1.0f, 1.0f, 1.0f, 0.96f)
            : FLinearColor(Accent.R * 0.09f, Accent.G * 0.09f, Accent.B * 0.09f, 0.78f));
    }
    if (SubtitleText)
    {
        SubtitleText->SetText(Heading);
    }
    if (ContentText)
    {
        ContentText->SetText(Body);
    }
    if (StatusText)
    {
        StatusText->SetText(Status);
    }
    if (ProgressBar)
    {
        ProgressBar->SetPercent(Progress);
        ProgressBar->SetFillColorAndOpacity(Accent);
    }
}

void URA4ShowcaseWidget::OpenMainMenu()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4MainMenuScreenWidget* MainMenu = CreateWidget<URA4MainMenuScreenWidget>(
            PlayerController, URA4MainMenuScreenWidget::StaticClass()))
        {
            MainMenu->AddToViewport(0);
            RemoveFromParent();
        }
    }
}
void URA4ShowcaseWidget::OpenCampaign() { SetScreen(1); }
void URA4ShowcaseWidget::OpenHud() { SetScreen(2); }
void URA4ShowcaseWidget::OpenLobby() { SetScreen(3); }
void URA4ShowcaseWidget::OpenSettings() { SetScreen(4); }

#undef LOCTEXT_NAMESPACE
