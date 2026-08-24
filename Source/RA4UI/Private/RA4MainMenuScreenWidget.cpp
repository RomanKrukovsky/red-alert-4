// Copyright (c) Red Alert 4 project.

#include "RA4MainMenuScreenWidget.h"

#include "RA4FactionData.h"

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
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "RA4AngularPanelWidget.h"
#include "RA4SkirmishSetupWidget.h"

#define LOCTEXT_NAMESPACE "RA4MainMenuScreenWidget"

namespace
{
// The command centre is neutral ground: the frame carries the shared Scarlet
// horizon line, while bloc colour arrives only with the selected direction.
// The command centre is neutral ground: cold steel chrome, with scarlet kept for
// the horizon line under the title and for alarm states only.
const FLinearColor MenuAccent(0.32f, 0.42f, 0.56f, 1.0f);
const FLinearColor MenuChromeTint(0.42f, 0.52f, 0.66f, 1.0f);
const FLinearColor MenuSelectedTint(0.45f, 0.62f, 0.95f, 1.0f);
const FLinearColor MenuPressedTint(0.65f, 0.80f, 1.00f, 1.0f);
const FLinearColor MenuHorizon = FRA4FactionDataRegistry::GetHorizonScarletColor();
constexpr FLinearColor MenuText(0.87f, 0.89f, 0.94f, 1.0f);
constexpr FLinearColor MutedText(0.55f, 0.58f, 0.64f, 1.0f);

void PlaceMenuWidget(
    UCanvasPanel* Canvas,
    UWidget* Widget,
    const FVector2D Position,
    const FVector2D Size,
    const int32 ZOrder = 0)
{
    UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
    Slot->SetPosition(Position);
    Slot->SetSize(Size);
    Slot->SetZOrder(ZOrder);
}

UTextBlock* MakeMenuText(
    UWidgetTree* Tree,
    const FText& Text,
    const int32 Size,
    const FLinearColor& Color,
    const FName Name,
    const bool bBold = false)
{
    UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    Label->SetText(Text);
    Label->SetColorAndOpacity(FSlateColor(Color));
    Label->SetAutoWrapText(true);
    const TCHAR* FontPath = bBold
        ? TEXT("/Game/RA4UI/Fonts/RA4_RobotoCondensedSemiBold_Font.RA4_RobotoCondensedSemiBold_Font")
        : TEXT("/Game/RA4UI/Fonts/RA4_RobotoCondensedRegular_Font.RA4_RobotoCondensedRegular_Font");
    if (UObject* Font = LoadObject<UObject>(nullptr, FontPath))
    {
        FSlateFontInfo FontInfo(Font, Size);
        FontInfo.LetterSpacing = bBold ? 40 : 18;
        Label->SetFont(FontInfo);
    }
    return Label;
}

FButtonStyle MakeMenuButtonStyle(const bool bSelected)
{
    // The supplied frame textures are warm red and carry their own pixel size, so
    // drawing them stretched both fought the palette and made rows overlap. The
    // menu plate is drawn procedurally instead: it takes the row height exactly,
    // scales to any resolution and reads its colour from the theme.
    FButtonStyle Style;

    const FLinearColor Edge = bSelected ? MenuSelectedTint : MenuAccent;
    Style.SetNormal(FSlateRoundedBoxBrush(
        bSelected ? FLinearColor(0.055f, 0.105f, 0.200f, 0.98f)
                  : FLinearColor(0.016f, 0.022f, 0.034f, 0.94f),
        3.0f, Edge, bSelected ? 2.0f : 1.0f));
    Style.SetHovered(FSlateRoundedBoxBrush(
        FLinearColor(0.085f, 0.155f, 0.280f, 0.98f), 3.0f, MenuSelectedTint, 2.0f));
    Style.SetPressed(FSlateRoundedBoxBrush(
        FLinearColor(0.140f, 0.245f, 0.420f, 1.0f), 3.0f, MenuPressedTint, 2.0f));
    Style.SetDisabled(FSlateRoundedBoxBrush(
        FLinearColor(0.012f, 0.014f, 0.018f, 0.55f), 3.0f,
        FLinearColor(0.22f, 0.24f, 0.28f, 0.8f), 1.0f));

    Style.NormalPadding = FMargin(0.0f);
    Style.PressedPadding = FMargin(2.0f, 2.0f, 0.0f, 0.0f);
    return Style;
}
} // namespace

URA4MainMenuScreenWidget::URA4MainMenuScreenWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetScreenIdentity(ERA4UIScreenId::MainMenu);
}

TSharedRef<SWidget> URA4MainMenuScreenWidget::RebuildWidget()
{
    const TSharedRef<SWidget> RootWidget = Super::RebuildWidget();
    if (!WidgetTree || !GetContentLayer())
    {
        return RootWidget;
    }

    MainMenuViewModel = NewObject<URA4MainMenuViewModel>(this);
    MenuButtons.Reset();

    if (UTexture2D* BackgroundTexture = LoadObject<UTexture2D>(
        nullptr,
        TEXT("/Game/RA4UI/Art/Remaster/T_SH_02_MainMenu.T_SH_02_MainMenu")))
    {
        GetBackgroundLayer()->SetBrushFromTexture(BackgroundTexture, false);
        // The remaster plate already carries the painted command centre; a light
        // scrim keeps it legible without recolouring it (the live panels own the
        // readable pixels).
        GetBackgroundLayer()->SetColorAndOpacity(FLinearColor(0.86f, 0.88f, 0.92f, 1.0f));
    }

    UBorder* Grade = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("CommandCentreGrade"));
    Grade->SetBrush(FSlateColorBrush(FLinearColor(0.004f, 0.006f, 0.012f, 0.42f)));
    Grade->SetVisibility(ESlateVisibility::HitTestInvisible);
    UOverlaySlot* GradeSlot = GetContentLayer()->AddChildToOverlay(Grade);
    GradeSlot->SetHorizontalAlignment(HAlign_Fill);
    GradeSlot->SetVerticalAlignment(VAlign_Fill);

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("MainMenuCanvas"));
    UOverlaySlot* CanvasSlot = GetContentLayer()->AddChildToOverlay(Canvas);
    CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
    CanvasSlot->SetVerticalAlignment(VAlign_Fill);

    // Top centred wordmark + scarlet horizon rule + subtitle, matching the
    // remaster 02_main_menu reference composition.
    UTextBlock* Wordmark = MakeMenuText(
        WidgetTree,
        LOCTEXT("MainMenuWordmark", "SCARLET HORIZON"), 44,
        FLinearColor(0.88f, 0.91f, 0.95f, 1.0f),
        TEXT("MainMenuWordmark"), true);
    Wordmark->SetAutoWrapText(false);
    Wordmark->SetJustification(ETextJustify::Center);
    Wordmark->SetVisibility(ESlateVisibility::HitTestInvisible);
    WordmarkText = Wordmark;
    PlaceMenuWidget(Canvas, Wordmark, FVector2D(560.0f, 24.0f), FVector2D(800.0f, 64.0f), 4);

    UBorder* HorizonRule = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("MainMenuHorizonRule"));
    HorizonRule->SetBrushColor(MenuHorizon);
    HorizonRule->SetVisibility(ESlateVisibility::HitTestInvisible);
    PlaceMenuWidget(Canvas, HorizonRule, FVector2D(744.0f, 90.0f), FVector2D(432.0f, 2.0f), 5);

    UTextBlock* Tagline = MakeMenuText(
        WidgetTree, LOCTEXT("MainMenuTagline", "ГЛОБАЛЬНЫЙ КОМАНДНЫЙ ЦЕНТР"), 15,
        FLinearColor(0.80f, 0.84f, 0.87f, 1.0f), TEXT("MainMenuTagline"), true);
    Tagline->SetJustification(ETextJustify::Center);
    Tagline->SetVisibility(ESlateVisibility::HitTestInvisible);
    PlaceMenuWidget(Canvas, Tagline, FVector2D(560.0f, 98.0f), FVector2D(800.0f, 30.0f), 5);

    // Left vertical menu: 8 entries, 330px wide, 52px tall, 9px gap, starting
    // 90px from the top — the remaster reference stacks the full menu on the
    // left rail of the command centre.
    UVerticalBox* MenuList = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("MainMenuEntries"));
    const TArray<FRA4MainMenuEntry>& Entries = MainMenuViewModel->GetMenuEntries();
    for (int32 Index = 0; Index < Entries.Num(); ++Index)
    {
        CreateMenuButton(MenuList, Entries[Index], Index);
    }
    PlaceMenuWidget(Canvas, MenuList, FVector2D(34.0f, 116.0f), FVector2D(330.0f, 488.0f), 6);

    // Bottom dashboard bar: four cards (commander / fronts / operation / network),
    // matching the remaster reference bottom strip.
    BuildCommanderCard(Canvas, FVector2D(34.0f, 906.0f), FVector2D(300.0f, 154.0f));
    BuildFrontsCard(Canvas, FVector2D(346.0f, 906.0f), FVector2D(612.0f, 154.0f));
    BuildOperationCard(Canvas, FVector2D(970.0f, 906.0f), FVector2D(612.0f, 154.0f));
    BuildNetworkCard(Canvas, FVector2D(1594.0f, 906.0f), FVector2D(292.0f, 154.0f));
    return RootWidget;
}

UButton* URA4MainMenuScreenWidget::CreateMenuButton(
    UVerticalBox* Menu,
    const FRA4MainMenuEntry& Entry,
    const int32 Index)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), FName(*FString::Printf(TEXT("MainMenuButton_%d"), Index)));
    Button->SetStyle(MakeMenuButtonStyle(Entry.bSelected));

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), FName(*FString::Printf(TEXT("MainMenuRow_%d"), Index)));
    // Remaster reference uses a per-entry glyph rather than a generic bullet.
    const TCHAR* EntryIcons[] = { TEXT("❖"), TEXT("🌐"), TEXT("⚔"), TEXT("🛠"),
                                  TEXT("📖"), TEXT("⚙"), TEXT("⚙"), TEXT("⮌") };
    const FText IconText = FText::FromString(
        Index < UE_ARRAY_COUNT(EntryIcons) ? EntryIcons[Index] : TEXT("◆"));
    UTextBlock* Icon = MakeMenuText(
        WidgetTree,
        IconText,
        19,
        Entry.bSelected ? FLinearColor(0.95f, 0.88f, 1.0f, 1.0f) : MutedText,
        FName(*FString::Printf(TEXT("MainMenuIcon_%d"), Index)),
        true);
    UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(Icon);
    IconSlot->SetPadding(FMargin(20.0f, 0.0f, 16.0f, 0.0f));
    IconSlot->SetVerticalAlignment(VAlign_Center);

    UTextBlock* Label = MakeMenuText(
        WidgetTree,
        Entry.Label,
        17,
        Entry.bSelected ? FLinearColor::White : FLinearColor(0.72f, 0.75f, 0.80f, 1.0f),
        FName(*FString::Printf(TEXT("MainMenuLabel_%d"), Index)),
        true);
    Label->SetAutoWrapText(false);
    UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
    LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LabelSlot->SetVerticalAlignment(VAlign_Center);
    Button->AddChild(Row);

    switch (Entry.Action)
    {
    case ERA4MainMenuAction::Campaign:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::OpenCampaign);
        break;
    case ERA4MainMenuAction::Multiplayer:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::OpenMultiplayer);
        break;
    case ERA4MainMenuAction::Skirmish:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::OpenSkirmish);
        break;
    case ERA4MainMenuAction::Editor:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::OpenEditor);
        break;
    case ERA4MainMenuAction::Encyclopedia:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::OpenEncyclopedia);
        break;
    case ERA4MainMenuAction::Modifications:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::OpenModifications);
        break;
    case ERA4MainMenuAction::Settings:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::OpenSettings);
        break;
    case ERA4MainMenuAction::Exit:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::ExitToSplash);
        break;
    default:
        checkNoEntry();
        break;
    }

    // Remaster reference rows are exactly 52px tall (the canvas panel allocates
    // the 9px gap separately via VerticalBox spacing).
    USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), FName(*FString::Printf(TEXT("MainMenuButtonSize_%d"), Index)));
    ButtonSize->SetHeightOverride(52.0f);
    ButtonSize->SetContent(Button);
    Menu->AddChildToVerticalBox(ButtonSize)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 9.0f));
    MenuButtons.Add(Button);
    return Button;
}

void URA4MainMenuScreenWidget::BuildCommanderCard(
    UCanvasPanel* Canvas,
    const FVector2D Position,
    const FVector2D Size)
{
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CommanderCard_Content"));
    UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CommanderCard_Top"));
    // Portrait emblem.
    UBorder* Portrait = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("CommanderCard_Portrait"));
    Portrait->SetBrushColor(FLinearColor(0.16f, 0.20f, 0.31f, 0.95f));
    Portrait->SetPadding(FMargin(0.0f));
    Portrait->SetHorizontalAlignment(HAlign_Center);
    Portrait->SetVerticalAlignment(VAlign_Center);
    UTextBlock* PortraitGlyph = MakeMenuText(
        WidgetTree, LOCTEXT("CommanderGlyph", "⚔"), 30,
        FLinearColor(0.62f, 0.71f, 0.87f, 1.0f), TEXT("CommanderCard_PortraitGlyph"), true);
    PortraitGlyph->SetJustification(ETextJustify::Center);
    Portrait->SetContent(PortraitGlyph);
    UHorizontalBoxSlot* PortraitSlot = Top->AddChildToHorizontalBox(Portrait);
    PortraitSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    PortraitSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));

    UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CommanderCard_Right"));
    Right->AddChildToVerticalBox(MakeMenuText(
        WidgetTree, LOCTEXT("CommanderTitle", "КОМАНДИР • УРОВЕНЬ 24"), 15,
        FLinearColor(0.93f, 0.95f, 0.97f, 1.0f), TEXT("CommanderCard_Title"), true));
    UProgressBar* XpBar = WidgetTree->ConstructWidget<UProgressBar>(
        UProgressBar::StaticClass(), TEXT("CommanderCard_XpBar"));
    XpBar->SetFillColorAndOpacity(FLinearColor(0.31f, 0.49f, 0.85f, 1.0f));
    XpBar->SetPercent(0.72f);
    Right->AddChildToVerticalBox(XpBar)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 4.0f));
    Right->AddChildToVerticalBox(MakeMenuText(
        WidgetTree, LOCTEXT("CommanderXp", "34 750 / 48 000 ОП"), 11,
        FLinearColor(0.58f, 0.63f, 0.70f, 1.0f), TEXT("CommanderCard_Xp"), false));
    UHorizontalBox* Badges = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CommanderCard_Badges"));
    const TCHAR* BadgeColors[] = {
        TEXT("b06cff"), TEXT("3f8dff"), TEXT("2fd98a"), TEXT("2fd4c8"), TEXT("e8a13d")};
    for (int32 I = 0; I < UE_ARRAY_COUNT(BadgeColors); ++I)
    {
        UBorder* Badge = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), FName(*FString::Printf(TEXT("CommanderCard_Badge_%d"), I)));
        Badge->SetBrushColor(FLinearColor(FColor::FromHex(BadgeColors[I])));
        Badge->SetPadding(FMargin(0.0f));
        Badge->SetHorizontalAlignment(HAlign_Center);
        Badge->SetVerticalAlignment(VAlign_Center);
        UTextBlock* Crest = MakeMenuText(
            WidgetTree, LOCTEXT("BadgeCrest", "❖"), 14,
            FLinearColor(FColor::FromHex(BadgeColors[I])),
            FName(*FString::Printf(TEXT("CommanderCard_Crest_%d"), I)), true);
        Crest->SetJustification(ETextJustify::Center);
        Badge->SetContent(Crest);
        Badges->AddChildToHorizontalBox(Badge)->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
    }
    Right->AddChildToVerticalBox(Badges)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
    UHorizontalBoxSlot* RightSlot = Top->AddChildToHorizontalBox(Right);
    RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Content->AddChildToVerticalBox(Top);

    URA4AngularPanelWidget* Panel = WidgetTree->ConstructWidget<URA4AngularPanelWidget>(
        URA4AngularPanelWidget::StaticClass(), TEXT("CommanderCard"));
    Panel->SetPanelRole(ERA4PanelRole::Compact);
    Panel->SetBrush(FSlateRoundedBoxBrush(
        FLinearColor(0.012f, 0.014f, 0.022f, 0.90f), 4.0f, MenuAccent, 1.25f));
    Panel->SetContent(Content);
    PlaceMenuWidget(Canvas, Panel, Position, Size, 2);
}

void URA4MainMenuScreenWidget::BuildFrontsCard(
    UCanvasPanel* Canvas,
    const FVector2D Position,
    const FVector2D Size)
{
    UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("FrontsCard_Content"));
    UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("FrontsCard_Left"));
    Left->AddChildToVerticalBox(MakeMenuText(
        WidgetTree, LOCTEXT("FrontsHeading", "СВОДКА ФРОНТОВ"), 15,
        FLinearColor(0.87f, 0.90f, 0.93f, 1.0f), TEXT("FrontsCard_Heading"), true))
        ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    const TCHAR* FrontNames[] = {
        TEXT("ЕВРАЗИЙСКИЙ ПАКТ"), TEXT("АТЛАНТИЧЕСКИЙ АЛЬЯНС"),
        TEXT("ВОСТОЧНАЯ КОАЛИЦИЯ"), TEXT("ТИХООКЕАНСКИЙ ПАКТ"),
        TEXT("НЕЗАВИСИМЫЕ ДЕРЖАВЫ")};
    const TCHAR* FrontColors[] = {
        TEXT("b06cff"), TEXT("3f8dff"), TEXT("2fd98a"), TEXT("2fd4c8"), TEXT("e8a13d")};
    for (int32 I = 0; I < UE_ARRAY_COUNT(FrontNames); ++I)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(), FName(*FString::Printf(TEXT("FrontsCard_Row_%d"), I)));
        UBorder* Dot = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), FName(*FString::Printf(TEXT("FrontsCard_Dot_%d"), I)));
        Dot->SetBrushColor(FLinearColor(FColor::FromHex(FrontColors[I])));
        Row->AddChildToHorizontalBox(Dot)->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 4.0f));
        Row->AddChildToHorizontalBox(MakeMenuText(
            WidgetTree, FText::FromString(FrontNames[I]), 11,
            FLinearColor(0.66f, 0.71f, 0.76f, 1.0f),
            FName(*FString::Printf(TEXT("FrontsCard_Name_%d"), I)), false));
        Left->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
    }
    UHorizontalBoxSlot* LeftSlot = Content->AddChildToHorizontalBox(Left);
    LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LeftSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));

    // Faction gradient swatch (right column).
    UBorder* Swatch = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("FrontsCard_Swatch"));
    Swatch->SetBrushColor(FLinearColor(0.69f, 0.42f, 1.0f, 0.32f));
    UHorizontalBoxSlot* SwatchSlot = Content->AddChildToHorizontalBox(Swatch);
    SwatchSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    SwatchSlot->SetHorizontalAlignment(HAlign_Right);

    URA4AngularPanelWidget* Panel = WidgetTree->ConstructWidget<URA4AngularPanelWidget>(
        URA4AngularPanelWidget::StaticClass(), TEXT("FrontsCard"));
    Panel->SetPanelRole(ERA4PanelRole::Compact);
    Panel->SetBrush(FSlateRoundedBoxBrush(
        FLinearColor(0.012f, 0.014f, 0.022f, 0.90f), 4.0f, MenuAccent, 1.25f));
    Panel->SetContent(Content);
    PlaceMenuWidget(Canvas, Panel, Position, Size, 2);
}

void URA4MainMenuScreenWidget::BuildOperationCard(
    UCanvasPanel* Canvas,
    const FVector2D Position,
    const FVector2D Size)
{
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("OperationCard_Content"));
    Content->AddChildToVerticalBox(MakeMenuText(
        WidgetTree, LOCTEXT("OperationHeading", "ТЕКУЩАЯ ОПЕРАЦИЯ"), 15,
        FLinearColor(0.87f, 0.90f, 0.93f, 1.0f), TEXT("OperationCard_Heading"), true))
        ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
    // Operation thumbnail strip.
    UBorder* Thumb = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("OperationCard_Thumb"));
    Thumb->SetBrushColor(FLinearColor(0.12f, 0.17f, 0.27f, 0.92f));
    Thumb->SetPadding(FMargin(8.0f, 4.0f));
    Thumb->SetVerticalAlignment(VAlign_Bottom);
    UTextBlock* ThumbLabel = MakeMenuText(
        WidgetTree, LOCTEXT("OperationZone", "ОПЕРАТИВНАЯ ЗОНА • АТЛАНТИКА"), 9,
        FLinearColor(0.49f, 0.56f, 0.66f, 1.0f), TEXT("OperationCard_ThumbLabel"), false);
    Thumb->SetContent(ThumbLabel);
    Content->AddChildToVerticalBox(Thumb)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("OperationCard_TitleRow"));
    UTextBlock* Title = MakeMenuText(
        WidgetTree, LOCTEXT("OperationTitle", "БАРЬЕР «ТИФОН»"), 14,
        FLinearColor::White, TEXT("OperationCard_Title"), true);
    UHorizontalBoxSlot* TitleSlot = TitleRow->AddChildToHorizontalBox(Title);
    TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    TitleRow->AddChildToHorizontalBox(MakeMenuText(
        WidgetTree, LOCTEXT("OperationPhase", "ФАЗА 2/4"), 11,
        FLinearColor(0.56f, 0.63f, 0.71f, 1.0f), TEXT("OperationCard_Phase"), false));
    Content->AddChildToVerticalBox(TitleRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
    Content->AddChildToVerticalBox(MakeMenuText(
        WidgetTree, LOCTEXT("OperationDesc", "Удержать острова и защитить логистические маршруты."),
        11, FLinearColor(0.58f, 0.63f, 0.70f, 1.0f),
        TEXT("OperationCard_Desc"), false))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
    // Phase progress segments (7 segments, first 4 filled = phase 2).
    UHorizontalBox* Segments = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("OperationCard_Segments"));
    for (int32 I = 0; I < 7; ++I)
    {
        UBorder* Seg = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), FName(*FString::Printf(TEXT("OperationCard_Seg_%d"), I)));
        Seg->SetBrushColor(I < 4
            ? FLinearColor(0.31f, 0.49f, 0.85f, 1.0f)
            : FLinearColor(1.0f, 1.0f, 1.0f, 0.12f));
        UHorizontalBoxSlot* SegSlot = Segments->AddChildToHorizontalBox(Seg);
        SegSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        SegSlot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
    }
    Content->AddChildToVerticalBox(Segments);

    URA4AngularPanelWidget* Panel = WidgetTree->ConstructWidget<URA4AngularPanelWidget>(
        URA4AngularPanelWidget::StaticClass(), TEXT("OperationCard"));
    Panel->SetPanelRole(ERA4PanelRole::Compact);
    Panel->SetBrush(FSlateRoundedBoxBrush(
        FLinearColor(0.012f, 0.014f, 0.022f, 0.90f), 4.0f, MenuAccent, 1.25f));
    Panel->SetContent(Content);
    PlaceMenuWidget(Canvas, Panel, Position, Size, 2);
}

void URA4MainMenuScreenWidget::BuildNetworkCard(
    UCanvasPanel* Canvas,
    const FVector2D Position,
    const FVector2D Size)
{
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("NetworkCard_Content"));
    Content->AddChildToVerticalBox(MakeMenuText(
        WidgetTree, LOCTEXT("NetworkHeading", "СОСТОЯНИЕ СЕТИ"), 15,
        FLinearColor(0.87f, 0.90f, 0.93f, 1.0f), TEXT("NetworkCard_Heading"), true))
        ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
    // Network graph placeholder plate.
    UBorder* Graph = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("NetworkCard_Graph"));
    Graph->SetBrushColor(FLinearColor(0.024f, 0.039f, 0.071f, 0.90f));
    Content->AddChildToVerticalBox(Graph)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    UHorizontalBox* Status = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("NetworkCard_Status"));
    Status->AddChildToHorizontalBox(MakeMenuText(
        WidgetTree, LOCTEXT("NetworkSignal", "📶"), 13,
        FLinearColor(0.34f, 0.91f, 0.60f, 1.0f), TEXT("NetworkCard_Signal"), false))
        ->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
    Status->AddChildToHorizontalBox(MakeMenuText(
        WidgetTree, LOCTEXT("NetworkOnline", "СЕТЬ: ПОДКЛЮЧЕНО"), 12,
        FLinearColor(0.34f, 0.91f, 0.60f, 1.0f), TEXT("NetworkCard_Online"), true));
    Content->AddChildToVerticalBox(Status);

    URA4AngularPanelWidget* Panel = WidgetTree->ConstructWidget<URA4AngularPanelWidget>(
        URA4AngularPanelWidget::StaticClass(), TEXT("NetworkCard"));
    Panel->SetPanelRole(ERA4PanelRole::Compact);
    Panel->SetBrush(FSlateRoundedBoxBrush(
        FLinearColor(0.012f, 0.014f, 0.022f, 0.90f), 4.0f, MenuAccent, 1.25f));
    Panel->SetContent(Content);
    PlaceMenuWidget(Canvas, Panel, Position, Size, 2);
}

int32 URA4MainMenuScreenWidget::GetSelectedMenuIndex() const
{
    return MainMenuViewModel ? MainMenuViewModel->GetSelectedMenuIndex() : INDEX_NONE;
}

void URA4MainMenuScreenWidget::OpenCampaign()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Campaign);
    NavigateToScreen(ERA4UIScreenId::CampaignSelect);
}

void URA4MainMenuScreenWidget::OpenMultiplayer()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Multiplayer);
    NavigateToScreen(ERA4UIScreenId::MultiplayerLobby);
}

void URA4MainMenuScreenWidget::OpenSkirmish()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Skirmish);
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4SkirmishSetupWidget* Skirmish = CreateWidget<URA4SkirmishSetupWidget>(
            PlayerController, URA4SkirmishSetupWidget::StaticClass()))
        {
            Skirmish->AddToViewport();
            RemoveFromParent();
        }
    }
}

void URA4MainMenuScreenWidget::OpenEditor()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Editor);
    NavigateToScreen(ERA4UIScreenId::TechTree);
}

void URA4MainMenuScreenWidget::OpenEncyclopedia()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Encyclopedia);
    NavigateToScreen(ERA4UIScreenId::Encyclopedia);
}

void URA4MainMenuScreenWidget::OpenModifications()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Modifications);
    NavigateToScreen(ERA4UIScreenId::Mods);
}

void URA4MainMenuScreenWidget::OpenSettings()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Settings);
    NavigateToScreen(ERA4UIScreenId::Settings);
}

void URA4MainMenuScreenWidget::ExitToSplash()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Exit);
    NavigateToScreen(ERA4UIScreenId::Splash);
}

#undef LOCTEXT_NAMESPACE
