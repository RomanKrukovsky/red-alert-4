// Copyright (c) Red Alert 4 project.

#include "RA4CommandCentreMenuWidget.h"

#include "RA4CampaignSelectWidget.h"
#include "RA4ShowcaseWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/AudioComponent.h"
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
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Sound/SoundBase.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "RA4MainMenuWidget"

namespace
{
const FVector2D ReferenceSize(1920.0f, 1080.0f);
constexpr FLinearColor Red(0.95f, 0.035f, 0.04f, 1.0f);
constexpr FLinearColor RedDim(0.34f, 0.012f, 0.016f, 1.0f);
constexpr FLinearColor MetalEdge(0.16f, 0.17f, 0.18f, 0.98f);
constexpr FLinearColor Panel(0.009f, 0.009f, 0.011f, 0.94f);
constexpr FLinearColor PanelSoft(0.018f, 0.018f, 0.021f, 0.90f);
constexpr FLinearColor Text(0.86f, 0.82f, 0.79f, 1.0f);
constexpr FLinearColor Muted(0.52f, 0.49f, 0.47f, 1.0f);

UTextBlock* MakeText(UWidgetTree* Tree, const FText& Value, const int32 Size, const FLinearColor& Color,
                     const FName Name, const bool bHeavy = true)
{
    UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    Label->SetText(Value);
    Label->SetColorAndOpacity(FSlateColor(Color));
    Label->SetJustification(ETextJustify::Left);

    const TCHAR* FontFile = bHeavy
        ? TEXT("RA4UI/Fonts/RA4_RobotoCondensedSemiBold.ttf")
        : TEXT("RA4UI/Fonts/RA4_RobotoCondensedRegular.ttf");
    FSlateFontInfo Font(FPaths::ProjectContentDir() / FontFile, Size);
    Font.LetterSpacing = bHeavy ? 55 : 22;
    Font.OutlineSettings.OutlineSize = 0;
    Font.OutlineSettings.OutlineColor = FLinearColor(0.02f, 0.01f, 0.01f, 0.95f);
    Label->SetFont(Font);
    Label->SetShadowOffset(FVector2D(2.0f, 2.0f));
    Label->SetShadowColorAndOpacity(FLinearColor::Black);
    return Label;
}

void PlaceOnCanvas(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D Position, const FVector2D Size,
                   const int32 ZOrder = 0)
{
    UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
    Slot->SetPosition(Position);
    Slot->SetSize(Size);
    Slot->SetAnchors(FAnchors(0.0f, 0.0f));
    Slot->SetAlignment(FVector2D::ZeroVector);
    Slot->SetZOrder(ZOrder);
}

UBorder* MakeFramedPanel(UWidgetTree* Tree, UWidget* Content, const FName Name, const FMargin Padding = FMargin(2.0f))
{
    UBorder* Metal = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(Name.ToString() + TEXT("_Metal")));
    Metal->SetBrushColor(MetalEdge);
    Metal->SetPadding(FMargin(2.0f));

    UBorder* Glow = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(Name.ToString() + TEXT("_Glow")));
    Glow->SetBrushColor(RedDim);
    Glow->SetPadding(FMargin(2.0f));
    Metal->SetContent(Glow);

    UBorder* Interior = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
    Interior->SetBrushColor(Panel);
    Interior->SetPadding(Padding);
    Glow->SetContent(Interior);
    Interior->SetContent(Content);
    return Metal;
}

UTexture2D* LoadCommandCentre()
{
    return LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_USSR_MainMenuBackground.T_RA4_USSR_MainMenuBackground"));
}

UImage* MakeCroppedArt(UWidgetTree* Tree, UTexture2D* Texture, const FName Name,
                       const FVector2D UVMin, const FVector2D UVMax)
{
    UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
    FSlateBrush Brush;
    Brush.SetResourceObject(Texture);
    Brush.DrawAs = ESlateBrushDrawType::Image;
    Brush.ImageSize = FVector2D(360.0f, 92.0f);
    Brush.SetUVRegion(FBox2f(FVector2f(UVMin), FVector2f(UVMax)));
    Image->SetBrush(Brush);
    return Image;
}

UImage* MakeChromeSection(UWidgetTree* Tree, UTexture2D* Texture, const FName Name,
                          const FVector2D UVMin, const FVector2D UVMax)
{
    UImage* Image = MakeCroppedArt(Tree, Texture, Name, UVMin, UVMax);
    Image->SetVisibility(ESlateVisibility::HitTestInvisible);
    return Image;
}
}

TSharedRef<SWidget> URA4CommandCentreMenuWidget::RebuildWidget()
{
    if (WidgetTree)
    {
        BuildLayout();
    }
    return Super::RebuildWidget();
}

void URA4CommandCentreMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    EntranceElapsed = 0.0f;
    if (MainCanvas)
    {
        MainCanvas->SetRenderOpacity(0.0f);
        MainCanvas->SetRenderTranslation(FVector2D(-28.0f, 0.0f));
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            EntranceTimer, this, &URA4CommandCentreMenuWidget::AnimateEntrance, 1.0f / 60.0f, true);
    }

    if (USoundBase* Theme = LoadObject<USoundBase>(
        nullptr, TEXT("/Game/RA4UI/Audio/S_RA4_MainMenu_Theme.S_RA4_MainMenu_Theme")))
    {
        MenuMusic = UGameplayStatics::SpawnSound2D(
            this, Theme, 0.62f, 1.0f, 0.0f, nullptr, false, true);
        if (MenuMusic)
        {
            MenuMusic->FadeIn(1.8f, 0.62f);
        }
    }
}

void URA4CommandCentreMenuWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(EntranceTimer);
    }
    if (MenuMusic)
    {
        MenuMusic->FadeOut(0.65f, 0.0f);
        MenuMusic = nullptr;
    }
    Super::NativeDestruct();
}

void URA4CommandCentreMenuWidget::AnimateEntrance()
{
    EntranceElapsed += 1.0f / 60.0f;
    const float Alpha = FMath::Clamp(EntranceElapsed / 0.42f, 0.0f, 1.0f);
    const float Eased = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);

    if (MainCanvas)
    {
        MainCanvas->SetRenderOpacity(Eased);
        MainCanvas->SetRenderTranslation(FVector2D(FMath::Lerp(-28.0f, 0.0f, Eased), 0.0f));
    }

    if (Alpha >= 1.0f)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(EntranceTimer);
        }
    }
}

void URA4CommandCentreMenuWidget::BuildLayout()
{
    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MainMenuRoot"));
    WidgetTree->RootWidget = Root;

    UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CommandCentreBackground"));
    UTexture2D* CommandCentreTexture = LoadCommandCentre();
    if (CommandCentreTexture)
    {
        Background->SetBrushFromTexture(CommandCentreTexture, false);
    }
    Background->SetColorAndOpacity(FLinearColor(0.72f, 0.72f, 0.72f, 1.0f));
    UOverlaySlot* BackgroundSlot = Root->AddChildToOverlay(Background);
    BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
    BackgroundSlot->SetVerticalAlignment(VAlign_Fill);

    UBorder* Grade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CinematicGrade"));
    Grade->SetBrushColor(FLinearColor(0.055f, 0.0f, 0.0f, 0.20f));
    UOverlaySlot* GradeSlot = Root->AddChildToOverlay(Grade);
    GradeSlot->SetHorizontalAlignment(HAlign_Fill);
    GradeSlot->SetVerticalAlignment(VAlign_Fill);

    UScaleBox* ScaleBox = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("ResponsiveScale"));
    ScaleBox->SetStretch(EStretch::ScaleToFit);
    ScaleBox->SetStretchDirection(EStretchDirection::Both);
    UOverlaySlot* ScaleSlot = Root->AddChildToOverlay(ScaleBox);
    ScaleSlot->SetHorizontalAlignment(HAlign_Fill);
    ScaleSlot->SetVerticalAlignment(VAlign_Fill);

    USizeBox* ReferenceFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ReferenceFrame"));
    ReferenceFrame->SetWidthOverride(ReferenceSize.X);
    ReferenceFrame->SetHeightOverride(ReferenceSize.Y);
    ScaleBox->SetContent(ReferenceFrame);

    MainCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MainCanvas"));
    ReferenceFrame->SetContent(MainCanvas);

    UVerticalBox* LogoBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LogoBox"));
    UTextBlock* Crest = MakeText(WidgetTree, LOCTEXT("Crest", "V"), 36, Red, TEXT("CommandCrest"));
    Crest->SetJustification(ETextJustify::Center);
    LogoBox->AddChildToVerticalBox(Crest);
    UTextBlock* Logo = MakeText(WidgetTree, LOCTEXT("Logo", "RED ALERT 4"), 86, Red, TEXT("GameLogo"));
    Logo->SetJustification(ETextJustify::Center);
    FSlateFontInfo LogoFont = Logo->GetFont();
    LogoFont.OutlineSettings.OutlineSize = 3;
    LogoFont.OutlineSettings.OutlineColor = FLinearColor(0.28f, 0.0f, 0.0f, 1.0f);
    Logo->SetFont(LogoFont);
    Logo->SetShadowOffset(FVector2D(5.0f, 6.0f));
    Logo->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
    LogoBox->AddChildToVerticalBox(Logo);
    UTextBlock* LogoRule = MakeText(
        WidgetTree, LOCTEXT("LogoRule", "==========  V  =========="), 17, RedDim, TEXT("LogoRule"));
    LogoRule->SetJustification(ETextJustify::Center);
    LogoBox->AddChildToVerticalBox(LogoRule);
    PlaceOnCanvas(MainCanvas, LogoBox, FVector2D(560.0f, 40.0f), FVector2D(900.0f, 190.0f), 4);

    if (UTexture2D* LogoTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_Logo.T_RA4_Logo")))
    {
        LogoBox->SetVisibility(ESlateVisibility::Collapsed);
        UImage* LogoImage = WidgetTree->ConstructWidget<UImage>(
            UImage::StaticClass(), TEXT("ProductionLogo"));
        LogoImage->SetBrushFromTexture(LogoTexture, false);
        LogoImage->SetVisibility(ESlateVisibility::HitTestInvisible);
        PlaceOnCanvas(
            MainCanvas, LogoImage, FVector2D(585.0f, 24.0f), FVector2D(850.0f, 283.0f), 8);
    }

    UTexture2D* MenuIconTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_MenuIcons.T_RA4_MenuIcons"));

    UVerticalBox* MenuContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuContent"));
    USizeBox* FactionMarkSize = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("FactionMarkSize"));
    FactionMarkSize->SetHeightOverride(70.0f);
    FactionMarkSize->SetContent(MakeCroppedArt(
        WidgetTree, MenuIconTexture, TEXT("FactionMark"),
        FVector2D(0.0f, 0.0f), FVector2D(0.25f, 0.5f)));
    MenuContent->AddChildToVerticalBox(FactionMarkSize)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
    UTextBlock* FactionLabel = MakeText(
        WidgetTree, LOCTEXT("FactionLabel", "СОВЕТСКОЕ ВЕРХОВНОЕ КОМАНДОВАНИЕ"), 12, Muted, TEXT("FactionLabel"));
    FactionLabel->SetJustification(ETextJustify::Center);
    MenuContent->AddChildToVerticalBox(FactionLabel)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));

    UTexture2D* ButtonNormalTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_Button_Normal.T_RA4_Button_Normal"));
    UTexture2D* ButtonHoveredTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_Button_Hovered.T_RA4_Button_Hovered"));
    UTexture2D* ButtonPressedTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_Button_Pressed.T_RA4_Button_Pressed"));
    UTexture2D* ButtonDisabledTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_Button_Disabled.T_RA4_Button_Disabled"));

    const auto AddMenuButton = [
        this, MenuContent, MenuIconTexture, ButtonNormalTexture, ButtonHoveredTexture,
        ButtonPressedTexture, ButtonDisabledTexture](
        const FVector2D IconUVMin, const FVector2D IconUVMax,
        const FText& Label, const FName Name, const bool bSelected) -> UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
        FButtonStyle Style;
        if (ButtonNormalTexture && ButtonHoveredTexture && ButtonPressedTexture && ButtonDisabledTexture)
        {
            FSlateBrush NormalBrush;
            NormalBrush.SetResourceObject(bSelected ? ButtonHoveredTexture : ButtonNormalTexture);
            NormalBrush.DrawAs = ESlateBrushDrawType::Image;
            NormalBrush.ImageSize = FVector2D(1024.0f, 128.0f);
            FSlateBrush HoveredBrush = NormalBrush;
            HoveredBrush.SetResourceObject(ButtonHoveredTexture);
            FSlateBrush PressedBrush = NormalBrush;
            PressedBrush.SetResourceObject(ButtonPressedTexture);
            FSlateBrush DisabledBrush = NormalBrush;
            DisabledBrush.SetResourceObject(ButtonDisabledTexture);
            Style.SetNormal(NormalBrush);
            Style.SetHovered(HoveredBrush);
            Style.SetPressed(PressedBrush);
            Style.SetDisabled(DisabledBrush);
        }
        else
        {
            Style.SetNormal(FSlateColorBrush(FLinearColor(0.018f, 0.014f, 0.015f, 0.98f)));
            Style.SetHovered(FSlateColorBrush(FLinearColor(0.19f, 0.012f, 0.016f, 1.0f)));
            Style.SetPressed(FSlateColorBrush(FLinearColor(0.48f, 0.018f, 0.022f, 1.0f)));
            Style.SetDisabled(FSlateColorBrush(FLinearColor(0.01f, 0.01f, 0.01f, 0.45f)));
        }
        Style.NormalPadding = FMargin(0.0f);
        Style.PressedPadding = FMargin(2.0f, 2.0f, 0.0f, 0.0f);
        Button->SetStyle(Style);

        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(), FName(Name.ToString() + TEXT("_Row")));
        USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(), FName(Name.ToString() + TEXT("_IconSize")));
        IconSize->SetWidthOverride(96.0f);
        UBorder* IconCell = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), FName(Name.ToString() + TEXT("_IconCell")));
        IconCell->SetBrushColor(FLinearColor(0.09f, 0.015f, 0.018f, 0.92f));
        IconCell->SetPadding(FMargin(13.0f, 9.0f));
        UImage* IconImage = MakeCroppedArt(
            WidgetTree, MenuIconTexture, FName(Name.ToString() + TEXT("_Icon")),
            IconUVMin, IconUVMax);
        IconImage->SetColorAndOpacity(FLinearColor(0.92f, 0.80f, 0.73f, 1.0f));
        IconCell->SetContent(IconImage);
        IconSize->SetContent(IconCell);
        Row->AddChildToHorizontalBox(IconSize);

        UBorder* Divider = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), FName(Name.ToString() + TEXT("_Divider")));
        Divider->SetBrushColor(RedDim);
        UHorizontalBoxSlot* DividerSlot = Row->AddChildToHorizontalBox(Divider);
        DividerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        DividerSlot->SetPadding(FMargin(0.0f));
        Divider->SetPadding(FMargin(1.0f, 0.0f));

        UTextBlock* ButtonLabel = MakeText(
            WidgetTree, Label, 24, Text, FName(Name.ToString() + TEXT("_Label")));
        UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(ButtonLabel);
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
        LabelSlot->SetPadding(FMargin(28.0f, 0.0f, 12.0f, 0.0f));
        Button->AddChild(Row);

        USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(), FName(Name.ToString() + TEXT("_Size")));
        ButtonSize->SetHeightOverride(58.0f);
        ButtonSize->SetContent(Button);
        UBorder* ButtonFrame = MakeFramedPanel(
            WidgetTree, ButtonSize, FName(Name.ToString() + TEXT("_Frame")), FMargin(0.0f));
        MenuContent->AddChildToVerticalBox(ButtonFrame)->SetPadding(FMargin(12.0f, 3.0f));
        return Button;
    };

    AddMenuButton(
        FVector2D(0.0f, 0.0f), FVector2D(0.25f, 0.5f),
        LOCTEXT("Campaign", "КАМПАНИЯ"), TEXT("CampaignButton"), true)
        ->OnClicked.AddDynamic(this, &URA4CommandCentreMenuWidget::OpenCampaign);
    AddMenuButton(
        FVector2D(0.25f, 0.0f), FVector2D(0.5f, 0.5f),
        LOCTEXT("Network", "СЕТЕВАЯ ИГРА"), TEXT("NetworkButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CommandCentreMenuWidget::OpenMultiplayer);
    AddMenuButton(
        FVector2D(0.5f, 0.0f), FVector2D(0.75f, 0.5f),
        LOCTEXT("Skirmish", "СХВАТКА"), TEXT("SkirmishButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CommandCentreMenuWidget::OpenSkirmish);
    AddMenuButton(
        FVector2D(0.75f, 0.0f), FVector2D(1.0f, 0.5f),
        LOCTEXT("Editor", "РЕДАКТОР"), TEXT("EditorButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CommandCentreMenuWidget::OpenEditor);
    AddMenuButton(
        FVector2D(0.0f, 0.5f), FVector2D(0.25f, 1.0f),
        LOCTEXT("Codex", "ЭНЦИКЛОПЕДИЯ"), TEXT("CodexButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CommandCentreMenuWidget::OpenEncyclopedia);
    AddMenuButton(
        FVector2D(0.25f, 0.5f), FVector2D(0.5f, 1.0f),
        LOCTEXT("Modifications", "МОДИФИКАЦИИ"), TEXT("ModificationsButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CommandCentreMenuWidget::OpenModifications);
    AddMenuButton(
        FVector2D(0.5f, 0.5f), FVector2D(0.75f, 1.0f),
        LOCTEXT("Settings", "НАСТРОЙКИ"), TEXT("SettingsButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CommandCentreMenuWidget::OpenSettings);
    AddMenuButton(
        FVector2D(0.75f, 0.5f), FVector2D(1.0f, 1.0f),
        LOCTEXT("Exit", "ВЫХОД"), TEXT("ExitButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CommandCentreMenuWidget::RequestExit);

    UBorder* MenuPanel = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("CommandMenu"));
    MenuPanel->SetBrushColor(FLinearColor(0.006f, 0.006f, 0.008f, 0.88f));
    MenuPanel->SetPadding(FMargin(10.0f));
    MenuPanel->SetContent(MenuContent);
    PlaceOnCanvas(MainCanvas, MenuPanel, FVector2D(44.0f, 74.0f), FVector2D(430.0f, 660.0f), 5);

    UVerticalBox* Commander = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Commander"));
    UTextBlock* CommanderHeader = MakeText(
        WidgetTree, LOCTEXT("CommanderHeader", "КОМАНДУЮЩИЙ"), 16, Text, TEXT("CommanderHeader"));
    CommanderHeader->SetJustification(ETextJustify::Center);
    Commander->AddChildToVerticalBox(CommanderHeader)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 6.0f));

    UHorizontalBox* CommanderRow = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CommanderRow"));
    USizeBox* RankMark = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("RankMark"));
    RankMark->SetWidthOverride(110.0f);
    RankMark->SetHeightOverride(86.0f);
    RankMark->SetContent(MakeCroppedArt(
        WidgetTree, MenuIconTexture, TEXT("RankMarkIcon"),
        FVector2D(0.0f, 0.0f), FVector2D(0.25f, 0.5f)));
    UHorizontalBoxSlot* RankSlot = CommanderRow->AddChildToHorizontalBox(RankMark);
    RankSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    RankSlot->SetVerticalAlignment(VAlign_Center);
    RankSlot->SetPadding(FMargin(18.0f, 0.0f, 18.0f, 0.0f));
    UTextBlock* Stats = MakeText(
        WidgetTree,
        LOCTEXT("CommanderStats", "РАНГ\nРЕПУТАЦИЯ\nПОБЕДЫ\nПОРАЖЕНИЯ"),
        12, Text, TEXT("CommanderStats"), false);
    UHorizontalBoxSlot* StatsSlot = CommanderRow->AddChildToHorizontalBox(Stats);
    StatsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    StatsSlot->SetPadding(FMargin(0.0f, 2.0f, 10.0f, 2.0f));
    UTextBlock* StatValues = MakeText(
        WidgetTree,
        LOCTEXT("CommanderStatValues", "ГЕНЕРАЛ-МАЙОР\n12 450\n87\n19"),
        12, Text, TEXT("CommanderStatValues"), false);
    StatValues->SetJustification(ETextJustify::Right);
    UHorizontalBoxSlot* ValuesSlot = CommanderRow->AddChildToHorizontalBox(StatValues);
    ValuesSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ValuesSlot->SetHorizontalAlignment(HAlign_Right);
    ValuesSlot->SetPadding(FMargin(8.0f, 2.0f, 14.0f, 2.0f));
    Commander->AddChildToVerticalBox(CommanderRow)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UHorizontalBox* LevelRow = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("LevelRow"));
    LevelRow->AddChildToHorizontalBox(
        MakeText(WidgetTree, LOCTEXT("Level", "УРОВЕНЬ 27"), 15, Text, TEXT("Level")));
    UTextBlock* Experience = MakeText(
        WidgetTree, LOCTEXT("Experience", "28 750 / 34 000 ОП"), 14, Text, TEXT("Experience"));
    UHorizontalBoxSlot* ExperienceSlot = LevelRow->AddChildToHorizontalBox(Experience);
    ExperienceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ExperienceSlot->SetHorizontalAlignment(HAlign_Right);
    Commander->AddChildToVerticalBox(LevelRow)->SetPadding(FMargin(12.0f, 3.0f));
    UProgressBar* ExperienceBar = WidgetTree->ConstructWidget<UProgressBar>(
        UProgressBar::StaticClass(), TEXT("ExperienceBar"));
    ExperienceBar->SetPercent(0.845f);
    ExperienceBar->SetFillColorAndOpacity(Red);
    Commander->AddChildToVerticalBox(ExperienceBar)->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 10.0f));

    UBorder* CommanderPanel = MakeFramedPanel(WidgetTree, Commander, TEXT("CommanderPanel"), FMargin(8.0f));
    PlaceOnCanvas(MainCanvas, CommanderPanel, FVector2D(18.0f, 804.0f), FVector2D(548.0f, 254.0f), 5);

    UVerticalBox* News = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("News"));
    UTextBlock* NewsHeader = MakeText(
        WidgetTree, LOCTEXT("NewsHeader", "СВОДКА НОВОСТЕЙ"), 16, Text, TEXT("NewsHeader"));
    News->AddChildToVerticalBox(NewsHeader)->SetPadding(FMargin(16.0f, 4.0f, 0.0f, 6.0f));

    UHorizontalBox* NewsCards = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("NewsCards"));
    const auto AddNewsCard = [this, NewsCards, CommandCentreTexture](
        const FText& Heading, const FText& Copy, const FVector2D UVMin, const FVector2D UVMax, const FName Name)
    {
        UVerticalBox* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
        Card->AddChildToVerticalBox(MakeCroppedArt(
            WidgetTree, CommandCentreTexture, FName(Name.ToString() + TEXT("_Image")), UVMin, UVMax));
        Card->AddChildToVerticalBox(MakeText(
            WidgetTree, Heading, 14, Text, FName(Name.ToString() + TEXT("_Heading"))))
            ->SetPadding(FMargin(10.0f, 5.0f, 8.0f, 0.0f));
        Card->AddChildToVerticalBox(MakeText(
            WidgetTree, Copy, 11, Muted, FName(Name.ToString() + TEXT("_Copy")), false))
            ->SetPadding(FMargin(10.0f, 0.0f, 8.0f, 6.0f));
        UBorder* Frame = MakeFramedPanel(
            WidgetTree, Card, FName(Name.ToString() + TEXT("_Frame")), FMargin(0.0f));
        UHorizontalBoxSlot* CardSlot = NewsCards->AddChildToHorizontalBox(Frame);
        CardSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        CardSlot->SetPadding(FMargin(6.0f, 0.0f));
    };

    AddNewsCard(
        LOCTEXT("NewsOne", "НОВАЯ ФРАКЦИЯ: АВАНГАРД"),
        LOCTEXT("NewsOneCopy", "Технологическое превосходство.\nТактическое устрашение."),
        FVector2D(0.53f, 0.49f), FVector2D(0.86f, 0.72f), TEXT("NewsCardOne"));
    AddNewsCard(
        LOCTEXT("NewsTwo", "ОБНОВЛЕНИЕ БАЛАНСА 1.2"),
        LOCTEXT("NewsTwoCopy", "Корректировка юнитов,\nулучшения и исправления."),
        FVector2D(0.62f, 0.05f), FVector2D(0.96f, 0.32f), TEXT("NewsCardTwo"));
    AddNewsCard(
        LOCTEXT("NewsThree", "СЕЗОННЫЙ ПРОПУСК"),
        LOCTEXT("NewsThreeCopy", "Эксклюзивные награды\nи ранний доступ к контенту."),
        FVector2D(0.40f, 0.54f), FVector2D(0.74f, 0.82f), TEXT("NewsCardThree"));
    News->AddChildToVerticalBox(NewsCards)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UTextBlock* Pager = MakeText(
        WidgetTree, LOCTEXT("Pager", "=======   ==   ==   =="), 12, Red, TEXT("NewsPager"));
    Pager->SetJustification(ETextJustify::Center);
    News->AddChildToVerticalBox(Pager);

    UBorder* NewsPanel = MakeFramedPanel(WidgetTree, News, TEXT("NewsPanel"), FMargin(8.0f));
    PlaceOnCanvas(MainCanvas, NewsPanel, FVector2D(580.0f, 804.0f), FVector2D(1060.0f, 254.0f), 5);

    UTextBlock* Version = MakeText(
        WidgetTree, LOCTEXT("Version", "v1.0.0  //  RU"), 12, Muted, TEXT("Version"), false);
    Version->SetJustification(ETextJustify::Center);
    UBorder* VersionPanel = MakeFramedPanel(WidgetTree, Version, TEXT("VersionPanel"), FMargin(8.0f));
    PlaceOnCanvas(MainCanvas, VersionPanel, FVector2D(1690.0f, 1010.0f), FVector2D(210.0f, 48.0f), 5);

    if (UTexture2D* ChromeTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_USSR_MainMenuChrome.T_RA4_USSR_MainMenuChrome")))
    {
        PlaceOnCanvas(
            MainCanvas,
            MakeChromeSection(WidgetTree, ChromeTexture, TEXT("MenuChrome"),
                FVector2D(21.0f / 1672.0f, 48.0f / 941.0f),
                FVector2D(427.0f / 1672.0f, 756.0f / 941.0f)),
            FVector2D(8.0f, 8.0f), FVector2D(510.0f, 790.0f), 12);
        PlaceOnCanvas(
            MainCanvas,
            MakeChromeSection(WidgetTree, ChromeTexture, TEXT("BottomChrome"),
                FVector2D(22.0f / 1672.0f, 763.0f / 941.0f),
                FVector2D(1659.0f / 1672.0f, 910.0f / 941.0f)),
            FVector2D(8.0f, 794.0f), FVector2D(1900.0f, 272.0f), 12);
    }

    ExitModal = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ExitModal"));
    ExitModal->SetVisibility(ESlateVisibility::Collapsed);
    UBorder* ModalShade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModalShade"));
    ModalShade->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.82f));
    UOverlaySlot* ShadeSlot = ExitModal->AddChildToOverlay(ModalShade);
    ShadeSlot->SetHorizontalAlignment(HAlign_Fill);
    ShadeSlot->SetVerticalAlignment(VAlign_Fill);

    UVerticalBox* Dialog = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ExitDialog"));
    UTextBlock* DialogTitle = MakeText(
        WidgetTree, LOCTEXT("ExitTitle", "ЗАВЕРШИТЬ СЕАНС?"), 28, Red, TEXT("ExitTitle"));
    DialogTitle->SetJustification(ETextJustify::Center);
    Dialog->AddChildToVerticalBox(DialogTitle)->SetPadding(FMargin(24.0f, 24.0f, 24.0f, 12.0f));
    UTextBlock* DialogCopy = MakeText(
        WidgetTree,
        LOCTEXT("ExitCopy", "Соединение с командным центром будет разорвано.\nНесохранённые данные текущей операции будут потеряны."),
        16, Text, TEXT("ExitCopy"), false);
    DialogCopy->SetJustification(ETextJustify::Center);
    Dialog->AddChildToVerticalBox(DialogCopy)->SetPadding(FMargin(24.0f, 0.0f, 24.0f, 22.0f));
    UHorizontalBox* DialogActions = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("ExitActions"));
    UButton* CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelExitButton"));
    CancelButton->AddChild(MakeText(WidgetTree, LOCTEXT("CancelExit", "ОТМЕНА"), 18, Text, TEXT("CancelExitLabel")));
    CancelButton->OnClicked.AddDynamic(this, &URA4CommandCentreMenuWidget::CancelExit);
    UHorizontalBoxSlot* CancelSlot = DialogActions->AddChildToHorizontalBox(CancelButton);
    CancelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CancelSlot->SetPadding(FMargin(18.0f, 0.0f, 8.0f, 18.0f));
    UButton* ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmExitButton"));
    ConfirmButton->SetBackgroundColor(RedDim);
    ConfirmButton->AddChild(MakeText(WidgetTree, LOCTEXT("ConfirmExit", "ВЫЙТИ"), 18, Text, TEXT("ConfirmExitLabel")));
    ConfirmButton->OnClicked.AddDynamic(this, &URA4CommandCentreMenuWidget::ConfirmExit);
    UHorizontalBoxSlot* ConfirmSlot = DialogActions->AddChildToHorizontalBox(ConfirmButton);
    ConfirmSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ConfirmSlot->SetPadding(FMargin(8.0f, 0.0f, 18.0f, 18.0f));
    Dialog->AddChildToVerticalBox(DialogActions);
    UBorder* DialogFrame = MakeFramedPanel(WidgetTree, Dialog, TEXT("ExitDialogFrame"), FMargin(2.0f));
    USizeBox* DialogSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ExitDialogSize"));
    DialogSize->SetWidthOverride(660.0f);
    DialogSize->SetHeightOverride(300.0f);
    DialogSize->SetContent(DialogFrame);
    UOverlaySlot* DialogSlot = ExitModal->AddChildToOverlay(DialogSize);
    DialogSlot->SetHorizontalAlignment(HAlign_Center);
    DialogSlot->SetVerticalAlignment(VAlign_Center);
    PlaceOnCanvas(MainCanvas, ExitModal, FVector2D::ZeroVector, ReferenceSize, 50);
}

void URA4CommandCentreMenuWidget::NavigateToScreen(const int32 ScreenIndex)
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

void URA4CommandCentreMenuWidget::OpenCampaign()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4CampaignSelectWidget* Campaign = CreateWidget<URA4CampaignSelectWidget>(
            PlayerController, URA4CampaignSelectWidget::StaticClass()))
        {
            Campaign->AddToViewport(0);
            RemoveFromParent();
        }
    }
}
void URA4CommandCentreMenuWidget::OpenMultiplayer() { NavigateToScreen(3); }
#include "RA4SkirmishSetupWidget.h"

void URA4CommandCentreMenuWidget::OpenSkirmish()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4SkirmishSetupWidget* Setup = CreateWidget<URA4SkirmishSetupWidget>(
            PlayerController, URA4SkirmishSetupWidget::StaticClass()))
        {
            Setup->AddToViewport(0);
            RemoveFromParent();
        }
    }
}
void URA4CommandCentreMenuWidget::OpenEditor() { NavigateToScreen(20); }
void URA4CommandCentreMenuWidget::OpenEncyclopedia() { NavigateToScreen(19); }
void URA4CommandCentreMenuWidget::OpenModifications() { NavigateToScreen(21); }
void URA4CommandCentreMenuWidget::OpenSettings() { NavigateToScreen(4); }

void URA4CommandCentreMenuWidget::RequestExit()
{
    if (ExitModal)
    {
        ExitModal->SetVisibility(ESlateVisibility::Visible);
    }
}

void URA4CommandCentreMenuWidget::CancelExit()
{
    if (ExitModal)
    {
        ExitModal->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void URA4CommandCentreMenuWidget::ConfirmExit()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
