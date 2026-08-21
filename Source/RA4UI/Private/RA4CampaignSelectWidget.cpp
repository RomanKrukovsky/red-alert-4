// Copyright (c) Red Alert 4 project.

#include "RA4CampaignSelectWidget.h"

#include "RA4MainMenuScreenWidget.h"
#include "RA4CampaignScreenWidget.h"
#include "RA4CampaignViewModel.h"
#include "RA4FactionData.h"
#include "RA4ShowcaseWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
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
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "RA4CampaignSelect"

namespace
{
const FVector2D ReferenceSize(1920.0f, 1080.0f);
constexpr FLinearColor TextPrimary(0.92f, 0.90f, 0.88f, 1.0f);
constexpr FLinearColor TextMuted(0.55f, 0.53f, 0.51f, 1.0f);
constexpr FLinearColor TextHighlight(1.0f, 1.0f, 1.0f, 1.0f);
constexpr FLinearColor PanelDark(0.012f, 0.014f, 0.018f, 0.95f);
constexpr FLinearColor PanelMetal(0.12f, 0.13f, 0.15f, 0.98f);
constexpr FLinearColor ScarletHorizon(0.95f, 0.12f, 0.16f, 1.0f);

UTextBlock* MakeText(
    UWidgetTree* Tree,
    const FText& Value,
    const int32 Size,
    const FLinearColor& Color,
    const FName Name,
    const bool bEmphasis = true,
    const bool bWrap = true)
{
    UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    Label->SetText(Value);
    Label->SetColorAndOpacity(FSlateColor(Color));
    const TCHAR* FontPath = bEmphasis
        ? TEXT("/Game/RA4UI/Fonts/RA4_RobotoCondensedSemiBold_Font.RA4_RobotoCondensedSemiBold_Font")
        : TEXT("/Game/RA4UI/Fonts/RA4_RobotoCondensedRegular_Font.RA4_RobotoCondensedRegular_Font");
    if (UObject* Font = LoadObject<UObject>(nullptr, FontPath))
    {
        FSlateFontInfo FontInfo(Font, Size);
        FontInfo.LetterSpacing = bEmphasis ? 40 : 12;
        Label->SetFont(FontInfo);
    }
    Label->SetShadowOffset(FVector2D(1.5f, 1.5f));
    Label->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
    // Narrow cards used to let long Russian titles spill over their neighbour.
    Label->SetAutoWrapText(bWrap);
    Label->SetClipping(EWidgetClipping::Inherit);
    return Label;
}

void Place(
    UCanvasPanel* Canvas,
    UWidget* Widget,
    const FVector2D Position,
    const FVector2D Size,
    const int32 ZOrder = 0)
{
    UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
    Slot->SetPosition(Position);
    Slot->SetSize(Size);
    Slot->SetAnchors(FAnchors(0.0f, 0.0f));
    Slot->SetAlignment(FVector2D::ZeroVector);
    Slot->SetZOrder(ZOrder);
}

UBorder* MakePanel(
    UWidgetTree* Tree,
    UWidget* Content,
    const FName Name,
    const FLinearColor& Accent,
    const FMargin Padding = FMargin(2.0f),
    const FMargin Rail = FMargin(1.5f))
{
    UBorder* Metal = Tree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), FName(Name.ToString() + TEXT("_Metal")));
    Metal->SetBrushColor(PanelMetal);
    // Rail thickness varies per direction, so the silhouette identifies the
    // bloc before its colour does.
    Metal->SetPadding(Rail);

    UBorder* Edge = Tree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), FName(Name.ToString() + TEXT("_Edge")));
    Edge->SetBrushColor(FLinearColor(Accent.R * 0.65f, Accent.G * 0.65f, Accent.B * 0.65f, 0.95f));
    Edge->SetPadding(FMargin(1.5f));
    Metal->SetContent(Edge);

    UBorder* Interior = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
    Interior->SetBrushColor(PanelDark);
    Interior->SetPadding(Padding);
    Interior->SetContent(Content);
    // A panel is a hard boundary: content that does not fit is cut, never drawn
    // over the neighbouring panel.
    Interior->SetClipping(EWidgetClipping::ClipToBounds);
    Edge->SetContent(Interior);
    return Metal;
}

FButtonStyle MakeCardButtonStyle(const FLinearColor& Accent)
{
    FButtonStyle Style;
    Style.SetNormal(FSlateColorBrush(FLinearColor(0.008f, 0.010f, 0.014f, 0.95f)));
    Style.SetHovered(FSlateColorBrush(FLinearColor(
        Accent.R * 0.16f,
        Accent.G * 0.16f,
        Accent.B * 0.16f,
        1.0f)));
    Style.SetPressed(FSlateColorBrush(FLinearColor(
        Accent.R * 0.45f,
        Accent.G * 0.45f,
        Accent.B * 0.45f,
        1.0f)));
    Style.SetDisabled(FSlateColorBrush(FLinearColor(0.005f, 0.005f, 0.008f, 0.65f)));
    return Style;
}

/** Interior padding for a direction, reusing the shared panel-role scale. */
FMargin DensityPadding(const ERA4PanelRole Role)
{
    switch (Role)
    {
    case ERA4PanelRole::Compact:  return FMargin(8.0f);
    case ERA4PanelRole::DenseHUD: return FMargin(10.0f);
    case ERA4PanelRole::Hero:     return FMargin(24.0f);
    default:                      return FMargin(16.0f);
    }
}

/** Fill slot carrying an explicit weight; FSlateChildSize sets Value separately. */
FSlateChildSize FillWeight(const float Weight)
{
    FSlateChildSize Size(ESlateSizeRule::Fill);
    Size.Value = Weight;
    return Size;
}
} // namespace

TSharedRef<SWidget> URA4CampaignSelectWidget::RebuildWidget()
{
    if (WidgetTree)
    {
        BuildLayout();
    }
    return Super::RebuildWidget();
}

void URA4CampaignSelectWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!CampaignViewModel)
    {
        CampaignViewModel = NewObject<URA4CampaignViewModel>(this);
    }
    EntranceElapsed = 0.0f;
    if (MainCanvas)
    {
        MainCanvas->SetRenderOpacity(0.0f);
        MainCanvas->SetRenderScale(FVector2D(1.02f, 1.02f));
    }
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            EntranceTimer, this, &URA4CampaignSelectWidget::AnimateEntrance, 1.0f / 60.0f, true);
    }

    SelectedBlocIndex = 0;
    SelectedCountryIndex = 0;
    SelectedDoctrineIndex = 0;
    CurrentStep = ERA4CampaignSelectStep::BlocSelection;

    if (DossierFrameWidget)
    {
        DossierFrameWidget->SetVisibility(ESlateVisibility::Collapsed);
    }

    RefreshBreadcrumbs();
    RefreshBlocCards();
    RefreshCountryCards();
    RefreshDoctrineCards();
    RefreshDossierPanel();
}

void URA4CampaignSelectWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(EntranceTimer);
    }
    Super::NativeDestruct();
}

void URA4CampaignSelectWidget::AnimateEntrance()
{
    EntranceElapsed += 1.0f / 60.0f;
    const float Alpha = FMath::Clamp(EntranceElapsed / 0.35f, 0.0f, 1.0f);
    const float Eased = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);
    if (MainCanvas)
    {
        MainCanvas->SetRenderOpacity(Eased);
        MainCanvas->SetRenderScale(FVector2D(FMath::Lerp(1.02f, 1.0f, Eased)));
    }
    if (Alpha >= 1.0f)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(EntranceTimer);
        }
    }
}

void URA4CampaignSelectWidget::BuildLayout()
{
    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CampaignRoot"));
    WidgetTree->RootWidget = Root;

    UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CampaignBackground"));
    if (UTexture2D* BackgroundTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_USSR_CommandCenter.T_RA4_USSR_CommandCenter")))
    {
        Background->SetBrushFromTexture(BackgroundTexture, false);
    }
    Background->SetColorAndOpacity(FLinearColor(0.20f, 0.22f, 0.25f, 1.0f));
    UOverlaySlot* BackgroundSlot = Root->AddChildToOverlay(Background);
    BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
    BackgroundSlot->SetVerticalAlignment(VAlign_Fill);

    UBorder* Shade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CampaignShade"));
    Shade->SetBrushColor(FLinearColor(0.005f, 0.007f, 0.012f, 0.72f));
    UOverlaySlot* ShadeSlot = Root->AddChildToOverlay(Shade);
    ShadeSlot->SetHorizontalAlignment(HAlign_Fill);
    ShadeSlot->SetVerticalAlignment(VAlign_Fill);

    UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("CampaignScale"));
    Scale->SetStretch(EStretch::ScaleToFit);
    Scale->SetStretchDirection(EStretchDirection::Both);
    UOverlaySlot* ScaleSlot = Root->AddChildToOverlay(Scale);
    ScaleSlot->SetHorizontalAlignment(HAlign_Fill);
    ScaleSlot->SetVerticalAlignment(VAlign_Fill);

    USizeBox* Frame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CampaignReferenceFrame"));
    Frame->SetWidthOverride(ReferenceSize.X);
    Frame->SetHeightOverride(ReferenceSize.Y);
    Scale->SetContent(Frame);

    MainCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CampaignCanvas"));
    Frame->SetContent(MainCanvas);

    // Global Scarlet Horizon thin line
    UBorder* HorizonLine = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HorizonLine"));
    HorizonLine->SetBrushColor(ScarletHorizon);
    Place(MainCanvas, HorizonLine, FVector2D(0.0f, 0.0f), FVector2D(1920.0f, 3.0f), 10);

    // Top Brand Logo / Title
    UImage* Logo = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CampaignLogo"));
    if (UTexture2D* LogoTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_Logo.T_RA4_Logo")))
    {
        Logo->SetBrushFromTexture(LogoTexture, false);
    }
    Place(MainCanvas, Logo, FVector2D(35.0f, 12.0f), FVector2D(360.0f, 75.0f), 4);

    // Top Navigation Bar
    UHorizontalBox* Navigation = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CampaignNavigation"));
    const auto AddNav = [this, Navigation](
        const FText& Label,
        const FName Name,
        const bool bActive) -> UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
        FButtonStyle Style;
        Style.SetNormal(FSlateColorBrush(bActive
            ? FLinearColor(0.20f, 0.08f, 0.32f, 0.96f)
            : FLinearColor(0.015f, 0.018f, 0.024f, 0.85f)));
        Style.SetHovered(FSlateColorBrush(FLinearColor(0.35f, 0.15f, 0.50f, 1.0f)));
        Style.SetPressed(FSlateColorBrush(ScarletHorizon));
        Style.SetDisabled(FSlateColorBrush(FLinearColor(0.18f, 0.07f, 0.28f, 0.95f)));
        Button->SetStyle(Style);
        UTextBlock* LabelText = MakeText(
            WidgetTree, Label, 14, bActive ? TextHighlight : TextMuted,
            FName(Name.ToString() + TEXT("_Label")), true, false);
        LabelText->SetJustification(ETextJustify::Center);
        LabelText->SetMinDesiredWidth(0.0f);
        Button->AddChild(LabelText);
        Button->SetClipping(EWidgetClipping::ClipToBounds);
        UHorizontalBoxSlot* Slot = Navigation->AddChildToHorizontalBox(Button);
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        Slot->SetPadding(FMargin(9.0f, 6.0f));
        Slot->SetHorizontalAlignment(HAlign_Center);
        return Button;
    };

    AddNav(LOCTEXT("MainNav", "ГЛАВНАЯ"), TEXT("MainNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenMainMenu);
    UButton* CampaignNav = AddNav(
        LOCTEXT("CampaignNav", "КАМПАНИЯ"), TEXT("CampaignNavButton"), true);
    CampaignNav->SetIsEnabled(false);
    AddNav(LOCTEXT("NetworkNav", "СЕТЕВАЯ ИГРА"), TEXT("NetworkNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenMultiplayer);
    AddNav(LOCTEXT("ChallengesNav", "ИСПЫТАНИЯ"), TEXT("ChallengesNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenChallenges);
    AddNav(LOCTEXT("BarracksNav", "АРХИВ"), TEXT("BarracksNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenBarracks);
    AddNav(LOCTEXT("SettingsNav", "ПАРАМЕТРЫ"), TEXT("SettingsNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenSettings);
    UWidget* NavigationFrame = MakePanel(
        WidgetTree, Navigation, TEXT("CampaignNavigationFrame"),
        FLinearColor(0.68f, 0.28f, 0.88f, 1.0f), FMargin(3.0f));
    NavigationFrame->SetVisibility(ESlateVisibility::Collapsed);
    Place(MainCanvas, NavigationFrame, FVector2D(420.0f, 84.0f), FVector2D(1040.0f, 54.0f), 5);

    // Profile & Network status
    UVerticalBox* Profile = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CampaignProfile"));
    Profile->AddChildToVerticalBox(MakeText(
        WidgetTree, LOCTEXT("ProfileHeader", "ШТАБ КОМАНДОВАНИЯ"), 13, ScarletHorizon, TEXT("ProfileHeader")));
    Profile->AddChildToVerticalBox(MakeText(
        WidgetTree, LOCTEXT("ProfileStatus", "СВЯЗЬ: АКТИВНА // ДОСТУП УР. 5"), 11, TextMuted, TEXT("ProfileStatus"), false));
    Place(
        MainCanvas,
        MakePanel(WidgetTree, Profile, TEXT("CampaignProfileFrame"), ScarletHorizon, FMargin(12.0f, 6.0f)),
        FVector2D(1480.0f, 15.0f), FVector2D(405.0f, 62.0f), 5);

    // ==========================================
    // BREADCRUMBS BAR (Step 1 -> Step 2 -> Step 3)
    // ==========================================
    UHorizontalBox* BreadcrumbsBox = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("BreadcrumbsBox"));

    BreadcrumbBlocBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BreadcrumbBlocBtn"));
    BreadcrumbBlocBtn->SetStyle(MakeCardButtonStyle(FLinearColor(0.68f, 0.28f, 0.88f, 1.0f)));
    BreadcrumbBlocText = MakeText(WidgetTree, LOCTEXT("BC1", "01 БЛОК / КАТЕГОРИЯ"), 14, TextHighlight, TEXT("BCText1"), true, false);
    BreadcrumbBlocBtn->AddChild(BreadcrumbBlocText);
    BreadcrumbBlocBtn->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::GotoBlocStep);
    BreadcrumbsBox->AddChildToHorizontalBox(BreadcrumbBlocBtn)->SetPadding(FMargin(4.0f, 2.0f));

    UTextBlock* Arrow1 = MakeText(WidgetTree, LOCTEXT("Arrow1", "›"), 16, TextMuted, TEXT("Arrow1"), true, false);
    BreadcrumbsBox->AddChildToHorizontalBox(Arrow1)->SetPadding(FMargin(6.0f, 4.0f));

    BreadcrumbCountryBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BreadcrumbCountryBtn"));
    BreadcrumbCountryBtn->SetStyle(MakeCardButtonStyle(FLinearColor(0.35f, 0.70f, 0.98f, 1.0f)));
    BreadcrumbCountryText = MakeText(WidgetTree, LOCTEXT("BC2", "02 СТРАНА"), 14, TextMuted, TEXT("BCText2"), true, false);
    BreadcrumbCountryBtn->AddChild(BreadcrumbCountryText);
    BreadcrumbCountryBtn->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::GotoCountryStep);
    BreadcrumbsBox->AddChildToHorizontalBox(BreadcrumbCountryBtn)->SetPadding(FMargin(4.0f, 2.0f));

    UTextBlock* Arrow2 = MakeText(WidgetTree, LOCTEXT("Arrow2", "›"), 16, TextMuted, TEXT("Arrow2"), true, false);
    BreadcrumbsBox->AddChildToHorizontalBox(Arrow2)->SetPadding(FMargin(6.0f, 4.0f));

    BreadcrumbDoctrineBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BreadcrumbDoctrineBtn"));
    BreadcrumbDoctrineBtn->SetStyle(MakeCardButtonStyle(FLinearColor(0.88f, 0.72f, 0.22f, 1.0f)));
    BreadcrumbDoctrineText = MakeText(WidgetTree, LOCTEXT("BC3", "03 ДОКТРИНА"), 14, TextMuted, TEXT("BCText3"), true, false);
    BreadcrumbDoctrineBtn->AddChild(BreadcrumbDoctrineText);
    BreadcrumbDoctrineBtn->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::GotoDoctrineStep);
    BreadcrumbsBox->AddChildToHorizontalBox(BreadcrumbDoctrineBtn)->SetPadding(FMargin(4.0f, 2.0f));

    Place(
        MainCanvas,
        MakePanel(WidgetTree, BreadcrumbsBox, TEXT("BreadcrumbsFrame"), FLinearColor(0.40f, 0.45f, 0.55f, 1.0f), FMargin(6.0f, 4.0f)),
        FVector2D(535.0f, 16.0f), FVector2D(760.0f, 60.0f), 7);

    // ==========================================
    // STEP 1 CONTAINER: 5 BLOC CARDS
    // ==========================================
    BlocCardsContainer = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("BlocCardsContainer"));
    Place(MainCanvas, BlocCardsContainer, FVector2D(0.0f, 0.0f), ReferenceSize, 5);

    // ==========================================
    // STEP 2 CONTAINER: COUNTRY CARDS
    // ==========================================
    CountryCardsContainer = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CountryCardsContainer"));
    CountryCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    Place(MainCanvas, CountryCardsContainer, FVector2D(35.0f, 160.0f), FVector2D(1320.0f, 780.0f), 5);

    // ==========================================
    // STEP 3 CONTAINER: DOCTRINE CARDS
    // ==========================================
    DoctrineCardsContainer = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("DoctrineCardsContainer"));
    DoctrineCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    Place(MainCanvas, DoctrineCardsContainer, FVector2D(35.0f, 160.0f), FVector2D(1320.0f, 780.0f), 5);

    // ==========================================
    // RIGHT COLUMN: HERO DOSSIER & INTEL PANEL
    // ==========================================
    UVerticalBox* DossierBox = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("DossierBox"));

    DossierHeaderTag = MakeText(
        WidgetTree, LOCTEXT("DossierTag", "ОПЕРАТИВНОЕ ДОСЬЕ"), 13, ScarletHorizon, TEXT("DossierHeaderTag"));
    DossierBox->AddChildToVerticalBox(DossierHeaderTag)->SetPadding(FMargin(12.0f, 8.0f, 12.0f, 4.0f));

    DossierTitleText = MakeText(
        WidgetTree, FText::GetEmpty(), 24, TextHighlight, TEXT("DossierTitleText"));
    DossierBox->AddChildToVerticalBox(DossierTitleText)->SetPadding(FMargin(12.0f, 2.0f, 12.0f, 2.0f));

    DossierSubtitleText = MakeText(
        WidgetTree, FText::GetEmpty(), 12, TextMuted, TEXT("DossierSubtitleText"), false);
    DossierBox->AddChildToVerticalBox(DossierSubtitleText)->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 10.0f));

    DossierSpecializationText = MakeText(
        WidgetTree, FText::GetEmpty(), 13, FLinearColor(0.88f, 0.72f, 0.22f, 1.0f), TEXT("DossierSpecText"));
    DossierBox->AddChildToVerticalBox(DossierSpecializationText)->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 8.0f));

    DossierDescriptionText = MakeText(
        WidgetTree, FText::GetEmpty(), 13, TextPrimary, TEXT("DossierDescText"), false);
    DossierDescriptionText->SetAutoWrapText(true);
    UVerticalBoxSlot* DescSlot = DossierBox->AddChildToVerticalBox(DossierDescriptionText);
    DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    DescSlot->SetPadding(FMargin(12.0f, 4.0f, 12.0f, 12.0f));

    // Ratings Section
    UTextBlock* RatingsHeader = MakeText(
        WidgetTree, LOCTEXT("RatingsHeader", "БОЕВЫЕ ХАРАКТЕРИСТИКИ"), 13, TextMuted, TEXT("RatingsHeader"));
    DossierBox->AddChildToVerticalBox(RatingsHeader)->SetPadding(FMargin(12.0f, 6.0f, 12.0f, 4.0f));

    const auto AddRatingBar = [this, DossierBox](
        const FText& Label,
        TObjectPtr<UProgressBar>& TargetBar,
        const FLinearColor& BarColor,
        const FName BarName)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        UTextBlock* RowLabel = MakeText(WidgetTree, Label, 12, TextPrimary, FName(BarName.ToString() + TEXT("_Lbl")), false);
        RowLabel->SetMinDesiredWidth(110.0f);
        Row->AddChildToHorizontalBox(RowLabel)->SetPadding(FMargin(0.0f, 2.0f));

        TargetBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), BarName);
        TargetBar->SetFillColorAndOpacity(BarColor);
        TargetBar->SetPercent(0.85f);
        UHorizontalBoxSlot* BarSlot = Row->AddChildToHorizontalBox(TargetBar);
        BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        BarSlot->SetPadding(FMargin(8.0f, 4.0f, 0.0f, 4.0f));
        BarSlot->SetVerticalAlignment(VAlign_Center);

        DossierBox->AddChildToVerticalBox(Row)->SetPadding(FMargin(12.0f, 2.0f));
    };

    AddRatingBar(LOCTEXT("R_Firepower", "ОГНЕВАЯ МОЩЬ"), FirepowerBar, FLinearColor(0.92f, 0.35f, 0.25f, 1.0f), TEXT("BarFP"));
    AddRatingBar(LOCTEXT("R_Armor", "БРОНЯ"), ArmorBar, FLinearColor(0.35f, 0.70f, 0.98f, 1.0f), TEXT("BarArm"));
    AddRatingBar(LOCTEXT("R_Mobility", "МОБИЛЬНОСТЬ"), MobilityBar, FLinearColor(0.25f, 0.85f, 0.55f, 1.0f), TEXT("BarMob"));
    AddRatingBar(LOCTEXT("R_Tech", "ТЕХНОЛОГИИ"), TechBar, FLinearColor(0.75f, 0.45f, 0.95f, 1.0f), TEXT("BarTech"));

    // Continue / Action Button
    ContinueButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ContinueButton"));
    FButtonStyle ActionStyle;
    ActionStyle.SetNormal(FSlateColorBrush(FLinearColor(0.35f, 0.08f, 0.12f, 0.98f)));
    ActionStyle.SetHovered(FSlateColorBrush(ScarletHorizon));
    ActionStyle.SetPressed(FSlateColorBrush(FLinearColor(1.0f, 0.30f, 0.35f, 1.0f)));
    ContinueButton->SetStyle(ActionStyle);

    ContinueLabelText = MakeText(
        WidgetTree, LOCTEXT("CTA_ChooseCountry", "ВЫБРАТЬ СТРАНУ ›"), 16, TextHighlight,
        TEXT("ContinueLabel"), true, false);
    ContinueLabelText->SetJustification(ETextJustify::Center);
    ContinueButton->AddChild(ContinueLabelText);
    ContinueButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::ContinueCampaign);

    UVerticalBoxSlot* ActionSlot = DossierBox->AddChildToVerticalBox(ContinueButton);
    ActionSlot->SetPadding(FMargin(12.0f, 16.0f, 12.0f, 12.0f));

    DossierFrameWidget = MakePanel(
        WidgetTree, DossierBox, TEXT("DossierFrame"),
        FLinearColor(0.68f, 0.28f, 0.88f, 1.0f), FMargin(6.0f));
    Place(MainCanvas, DossierFrameWidget, FVector2D(1380.0f, 95.0f), FVector2D(505.0f, 845.0f), 6);

    // ==========================================
    // BOTTOM STATUS / FOOTER
    // ==========================================
    UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CampaignFooter"));
    UButton* BackButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("CampaignBackButton"));
    BackButton->AddChild(MakeText(
        WidgetTree, LOCTEXT("BackButton", "‹  НАЗАД"), 14, TextPrimary,
        TEXT("CampaignBackLabel"), true, false));
    BackButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::GotoBlocStep);
    Footer->AddChildToHorizontalBox(BackButton)->SetPadding(FMargin(6.0f, 2.0f));

    UButton* TrainingButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("TrainingButton"));
    TrainingButton->AddChild(MakeText(
        WidgetTree, LOCTEXT("Training", "ОБУЧЕНИЕ"), 14, TextPrimary,
        TEXT("TrainingLabel"), true, false));
    TrainingButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenChallenges);
    Footer->AddChildToHorizontalBox(TrainingButton)->SetPadding(FMargin(6.0f, 2.0f));

    UTextBlock* EraText = MakeText(
        WidgetTree, LOCTEXT("Era", "SCARLET HORIZON  //  ГЛОБАЛЬНАЯ СЕТЬ ОПЕРАЦИЙ  //  v1.0-RC1"),
        12, TextMuted, TEXT("CampaignEra"), false);
    EraText->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* EraSlot = Footer->AddChildToHorizontalBox(EraText);
    EraSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    EraSlot->SetVerticalAlignment(VAlign_Center);

    Place(
        MainCanvas,
        MakePanel(WidgetTree, Footer, TEXT("CampaignFooterFrame"), FLinearColor(0.30f, 0.35f, 0.40f, 1.0f), FMargin(4.0f)),
        FVector2D(35.0f, 1005.0f), FVector2D(1560.0f, 52.0f), 5);
}

void URA4CampaignSelectWidget::RefreshBreadcrumbs()
{
    const FRA4FactionDataRegistry& Registry = FRA4FactionDataRegistry::Get();
    const TArray<FRA4BlocInfo>& Blocs = Registry.GetAllBlocs();

    FText BlocName = LOCTEXT("BC_BlocSelect", "01 БЛОК / КАТЕГОРИЯ");
    FText CountryName = LOCTEXT("BC_CountrySelect", "02 СТРАНА");
    FText DoctrineName = LOCTEXT("BC_DoctrineSelect", "03 ДОКТРИНА");

    if (Blocs.IsValidIndex(SelectedBlocIndex))
    {
        const FRA4BlocInfo& ActiveBloc = Blocs[SelectedBlocIndex];
        BlocName = FText::Format(LOCTEXT("BC_BlocFmt", "01 {0}"), ActiveBloc.DisplayName);

        if (ActiveBloc.Countries.IsValidIndex(SelectedCountryIndex))
        {
            const FRA4CountryInfo& ActiveCountry = ActiveBloc.Countries[SelectedCountryIndex];
            CountryName = FText::Format(LOCTEXT("BC_CountryFmt", "02 {0}"), ActiveCountry.DisplayName);

            if (ActiveCountry.Doctrines.IsValidIndex(SelectedDoctrineIndex))
            {
                const FRA4DoctrineInfo& ActiveDoc = ActiveCountry.Doctrines[SelectedDoctrineIndex];
                DoctrineName = FText::Format(LOCTEXT("BC_DocFmt", "03 {0}"), ActiveDoc.DisplayName);
            }
        }
    }

    if (BreadcrumbBlocText)
    {
        BreadcrumbBlocText->SetText(BlocName);
        BreadcrumbBlocText->SetColorAndOpacity(FSlateColor(
            CurrentStep == ERA4CampaignSelectStep::BlocSelection ? TextHighlight : TextMuted));
    }
    if (BreadcrumbCountryText)
    {
        BreadcrumbCountryText->SetText(CountryName);
        BreadcrumbCountryText->SetColorAndOpacity(FSlateColor(
            CurrentStep == ERA4CampaignSelectStep::CountrySelection ? TextHighlight : TextMuted));
    }
    if (BreadcrumbDoctrineText)
    {
        BreadcrumbDoctrineText->SetText(DoctrineName);
        BreadcrumbDoctrineText->SetColorAndOpacity(FSlateColor(
            CurrentStep == ERA4CampaignSelectStep::DoctrineSelection ? TextHighlight : TextMuted));
    }
}

void URA4CampaignSelectWidget::RefreshBlocCards()
{
    if (!BlocCardsContainer)
    {
        return;
    }
    BlocCardsContainer->ClearChildren();
    BlocCardFrames.Reset();

    const FRA4FactionDataRegistry& Registry = FRA4FactionDataRegistry::Get();
    const TArray<FRA4BlocInfo>& Blocs = Registry.GetAllBlocs();

    // Geometry read off 05_block_overview_cinematic_vivid.png. The reference is a
    // 1672x941 frame while this screen is authored at ReferenceSize, so every
    // measurement is carried across by ReferenceSize.X / 1672.
    const float ToAuthoring = ReferenceSize.X / 1672.0f;
    const auto Ref = [ToAuthoring](const float X, const float Y, const float W, const float H)
    {
        return FBox2D(
            FVector2D(X * ToAuthoring, Y * ToAuthoring),
            FVector2D((X + W) * ToAuthoring, (Y + H) * ToAuthoring));
    };

    // One main theatre and four secondary zones of deliberately unequal size.
    const FBox2D HeroRect = Ref(28.0f, 88.0f, 762.0f, 632.0f);
    const FBox2D PlateRects[4] = {
        Ref(935.0f, 88.0f, 710.0f, 202.0f),   // wide and short, top right
        Ref(800.0f, 300.0f, 845.0f, 200.0f),  // the widest band, reaching furthest left
        Ref(800.0f, 505.0f, 400.0f, 210.0f),  // bottom left of the mosaic
        Ref(1210.0f, 505.0f, 435.0f, 210.0f)  // bottom right of the mosaic
    };

    // Bottom summary strip: operational figures, the direction's doctrine line and
    // its combat profile. Reference bands y 735..865 and the "next step" action.
    if (Blocs.IsValidIndex(SelectedBlocIndex))
    {
        const FRA4BlocInfo& Active = Blocs[SelectedBlocIndex];

        UVerticalBox* Summary = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), TEXT("BlocSummaryBox"));
        Summary->AddChildToVerticalBox(MakeText(
            WidgetTree, LOCTEXT("OpsSummary", "ОПЕРАЦИОННАЯ СВОДКА"), 12, Active.GlowColor,
            TEXT("OpsSummaryHeader"), true, false))->SetPadding(FMargin(14.0f, 10.0f, 14.0f, 8.0f));

        UHorizontalBox* Figures = WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(), TEXT("BlocSummaryFigures"));
        const auto AddFigure = [this, Figures](const FText& Caption, const FText& Value, const FName Name)
        {
            UVerticalBox* Cell = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
            Cell->AddChildToVerticalBox(MakeText(
                WidgetTree, Caption, 10, TextMuted, FName(Name.ToString() + TEXT("_Cap")), false));
            Cell->AddChildToVerticalBox(MakeText(
                WidgetTree, Value, 22, TextHighlight, FName(Name.ToString() + TEXT("_Val")), true, false))
                ->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
            UHorizontalBoxSlot* Slot = Figures->AddChildToHorizontalBox(Cell);
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            Slot->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 0.0f));
        };
        AddFigure(LOCTEXT("Fig_Regions", "РЕГИОНОВ ПОД КОНТРОЛЕМ"),
            FText::AsNumber(Active.ControlledRegions), TEXT("FigRegions"));
        AddFigure(LOCTEXT("Fig_Personnel", "АКТИВНЫХ ПОДРАЗДЕЛЕНИЙ"),
            FText::AsNumber(Active.ActivePersonnel), TEXT("FigPersonnel"));
        AddFigure(LOCTEXT("Fig_Readiness", "УРОВЕНЬ ГОТОВНОСТИ"),
            FText::Format(LOCTEXT("PercentFmt", "{0}%"),
                FText::AsNumber(FMath::RoundToInt(Active.ReadinessRatio * 100.0f))),
            TEXT("FigReadiness"));
        Summary->AddChildToVerticalBox(Figures)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

        Place(BlocCardsContainer,
            MakePanel(WidgetTree, Summary, TEXT("BlocSummaryFrame"), Active.PrimaryColor,
                DensityPadding(Active.PanelDensity), Active.FrameRail),
            Ref(28.0f, 735.0f, 612.0f, 122.0f).Min,
            Ref(28.0f, 735.0f, 612.0f, 122.0f).GetSize(), 2);

        // Combat profile of the direction, mirroring the reference's right block.
        UVerticalBox* Profile = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), TEXT("BlocProfileBox"));
        Profile->AddChildToVerticalBox(MakeText(
            WidgetTree, LOCTEXT("CombatProfile", "БОЕВАЯ ХАРАКТЕРИСТИКА"), 12, Active.GlowColor,
            TEXT("BlocProfileHeader"), true, false))->SetPadding(FMargin(14.0f, 7.0f, 14.0f, 5.0f));

        const FRA4CountryInfo* Lead = Active.Countries.Num() > 0 ? &Active.Countries[0] : nullptr;
        const auto AddProfileRow = [this, Profile](const FText& Label, const float Value, const FName Name)
        {
            UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), Name);
            UTextBlock* Caption = MakeText(
                WidgetTree, Label, 11, TextPrimary, FName(Name.ToString() + TEXT("_Lbl")), false, false);
            Row->AddChildToHorizontalBox(Caption)->SetSize(FillWeight(1.0f));
            UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(
                UProgressBar::StaticClass(), FName(Name.ToString() + TEXT("_Bar")));
            Bar->SetPercent(Value);
            UHorizontalBoxSlot* BarSlot = Row->AddChildToHorizontalBox(Bar);
            BarSlot->SetSize(FillWeight(2.0f));
            BarSlot->SetVerticalAlignment(VAlign_Center);
            BarSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
            Profile->AddChildToVerticalBox(Row)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 3.0f));
        };
        AddProfileRow(LOCTEXT("P_Fire", "ОГНЕВАЯ МОЩЬ"), Lead ? Lead->FirepowerRating : 0.8f, TEXT("PFire"));
        AddProfileRow(LOCTEXT("P_Armor", "ЗАЩИТА"), Lead ? Lead->ArmorRating : 0.8f, TEXT("PArmor"));
        AddProfileRow(LOCTEXT("P_Mob", "МОБИЛЬНОСТЬ"), Lead ? Lead->MobilityRating : 0.8f, TEXT("PMob"));
        AddProfileRow(LOCTEXT("P_Tech", "ТЕХНОЛОГИИ"), Lead ? Lead->TechRating : 0.8f, TEXT("PTech"));

        Place(BlocCardsContainer,
            MakePanel(WidgetTree, Profile, TEXT("BlocProfileFrame"), Active.PrimaryColor,
                DensityPadding(Active.PanelDensity), Active.FrameRail),
            Ref(655.0f, 735.0f, 990.0f, 122.0f).Min,
            Ref(655.0f, 735.0f, 990.0f, 122.0f).GetSize(), 2);

        // "Next step" action sits bottom right in the reference.
        UButton* NextButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BlocNextButton"));
        NextButton->SetStyle(MakeCardButtonStyle(Active.GlowColor));
        UTextBlock* NextLabel = MakeText(
            WidgetTree, LOCTEXT("NextCountry", "ДАЛЕЕ: СТРАНА  ›"), 16, TextHighlight,
            TEXT("BlocNextLabel"), true, false);
        NextLabel->SetJustification(ETextJustify::Center);
        NextButton->AddChild(NextLabel);
        NextButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::GotoCountryStep);
        Place(BlocCardsContainer, NextButton,
            Ref(1390.0f, 875.0f, 255.0f, 50.0f).Min,
            Ref(1390.0f, 875.0f, 255.0f, 50.0f).GetSize(), 3);
    }

    int32 PlateIndex = 0;
    for (int32 Index = 0; Index < Blocs.Num(); ++Index)
    {
        const FRA4BlocInfo& Bloc = Blocs[Index];
        const bool bHero = (Index == SelectedBlocIndex);
        const FBox2D Rect = bHero
            ? HeroRect
            : PlateRects[FMath::Min(PlateIndex++, 3)];
        const FVector2D Size = Rect.GetSize();

        UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), FName(*FString::Printf(TEXT("BlocContent_%d"), Index)));

        UTextBlock* Numeral = MakeText(
            WidgetTree, FText::Format(LOCTEXT("BlocNum", "0{0}"), FText::AsNumber(Index + 1)),
            bHero ? 15 : 12, Bloc.GlowColor,
            FName(*FString::Printf(TEXT("BlocNum_%d"), Index)), true, false);
        CardContent->AddChildToVerticalBox(Numeral)->SetPadding(FMargin(14.0f, 12.0f, 14.0f, 2.0f));

        UTextBlock* Title = MakeText(
            WidgetTree, Bloc.DisplayName, bHero ? 34 : 19, TextHighlight,
            FName(*FString::Printf(TEXT("BlocTitle_%d"), Index)));
        CardContent->AddChildToVerticalBox(Title)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 4.0f));

        if (Bloc.bIsCategoryOnly)
        {
            UTextBlock* Badge = MakeText(
                WidgetTree, LOCTEXT("CatBadge", "[ КАТЕГОРИЯ ВЫБОРА ]"), 11,
                FLinearColor(0.95f, 0.75f, 0.30f, 1.0f),
                FName(*FString::Printf(TEXT("CatBadge_%d"), Index)), true, false);
            CardContent->AddChildToVerticalBox(Badge)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 4.0f));
        }

        UTextBlock* Motto = MakeText(
            WidgetTree, Bloc.Motto, bHero ? 13 : 11, Bloc.GlowColor,
            FName(*FString::Printf(TEXT("BlocMotto_%d"), Index)), false);
        CardContent->AddChildToVerticalBox(Motto)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 10.0f));

        // Only the hero has the height for the full dossier. A secondary plate
        // that tried to carry it would just clip its own text.
        if (bHero)
        {
            UTextBlock* Desc = MakeText(
                WidgetTree, Bloc.Description, 14, TextPrimary,
                FName(*FString::Printf(TEXT("BlocDesc_%d"), Index)), false);
            CardContent->AddChildToVerticalBox(Desc)->SetPadding(FMargin(14.0f, 4.0f, 14.0f, 14.0f));

            UTextBlock* AdvHeader = MakeText(
                WidgetTree, LOCTEXT("KeyAdvantages", "КЛЮЧЕВЫЕ ПРЕИМУЩЕСТВА"), 12, Bloc.GlowColor,
                FName(*FString::Printf(TEXT("BlocAdvHdr_%d"), Index)), true, false);
            CardContent->AddChildToVerticalBox(AdvHeader)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 8.0f));

            for (int32 A = 0; A < Bloc.KeyAdvantages.Num(); ++A)
            {
                UTextBlock* Row = MakeText(
                    WidgetTree, Bloc.KeyAdvantages[A], 12, TextPrimary,
                    FName(*FString::Printf(TEXT("BlocAdv_%d_%d"), Index, A)), false);
                CardContent->AddChildToVerticalBox(Row)->SetPadding(FMargin(22.0f, 0.0f, 14.0f, 6.0f));
            }
        }

        UTextBlock* CountriesHeader = MakeText(
            WidgetTree,
            Bloc.bIsCategoryOnly
                ? LOCTEXT("CountriesInCategory", "СТРАНЫ КАТЕГОРИИ:")
                : LOCTEXT("CountriesInBloc", "СОСТАВ БЛОКА:"),
            11, TextMuted,
            FName(*FString::Printf(TEXT("BlocCntHeader_%d"), Index)), true, false);
        UVerticalBoxSlot* HeaderSlot = CardContent->AddChildToVerticalBox(CountriesHeader);
        HeaderSlot->SetPadding(FMargin(14.0f, 6.0f, 14.0f, 2.0f));
        HeaderSlot->SetVerticalAlignment(VAlign_Bottom);

        FString CountryNamesList;
        for (int32 CIdx = 0; CIdx < Bloc.Countries.Num(); ++CIdx)
        {
            CountryNamesList += (CIdx > 0 ? TEXT(" • ") : TEXT("")) + Bloc.Countries[CIdx].DisplayName.ToString();
        }
        UTextBlock* CountriesList = MakeText(
            WidgetTree, FText::FromString(CountryNamesList), 11, TextHighlight,
            FName(*FString::Printf(TEXT("BlocCntList_%d"), Index)), false);
        CardContent->AddChildToVerticalBox(CountriesList)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 12.0f));

        UButton* CardButton = WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(), FName(*FString::Printf(TEXT("BlocBtn_%d"), Index)));
        CardButton->SetStyle(MakeCardButtonStyle(Bloc.PrimaryColor));
        CardButton->AddChild(CardContent);

        switch (Index)
        {
        case 0: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnBloc0); break;
        case 1: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnBloc1); break;
        case 2: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnBloc2); break;
        case 3: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnBloc3); break;
        case 4: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnBloc4); break;
        default: break;
        }

        UBorder* CardFrame = MakePanel(
            WidgetTree, CardButton, FName(*FString::Printf(TEXT("BlocFrame_%d"), Index)),
            bHero ? Bloc.GlowColor : Bloc.PrimaryColor,
            DensityPadding(Bloc.PanelDensity), Bloc.FrameRail);
        BlocCardFrames.Add(CardFrame);

        Place(BlocCardsContainer, CardFrame, Rect.Min, Size, bHero ? 3 : 2);
    }
}

void URA4CampaignSelectWidget::RefreshCountryCards()
{
    if (!CountryCardsContainer)
    {
        return;
    }
    CountryCardsContainer->ClearChildren();
    CountryCardFrames.Reset();

    const FRA4FactionDataRegistry& Registry = FRA4FactionDataRegistry::Get();
    const TArray<FRA4BlocInfo>& Blocs = Registry.GetAllBlocs();
    if (!Blocs.IsValidIndex(SelectedBlocIndex))
    {
        return;
    }

    const FRA4BlocInfo& ActiveBloc = Blocs[SelectedBlocIndex];
    for (int32 Index = 0; Index < ActiveBloc.Countries.Num(); ++Index)
    {
        const FRA4CountryInfo& Country = ActiveBloc.Countries[Index];
        const bool bSelected = (Index == SelectedCountryIndex);

        UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), FName(*FString::Printf(TEXT("CountryContent_%d"), Index)));

        // Flag / code
        UTextBlock* CodeText = MakeText(
            WidgetTree, FText::FromString(Country.CountryId.ToString().ToUpper()),
            12, Country.AccentColor, FName(*FString::Printf(TEXT("CountryCode_%d"), Index)));
        CardContent->AddChildToVerticalBox(CodeText)->SetPadding(FMargin(12.0f, 12.0f, 12.0f, 2.0f));

        UTextBlock* Title = MakeText(
            WidgetTree, Country.DisplayName, 20, TextHighlight,
            FName(*FString::Printf(TEXT("CountryTitle_%d"), Index)));
        CardContent->AddChildToVerticalBox(Title)->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 4.0f));

        UTextBlock* Spec = MakeText(
            WidgetTree, Country.Specialization, 12, Country.AccentColor,
            FName(*FString::Printf(TEXT("CountrySpec_%d"), Index)), false);
        CardContent->AddChildToVerticalBox(Spec)->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 10.0f));

        UTextBlock* Desc = MakeText(
            WidgetTree, Country.LoreDescription, 12, TextPrimary,
            FName(*FString::Printf(TEXT("CountryDesc_%d"), Index)), false);
        Desc->SetAutoWrapText(true);
        UVerticalBoxSlot* DescSlot = CardContent->AddChildToVerticalBox(Desc);
        DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        DescSlot->SetPadding(FMargin(12.0f, 4.0f, 12.0f, 12.0f));

        // Ratings summary
        UTextBlock* RatingsTitle = MakeText(
            WidgetTree, LOCTEXT("RatingsSummary", "ПОКАЗАТЕЛИ АРМИИ:"), 11, TextMuted,
            FName(*FString::Printf(TEXT("CRatTitle_%d"), Index)));
        CardContent->AddChildToVerticalBox(RatingsTitle)->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 2.0f));

        FText RatingsSummaryText = FText::Format(
            LOCTEXT("RatingsSummaryFmt", "ОГНЕВАЯ МОЩЬ: {0}%  •  БРОНЯ: {1}%\nМОБИЛЬНОСТЬ: {2}%  •  ТЕХНОЛОГИИ: {3}%"),
            FText::AsNumber(FMath::RoundToInt(Country.FirepowerRating * 100.0f)),
            FText::AsNumber(FMath::RoundToInt(Country.ArmorRating * 100.0f)),
            FText::AsNumber(FMath::RoundToInt(Country.MobilityRating * 100.0f)),
            FText::AsNumber(FMath::RoundToInt(Country.TechRating * 100.0f)));
        UTextBlock* RatingsValues = MakeText(
            WidgetTree, RatingsSummaryText, 11, TextHighlight,
            FName(*FString::Printf(TEXT("CRatVal_%d"), Index)), false);
        CardContent->AddChildToVerticalBox(RatingsValues)->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 8.0f));

        // Doctrines count
        UTextBlock* DocCount = MakeText(
            WidgetTree, FText::Format(LOCTEXT("DocCountFmt", "ДОСТУПНО ДОКТРИН: {0}"), FText::AsNumber(Country.Doctrines.Num())),
            12, FLinearColor(0.88f, 0.72f, 0.22f, 1.0f), FName(*FString::Printf(TEXT("CDocCount_%d"), Index)));
        CardContent->AddChildToVerticalBox(DocCount)->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 12.0f));

        // Card Button
        UButton* CardButton = WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(), FName(*FString::Printf(TEXT("CountryBtn_%d"), Index)));
        CardButton->SetStyle(MakeCardButtonStyle(Country.AccentColor));
        CardButton->AddChild(CardContent);

        switch (Index)
        {
        case 0: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnCountry0); break;
        case 1: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnCountry1); break;
        case 2: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnCountry2); break;
        case 3: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnCountry3); break;
        case 4: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnCountry4); break;
        case 5: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnCountry5); break;
        default: break;
        }

        // A category has no shared frame: each independent country keeps its own
        // accent instead of borrowing a bloc colour it does not belong to.
        const FLinearColor FrameAccent = bSelected || ActiveBloc.bIsCategoryOnly
            ? Country.AccentColor
            : ActiveBloc.PrimaryColor;

        UBorder* CardFrame = MakePanel(
            WidgetTree, CardButton, FName(*FString::Printf(TEXT("CountryFrame_%d"), Index)),
            FrameAccent, DensityPadding(ActiveBloc.PanelDensity), ActiveBloc.FrameRail);
        CountryCardFrames.Add(CardFrame);

        // The bloc leader dominates its mosaic; partners read as smaller plates.
        UHorizontalBoxSlot* CardSlot = CountryCardsContainer->AddChildToHorizontalBox(CardFrame);
        CardSlot->SetSize(FillWeight(Country.LayoutWeight));
        CardSlot->SetPadding(bSelected ? FMargin(4.0f, 0.0f) : FMargin(4.0f, 18.0f));
    }
}

void URA4CampaignSelectWidget::RefreshDoctrineCards()
{
    if (!DoctrineCardsContainer)
    {
        return;
    }
    DoctrineCardsContainer->ClearChildren();
    DoctrineCardFrames.Reset();

    const FRA4FactionDataRegistry& Registry = FRA4FactionDataRegistry::Get();
    const TArray<FRA4BlocInfo>& Blocs = Registry.GetAllBlocs();
    if (!Blocs.IsValidIndex(SelectedBlocIndex))
    {
        return;
    }

    const FRA4BlocInfo& ActiveBloc = Blocs[SelectedBlocIndex];
    if (!ActiveBloc.Countries.IsValidIndex(SelectedCountryIndex))
    {
        return;
    }

    const FRA4CountryInfo& ActiveCountry = ActiveBloc.Countries[SelectedCountryIndex];
    for (int32 Index = 0; Index < ActiveCountry.Doctrines.Num(); ++Index)
    {
        const FRA4DoctrineInfo& Doctrine = ActiveCountry.Doctrines[Index];
        const bool bSelected = (Index == SelectedDoctrineIndex);

        UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), FName(*FString::Printf(TEXT("DoctrineContent_%d"), Index)));

        // Roman numeral & Doctrine badge
        UTextBlock* Numeral = MakeText(
            WidgetTree, FText::Format(LOCTEXT("DocNum", "ДОКТРИНА 0{0}"), FText::AsNumber(Index + 1)),
            12, FLinearColor(0.88f, 0.72f, 0.22f, 1.0f), FName(*FString::Printf(TEXT("DocNum_%d"), Index)));
        CardContent->AddChildToVerticalBox(Numeral)->SetPadding(FMargin(14.0f, 14.0f, 14.0f, 2.0f));

        UTextBlock* Title = MakeText(
            WidgetTree, Doctrine.DisplayName, 20, TextHighlight,
            FName(*FString::Printf(TEXT("DocTitle_%d"), Index)));
        CardContent->AddChildToVerticalBox(Title)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 4.0f));

        UTextBlock* Subtitle = MakeText(
            WidgetTree, Doctrine.CombatPhilosophy, 12, TextMuted,
            FName(*FString::Printf(TEXT("DocSub_%d"), Index)), false);
        CardContent->AddChildToVerticalBox(Subtitle)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 10.0f));

        UTextBlock* Desc = MakeText(
            WidgetTree, Doctrine.Description, 13, TextPrimary,
            FName(*FString::Printf(TEXT("DocDesc_%d"), Index)), false);
        Desc->SetAutoWrapText(true);
        UVerticalBoxSlot* DescSlot = CardContent->AddChildToVerticalBox(Desc);
        DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        DescSlot->SetPadding(FMargin(14.0f, 4.0f, 14.0f, 12.0f));

        // Replaced units list (25% composition replacement)
        UTextBlock* SwapHeader = MakeText(
            WidgetTree, LOCTEXT("SwapHeader", "МОДИФИКАЦИЯ СОСТАВА АРМИИ (25%):"), 11, ScarletHorizon,
            FName(*FString::Printf(TEXT("DocSwapHdr_%d"), Index)));
        CardContent->AddChildToVerticalBox(SwapHeader)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 4.0f));

        FString UnitsList;
        for (int32 UIdx = 0; UIdx < Doctrine.ModifiedUnits.Num(); ++UIdx)
        {
            UnitsList += (UIdx > 0 ? TEXT("\n• ") : TEXT("• ")) + Doctrine.ModifiedUnits[UIdx].ToString();
        }
        UTextBlock* UnitsBlock = MakeText(
            WidgetTree, FText::FromString(UnitsList), 11, TextHighlight,
            FName(*FString::Printf(TEXT("DocUnits_%d"), Index)), false);
        CardContent->AddChildToVerticalBox(UnitsBlock)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 10.0f));

        // Signature superweapon / trait
        UTextBlock* SuperHeader = MakeText(
            WidgetTree, LOCTEXT("SuperHeader", "КЛЮЧЕВАЯ СПОСОБНОСТЬ:"), 11, TextMuted,
            FName(*FString::Printf(TEXT("DocSpHdr_%d"), Index)));
        CardContent->AddChildToVerticalBox(SuperHeader)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 2.0f));

        UTextBlock* SuperTrait = MakeText(
            WidgetTree, Doctrine.SignatureSuperweapon, 12, FLinearColor(0.88f, 0.72f, 0.22f, 1.0f),
            FName(*FString::Printf(TEXT("DocSpTrait_%d"), Index)));
        CardContent->AddChildToVerticalBox(SuperTrait)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 14.0f));

        // Button
        UButton* CardButton = WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(), FName(*FString::Printf(TEXT("DoctrineBtn_%d"), Index)));
        CardButton->SetStyle(MakeCardButtonStyle(FLinearColor(0.88f, 0.72f, 0.22f, 1.0f)));
        CardButton->AddChild(CardContent);

        switch (Index)
        {
        case 0: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnDoctrine0); break;
        case 1: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnDoctrine1); break;
        case 2: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnDoctrine2); break;
        default: break;
        }

        UBorder* CardFrame = MakePanel(
            WidgetTree, CardButton, FName(*FString::Printf(TEXT("DoctrineFrame_%d"), Index)),
            bSelected ? FLinearColor(0.88f, 0.72f, 0.22f, 1.0f) : ActiveCountry.AccentColor,
            DensityPadding(ActiveBloc.PanelDensity), ActiveBloc.FrameRail);
        DoctrineCardFrames.Add(CardFrame);

        // Doctrines are a stepped ribbon, not three identical tiles: the active
        // one opens up and the others sit lower.
        UHorizontalBoxSlot* CardSlot = DoctrineCardsContainer->AddChildToHorizontalBox(CardFrame);
        CardSlot->SetSize(FillWeight(bSelected ? 1.45f : 1.0f));
        CardSlot->SetPadding(bSelected
            ? FMargin(6.0f, 0.0f)
            : FMargin(6.0f, 16.0f + 6.0f * static_cast<float>(Index), 6.0f, 0.0f));
    }
}

void URA4CampaignSelectWidget::RefreshDossierPanel()
{
    const FRA4FactionDataRegistry& Registry = FRA4FactionDataRegistry::Get();
    const TArray<FRA4BlocInfo>& Blocs = Registry.GetAllBlocs();
    if (!Blocs.IsValidIndex(SelectedBlocIndex))
    {
        return;
    }

    const FRA4BlocInfo& ActiveBloc = Blocs[SelectedBlocIndex];
    const bool bHasCountry = ActiveBloc.Countries.IsValidIndex(SelectedCountryIndex);
    const FRA4CountryInfo* ActiveCountry = bHasCountry ? &ActiveBloc.Countries[SelectedCountryIndex] : nullptr;
    const bool bHasDoctrine = (ActiveCountry && ActiveCountry->Doctrines.IsValidIndex(SelectedDoctrineIndex));
    const FRA4DoctrineInfo* ActiveDoc = bHasDoctrine ? &ActiveCountry->Doctrines[SelectedDoctrineIndex] : nullptr;

    switch (CurrentStep)
    {
    case ERA4CampaignSelectStep::BlocSelection:
        if (DossierHeaderTag) DossierHeaderTag->SetText(LOCTEXT("TagBloc", "[ СТРАТЕГИЧЕСКИЙ БЛОК ]"));
        if (DossierTitleText) DossierTitleText->SetText(ActiveBloc.DisplayName);
        if (DossierSubtitleText) DossierSubtitleText->SetText(ActiveBloc.Motto);
        if (DossierSpecializationText)
        {
            FString AdvList;
            for (int32 AIdx = 0; AIdx < ActiveBloc.KeyAdvantages.Num(); ++AIdx)
            {
                AdvList += (AIdx > 0 ? TEXT(" • ") : TEXT("")) + ActiveBloc.KeyAdvantages[AIdx].ToString();
            }
            DossierSpecializationText->SetText(FText::FromString(AdvList));
        }
        if (DossierDescriptionText) DossierDescriptionText->SetText(ActiveBloc.Description);
        if (FirepowerBar) FirepowerBar->SetPercent(0.85f);
        if (ArmorBar) ArmorBar->SetPercent(0.80f);
        if (MobilityBar) MobilityBar->SetPercent(0.75f);
        if (TechBar) TechBar->SetPercent(0.85f);
        if (ContinueLabelText) ContinueLabelText->SetText(LOCTEXT("CTA_ChooseCountry", "ВЫБРАТЬ СТРАНУ ›"));
        break;

    case ERA4CampaignSelectStep::CountrySelection:
        if (ActiveCountry)
        {
            if (DossierHeaderTag) DossierHeaderTag->SetText(LOCTEXT("TagCountry", "[ ДОСЬЕ СТРАНЫ ]"));
            if (DossierTitleText) DossierTitleText->SetText(ActiveCountry->DisplayName);
            if (DossierSubtitleText) DossierSubtitleText->SetText(ActiveCountry->Specialization);
            if (DossierSpecializationText)
            {
                DossierSpecializationText->SetText(FText::Format(
                    LOCTEXT("CountryDossierSpecFmt", "БАЗОВЫЙ ШТАБ: {0}  •  ДОКТРИН: {1}"),
                    ActiveCountry->BaseHeadquarters, FText::AsNumber(ActiveCountry->Doctrines.Num())));
            }
            if (DossierDescriptionText) DossierDescriptionText->SetText(ActiveCountry->LoreDescription);
            if (FirepowerBar) FirepowerBar->SetPercent(ActiveCountry->FirepowerRating);
            if (ArmorBar) ArmorBar->SetPercent(ActiveCountry->ArmorRating);
            if (MobilityBar) MobilityBar->SetPercent(ActiveCountry->MobilityRating);
            if (TechBar) TechBar->SetPercent(ActiveCountry->TechRating);
            if (ContinueLabelText) ContinueLabelText->SetText(LOCTEXT("CTA_ChooseDoc", "ВЫБРАТЬ ДОКТРИНУ ›"));
        }
        break;

    case ERA4CampaignSelectStep::DoctrineSelection:
        if (ActiveDoc && ActiveCountry)
        {
            if (DossierHeaderTag) DossierHeaderTag->SetText(LOCTEXT("TagDoc", "[ ТАКТИЧЕСКАЯ ДОКТРИНА ]"));
            if (DossierTitleText) DossierTitleText->SetText(ActiveDoc->DisplayName);
            if (DossierSubtitleText) DossierSubtitleText->SetText(ActiveDoc->CombatPhilosophy);
            if (DossierSpecializationText)
            {
                DossierSpecializationText->SetText(FText::Format(
                    LOCTEXT("DocDossierSpecFmt", "ФЛАГМАНСКИЙ ЮНИТ: {0}"),
                    ActiveDoc->SignatureUnit));
            }
            if (DossierDescriptionText) DossierDescriptionText->SetText(ActiveDoc->Description);
            if (FirepowerBar) FirepowerBar->SetPercent(ActiveCountry->FirepowerRating);
            if (ArmorBar) ArmorBar->SetPercent(ActiveCountry->ArmorRating);
            if (MobilityBar) MobilityBar->SetPercent(ActiveCountry->MobilityRating);
            if (TechBar) TechBar->SetPercent(ActiveCountry->TechRating);
            if (ContinueLabelText) ContinueLabelText->SetText(LOCTEXT("CTA_StartOp", "НАЧАТЬ ОПЕРАЦИЮ ›"));
        }
        break;
    }
}

void URA4CampaignSelectWidget::OnBlocCardClicked(const int32 BlocIndex)
{
    SelectedBlocIndex = FMath::Clamp(BlocIndex, 0, 4);
    SelectedCountryIndex = 0;
    SelectedDoctrineIndex = 0;
    CurrentStep = ERA4CampaignSelectStep::CountrySelection;

    if (BlocCardsContainer) BlocCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (DossierFrameWidget) DossierFrameWidget->SetVisibility(ESlateVisibility::Visible);
    if (CountryCardsContainer) CountryCardsContainer->SetVisibility(ESlateVisibility::Visible);
    if (DoctrineCardsContainer) DoctrineCardsContainer->SetVisibility(ESlateVisibility::Collapsed);

    RefreshBreadcrumbs();
    RefreshCountryCards();
    RefreshDossierPanel();
}

void URA4CampaignSelectWidget::OnCountryCardClicked(const int32 CountryIndex)
{
    SelectedCountryIndex = CountryIndex;
    SelectedDoctrineIndex = 0;
    CurrentStep = ERA4CampaignSelectStep::DoctrineSelection;

    if (BlocCardsContainer) BlocCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (DossierFrameWidget) DossierFrameWidget->SetVisibility(ESlateVisibility::Visible);
    if (CountryCardsContainer) CountryCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (DoctrineCardsContainer) DoctrineCardsContainer->SetVisibility(ESlateVisibility::Visible);

    RefreshBreadcrumbs();
    RefreshDoctrineCards();
    RefreshDossierPanel();
}

void URA4CampaignSelectWidget::OnDoctrineCardClicked(const int32 DoctrineIndex)
{
    SelectedDoctrineIndex = DoctrineIndex;
    RefreshBreadcrumbs();
    RefreshDoctrineCards();
    RefreshDossierPanel();
}

void URA4CampaignSelectWidget::GotoBlocStep()
{
    CurrentStep = ERA4CampaignSelectStep::BlocSelection;
    if (BlocCardsContainer) BlocCardsContainer->SetVisibility(ESlateVisibility::Visible);
    if (DossierFrameWidget) DossierFrameWidget->SetVisibility(ESlateVisibility::Collapsed);
    if (CountryCardsContainer) CountryCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (DoctrineCardsContainer) DoctrineCardsContainer->SetVisibility(ESlateVisibility::Collapsed);

    RefreshBreadcrumbs();
    RefreshBlocCards();
    RefreshDossierPanel();
}

void URA4CampaignSelectWidget::GotoCountryStep()
{
    CurrentStep = ERA4CampaignSelectStep::CountrySelection;
    if (BlocCardsContainer) BlocCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (DossierFrameWidget) DossierFrameWidget->SetVisibility(ESlateVisibility::Visible);
    if (CountryCardsContainer) CountryCardsContainer->SetVisibility(ESlateVisibility::Visible);
    if (DoctrineCardsContainer) DoctrineCardsContainer->SetVisibility(ESlateVisibility::Collapsed);

    RefreshBreadcrumbs();
    RefreshCountryCards();
    RefreshDossierPanel();
}

void URA4CampaignSelectWidget::GotoDoctrineStep()
{
    CurrentStep = ERA4CampaignSelectStep::DoctrineSelection;
    if (BlocCardsContainer) BlocCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (DossierFrameWidget) DossierFrameWidget->SetVisibility(ESlateVisibility::Visible);
    if (CountryCardsContainer) CountryCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (DoctrineCardsContainer) DoctrineCardsContainer->SetVisibility(ESlateVisibility::Visible);

    RefreshBreadcrumbs();
    RefreshDoctrineCards();
    RefreshDossierPanel();
}

void URA4CampaignSelectWidget::OnBloc0() { OnBlocCardClicked(0); }
void URA4CampaignSelectWidget::OnBloc1() { OnBlocCardClicked(1); }
void URA4CampaignSelectWidget::OnBloc2() { OnBlocCardClicked(2); }
void URA4CampaignSelectWidget::OnBloc3() { OnBlocCardClicked(3); }
void URA4CampaignSelectWidget::OnBloc4() { OnBlocCardClicked(4); }

void URA4CampaignSelectWidget::OnCountry0() { OnCountryCardClicked(0); }
void URA4CampaignSelectWidget::OnCountry1() { OnCountryCardClicked(1); }
void URA4CampaignSelectWidget::OnCountry2() { OnCountryCardClicked(2); }
void URA4CampaignSelectWidget::OnCountry3() { OnCountryCardClicked(3); }
void URA4CampaignSelectWidget::OnCountry4() { OnCountryCardClicked(4); }
void URA4CampaignSelectWidget::OnCountry5() { OnCountryCardClicked(5); }

void URA4CampaignSelectWidget::OnDoctrine0() { OnDoctrineCardClicked(0); }
void URA4CampaignSelectWidget::OnDoctrine1() { OnDoctrineCardClicked(1); }
void URA4CampaignSelectWidget::OnDoctrine2() { OnDoctrineCardClicked(2); }

void URA4CampaignSelectWidget::ContinueCampaign()
{
    if (CurrentStep == ERA4CampaignSelectStep::BlocSelection)
    {
        GotoCountryStep();
        return;
    }
    if (CurrentStep == ERA4CampaignSelectStep::CountrySelection)
    {
        GotoDoctrineStep();
        return;
    }

    // Step 3: Launch Campaign for the selected faction
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4CampaignScreenWidget* Campaign = CreateWidget<URA4CampaignScreenWidget>(
            PlayerController, URA4CampaignScreenWidget::StaticClass()))
        {
            Campaign->ConfigureCampaign(static_cast<ERA4FactionTheme>(SelectedBlocIndex));
            Campaign->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

void URA4CampaignSelectWidget::OpenMainMenu()
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

void URA4CampaignSelectWidget::OpenMultiplayer()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4ShowcaseWidget* Screen = CreateWidget<URA4ShowcaseWidget>(
            PlayerController, URA4ShowcaseWidget::StaticClass()))
        {
            Screen->SetInitialScreenForPresentation(3); // Multiplayer lobby
            Screen->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

void URA4CampaignSelectWidget::OpenChallenges()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4ShowcaseWidget* Screen = CreateWidget<URA4ShowcaseWidget>(
            PlayerController, URA4ShowcaseWidget::StaticClass()))
        {
            Screen->SetInitialScreenForPresentation(23); // Challenges
            Screen->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

void URA4CampaignSelectWidget::OpenBarracks()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4ShowcaseWidget* Screen = CreateWidget<URA4ShowcaseWidget>(
            PlayerController, URA4ShowcaseWidget::StaticClass()))
        {
            Screen->SetInitialScreenForPresentation(19); // Barracks
            Screen->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

void URA4CampaignSelectWidget::OpenSettings()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4ShowcaseWidget* Screen = CreateWidget<URA4ShowcaseWidget>(
            PlayerController, URA4ShowcaseWidget::StaticClass()))
        {
            Screen->SetInitialScreenForPresentation(4); // Settings
            Screen->AddToViewport(0);
            RemoveFromParent();
        }
    }
}
#undef LOCTEXT_NAMESPACE
