// Copyright (c) Red Alert 4 project.
#include "RA4SkirmishSetupWidget.h"

#include "RA4MainMenuScreenWidget.h"
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

UButton* MakeSetupButton(
    UWidgetTree* Tree,
    const FText& Text,
    const FName ButtonName,
    const FLinearColor& NormalColor,
    const FLinearColor& HoverColor,
    const FLinearColor& PressedColor,
    const FLinearColor& LabelColor,
    const int32 FontSize = 20)
{
    UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);

    FButtonStyle Style;
    Style.SetNormal(FSlateColorBrush(NormalColor));
    Style.SetHovered(FSlateColorBrush(HoverColor));
    Style.SetPressed(FSlateColorBrush(PressedColor));
    Style.SetDisabled(FSlateColorBrush(FLinearColor(0.08f, 0.08f, 0.10f, 0.45f)));
    Style.NormalPadding = FMargin(0.0f);
    Style.PressedPadding = FMargin(2.0f, 2.0f, 0.0f, 0.0f);
    Button->SetStyle(Style);

    UTextBlock* Label = MakeSetupText(Tree, Text, FontSize, LabelColor, FName(ButtonName.ToString() + TEXT("_Label")), true);
    Label->SetJustification(ETextJustify::Center);
    Label->SetShadowOffset(FVector2D(2.0f, 2.0f));
    Label->SetShadowColorAndOpacity(FLinearColor::Black);

    Button->AddChild(Label);
    if (UButtonSlot* BSlot = Cast<UButtonSlot>(Label->Slot))
    {
        BSlot->SetPadding(FMargin(24.0f, 12.0f));
        BSlot->SetHorizontalAlignment(HAlign_Center);
        BSlot->SetVerticalAlignment(VAlign_Center);
    }
    return Button;
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
        WidgetTree, LOCTEXT("Subtitle", "Параметры сражения, выбор блоков, стран, стартовых позиций и правил"), 14, Muted, TEXT("SubtitleText"), false);
    PlaceSetupWidget(MainCanvas, Subtitle, FVector2D(82.0f, 95.0f), FVector2D(800.0f, 30.0f), 2);

    // Left Column: Map & Game Options
    UVerticalBox* LeftBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftBox"));

    // Map Selection
    UTextBlock* MapLabel = MakeSetupText(WidgetTree, LOCTEXT("MapLabel", "КАРТА СРАЖЕНИЯ"), 16, TextColor, TEXT("MapLabelText"));
    LeftBox->AddChildToVerticalBox(MapLabel)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 4.0f));

    MapCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("MapCombo"));
    MapCombo->AddOption(TEXT("Архипелаг — Холмы и Проливы (2 игрока)"));
    MapCombo->SetSelectedIndex(0);
    MapCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    LeftBox->AddChildToVerticalBox(MapCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

    // Starting Credits
    UTextBlock* CreditsLabel = MakeSetupText(WidgetTree, LOCTEXT("CreditsLabel", "СТАРТОВЫЙ БЮДЖЕТ (КРЕДИТЫ)"), 16, TextColor, TEXT("CreditsLabelText"));
    LeftBox->AddChildToVerticalBox(CreditsLabel)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 4.0f));

    CreditsCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("CreditsCombo"));
    CreditsCombo->AddOption(TEXT("5 000 Кредитов (Малый)"));
    CreditsCombo->AddOption(TEXT("10 000 Кредитов (Стандарт)"));
    CreditsCombo->AddOption(TEXT("20 000 Кредитов (Большой)"));
    CreditsCombo->SetSelectedIndex(1);
    CreditsCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    LeftBox->AddChildToVerticalBox(CreditsCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

    // AI Difficulty
    UTextBlock* DiffLabel = MakeSetupText(WidgetTree, LOCTEXT("DiffLabel", "СЛОЖНОСТЬ ИИ"), 16, TextColor, TEXT("DiffLabelText"));
    LeftBox->AddChildToVerticalBox(DiffLabel)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 4.0f));

    DifficultyCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("DifficultyCombo"));
    DifficultyCombo->AddOption(TEXT("Базовая (Легкий)"));
    DifficultyCombo->AddOption(TEXT("Тактическая (Средний)"));
    DifficultyCombo->AddOption(TEXT("Командная (Тяжёлый)"));
    DifficultyCombo->AddOption(TEXT("Экстремальная (Эксперт)"));
    DifficultyCombo->SetSelectedIndex(1);
    DifficultyCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    LeftBox->AddChildToVerticalBox(DifficultyCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

    UBorder* LeftPanel = MakeFramedSetupPanel(WidgetTree, LeftBox, TEXT("LeftPanel"));
    PlaceSetupWidget(MainCanvas, LeftPanel, FVector2D(80.0f, 140.0f), FVector2D(520.0f, 520.0f), 2);

    // Right Column: Player & AI Setup
    UVerticalBox* RightBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightBox"));

    // Player Setup
    UTextBlock* PlayerHeader = MakeSetupText(WidgetTree, LOCTEXT("PlayerHeader", "ИГРОК 1 (КОМАНДИР)"), 18, Red, TEXT("PlayerHeader"));
    RightBox->AddChildToVerticalBox(PlayerHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    PlayerFactionCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PlayerFactionCombo"));
    PlayerFactionCombo->AddOption(TEXT("Евразийский пакт (Россия)"));
    PlayerFactionCombo->AddOption(TEXT("Атлантический альянс (США)"));
    PlayerFactionCombo->AddOption(TEXT("Восточная коалиция (Китай)"));
    PlayerFactionCombo->AddOption(TEXT("Тихоокеанский пакт (Япония)"));
    PlayerFactionCombo->AddOption(TEXT("Независимые державы (Иран)"));
    PlayerFactionCombo->SetSelectedIndex(0);
    PlayerFactionCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(PlayerFactionCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    PlayerColorCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PlayerColorCombo"));
    PlayerColorCombo->AddOption(TEXT("Цвет: Фиолетовый (Пакт)"));
    PlayerColorCombo->AddOption(TEXT("Цвет: Кобальтовый (Альянс)"));
    PlayerColorCombo->AddOption(TEXT("Цвет: Золотой (Коалиция)"));
    PlayerColorCombo->AddOption(TEXT("Цвет: Бирюзовый (Тихоокеанский)"));
    PlayerColorCombo->AddOption(TEXT("Цвет: Янтарный (Независимые)"));
    PlayerColorCombo->SetSelectedIndex(0);
    PlayerColorCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(PlayerColorCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    PlayerSpotCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PlayerSpotCombo"));
    PlayerSpotCombo->AddOption(TEXT("Старт: Позиция 1 (Запад)"));
    PlayerSpotCombo->AddOption(TEXT("Старт: Позиция 2 (Восток)"));
    PlayerSpotCombo->SetSelectedIndex(0);
    PlayerSpotCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(PlayerSpotCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));

    // AI Setup
    UTextBlock* AIHeader = MakeSetupText(WidgetTree, LOCTEXT("AIHeader", "ИГРОК 2 (ИИ КОМАНДИР)"), 18, Red, TEXT("AIHeader"));
    RightBox->AddChildToVerticalBox(AIHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    AIFactionCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("AIFactionCombo"));
    AIFactionCombo->AddOption(TEXT("Евразийский пакт (Россия)"));
    AIFactionCombo->AddOption(TEXT("Атлантический альянс (США)"));
    AIFactionCombo->AddOption(TEXT("Восточная коалиция (Китай)"));
    AIFactionCombo->AddOption(TEXT("Тихоокеанский пакт (Япония)"));
    AIFactionCombo->AddOption(TEXT("Независимые державы (Иран)"));
    AIFactionCombo->SetSelectedIndex(1);
    AIFactionCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(AIFactionCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    AIColorCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("AIColorCombo"));
    AIColorCombo->AddOption(TEXT("Цвет: Фиолетовый (Пакт)"));
    AIColorCombo->AddOption(TEXT("Цвет: Кобальтовый (Альянс)"));
    AIColorCombo->AddOption(TEXT("Цвет: Золотой (Коалиция)"));
    AIColorCombo->AddOption(TEXT("Цвет: Бирюзовый (Тихоокеанский)"));
    AIColorCombo->AddOption(TEXT("Цвет: Янтарный (Независимые)"));
    AIColorCombo->SetSelectedIndex(1);
    AIColorCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(AIColorCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    AISpotCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("AISpotCombo"));
    AISpotCombo->AddOption(TEXT("Старт: Позиция 1 (Запад)"));
    AISpotCombo->AddOption(TEXT("Старт: Позиция 2 (Восток)"));
    AISpotCombo->SetSelectedIndex(1);
    AISpotCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(AISpotCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

    UBorder* RightPanel = MakeFramedSetupPanel(WidgetTree, RightBox, TEXT("RightPanel"));
    PlaceSetupWidget(MainCanvas, RightPanel, FVector2D(630.0f, 140.0f), FVector2D(530.0f, 500.0f), 2);

    // Right Column: Tactical Intel & Match Rules
    UVerticalBox* IntelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("IntelBox"));
    UTextBlock* IntelHeader = MakeSetupText(
        WidgetTree, LOCTEXT("IntelHeader", "ТАКТИЧЕСКИЕ ДАННЫЕ ОПЕРАЦИИ"), 18, TextColor, TEXT("IntelHeader"));
    IntelBox->AddChildToVerticalBox(IntelHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

    UTextBlock* IntelDesc = MakeSetupText(
        WidgetTree,
        LOCTEXT("IntelDesc", "ЛАНДШАФТ: Архипелаг с возвышенностями, водными преградами и узкими проходами.\n"
                             "БАЗОСТРОЕНИЕ: Развёртывание Сборочного цеха (MCV) и добыча руды.\n"
                             "ТУМАН ВОЙНЫ: Включён (Требуется разведка радаром и мобильными силами).\n"
                             "СУПЕРОРУЖИЕ: Активно после достижения 3-го технологического уровня.\n"
                             "УСЛОВИЕ ПОБЕДЫ: Полная ликвидация всех баз и боевых подразделений противника."),
        14, Muted, TEXT("IntelDesc"), false);
    IntelBox->AddChildToVerticalBox(IntelDesc)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

    UBorder* IntelPanel = MakeFramedSetupPanel(WidgetTree, IntelBox, TEXT("IntelPanel"));
    PlaceSetupWidget(MainCanvas, IntelPanel, FVector2D(1190.0f, 140.0f), FVector2D(650.0f, 500.0f), 2);

    // Validation Status Bar (Bottom Row 1)
    UVerticalBox* BannerStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BannerStack"));
    ValidationWarningText = MakeSetupText(
        WidgetTree,
        LOCTEXT("ValidationOk", "Параметры матча проверены. Нажмите «НАЧАТЬ МАТЧ» для загрузки."),
        16, GreenOk, TEXT("ValidationWarningText"), true);
    BannerStack->AddChildToVerticalBox(ValidationWarningText)->SetPadding(FMargin(4.0f, 2.0f, 4.0f, 2.0f));

    ValidationBanner = MakeFramedSetupPanel(WidgetTree, BannerStack, TEXT("ValidationBanner"), FMargin(16.0f, 10.0f));
    PlaceSetupWidget(MainCanvas, ValidationBanner, FVector2D(80.0f, 660.0f), FVector2D(1760.0f, 60.0f), 2);

    // Action Buttons Panel (Bottom Row 2)
    UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActionRow"));

    // Back to Main Menu Button
    USizeBox* BackBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BackBox"));
    BackBox->SetWidthOverride(560.0f);
    BackBox->SetHeightOverride(64.0f);

    UButton* BackButton = MakeSetupButton(
        WidgetTree,
        LOCTEXT("Back", "◄ В ГЛАВНОЕ МЕНЮ"),
        TEXT("BackButton"),
        FLinearColor(0.10f, 0.11f, 0.13f, 0.98f),
        FLinearColor(0.24f, 0.26f, 0.30f, 1.0f),
        FLinearColor(0.06f, 0.07f, 0.08f, 1.0f),
        TextColor,
        18);
    BackButton->OnClicked.AddDynamic(this, &URA4SkirmishSetupWidget::HandleBackClicked);
    BackBox->SetContent(BackButton);

    UHorizontalBoxSlot* BackSlot = ActionRow->AddChildToHorizontalBox(BackBox);
    BackSlot->SetPadding(FMargin(0.0f, 0.0f, 32.0f, 0.0f));
    BackSlot->SetHorizontalAlignment(HAlign_Left);
    BackSlot->SetVerticalAlignment(VAlign_Center);

    // Start Match Button
    USizeBox* StartBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StartBox"));
    StartBox->SetWidthOverride(1168.0f);
    StartBox->SetHeightOverride(64.0f);

    StartButton = MakeSetupButton(
        WidgetTree,
        LOCTEXT("StartMatch", "▶ НАЧАТЬ МАТЧ"),
        TEXT("StartButton"),
        FLinearColor(0.85f, 0.08f, 0.09f, 1.0f),
        FLinearColor(0.98f, 0.15f, 0.16f, 1.0f),
        FLinearColor(0.48f, 0.02f, 0.03f, 1.0f),
        FLinearColor::White,
        22);
    StartButton->OnClicked.AddDynamic(this, &URA4SkirmishSetupWidget::HandleStartMatchClicked);
    StartBox->SetContent(StartButton);

    UHorizontalBoxSlot* StartSlot = ActionRow->AddChildToHorizontalBox(StartBox);
    StartSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    StartSlot->SetHorizontalAlignment(HAlign_Fill);
    StartSlot->SetVerticalAlignment(VAlign_Center);

    PlaceSetupWidget(MainCanvas, ActionRow, FVector2D(80.0f, 736.0f), FVector2D(1760.0f, 68.0f), 3);
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
        if (URA4MainMenuScreenWidget* Menu = CreateWidget<URA4MainMenuScreenWidget>(
            PC, URA4MainMenuScreenWidget::StaticClass()))
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
