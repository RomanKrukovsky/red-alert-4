// Copyright (c) Red Alert 4 project.

#include "RA4CampaignSelectWidget.h"

#include "RA4CommandCentreMenuWidget.h"
#include "RA4ShowcaseWidget.h"
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
constexpr FLinearColor SovietRed(0.92f, 0.035f, 0.045f, 1.0f);
constexpr FLinearColor AllianceBlue(0.25f, 0.58f, 0.92f, 1.0f);
constexpr FLinearColor EasternGold(0.78f, 0.62f, 0.18f, 1.0f);
constexpr FLinearColor ChronoViolet(0.66f, 0.26f, 0.94f, 1.0f);
constexpr FLinearColor TextPrimary(0.86f, 0.82f, 0.78f, 1.0f);
constexpr FLinearColor TextMuted(0.52f, 0.49f, 0.47f, 1.0f);
constexpr FLinearColor PanelBlack(0.008f, 0.009f, 0.012f, 0.94f);

UTextBlock* MakeText(
    UWidgetTree* Tree,
    const FText& Value,
    const int32 Size,
    const FLinearColor& Color,
    const FName Name,
    const bool bEmphasis = true)
{
    UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    Label->SetText(Value);
    Label->SetColorAndOpacity(FSlateColor(Color));
    const TCHAR* FontFile = bEmphasis
        ? TEXT("RA4UI/Fonts/RA4_RobotoCondensedSemiBold.ttf")
        : TEXT("RA4UI/Fonts/RA4_RobotoCondensedRegular.ttf");
    FSlateFontInfo Font(FPaths::ProjectContentDir() / FontFile, Size);
    Font.LetterSpacing = bEmphasis ? 65 : 24;
    Label->SetFont(Font);
    Label->SetShadowOffset(FVector2D(2.0f, 2.0f));
    Label->SetShadowColorAndOpacity(FLinearColor::Black);
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
    const FMargin Padding = FMargin(2.0f))
{
    UBorder* Metal = Tree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), FName(Name.ToString() + TEXT("_Metal")));
    Metal->SetBrushColor(FLinearColor(0.15f, 0.16f, 0.17f, 0.98f));
    Metal->SetPadding(FMargin(2.0f));

    UBorder* Edge = Tree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), FName(Name.ToString() + TEXT("_Edge")));
    Edge->SetBrushColor(FLinearColor(Accent.R * 0.48f, Accent.G * 0.48f, Accent.B * 0.48f, 1.0f));
    Edge->SetPadding(FMargin(2.0f));
    Metal->SetContent(Edge);

    UBorder* Interior = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
    Interior->SetBrushColor(PanelBlack);
    Interior->SetPadding(Padding);
    Interior->SetContent(Content);
    Edge->SetContent(Interior);
    return Metal;
}

UImage* MakeCroppedImage(
    UWidgetTree* Tree,
    UTexture2D* Texture,
    const FName Name,
    const FVector2D UVMin,
    const FVector2D UVMax)
{
    UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
    FSlateBrush Brush;
    Brush.SetResourceObject(Texture);
    Brush.DrawAs = ESlateBrushDrawType::Image;
    Brush.ImageSize = FVector2D(300.0f, 520.0f);
    Brush.SetUVRegion(FBox2f(FVector2f(UVMin), FVector2f(UVMax)));
    Image->SetBrush(Brush);
    return Image;
}
}

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
    EntranceElapsed = 0.0f;
    if (MainCanvas)
    {
        MainCanvas->SetRenderOpacity(0.0f);
        MainCanvas->SetRenderScale(FVector2D(1.025f, 1.025f));
    }
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            EntranceTimer, this, &URA4CampaignSelectWidget::AnimateEntrance, 1.0f / 60.0f, true);
    }
    SelectFaction(0);
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
    const float Alpha = FMath::Clamp(EntranceElapsed / 0.38f, 0.0f, 1.0f);
    const float Eased = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);
    if (MainCanvas)
    {
        MainCanvas->SetRenderOpacity(Eased);
        MainCanvas->SetRenderScale(FVector2D(FMath::Lerp(1.025f, 1.0f, Eased)));
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
    Background->SetColorAndOpacity(FLinearColor(0.38f, 0.34f, 0.34f, 1.0f));
    UOverlaySlot* BackgroundSlot = Root->AddChildToOverlay(Background);
    BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
    BackgroundSlot->SetVerticalAlignment(VAlign_Fill);

    UBorder* Shade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CampaignShade"));
    Shade->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.58f));
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

    UImage* Logo = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CampaignLogo"));
    if (UTexture2D* LogoTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_Logo.T_RA4_Logo")))
    {
        Logo->SetBrushFromTexture(LogoTexture, false);
    }
    Place(MainCanvas, Logo, FVector2D(26.0f, 10.0f), FVector2D(420.0f, 136.0f), 4);

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
            ? FLinearColor(0.34f, 0.012f, 0.018f, 0.96f)
            : FLinearColor(0.01f, 0.012f, 0.016f, 0.84f)));
        Style.SetHovered(FSlateColorBrush(FLinearColor(0.25f, 0.012f, 0.018f, 1.0f)));
        Style.SetPressed(FSlateColorBrush(SovietRed));
        Style.SetDisabled(FSlateColorBrush(FLinearColor(0.30f, 0.012f, 0.018f, 0.94f)));
        Button->SetStyle(Style);
        UTextBlock* LabelText = MakeText(
            WidgetTree, Label, 15, bActive ? TextPrimary : TextMuted,
            FName(Name.ToString() + TEXT("_Label")));
        LabelText->SetJustification(ETextJustify::Center);
        Button->AddChild(LabelText);
        UHorizontalBoxSlot* Slot = Navigation->AddChildToHorizontalBox(Button);
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Slot->SetPadding(FMargin(4.0f, 8.0f));
        return Button;
    };

    AddNav(LOCTEXT("MainNav", "GLAVNAYa"), TEXT("MainNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenMainMenu);
    UButton* CampaignNav = AddNav(
        LOCTEXT("CampaignNav", "KAMPANIYa"), TEXT("CampaignNavButton"), true);
    CampaignNav->SetIsEnabled(false);
    AddNav(LOCTEXT("NetworkNav", "SETEVAYa IGRA"), TEXT("NetworkNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenMultiplayer);
    AddNav(LOCTEXT("ChallengesNav", "ISPYTANIYa"), TEXT("ChallengesNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenChallenges);
    AddNav(LOCTEXT("BarracksNav", "BARRACKS"), TEXT("BarracksNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenBarracks);
    AddNav(LOCTEXT("SettingsNav", "NASTROYKI"), TEXT("SettingsNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenSettings);
    Place(
        MainCanvas,
        MakePanel(WidgetTree, Navigation, TEXT("CampaignNavigationFrame"), SovietRed, FMargin(4.0f)),
        FVector2D(470.0f, 16.0f), FVector2D(990.0f, 72.0f), 5);

    UVerticalBox* Profile = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CampaignProfile"));
    Profile->AddChildToVerticalBox(MakeText(
        WidgetTree, LOCTEXT("ProfileCommander", "TOVARIShch KOMANDIR"), 15, SovietRed, TEXT("ProfileCommander")));
    // Level and XP ratio must match the SC-02 commander card (Level 25,
    // 45,780 / 75,000): the same profile is shown on both screens.
    Profile->AddChildToVerticalBox(MakeText(
        WidgetTree, LOCTEXT("ProfileLevel", "UROVEN 25  //  SET PODKLYuChENA"), 11, TextMuted, TEXT("ProfileLevel"), false));
    UProgressBar* ProfileProgress = WidgetTree->ConstructWidget<UProgressBar>(
        UProgressBar::StaticClass(), TEXT("ProfileProgress"));
    ProfileProgress->SetPercent(45780.0f / 75000.0f);
    ProfileProgress->SetFillColorAndOpacity(SovietRed);
    Profile->AddChildToVerticalBox(ProfileProgress)->SetPadding(FMargin(0.0f, 8.0f));
    Place(
        MainCanvas,
        MakePanel(WidgetTree, Profile, TEXT("CampaignProfileFrame"), SovietRed, FMargin(16.0f, 10.0f)),
        FVector2D(1480.0f, 16.0f), FVector2D(410.0f, 82.0f), 5);

    UTextBlock* Title = MakeText(
        WidgetTree, LOCTEXT("CampaignTitle", "VYBOR KAMPANII"), 52, TextPrimary, TEXT("CampaignTitle"));
    Title->SetJustification(ETextJustify::Center);
    Place(MainCanvas, Title, FVector2D(505.0f, 118.0f), FVector2D(910.0f, 72.0f), 5);
    UBorder* TitleRule = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CampaignTitleRule"));
    TitleRule->SetBrushColor(SovietRed);
    Place(MainCanvas, TitleRule, FVector2D(610.0f, 202.0f), FVector2D(700.0f, 2.0f), 5);

    UTexture2D* ReferenceTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_CampaignReference.T_RA4_CampaignReference"));
    const FVector2D PortraitUVMin[] = {
        FVector2D(99.0f / 1672.0f, 280.0f / 941.0f),
        FVector2D(417.0f / 1672.0f, 280.0f / 941.0f),
        FVector2D(729.0f / 1672.0f, 280.0f / 941.0f),
        FVector2D(1044.0f / 1672.0f, 280.0f / 941.0f)
    };
    const FVector2D PortraitUVMax[] = {
        FVector2D(354.0f / 1672.0f, 706.0f / 941.0f),
        FVector2D(669.0f / 1672.0f, 706.0f / 941.0f),
        FVector2D(982.0f / 1672.0f, 706.0f / 941.0f),
        FVector2D(1295.0f / 1672.0f, 706.0f / 941.0f)
    };
    const FText FactionLabels[] = {
        LOCTEXT("USSRCard", "Soviet"),
        LOCTEXT("AllianceCard", "ALYaNS"),
        LOCTEXT("EasternCard", "VOSTOChNAYa\nKOALITsIYa"),
        LOCTEXT("ChronoCard", "KhRONOLEGION")
    };
    const FLinearColor FactionAccents[] = {
        SovietRed, AllianceBlue, EasternGold, ChronoViolet
    };
    const FName CardNames[] = {
        TEXT("USSRCard"), TEXT("AllianceCard"), TEXT("EasternCard"), TEXT("ChronoCard")
    };
    const float CardX[] = { 72.0f, 400.0f, 728.0f, 1056.0f };

    CardFrames.Reset();
    for (int32 Index = 0; Index < 4; ++Index)
    {
        UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), FName(CardNames[Index].ToString() + TEXT("_Content")));
        UImage* Portrait = MakeCroppedImage(
            WidgetTree, ReferenceTexture,
            FName(CardNames[Index].ToString() + TEXT("_Portrait")),
            PortraitUVMin[Index], PortraitUVMax[Index]);
        UVerticalBoxSlot* PortraitSlot = Card->AddChildToVerticalBox(Portrait);
        PortraitSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

        UTextBlock* CardLabel = MakeText(
            WidgetTree, FactionLabels[Index], Index == 2 ? 25 : 29,
            FactionAccents[Index], FName(CardNames[Index].ToString() + TEXT("_Label")));
        CardLabel->SetJustification(ETextJustify::Center);
        Card->AddChildToVerticalBox(CardLabel)->SetPadding(FMargin(4.0f, 16.0f, 4.0f, 18.0f));

        UButton* CardButton = WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(), FName(CardNames[Index].ToString() + TEXT("_Button")));
        FButtonStyle CardStyle;
        CardStyle.SetNormal(FSlateColorBrush(FLinearColor(0.005f, 0.006f, 0.008f, 0.98f)));
        CardStyle.SetHovered(FSlateColorBrush(FLinearColor(
            FactionAccents[Index].R * 0.16f,
            FactionAccents[Index].G * 0.16f,
            FactionAccents[Index].B * 0.16f,
            1.0f)));
        CardStyle.SetPressed(FSlateColorBrush(FLinearColor(
            FactionAccents[Index].R * 0.32f,
            FactionAccents[Index].G * 0.32f,
            FactionAccents[Index].B * 0.32f,
            1.0f)));
        CardButton->SetStyle(CardStyle);
        CardButton->AddChild(Card);

        switch (Index)
        {
        case 0:
            CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::SelectUSSR);
            break;
        case 1:
            CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::SelectAlliance);
            break;
        case 2:
            CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::SelectEasternCoalition);
            break;
        case 3:
            CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::SelectChronolegion);
            break;
        default:
            checkNoEntry();
            break;
        }

        UBorder* CardFrame = MakePanel(
            WidgetTree, CardButton, FName(CardNames[Index].ToString() + TEXT("_Frame")),
            FactionAccents[Index], FMargin(0.0f));
        CardFrames.Add(CardFrame);
        Place(MainCanvas, CardFrame, FVector2D(CardX[Index], 274.0f), FVector2D(300.0f, 638.0f), 5);
    }

    UVerticalBox* Information = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CampaignInformation"));
    UTextBlock* InformationHeader = MakeText(
        WidgetTree, LOCTEXT("InformationHeader", "O VYBRANNOY KAMPANII"), 17,
        SovietRed, TEXT("InformationHeader"));
    InformationHeader->SetJustification(ETextJustify::Center);
    Information->AddChildToVerticalBox(InformationHeader)->SetPadding(FMargin(8.0f, 12.0f, 8.0f, 20.0f));
    FactionNameText = MakeText(WidgetTree, FText::GetEmpty(), 28, TextPrimary, TEXT("FactionName"));
    Information->AddChildToVerticalBox(FactionNameText)->SetPadding(FMargin(22.0f, 8.0f, 22.0f, 4.0f));
    FactionMottoText = MakeText(WidgetTree, FText::GetEmpty(), 13, TextMuted, TEXT("FactionMotto"), false);
    Information->AddChildToVerticalBox(FactionMottoText)->SetPadding(FMargin(22.0f, 0.0f, 22.0f, 18.0f));
    FactionDescriptionText = MakeText(
        WidgetTree, FText::GetEmpty(), 15, TextPrimary, TEXT("FactionDescription"), false);
    FactionDescriptionText->SetAutoWrapText(true);
    UVerticalBoxSlot* DescriptionSlot = Information->AddChildToVerticalBox(FactionDescriptionText);
    DescriptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    DescriptionSlot->SetPadding(FMargin(22.0f, 8.0f, 22.0f, 18.0f));
    UTextBlock* ProgressHeader = MakeText(
        WidgetTree, LOCTEXT("ProgressHeader", "PROGRESS KAMPANII"), 16,
        SovietRed, TEXT("ProgressHeader"));
    Information->AddChildToVerticalBox(ProgressHeader)->SetPadding(FMargin(22.0f, 8.0f, 22.0f, 10.0f));
    CampaignProgressText = MakeText(
        WidgetTree, FText::GetEmpty(), 13, TextMuted, TEXT("CampaignProgress"), false);
    Information->AddChildToVerticalBox(CampaignProgressText)->SetPadding(FMargin(22.0f, 0.0f, 22.0f, 14.0f));

    UButton* ContinueButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("ContinueCampaignButton"));
    FButtonStyle ContinueStyle;
    ContinueStyle.SetNormal(FSlateColorBrush(FLinearColor(0.08f, 0.012f, 0.016f, 0.98f)));
    ContinueStyle.SetHovered(FSlateColorBrush(FLinearColor(0.34f, 0.015f, 0.020f, 1.0f)));
    ContinueStyle.SetPressed(FSlateColorBrush(SovietRed));
    ContinueButton->SetStyle(ContinueStyle);
    ContinueLabelText = MakeText(
        WidgetTree, FText::GetEmpty(), 15, TextPrimary, TEXT("ContinueCampaignLabel"));
    ContinueLabelText->SetJustification(ETextJustify::Center);
    ContinueButton->AddChild(ContinueLabelText);
    ContinueButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::ContinueCampaign);
    UVerticalBoxSlot* ContinueSlot = Information->AddChildToVerticalBox(ContinueButton);
    ContinueSlot->SetPadding(FMargin(18.0f, 8.0f, 18.0f, 18.0f));

    Place(
        MainCanvas,
        MakePanel(WidgetTree, Information, TEXT("CampaignInformationFrame"), SovietRed, FMargin(4.0f)),
        FVector2D(1384.0f, 274.0f), FVector2D(502.0f, 638.0f), 5);

    UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CampaignFooter"));
    UButton* BackButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("CampaignBackButton"));
    BackButton->AddChild(MakeText(
        WidgetTree, LOCTEXT("BackButton", "<  BACK"), 17, TextPrimary, TEXT("CampaignBackLabel")));
    BackButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenMainMenu);
    Footer->AddChildToHorizontalBox(BackButton)->SetPadding(FMargin(8.0f));
    UButton* TrainingButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("TrainingButton"));
    TrainingButton->AddChild(MakeText(
        WidgetTree, LOCTEXT("Training", "OBUChENIE"), 17, TextPrimary, TEXT("TrainingLabel")));
    TrainingButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenChallenges);
    Footer->AddChildToHorizontalBox(TrainingButton)->SetPadding(FMargin(8.0f));
    UTextBlock* EraText = MakeText(
        WidgetTree, LOCTEXT("Era", "1927  —  2047  //  ARKhIV KOMANDOVANIYa"),
        13, TextMuted, TEXT("CampaignEra"), false);
    EraText->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* EraSlot = Footer->AddChildToHorizontalBox(EraText);
    EraSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    EraSlot->SetVerticalAlignment(VAlign_Center);
    Place(
        MainCanvas,
        MakePanel(WidgetTree, Footer, TEXT("CampaignFooterFrame"), SovietRed, FMargin(6.0f)),
        FVector2D(22.0f, 958.0f), FVector2D(1876.0f, 94.0f), 5);
}

void URA4CampaignSelectWidget::SelectFaction(const int32 FactionIndex)
{
    SelectedFactionIndex = FMath::Clamp(FactionIndex, 0, 3);
    const FLinearColor Accents[] = {
        SovietRed, AllianceBlue, EasternGold, ChronoViolet
    };
    for (int32 Index = 0; Index < CardFrames.Num(); ++Index)
    {
        const FLinearColor Accent = Accents[Index];
        CardFrames[Index]->SetBrushColor(Index == SelectedFactionIndex
            ? FLinearColor(
                FMath::Min(Accent.R * 1.45f, 1.0f),
                FMath::Min(Accent.G * 1.45f, 1.0f),
                FMath::Min(Accent.B * 1.45f, 1.0f),
                1.0f)
            : FLinearColor(0.12f, 0.13f, 0.14f, 0.98f));
    }

    const FText Names[] = {
        LOCTEXT("USSRName", "Soviet"),
        LOCTEXT("AllianceName", "ALYaNS"),
        LOCTEXT("EasternName", "VOSTOChNAYa KOALITsIYa"),
        LOCTEXT("ChronoName", "KhRONOLEGION")
    };
    const FText Mottos[] = {
        LOCTEXT("USSRMotto", "SLAVA RODINE. BUDUShchEE ZA NAMI."),
        LOCTEXT("AllianceMotto", "SVOBODA. TOChNOST. PREVOSKhODSTVO."),
        LOCTEXT("EasternMotto", "EDINSTVO SOZDAYoT POBEDU."),
        LOCTEXT("ChronoMotto", "VREMYa — NAShE ORUZhIE.")
    };
    const FText Descriptions[] = {
        LOCTEXT("USSRDescription", "Vozglavte vozrozhdyonnyy Sovetskiy Soyuz v borbe za mirovoe gospodstvo. Tyazhyolaya bronya, distsiplina i nesokrushimaya volya sokrushat vragov revolyutsii."),
        LOCTEXT("AllianceDescription", "Soberite koalitsiyu demokraticheskikh derzhav. Ispolzuyte aviatsiyu, vysokotochnoe oruzhie i mobilnye sily dlya zashchity svobodnogo mira."),
        LOCTEXT("EasternDescription", "Obedinite promyshlennuyu moshch Vostoka. Razvivayte proizvodstvo, boevye mekhanizmy i kontrol energeticheskikh uzlov."),
        LOCTEXT("ChronoDescription", "Komanduyte armiey vne vremeni. Iskazhayte pole boya, peremeshchayte voyska cherez khronokoridory i perepisyvayte iskhod voyny.")
    };
    const FText Progress[] = {
        LOCTEXT("USSRProgress", "MISSII PROYDENO          06 / 18\nDOP. ZADANIYa             09 / 36\nDIFFICULTY                VETERAN"),
        LOCTEXT("AllianceProgress", "MISSII PROYDENO          02 / 16\nDOP. ZADANIYa             03 / 28\nDIFFICULTY                OFITsER"),
        LOCTEXT("EasternProgress", "MISSII PROYDENO          00 / 15\nDOP. ZADANIYa             00 / 30\nDIFFICULTY                NE VYBRANA"),
        LOCTEXT("ChronoProgress", "MISSII PROYDENO          00 / 12\nVREMENNYE UZLY           00 / 24\nDIFFICULTY                NE VYBRANA")
    };
    if (FactionNameText)
    {
        FactionNameText->SetText(Names[SelectedFactionIndex]);
        FactionNameText->SetColorAndOpacity(FSlateColor(Accents[SelectedFactionIndex]));
    }
    if (FactionMottoText)
    {
        FactionMottoText->SetText(Mottos[SelectedFactionIndex]);
    }
    if (FactionDescriptionText)
    {
        FactionDescriptionText->SetText(Descriptions[SelectedFactionIndex]);
    }
    if (CampaignProgressText)
    {
        CampaignProgressText->SetText(Progress[SelectedFactionIndex]);
    }
    if (ContinueLabelText)
    {
        ContinueLabelText->SetText(SelectedFactionIndex == 0
            ? LOCTEXT("ContinueCampaign", "CONTINUE KAMPANIYu")
            : LOCTEXT("StartCampaign", "NAChAT KAMPANIYu"));
    }
}

void URA4CampaignSelectWidget::SelectUSSR() { SelectFaction(0); }
void URA4CampaignSelectWidget::SelectAlliance() { SelectFaction(1); }
void URA4CampaignSelectWidget::SelectEasternCoalition() { SelectFaction(2); }
void URA4CampaignSelectWidget::SelectChronolegion() { SelectFaction(3); }

void URA4CampaignSelectWidget::ContinueCampaign()
{
    const int32 TargetScreens[] = { 10, 7, 8, 9 };
    NavigateToScreen(TargetScreens[SelectedFactionIndex]);
}

void URA4CampaignSelectWidget::NavigateToScreen(const int32 ScreenIndex)
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4ShowcaseWidget* Screen = CreateWidget<URA4ShowcaseWidget>(
            PlayerController, URA4ShowcaseWidget::StaticClass()))
        {
            Screen->SetInitialScreenForPresentation(ScreenIndex);
            Screen->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

void URA4CampaignSelectWidget::OpenMainMenu()
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

void URA4CampaignSelectWidget::OpenMultiplayer() { NavigateToScreen(3); }
void URA4CampaignSelectWidget::OpenChallenges() { NavigateToScreen(23); }
void URA4CampaignSelectWidget::OpenBarracks() { NavigateToScreen(19); }
void URA4CampaignSelectWidget::OpenSettings() { NavigateToScreen(4); }

#undef LOCTEXT_NAMESPACE
