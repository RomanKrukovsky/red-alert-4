// Copyright (c) Red Alert 4 project.
#include "RA4SkirmishSetupWidget.h"

#include "RA4CommandCentreMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
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

    MainCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MainCanvas"));
    UOverlaySlot* CanvasSlot = Root->AddChildToOverlay(MainCanvas);
    CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
    CanvasSlot->SetVerticalAlignment(VAlign_Fill);

    // Title Header
    UTextBlock* Title = MakeSetupText(WidgetTree, LOCTEXT("Title", "НАСТРОЙКА СХВАТКИ (SKIRMISH)"), 36, Red, TEXT("TitleText"));
    PlaceSetupWidget(MainCanvas, Title, FVector2D(80.0f, 40.0f), FVector2D(800.0f, 60.0f), 2);

    UTextBlock* Subtitle = MakeSetupText(
        WidgetTree, LOCTEXT("Subtitle", "Параметры сражения, выбор фракций, стартовых позиций и правил"), 14, Muted, TEXT("SubtitleText"), false);
    PlaceSetupWidget(MainCanvas, Subtitle, FVector2D(82.0f, 95.0f), FVector2D(800.0f, 30.0f), 2);

    // Left Column: Map & Game Options
    UVerticalBox* LeftBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftBox"));

    // Map Selection
    UTextBlock* MapLabel = MakeSetupText(WidgetTree, LOCTEXT("MapLabel", "КАРТА СРАЖЕНИЯ"), 16, TextColor, TEXT("MapLabelText"));
    LeftBox->AddChildToVerticalBox(MapLabel)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 4.0f));

    MapCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("MapCombo"));
    MapCombo->AddOption(TEXT("RA4_Skirmish — Равнина Колымы (2 игрока)"));
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
    DifficultyCombo->AddOption(TEXT("Легкий (Easy)"));
    DifficultyCombo->AddOption(TEXT("Средний (Medium)"));
    DifficultyCombo->AddOption(TEXT("Тяжёлый (Hard)"));
    DifficultyCombo->AddOption(TEXT("Безумный (Brutal)"));
    DifficultyCombo->SetSelectedIndex(1);
    DifficultyCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    LeftBox->AddChildToVerticalBox(DifficultyCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

    UBorder* LeftPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LeftPanel"));
    LeftPanel->SetBrushColor(Panel);
    LeftPanel->SetPadding(FMargin(16.0f));
    LeftPanel->SetContent(LeftBox);
    PlaceSetupWidget(MainCanvas, LeftPanel, FVector2D(80.0f, 140.0f), FVector2D(520.0f, 520.0f), 2);

    // Right Column: Player & AI Setup
    UVerticalBox* RightBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightBox"));

    // Player Setup
    UTextBlock* PlayerHeader = MakeSetupText(WidgetTree, LOCTEXT("PlayerHeader", "ИГРОК 1 (HUMAN)"), 18, Red, TEXT("PlayerHeader"));
    RightBox->AddChildToVerticalBox(PlayerHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    PlayerFactionCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PlayerFactionCombo"));
    PlayerFactionCombo->AddOption(TEXT("Фракция: СССР (Soviet Union)"));
    PlayerFactionCombo->AddOption(TEXT("Фракция: Альянс (Alliance)"));
    PlayerFactionCombo->AddOption(TEXT("Фракция: Коалиция (Coalition)"));
    PlayerFactionCombo->AddOption(TEXT("Фракция: Хроно (Chrono)"));
    PlayerFactionCombo->SetSelectedIndex(0);
    PlayerFactionCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(PlayerFactionCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    PlayerColorCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PlayerColorCombo"));
    PlayerColorCombo->AddOption(TEXT("Цвет: Красный"));
    PlayerColorCombo->AddOption(TEXT("Цвет: Синий"));
    PlayerColorCombo->AddOption(TEXT("Цвет: Зелёный"));
    PlayerColorCombo->AddOption(TEXT("Цвет: Жёлтый"));
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
    UTextBlock* AIHeader = MakeSetupText(WidgetTree, LOCTEXT("AIHeader", "ИГРОК 2 (AI COMMANDER)"), 18, Red, TEXT("AIHeader"));
    RightBox->AddChildToVerticalBox(AIHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    AIFactionCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("AIFactionCombo"));
    AIFactionCombo->AddOption(TEXT("Фракция: СССР (Soviet Union)"));
    AIFactionCombo->AddOption(TEXT("Фракция: Альянс (Alliance)"));
    AIFactionCombo->AddOption(TEXT("Фракция: Коалиция (Coalition)"));
    AIFactionCombo->AddOption(TEXT("Фракция: Хроно (Chrono)"));
    AIFactionCombo->SetSelectedIndex(1);
    AIFactionCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(AIFactionCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    AIColorCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("AIColorCombo"));
    AIColorCombo->AddOption(TEXT("Цвет: Красный"));
    AIColorCombo->AddOption(TEXT("Цвет: Синий"));
    AIColorCombo->AddOption(TEXT("Цвет: Зелёный"));
    AIColorCombo->AddOption(TEXT("Цвет: Жёлтый"));
    AIColorCombo->SetSelectedIndex(1);
    AIColorCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(AIColorCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    AISpotCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("AISpotCombo"));
    AISpotCombo->AddOption(TEXT("Старт: Позиция 1 (Запад)"));
    AISpotCombo->AddOption(TEXT("Старт: Позиция 2 (Восток)"));
    AISpotCombo->SetSelectedIndex(1);
    AISpotCombo->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
    RightBox->AddChildToVerticalBox(AISpotCombo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));

    UBorder* RightPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightPanel"));
    RightPanel->SetBrushColor(Panel);
    RightPanel->SetPadding(FMargin(16.0f));
    RightPanel->SetContent(RightBox);
    PlaceSetupWidget(MainCanvas, RightPanel, FVector2D(640.0f, 140.0f), FVector2D(520.0f, 520.0f), 2);

    // Validation & Action Banner (Bottom)
    ValidationBanner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ValidationBanner"));
    ValidationBanner->SetBrushColor(FLinearColor(0.04f, 0.01f, 0.01f, 0.95f));
    ValidationBanner->SetPadding(FMargin(16.0f, 12.0f));

    UVerticalBox* BannerStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BannerStack"));

    ValidationWarningText = MakeSetupText(
        WidgetTree, LOCTEXT("ValidationOk", "Параметры матча корректны."), 15, GreenOk, TEXT("ValidationWarningText"));
    BannerStack->AddChildToVerticalBox(ValidationWarningText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

    UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActionRow"));

    UButton* BackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackButton"));
    BackButton->AddChild(MakeSetupText(WidgetTree, LOCTEXT("Back", "НАЗАД В МЕНЮ"), 18, TextColor, TEXT("BackLabel")));
    BackButton->OnClicked.AddDynamic(this, &URA4SkirmishSetupWidget::HandleBackClicked);
    ActionRow->AddChildToHorizontalBox(BackButton)->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));

    StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
    StartButton->SetBackgroundColor(RedDim);
    StartButton->AddChild(MakeSetupText(WidgetTree, LOCTEXT("StartMatch", "НАЧАТЬ МАТЧ"), 20, TextColor, TEXT("StartMatchLabel")));
    StartButton->OnClicked.AddDynamic(this, &URA4SkirmishSetupWidget::HandleStartMatchClicked);
    ActionRow->AddChildToHorizontalBox(StartButton);

    BannerStack->AddChildToVerticalBox(ActionRow);
    ValidationBanner->SetContent(BannerStack);
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
        Message = LOCTEXT("ColorConflict", "ОШИБКА: Игрок и ИИ выставили одинаковый цвет! Выберите разные цвета.");
    }
    else if (PlayerSpotIndex == AISpotIndex)
    {
        bHasConflict = true;
        Message = LOCTEXT("SpotConflict", "ОШИБКА: Конфликт стартовых позиций! Игроки не могут стартовать на одном спавне.");
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

        UGameplayStatics::OpenLevel(World, TEXT("/Game/Maps/RA4_Skirmish"), true, Options);
    }
}

#undef LOCTEXT_NAMESPACE
