// Copyright (c) Red Alert 4 project.

#include "RA4ShowcaseWidget.h"

#include "RA4CommandCentreMenuWidget.h"
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
constexpr FLinearColor PanelBlack(0.005f, 0.008f, 0.013f, 0.58f);
constexpr FLinearColor PanelDark(0.012f, 0.018f, 0.028f, 0.80f);
constexpr FLinearColor TextWhite(0.87f, 0.91f, 0.95f, 1.0f);
constexpr FLinearColor SovietRed(0.90f, 0.07f, 0.08f, 1.0f);
constexpr FLinearColor AlliesBlue(0.10f, 0.48f, 0.95f, 1.0f);
constexpr FLinearColor EasternGold(0.92f, 0.64f, 0.13f, 1.0f);
constexpr FLinearColor ChronoViolet(0.61f, 0.25f, 0.94f, 1.0f);

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
    TitleText = CreateText(LOCTEXT("Title", "RED ALERT 4"), 36.0f, SovietRed, TEXT("TitleText"));
    BrandBlock->AddChildToVerticalBox(TitleText);
    UTextBlock* CommandLink = CreateText(
        LOCTEXT("CommandLink", "ZAShchIShchYoNNAYa SET KOMANDOVANIYa  //  KANAL 04"), 11.0f,
        FLinearColor(0.48f, 0.46f, 0.45f, 1.0f), TEXT("CommandLink"));
    BrandBlock->AddChildToVerticalBox(CommandLink);
    UHorizontalBoxSlot* BrandSlot = HeaderRow->AddChildToHorizontalBox(BrandBlock);
    BrandSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BrandSlot->SetVerticalAlignment(VAlign_Center);

    UTextBlock* SecurityStatus = CreateText(
        LOCTEXT("SecurityStatus", "SVYaZ: STABILNAYa\nShIFROVANIE: AKTIVNO"), 12.0f,
        FLinearColor(0.70f, 0.68f, 0.65f, 1.0f), TEXT("SecurityStatus"));
    SecurityStatus->SetJustification(ETextJustify::Right);
    HeaderRow->AddChildToHorizontalBox(SecurityStatus)->SetPadding(FMargin(20.0f, 18.0f));
    PlaceOnReferenceCanvas(
        Canvas, FramePanel(HeaderRow, TEXT("ScreenHeader"), FMargin(6.0f)),
        FVector2D(32.0f, 26.0f), FVector2D(1856.0f, 112.0f), 3);

    UVerticalBox* Rail = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("FactionRail"));
    UTextBlock* RailMark = CreateText(LOCTEXT("RailMark", "RA4"), 28.0f, SovietRed, TEXT("RailMark"));
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
        SovietRed, TEXT("SectionRule"));
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
            LOCTEXT("VideoTab", "IZOBRAZhENIE"),
            LOCTEXT("AudioTab", "ZVUK"),
            LOCTEXT("ControlsTab", "UPRAVLENIE"),
            LOCTEXT("GameTab", "IGRA")
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
            Slider->SetSliderHandleColor(SovietRed);
            USizeBox* SliderSize = WidgetTree->ConstructWidget<USizeBox>(
                USizeBox::StaticClass(), FName(Name.ToString() + TEXT("_Size")));
            SliderSize->SetWidthOverride(420.0f);
            SliderSize->SetContent(Slider);
            UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(SliderSize);
            SliderSlot->SetVerticalAlignment(VAlign_Center);
            SliderSlot->SetPadding(FMargin(20.0f, 0.0f, 0.0f, 0.0f));
            ContentColumn->AddChildToVerticalBox(Row)->SetPadding(FMargin(34.0f, 7.0f));
        };
        AddSlider(LOCTEXT("Brightness", "YaRKOST  54%"), 0.54f, TEXT("BrightnessSlider"));
        AddSlider(LOCTEXT("InterfaceScale", "MASHQ INTERFEYSA  100%"), 0.50f, TEXT("InterfaceScaleSlider"));
        AddSlider(LOCTEXT("MusicVolume", "MUZYKA  80%"), 0.80f, TEXT("MusicVolumeSlider"));
        AddSlider(LOCTEXT("EffectsVolume", "EFFEKTY  90%"), 0.90f, TEXT("EffectsVolumeSlider"));
    }

    ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
        UProgressBar::StaticClass(), TEXT("ScreenProgress"));
    ProgressBar->SetPercent(0.72f);
    ProgressBar->SetFillColorAndOpacity(SovietRed);
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
        LOCTEXT("StatusHeader", "OPERATIVNYE DANNYE"), 16.0f, TextWhite, TEXT("StatusHeader"));
    StatusColumn->AddChildToVerticalBox(StatusHeader)->SetPadding(FMargin(24.0f, 24.0f, 24.0f, 12.0f));
    UTextBlock* StatusRule = CreateText(
        LOCTEXT("StatusRule", "================"), 11.0f, SovietRed, TEXT("StatusRule"));
    StatusColumn->AddChildToVerticalBox(StatusRule)->SetPadding(FMargin(24.0f, 0.0f, 24.0f, 18.0f));
    StatusText = CreateText(FText::GetEmpty(), 17.0f, TextWhite, TEXT("StatusText"));
    StatusText->SetLineHeightPercentage(1.15f);
    UVerticalBoxSlot* StatusTextSlot = StatusColumn->AddChildToVerticalBox(StatusText);
    StatusTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    StatusTextSlot->SetPadding(FMargin(24.0f));
    UTextBlock* Access = CreateText(
        LOCTEXT("Access", "DOPUSK: ALFA\nSEANS: ZAShIFROVAN"), 12.0f,
        FLinearColor(0.56f, 0.53f, 0.50f, 1.0f), TEXT("Access"));
    StatusColumn->AddChildToVerticalBox(Access)->SetPadding(FMargin(24.0f, 12.0f, 24.0f, 28.0f));
    PlaceOnReferenceCanvas(
        Canvas, FramePanel(StatusColumn, TEXT("StatusPanel"), FMargin(2.0f)),
        FVector2D(1340.0f, 164.0f), FVector2D(548.0f, 786.0f), 3);

    UHorizontalBox* FooterRow = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("FooterRow"));
    UTextBlock* Footer = CreateText(
        LOCTEXT("Footer", "SISTEMA GOTOVA  ·  NAIRGATsIYa AKTIVNA  ·  RUSSKIY"),
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
    TitleText = CreateText(LOCTEXT("HudTitle", "Soviet // OPERATsIYa"), 25.0f, SovietRed, TEXT("HudTitleText"));
    ResourceRow->AddChildToHorizontalBox(TitleText)->SetPadding(FMargin(18.0f, 11.0f));
    UTextBlock* Credits = CreateText(LOCTEXT("HudCredits", "KREDITY  12 450"), 18.0f, TextWhite, TEXT("HudCredits"));
    ResourceRow->AddChildToHorizontalBox(Credits)->SetPadding(FMargin(28.0f, 14.0f));
    UTextBlock* Power = CreateText(LOCTEXT("HudPower", "ENERGIYa  780 / 920"), 18.0f, TextWhite, TEXT("HudPower"));
    ResourceRow->AddChildToHorizontalBox(Power)->SetPadding(FMargin(20.0f, 14.0f));
    UTextBlock* Clock = CreateText(LOCTEXT("HudClock", "00:18:42"), 18.0f, TextWhite, TEXT("HudClock"));
    UHorizontalBoxSlot* ClockSlot = ResourceRow->AddChildToHorizontalBox(Clock);
    ClockSlot->SetPadding(FMargin(20.0f, 14.0f));
    ClockSlot->SetHorizontalAlignment(HAlign_Right);
    ClockSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
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
    UTextBlock* ProductionHeader = CreateText(LOCTEXT("ProductionHeader", "PROIZVODSTVO"), 22.0f, TextWhite, TEXT("ProductionHeader"));
    ProductionColumn->AddChildToVerticalBox(ProductionHeader)->SetPadding(FMargin(18.0f, 16.0f, 18.0f, 8.0f));
    UTextBlock* QueueOne = CreateText(LOCTEXT("QueueOne", "TESLA-TANK\nTyazhyolaya bronya  ·  ochered 01"), 16.0f, TextWhite, TEXT("QueueOne"));
    ProductionColumn->AddChildToVerticalBox(QueueOne)->SetPadding(FMargin(18.0f, 12.0f));
    ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ProductionProgress"));
    ProgressBar->SetPercent(0.72f);
    ProgressBar->SetFillColorAndOpacity(SovietRed);
    ProductionColumn->AddChildToVerticalBox(ProgressBar)->SetPadding(FMargin(18.0f, 0.0f, 18.0f, 18.0f));
    UTextBlock* QueueTwo = CreateText(LOCTEXT("QueueTwo", "PRIZYVNIK ×5\nBarracks  ·  ochered 02\n\nMIG «KUZNETs»\nAerodrom  ·  ozhidanie"), 16.0f, TextWhite, TEXT("QueueTwo"));
    UVerticalBoxSlot* QueueTwoSlot = ProductionColumn->AddChildToVerticalBox(QueueTwo);
    QueueTwoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    QueueTwoSlot->SetPadding(FMargin(18.0f, 8.0f));
    UTextBlock* Intel = CreateText(LOCTEXT("Intel", "RAZVEDKA // YuZhNYY MOST\nKONTAKTY: 12  ·  UGROZA: VYSOKAYa"), 14.0f, TextWhite, TEXT("Intel"));
    ProductionColumn->AddChildToVerticalBox(Intel)->SetPadding(FMargin(18.0f, 8.0f, 18.0f, 16.0f));
    USizeBox* ProductionSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ProductionSize"));
    ProductionSize->SetWidthOverride(330.0f);
    ProductionSize->SetContent(ProductionPanel);
    TacticalSpace->AddChildToHorizontalBox(ProductionSize);

    UBorder* CommandStrip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CommandStrip"));
    CommandStrip->SetBrushColor(FLinearColor(0.005f, 0.008f, 0.013f, 0.88f));
    CommandStrip->SetContent(CreateText(LOCTEXT("HudCommands", "OTRYaD: 3 × TESLA-TANK   ·   12 × PRIZYVNIK   ·   PRIKAZ: UDERZhIVAT POZITsIYu   ·   SILY SPETsNAZNAChENIYa: GOTOVY"), 15.0f, TextWhite, TEXT("HudCommands")));
    Frame->AddChildToVerticalBox(CommandStrip)->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
}

void URA4ShowcaseWidget::SetScreen(const int32 InScreen)
{
    ActiveScreen = InScreen;
    UE_LOG(LogTemp, Display, TEXT("RA4 UI showcase selected screen %d."), ActiveScreen);

    FLinearColor Accent = SovietRed;
    FText Heading = LOCTEXT("MainHeading", "GLAVNOE KOMANDOVANIE");
    FText Body = LOCTEXT("MainBody", "Vyberite napravlenie operatsii. Kampaniya, setevoy boy i sistemnye parametry dostupny iz zashchishchyonnogo komandnogo tsentra.");
    FText Status = LOCTEXT("MainStatus", "COMMANDER\nGotov k operatsii\n\nSOSTOYaNIE SETI\nStabilnoe\n\nTEATR VOYNY\nEvropa // 2049");
    float Progress = 1.0f;

    switch (ActiveScreen)
    {
    case 1:
        Accent = EasternGold;
        Heading = LOCTEXT("CampaignHeading", "VYBOR KAMPANII");
        Body = LOCTEXT("CampaignBody", "Soviet — krasno-chyornaya doktrina podavleniya. Alliance — sine-stalnaya mobilnost. Eastern Coalition — nefrit i zoloto. Chrono Legion — tekhnologii vne vremeni.");
        Status = LOCTEXT("CampaignStatus", "OPERATsIYa 01\nPepel stolitsy\n\nDIFFICULTY\nVeteran\n\nVYBRANA FACTION\nSoviet");
        Progress = 0.18f;
        break;
    case 2:
        Accent = SovietRed;
        Heading = LOCTEXT("HudHeading", "OPERATsIYa: PEPEL STOLITsY");
        Body = LOCTEXT("HudBody", "BOEVOY HUD Soviet\n\nKREDITY 12 450     ENERGIYa 780 / 920\n\nOChERED PROIZVODSTVA\nTesla Tank — 72%\nConscript ×5\n\nZADAChA: uderzhat platsdarm i unichtozhit komandnyy uzel Enemya.");
        Status = LOCTEXT("HudStatus", "MINI-MAP\nSektor: M-14\n\nVYDELENO\n3 × Tesla Tank\n12 × Conscript\n\nTREVOGA\nNizkaya");
        Progress = 0.72f;
        break;
    case 3:
        Accent = AlliesBlue;
        Heading = LOCTEXT("LobbyHeading", "SETEVOE LOBBI");
        Body = LOCTEXT("LobbyBody", "MAP: KRASNYY PEREVAL\nMODE: STANDARTNYY BOY\n\nCOMMANDER — Soviet — GOTOV\nADMIRAL WARD — ALYaNS — GOTOV\nGENERAL GAO — VOSTOChNAYa KOALITsIYa — OZhIDANIE\nCHRONOS-07 — KhRONOLEGION — OZhIDANIE");
        Status = LOCTEXT("LobbyStatus", "ChAT KOMANDOVANIYa\n[20:49] Vard: Gotov.\n[20:49] Gao: Proveryayu svyaz.\n\nPING\n34 ms");
        Progress = 0.50f;
        break;
    case 4:
        Accent = SovietRed;
        Heading = LOCTEXT("SettingsHeading", "NASTROYKI SISTEMY");
        Body = LOCTEXT("SettingsBody", "IZOBRAZhENIE\nRazreshenie: 1920×1080\nMashq interfeysa: avtomaticheski\n\nZVUK\nMuzyka: 80%\nEffekty: 90%\n\nUPRAVLENIE\nEnhanced Input: aktivno");
        Status = LOCTEXT("SettingsStatus", "PROFIL\nKomanduyushchiy\n\nLOKALIZATsIYa\nRusskiy\n\nVERSIYa\n0.1.0");
        Progress = 0.86f;
        break;
    case 5:
        Accent = SovietRed;
        Heading = LOCTEXT("SplashHeading", "RED ALERT 4");
        Body = LOCTEXT("SplashBody", "SISTEMA KOMANDOVANIYa I SVYaZI\n\nNazhmite lyubuyu klavishu, chtoby voyti v zashchishchyonnuyu set.");
        Status = LOCTEXT("SplashStatus", "STATUS SISTEM\nVse kontury gotovy\n\nSEANS\nZashifrovan");
        Progress = 0.08f;
        break;
    case 6:
        Accent = EasternGold;
        Heading = LOCTEXT("FactionHeading", "VYBOR FRAKTsII");
        Body = LOCTEXT("FactionBody", "Soviet — tyazhyolaya bronya i podavlenie.\nALYaNS — mobilnost i tochnye udary.\nVOSTOChNAYa KOALITsIYa — massovoe proizvodstvo.\nKhRONOLEGION — tekhnologii vne vremeni.");
        Status = LOCTEXT("FactionStatus", "VYBRANO\nSoviet\n\nDOKTRINA\nShturm\n\nVEHNOLOGII\nUroven 1");
        Progress = 0.25f;
        break;
    case 7:
        Accent = AlliesBlue;
        Heading = LOCTEXT("AlliesCampaignHeading", "KAMPANIYa ALYaNSA");
        Body = LOCTEXT("AlliesCampaignBody", "Operatsiya «Severnyy shchit». Sderzhite nastuplenie Soviet, vernite kontrol nad portami i otkroyte marshrut dlya navala.");
        Status = LOCTEXT("AlliesCampaignStatus", "COMMANDER\nAdmiral Vard\n\nTEATR\nSevernaya Atlantika\n\nMISSIYa\n01 / 09");
        Progress = 0.12f;
        break;
    case 8:
        Accent = EasternGold;
        Heading = LOCTEXT("EasternCampaignHeading", "KAMPANIYa VOSTOChNOY KOALITsII");
        Body = LOCTEXT("EasternCampaignBody", "Operatsiya «Zolotoy rassvet». Zakhvatite energeticheskie uzly i prevratite vrazheskiy platsdarm v opornuyu bazu koalitsii.");
        Status = LOCTEXT("EasternCampaignStatus", "COMMANDER\nGeneral Gao\n\nTEATR\nTikhookeanskiy poyas\n\nMISSIYa\n01 / 08");
        Progress = 0.16f;
        break;
    case 9:
        Accent = ChronoViolet;
        Heading = LOCTEXT("ChronoCampaignHeading", "KAMPANIYa KhRONOLEGIONA");
        Body = LOCTEXT("ChronoCampaignBody", "Operatsiya «Paradoks». Vosstanovite vremennoy koridor do togo, kak Enemy zakrepitsya v klyuchevykh tochkakh khronolinii.");
        Status = LOCTEXT("ChronoCampaignStatus", "KOORDINATOR\nKhronos-07\n\nVREMENNOY SLOY\nNestabilen\n\nMISSIYa\n01 / 07");
        Progress = 0.09f;
        break;
    case 10:
        Accent = SovietRed;
        Heading = LOCTEXT("MissionMapHeading", "MAP OPERATsIY: EVROPA");
        Body = LOCTEXT("MissionMapBody", "MISSIYa 01 — PEPEL STOLITsY\nUderzhite most cherez Reyn.\n\nMISSIYa 02 — ZhELEZNYY KORIDOR\nPererezhte liniyu snabzheniya Alliancea.\n\nMISSIYa 03 — POLYaRNAYa NOCh\nZakhvatite stantsiyu rannego preduprezhdeniya.");
        Status = LOCTEXT("MissionMapStatus", "VYBRANNAYa MISSIYa\nPepel stolitsy\n\nDIFFICULTY\nVeteran\n\nNAGRADA\nTesla Tank");
        Progress = 0.33f;
        break;
    case 11:
        Accent = SovietRed;
        Heading = LOCTEXT("BriefingHeading", "BRIFING: PEPEL STOLITsY");
        Body = LOCTEXT("BriefingBody", "Voyska Alliancea uderzhivayut gorodskoy komandnyy uzel. Razvernite bazu na vostochnom beregu, obespechte energosnabzhenie i unichtozhte uzel do pribytiya podkrepleniya.");
        Status = LOCTEXT("BriefingStatus", "OSNOVNAYa TsEL\nUnichtozhit komandnyy uzel\n\nDOPOLNITELNAYa\nSokhranit most\n\nVREMYa\n06:30");
        Progress = 0.44f;
        break;
    case 12:
        Accent = AlliesBlue;
        Heading = LOCTEXT("VideoHeading", "ZAShchIShchYoNNYY VIDEOKANAL");
        Body = LOCTEXT("VideoBody", "ADMIRAL WARD:\n«Komanduyushchiy, skanery fiksiruyut dvizhenie bronekolonn. Ne davayte im zakrepitsya u mosta. Vozdushnyy koridor budet otkryt na tri minuty.»");
        Status = LOCTEXT("VideoStatus", "KANAL\nVarshava-01\n\nShIFROVANIE\nAktivno\n\nZADERZhKA\n18 ms");
        Progress = 0.61f;
        break;
    case 13:
        Accent = SovietRed;
        Heading = LOCTEXT("LoadingHeading", "PODGOTOVKA OPERATsII");
        Body = LOCTEXT("LoadingBody", "Zagruzka teatra boevykh deystviy. Synchronization dannykh komandovaniya, marshrutov snabzheniya i takticheskoy razvedki.");
        Status = LOCTEXT("LoadingStatus", "MAP\nPepel stolitsy\n\nFACTION\nSoviet\n\nSOSTOYaNIE\nSynchronization");
        Progress = 0.72f;
        break;
    case 14:
        Accent = AlliesBlue;
        Heading = LOCTEXT("AlliesHudHeading", "OPERATsIYa: SEVERNYY ShchIT");
        Body = LOCTEXT("AlliesHudBody", "BOEVOY HUD ALYaNSA\n\nKREDITY 10 280     ENERGIYa 640 / 760\n\nOChERED PROIZVODSTVA\nStrazh — 54%\nAviakrylo «Kopyo» ×2\n\nZADAChA: uderzhat gavan i zashchitit konvoy.");
        Status = LOCTEXT("AlliesHudStatus", "MINI-MAP\nSektor: N-07\n\nVYDELENO\n2 × Strazh\n4 × Raketchik\n\nNAVAL\nGotov");
        Progress = 0.54f;
        break;
    case 15:
        Accent = EasternGold;
        Heading = LOCTEXT("EasternHudHeading", "OPERATsIYa: ZOLOTOY RASSVET");
        Body = LOCTEXT("EasternHudBody", "BOEVOY HUD VOSTOChNOY KOALITsII\n\nKREDITY 15 600     ENERGIYa 850 / 940\n\nOChERED PROIZVODSTVA\nShturmovoy mekh — 43%\nDron «Khuan» ×3\n\nZADAChA: uderzhat energeticheskie uzly.");
        Status = LOCTEXT("EasternHudStatus", "MINI-MAP\nSektor: P-21\n\nVYDELENO\n1 × Shturmovoy mekh\n8 × Infantry\n\nREZERV\nGotov");
        Progress = 0.43f;
        break;
    case 16:
        Accent = ChronoViolet;
        Heading = LOCTEXT("ChronoHudHeading", "OPERATsIYa: PARADOKS");
        Body = LOCTEXT("ChronoHudBody", "BOEVOY HUD KhRONOLEGIONA\n\nKREDITY 13 040     KhRONOENERGIYa 72 / 100\n\nOChERED PROIZVODSTVA\nTemporal strazh — 68%\nRazvedchik «Ekho» ×2\n\nZADAChA: stabilizirovat vremennoy yakor.");
        Status = LOCTEXT("ChronoHudStatus", "KhRONO-MAP\nSloy: 3-A\n\nVYDELENO\n2 × Strazh\n1 × Yakor\n\nRAZRYV\nSderzhan");
        Progress = 0.68f;
        break;
    case 17:
        Accent = SovietRed;
        Heading = LOCTEXT("PauseHeading", "PAUSED");
        Body = LOCTEXT("PauseBody", "Operatsiya priostanovlena. Prodolzhite boy, otkroyte nastroyki, sokhranite progress ili vernites v komandnyy tsentr.");
        Status = LOCTEXT("PauseStatus", "OPERATsIYa\nPepel stolitsy\n\nVREMYa BOYa\n18:42\n\nSOKhRANENIE\nAvtomaticheskoe");
        Progress = 0.50f;
        break;
    case 18:
        Accent = SovietRed;
        Heading = LOCTEXT("VictoryHeading", "VICTORY");
        Body = LOCTEXT("VictoryBody", "Komandnyy uzel Enemya unichtozhen. Most i platsdarm uderzhany. Teatr operatsiy otkryt dlya sleduyushchey fazy nastupleniya.");
        Status = LOCTEXT("VictoryStatus", "REZULTAT\nVictory Soviet\n\nVREMYa\n24:18\n\nPOTERI\nPriemlemye");
        Progress = 1.0f;
        break;
    case 19:
        Accent = AlliesBlue;
        Heading = LOCTEXT("EncyclopediaHeading", "ENTsIKLOPEDIYa VOYNY");
        Body = LOCTEXT("EncyclopediaBody", "TESLA-TANK\nTyazhyolaya bronirovannaya edinitsa Soviet s tsepnoy elektricheskoy pushkoy.\n\nSTRAZh\nUniversalnaya boevaya mashina Alliancea s modulnoy zashchitoy.\n\nTEMPORALNYY STRAZh\nPeredovaya edinitsa Chrono Legiona.");
        Status = LOCTEXT("EncyclopediaStatus", "KATEGORIYa\nBoevye edinitsy\n\nZAPISEY\n128\n\nFILTR\nVse fraktsii");
        Progress = 0.40f;
        break;
    case 20:
        Accent = EasternGold;
        Heading = LOCTEXT("TechTreeHeading", "VEHNOLOGIChESKOE DEREVO");
        Body = LOCTEXT("TechTreeBody", "KOMANDNYY TsENTR → BARAKI → VOENNYY FACTORY\n\nVOENNYY FACTORY → TESLA-LABORATORIYa → TESLA-TANK\n\nELEKTROSTANTsIYa → RADAR → SPUTNIKOVAYa SVYaZ");
        Status = LOCTEXT("TechTreeStatus", "FACTION\nSoviet\n\nOTKRYTO\n18 / 46\n\nOChKI\n3");
        Progress = 0.39f;
        break;
    case 21:
        Accent = ChronoViolet;
        Heading = LOCTEXT("ModsHeading", "MODIFIKATsII");
        Body = LOCTEXT("ModsBody", "RASShIRENNYE KARTY — vklyucheno\nNovye stsenarii i tablitsy balansa.\n\nTAKTIChESKIE PORTRETY — vklyucheno\nDopolnitelnye portrety komanduyushchikh.\n\nEKSPERIMENTALNYY BALANS — vyklyucheno");
        Status = LOCTEXT("ModsStatus", "AKTIVNYKh MODOV\n2\n\nSOVMESTIMOST\nProverena\n\nPEREZAPUSK\nNe trebuetsya");
        Progress = 0.67f;
        break;
    case 22:
        Accent = EasternGold;
        Heading = LOCTEXT("EasternDetailHeading", "VOSTOChNAYa KOALITsIYa: GENERAL GAO");
        Body = LOCTEXT("EasternDetailBody", "Gao stroit voynu na distsipline, tempe proizvodstva i kontrole uzlov snabzheniya. Ego hq vedyot nastuplenie ot tikhookeanskogo poberezhya k promyshlennomu serdtsu Evropy.");
        Status = LOCTEXT("EasternDetailStatus", "DOKTRINA\nNefritovyy molot\n\nBONUS\nUskorennoe proizvodstvo\n\nKAMPANIYa\n01 / 08");
        Progress = 0.20f;
        break;
    case 23:
        Accent = SovietRed;
        Heading = LOCTEXT("BattleHudHeading", "TREVOGA: BRONEKOLONNA");
        Body = LOCTEXT("BattleHudBody", "KREDITY 18 300     ENERGIYa 1020 / 1180\n\nPROIZVODSTVO\nApokalipsis — 89%\nMiG «Kuznets» — 31%\n\nTAKTIChESKOE SOBYTIE\nVrag atakuet yuzhnyy most. Aktiviruyte zagraditelnyy ogon.");
        Status = LOCTEXT("BattleHudStatus", "MINI-MAP\nYug: krasnyy kontakt\n\nVYDELENO\n4 × Tesla Tank\n\nUGROZA\nVysokaya");
        Progress = 0.89f;
        break;
    case 24:
        Accent = SovietRed;
        Heading = LOCTEXT("AlertHudHeading", "PREDUPREZhDENIE: AIRANALYoT");
        Body = LOCTEXT("AlertHudBody", "Obnaruzheny samolyoty Alliancea. Razvernite Anti-Air, peremestite mobilnye generatory i zashchitite Tesla-laboratoriyu.");
        Status = LOCTEXT("AlertHudStatus", "KONTAKTOV\n12\n\nDO PODLYoTA\n00:32\n\nAnti-Air\n4 batarei");
        Progress = 0.32f;
        break;
    case 25:
        Accent = AlliesBlue;
        Heading = LOCTEXT("NavalHudHeading", "MORSKAYa OPERATsIYa: LEDYaNOY PROLIV");
        Body = LOCTEXT("NavalHudBody", "KREDITY 16 800     ENERGIYa 720 / 840\n\nNAVAL\nAvianosets «Svoboda» — gotov\nEsminets ×3\n\nZADAChA: provesti konvoy cherez proliv i podavit beregovye batarei.");
        Status = LOCTEXT("NavalHudStatus", "MORSKAYa MAP\nKvadrat D-4\n\nKONVOY\n5 / 6 korabley\n\nPOGODA\nShtorm");
        Progress = 0.58f;
        break;
    case 26:
        Accent = AlliesBlue;
        Heading = LOCTEXT("AirHudHeading", "VOZDUShNAYa OPERATsIYa: ChISTOE NEBO");
        Body = LOCTEXT("AirHudBody", "KREDITY 11 900     ENERGIYa 680 / 820\n\nAIRAKRYLO\nIstrebitel «Kopyo» ×6\nBombardirovshchik «Grom» ×2\n\nZADAChA: unichtozhit radar Enemya i uderzhat vozdushnyy koridor.");
        Status = LOCTEXT("AirHudStatus", "VOZDUShNAYa MAP\nVysota 8 000 m\n\nTOPLIVO\n76%\n\nKONTAKTY\n3 eskadrili");
        Progress = 0.76f;
        break;
    case 27:
        Accent = ChronoViolet;
        Heading = LOCTEXT("SuperweaponHudHeading", "KhRONO-ORUZhIE: GOTOVNOST 94%");
        Body = LOCTEXT("SuperweaponHudBody", "KhRONOENERGIYa 94 / 100\n\nVREMENNOY YaKOR\nStabilen\n\nVYBERITE ZONU NAZNAChENIYa\nPosle aktivatsii tsel budet vyvedena iz tekushchey vremennoy linii na 12 sekund.");
        Status = LOCTEXT("SuperweaponHudStatus", "RADIUS\n420 m\n\nPEREZARYaDKA\n00:18\n\nRISK PARADOKSA\nUmerennyy");
        Progress = 0.94f;
        break;
    default:
        break;
    }

    TitleText->SetColorAndOpacity(FSlateColor(Accent));
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
    AccentPanel->SetBrushColor(ActiveScreen == 4
        ? FLinearColor(1.0f, 1.0f, 1.0f, 0.96f)
        : FLinearColor(Accent.R * 0.09f, Accent.G * 0.09f, Accent.B * 0.09f, 0.78f));
    SubtitleText->SetText(Heading);
    ContentText->SetText(Body);
    StatusText->SetText(Status);
    ProgressBar->SetPercent(Progress);
    ProgressBar->SetFillColorAndOpacity(Accent);
}

void URA4ShowcaseWidget::OpenMainMenu()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4CommandCentreMenuWidget* MainMenu = CreateWidget<URA4CommandCentreMenuWidget>(
            PlayerController, URA4CommandCentreMenuWidget::StaticClass()))
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
