// Copyright (c) Red Alert 4 project.
#include "RA4SkirmishSetupWidget.h"

#include "RA4CommandCentreMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Styling/SlateBrush.h"

#define LOCTEXT_NAMESPACE "RA4SkirmishSetup"

namespace
{
const FVector2D ReferenceSize(1920.0f, 1080.0f);
constexpr FLinearColor Red(0.95f, 0.035f, 0.04f, 1.0f);
constexpr FLinearColor RedDim(0.34f, 0.012f, 0.016f, 1.0f);
constexpr FLinearColor MetalEdge(0.16f, 0.17f, 0.18f, 0.98f);
constexpr FLinearColor Panel(0.009f, 0.009f, 0.011f, 0.94f);
constexpr FLinearColor TextColor(0.86f, 0.82f, 0.79f, 1.0f);
constexpr FLinearColor Muted(0.52f, 0.49f, 0.47f, 1.0f);
constexpr FLinearColor GreenOk(0.20f, 0.88f, 0.35f, 1.0f);
constexpr FLinearColor YellowWarn(0.95f, 0.75f, 0.15f, 1.0f);

UTextBlock* MakeSetupText(UWidgetTree* Tree, const FText& Value, const int32 Size, const FLinearColor& Color,
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
    Font.LetterSpacing = bHeavy ? 35 : 15;
    Label->SetFont(Font);
    return Label;
}

void PlaceSetupWidget(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D Position, const FVector2D Size,
                      const int32 ZOrder = 0)
{
    UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
    Slot->SetPosition(Position);
    Slot->SetSize(Size);
    Slot->SetAnchors(FAnchors(0.0f, 0.0f));
    Slot->SetAlignment(FVector2D::ZeroVector);
    Slot->SetZOrder(ZOrder);
}

// Metal edge + dim red glow + dark interior, matching the main menu chrome so the
// skirmish screen no longer looks like an unstyled placeholder next to it.
UBorder* MakeFramedSetupPanel(UWidgetTree* Tree, UWidget* Content, const FName Name,
                              const FMargin Padding = FMargin(16.0f))
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

void StyleSetupButton(UButton* Button, const FLinearColor& Base)
{
    FButtonStyle Style = Button->GetStyle();
    Style.Normal.TintColor = FSlateColor(Base);
    Style.Hovered.TintColor = FSlateColor(Base * 1.45f);
    Style.Pressed.TintColor = FSlateColor(Base * 0.70f);
    Style.Disabled.TintColor = FSlateColor(Base * 0.45f);
    Button->SetStyle(Style);
}
}

URA4SkirmishSetupWidget::URA4SkirmishSetupWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

TSharedRef<SWidget> URA4SkirmishSetupWidget::RebuildWidget()
{
    if (WidgetTree)
    {
        BuildLayout();
    }
    return Super::RebuildWidget();
}

void URA4SkirmishSetupWidget::NativeConstruct()
{
    Super::NativeConstruct();
    UpdateConflictValidation();
}

void URA4SkirmishSetupWidget::BuildLayout()
{
    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SkirmishSetupRoot"));
    WidgetTree->RootWidget = Root;

    UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
    Background->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.98f));
    UOverlaySlot* BgSlot = Root->AddChildToOverlay(Background);
    BgSlot->SetHorizontalAlignment(HAlign_Fill);
    BgSlot->SetVerticalAlignment(VAlign_Fill);

    // Scale the fixed 1920x1080 reference layout to any viewport instead of
    // letting canvas positions overflow (or float) at non-1080p resolutions.
    // Mirrors the pattern already used by the main menu and campaign screens.
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

    // Title Header
    UTextBlock* Title = MakeSetupText(WidgetTree, LOCTEXT("Title", "НАСТРОЙКА СХВАТКИ"), 36, Red, TEXT("TitleText"));
    PlaceSetupWidget(MainCanvas, Title, FVector2D(80.0f, 40.0f), FVector2D(800.0f, 60.0f), 2);

    UTextBlock* Subtitle = MakeSetupText(
        WidgetTree, LOCTEXT("Subtitle", "Parametry srazheniya, vybor fraktsiy, startovykh pozitsiy i pravil"), 14, Muted, TEXT("SubtitleText"), false);
    PlaceSetupWidget(MainCanvas, Subtitle, FVector2D(82.0f, 95.0f), FVector2D(800.0f, 30.0f), 2);

    // Left Column: Map & Game Options
    UVerticalBox* LeftBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftBox"));

    // Map Selection
    UTextBlock* MapLabel = MakeSetupText(WidgetTree, LOCTEXT("MapLabel", "КАРТА СРАЖЕНИЯ"), 16, TextColor, TEXT("MapLabelText"));
    LeftBox->AddChildToVerticalBox(MapLabel)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 4.0f));

    MapCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("MapCombo"));
    MapCombo->AddOption(TEXT("RA4_Skirmish_Production — Архипелаг (Холмы и Вода, 2 игрока)"));
    MapCombo->SetSelectedIndex(0);
    MapCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    LeftBox->AddChildToVerticalBox(MapCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

    // Starting Credits
    UTextBlock* CreditsLabel = MakeSetupText(WidgetTree, LOCTEXT("CreditsLabel", "СТАРТОВЫЙ БЮДЖЕТ (КРЕДИТЫ)"), 16, TextColor, TEXT("CreditsLabelText"));
    LeftBox->AddChildToVerticalBox(CreditsLabel)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 4.0f));

    CreditsCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("CreditsCombo"));
    CreditsCombo->AddOption(TEXT("5 000 Kreditov (Malyy)"));
    CreditsCombo->AddOption(TEXT("10 000 Kreditov (Standart)"));
    CreditsCombo->AddOption(TEXT("20 000 Kreditov (Bolshoy)"));
    CreditsCombo->SetSelectedIndex(1);
    CreditsCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    LeftBox->AddChildToVerticalBox(CreditsCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

    // AI Difficulty
    UTextBlock* DiffLabel = MakeSetupText(WidgetTree, LOCTEXT("DiffLabel", "SLOZhNOST II (AI)"), 16, TextColor, TEXT("DiffLabelText"));
    LeftBox->AddChildToVerticalBox(DiffLabel)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 4.0f));

    DifficultyCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("DifficultyCombo"));
    DifficultyCombo->AddOption(TEXT("Legkiy (Easy)"));
    DifficultyCombo->AddOption(TEXT("Sredniy (Medium)"));
    DifficultyCombo->AddOption(TEXT("Tyazhyolyy (Hard)"));
    DifficultyCombo->AddOption(TEXT("Bezumnyy (Brutal)"));
    DifficultyCombo->SetSelectedIndex(1);
    DifficultyCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    LeftBox->AddChildToVerticalBox(DifficultyCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

    UBorder* LeftPanel = MakeFramedSetupPanel(WidgetTree, LeftBox, TEXT("LeftPanel"));
    PlaceSetupWidget(MainCanvas, LeftPanel, FVector2D(80.0f, 140.0f), FVector2D(520.0f, 520.0f), 2);

    // Right Column: Player & AI Setup
    UVerticalBox* RightBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightBox"));

    // Player Setup
    UTextBlock* PlayerHeader = MakeSetupText(WidgetTree, LOCTEXT("PlayerHeader", "IGROK 1 (HUMAN)"), 18, Red, TEXT("PlayerHeader"));
    RightBox->AddChildToVerticalBox(PlayerHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    PlayerFactionCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PlayerFactionCombo"));
    PlayerFactionCombo->AddOption(TEXT("Fraktsiya: Soviet (Soviet Union)"));
    PlayerFactionCombo->AddOption(TEXT("Fraktsiya: Alliance (Alliance)"));
    PlayerFactionCombo->AddOption(TEXT("Fraktsiya: Koalitsiya (Coalition)"));
    PlayerFactionCombo->AddOption(TEXT("Fraktsiya: Khrono (Chrono)"));
    PlayerFactionCombo->SetSelectedIndex(0);
    PlayerFactionCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(PlayerFactionCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    PlayerColorCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PlayerColorCombo"));
    PlayerColorCombo->AddOption(TEXT("Tsvet: Krasnyy"));
    PlayerColorCombo->AddOption(TEXT("Tsvet: Siniy"));
    PlayerColorCombo->AddOption(TEXT("Tsvet: Zelyonyy"));
    PlayerColorCombo->AddOption(TEXT("Tsvet: Zhyoltyy"));
    PlayerColorCombo->SetSelectedIndex(0);
    PlayerColorCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(PlayerColorCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    PlayerSpotCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PlayerSpotCombo"));
    PlayerSpotCombo->AddOption(TEXT("Start: Pozitsiya 1 (Zapad)"));
    PlayerSpotCombo->AddOption(TEXT("Start: Pozitsiya 2 (Vostok)"));
    PlayerSpotCombo->SetSelectedIndex(0);
    PlayerSpotCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(PlayerSpotCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));

    // AI Setup
    UTextBlock* AIHeader = MakeSetupText(WidgetTree, LOCTEXT("AIHeader", "IGROK 2 (AI COMMANDER)"), 18, Red, TEXT("AIHeader"));
    RightBox->AddChildToVerticalBox(AIHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    AIFactionCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("AIFactionCombo"));
    AIFactionCombo->AddOption(TEXT("Fraktsiya: Soviet (Soviet Union)"));
    AIFactionCombo->AddOption(TEXT("Fraktsiya: Alliance (Alliance)"));
    AIFactionCombo->AddOption(TEXT("Fraktsiya: Koalitsiya (Coalition)"));
    AIFactionCombo->AddOption(TEXT("Fraktsiya: Khrono (Chrono)"));
    AIFactionCombo->SetSelectedIndex(1);
    AIFactionCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(AIFactionCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    AIColorCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("AIColorCombo"));
    AIColorCombo->AddOption(TEXT("Tsvet: Krasnyy"));
    AIColorCombo->AddOption(TEXT("Tsvet: Siniy"));
    AIColorCombo->AddOption(TEXT("Tsvet: Zelyonyy"));
    AIColorCombo->AddOption(TEXT("Tsvet: Zhyoltyy"));
    AIColorCombo->SetSelectedIndex(1);
    AIColorCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(AIColorCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    AISpotCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("AISpotCombo"));
    AISpotCombo->AddOption(TEXT("Start: Pozitsiya 1 (Zapad)"));
    AISpotCombo->AddOption(TEXT("Start: Pozitsiya 2 (Vostok)"));
    AISpotCombo->SetSelectedIndex(1);
    AISpotCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(AISpotCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

    UBorder* RightPanel = MakeFramedSetupPanel(WidgetTree, RightBox, TEXT("RightPanel"));
    PlaceSetupWidget(MainCanvas, RightPanel, FVector2D(640.0f, 140.0f), FVector2D(520.0f, 520.0f), 2);

    // Validation & Action Banner (Bottom)
    UVerticalBox* BannerStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BannerStack"));

    ValidationWarningText = MakeSetupText(
        WidgetTree, LOCTEXT("ValidationOk", "Параметры матча корректны."), 15, GreenOk, TEXT("ValidationWarningText"));
    BannerStack->AddChildToVerticalBox(ValidationWarningText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

    UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActionRow"));

    UButton* BackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackButton"));
    StyleSetupButton(BackButton, FLinearColor(0.055f, 0.05f, 0.052f, 1.0f));
    BackButton->AddChild(MakeSetupText(
        WidgetTree, LOCTEXT("Back", "В ГЛАВНОЕ МЕНЮ"), 18, TextColor, TEXT("BackLabel")));
    BackButton->OnClicked.AddDynamic(this, &URA4SkirmishSetupWidget::HandleBackClicked);
    UHorizontalBoxSlot* BackSlot = ActionRow->AddChildToHorizontalBox(BackButton);
    BackSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BackSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));

    StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
    StyleSetupButton(StartButton, RedDim);
    StartButton->AddChild(MakeSetupText(WidgetTree, LOCTEXT("StartMatch", "НАЧАТЬ МАТЧ"), 20, TextColor, TEXT("StartMatchLabel")));
    StartButton->OnClicked.AddDynamic(this, &URA4SkirmishSetupWidget::HandleStartMatchClicked);
    UHorizontalBoxSlot* StartSlot = ActionRow->AddChildToHorizontalBox(StartButton);
    StartSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BannerStack->AddChildToVerticalBox(ActionRow)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    ValidationBanner = MakeFramedSetupPanel(WidgetTree, BannerStack, TEXT("ValidationBanner"), FMargin(16.0f, 12.0f));
    PlaceSetupWidget(MainCanvas, ValidationBanner, FVector2D(80.0f, 680.0f), FVector2D(1080.0f, 140.0f), 2);
}

void URA4SkirmishSetupWidget::HandleOptionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectedItem;
    (void)SelectionType;

    if (PlayerFactionCombo) PlayerFactionIndex = PlayerFactionCombo->GetSelectedIndex();
    if (AIFactionCombo) AIFactionIndex = AIFactionCombo->GetSelectedIndex();
    if (PlayerColorCombo) PlayerColorIndex = PlayerColorCombo->GetSelectedIndex();
    if (AIColorCombo) AIColorIndex = AIColorCombo->GetSelectedIndex();
    if (PlayerSpotCombo) PlayerSpotIndex = PlayerSpotCombo->GetSelectedIndex();
    if (AISpotCombo) AISpotIndex = AISpotCombo->GetSelectedIndex();
    if (DifficultyCombo) DifficultyIndex = DifficultyCombo->GetSelectedIndex();
    if (CreditsCombo) CreditsIndex = CreditsCombo->GetSelectedIndex();

    UpdateConflictValidation();
}

void URA4SkirmishSetupWidget::UpdateConflictValidation()
{
    if (ValidationWarningText == nullptr || StartButton == nullptr)
    {
        return;
    }

    bool bHasConflict = false;
    FText Message;

    if (PlayerColorIndex == AIColorIndex)
    {
        bHasConflict = true;
        Message = LOCTEXT("ColorConflict", "ОШИБКА: игрок и ИИ выставили одинаковый цвет! Выберите разные цвета.");
    }
    else if (PlayerSpotIndex == AISpotIndex)
    {
        bHasConflict = true;
        Message = LOCTEXT("SpotConflict", "ОШИБКА: конфликт стартовых позиций! Игроки не могут стартовать на одном спавне.");
    }
    else
    {
        Message = LOCTEXT("ValidationOk", "Параметры матча проверены. Нажмите «НАЧАТЬ МАТЧ» для загрузки.");
    }

    if (bHasConflict)
    {
        ValidationWarningText->SetText(Message);
        ValidationWarningText->SetColorAndOpacity(FSlateColor(YellowWarn));
        StartButton->SetIsEnabled(false);
    }
    else
    {
        ValidationWarningText->SetText(Message);
        ValidationWarningText->SetColorAndOpacity(FSlateColor(GreenOk));
        StartButton->SetIsEnabled(true);
    }
}

void URA4SkirmishSetupWidget::HandleStartMatchClicked()
{
    LaunchSkirmishMatch();
}

void URA4SkirmishSetupWidget::HandleBackClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (URA4CommandCentreMenuWidget* Menu = CreateWidget<URA4CommandCentreMenuWidget>(
            PC, URA4CommandCentreMenuWidget::StaticClass()))
        {
            Menu->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

void URA4SkirmishSetupWidget::LaunchSkirmishMatch()
{
    if (UWorld* World = GetWorld())
    {
        const FString Options = FString::Printf(
            TEXT("?PlayerFaction=%d?EnemyFaction=%d?PlayerSpot=%d?AISpot=%d?Difficulty=%d?Credits=%d"),
            PlayerFactionIndex, AIFactionIndex, PlayerSpotIndex, AISpotIndex, DifficultyIndex, CreditsIndex);

        UGameplayStatics::OpenLevel(World, TEXT("/Game/Maps/RA4_Skirmish_Production"), true, Options);
    }
}

#undef LOCTEXT_NAMESPACE
