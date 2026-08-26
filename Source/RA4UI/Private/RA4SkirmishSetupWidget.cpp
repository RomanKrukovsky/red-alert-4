// Copyright (c) Red Alert 4 project.
#include "RA4SkirmishSetupWidget.h"

#include "RA4MainMenuScreenWidget.h"
#include "RA4Core/Ids.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/Base64.h"
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
    MapCombo->AddOption(TEXT("Архипелаг — Холмы и Проливы (до 9 участников)"));
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

    // Player slots: one local commander plus eight independently configurable AI vacancies.
    UVerticalBox* RightBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightBox"));
    UTextBlock* SlotsHeader = MakeSetupText(WidgetTree, LOCTEXT("SlotsHeader", "СЛОТЫ УЧАСТНИКОВ"), 18, Red, TEXT("SlotsHeader"));
    RightBox->AddChildToVerticalBox(SlotsHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

    SlotStatusCombos.Reset();
    SlotFactionCombos.Reset();
    SlotTeamCombos.Reset();
    SlotSpotCombos.Reset();
    for (int32 SlotIndex = 0; SlotIndex < RA4::kMaxPlayers; ++SlotIndex)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(), FName(*FString::Printf(TEXT("SlotRow%d"), SlotIndex)));

        UTextBlock* SlotLabel = MakeSetupText(WidgetTree,
            FText::FromString(FString::Printf(TEXT("%d"), SlotIndex + 1)), 14, TextColor,
            FName(*FString::Printf(TEXT("SlotLabel%d"), SlotIndex)));
        USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        LabelBox->SetWidthOverride(38.0f);
        LabelBox->SetContent(SlotLabel);
        Row->AddChildToHorizontalBox(LabelBox);

        UComboBoxString* Status = WidgetTree->ConstructWidget<UComboBoxString>(
            UComboBoxString::StaticClass(), FName(*FString::Printf(TEXT("SlotStatus%d"), SlotIndex)));
        if (SlotIndex == 0)
        {
            Status->AddOption(TEXT("Игрок"));
        }
        else
        {
            Status->AddOption(TEXT("ИИ"));
            Status->AddOption(TEXT("Закрыто"));
            Status->SetSelectedIndex(SlotIndex == 1 ? 0 : 1);
        }
        Status->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
        USizeBox* StatusBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        StatusBox->SetWidthOverride(180.0f);
        StatusBox->SetContent(Status);
        Row->AddChildToHorizontalBox(StatusBox)->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
        SlotStatusCombos.Add(Status);

        UComboBoxString* Faction = WidgetTree->ConstructWidget<UComboBoxString>(
            UComboBoxString::StaticClass(), FName(*FString::Printf(TEXT("SlotFaction%d"), SlotIndex)));
        Faction->AddOption(TEXT("Евразийский пакт"));
        Faction->AddOption(TEXT("Атлантический альянс"));
        Faction->SetSelectedIndex(SlotIndex % 2);
        Faction->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
        USizeBox* FactionBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        FactionBox->SetWidthOverride(300.0f);
        FactionBox->SetContent(Faction);
        Row->AddChildToHorizontalBox(FactionBox)->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
        SlotFactionCombos.Add(Faction);

        UComboBoxString* Team = WidgetTree->ConstructWidget<UComboBoxString>(
            UComboBoxString::StaticClass(), FName(*FString::Printf(TEXT("SlotTeam%d"), SlotIndex)));
        Team->AddOption(TEXT("Без союза"));
        Team->AddOption(TEXT("Союз A"));
        Team->AddOption(TEXT("Союз B"));
        Team->AddOption(TEXT("Союз C"));
        Team->AddOption(TEXT("Союз D"));
        Team->SetSelectedIndex(0);
        Team->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
        USizeBox* TeamBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        TeamBox->SetWidthOverride(210.0f);
        TeamBox->SetContent(Team);
        Row->AddChildToHorizontalBox(TeamBox)->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
        SlotTeamCombos.Add(Team);

        UComboBoxString* Spot = WidgetTree->ConstructWidget<UComboBoxString>(
            UComboBoxString::StaticClass(), FName(*FString::Printf(TEXT("SlotSpot%d"), SlotIndex)));
        for (int32 SpotIndex = 0; SpotIndex < RA4::kMaxPlayers; ++SpotIndex)
        {
            Spot->AddOption(FString::Printf(TEXT("Позиция %d"), SpotIndex + 1));
        }
        Spot->SetSelectedIndex(SlotIndex);
        Spot->OnSelectionChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleOptionChanged);
        USizeBox* SpotBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        SpotBox->SetWidthOverride(180.0f);
        SpotBox->SetContent(Spot);
        Row->AddChildToHorizontalBox(SpotBox);
        SlotSpotCombos.Add(Spot);

        RightBox->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 2.0f));
    }

    UHorizontalBox* AllianceRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AllianceNames"));
    AllianceNameEdits.Reset();
    for (int32 TeamIndex = 0; TeamIndex < 4; ++TeamIndex)
    {
        UEditableTextBox* NameEdit = WidgetTree->ConstructWidget<UEditableTextBox>(
            UEditableTextBox::StaticClass(), FName(*FString::Printf(TEXT("AllianceName%d"), TeamIndex)));
        NameEdit->SetText(FText::FromString(FString::Printf(TEXT("Союз %c"), TCHAR('A' + TeamIndex))));
        NameEdit->SetHintText(FText::FromString(FString::Printf(TEXT("Имя союза %c"), TCHAR('A' + TeamIndex))));
        NameEdit->OnTextChanged.AddDynamic(this, &URA4SkirmishSetupWidget::HandleAllianceNameChanged);
        USizeBox* NameBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        NameBox->SetWidthOverride(268.0f);
        NameBox->SetContent(NameEdit);
        AllianceRow->AddChildToHorizontalBox(NameBox)->SetPadding(FMargin(0.0f, 8.0f, 8.0f, 0.0f));
        AllianceNameEdits.Add(NameEdit);
    }
    RightBox->AddChildToVerticalBox(AllianceRow)->SetPadding(FMargin(38.0f, 8.0f, 0.0f, 0.0f));

    UBorder* RightPanel = MakeFramedSetupPanel(WidgetTree, RightBox, TEXT("RightPanel"));
    PlaceSetupWidget(MainCanvas, RightPanel, FVector2D(630.0f, 140.0f), FVector2D(1210.0f, 500.0f), 2);

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

    if (DifficultyCombo) DifficultyIndex = DifficultyCombo->GetSelectedIndex();
    if (CreditsCombo) CreditsIndex = CreditsCombo->GetSelectedIndex();

    UpdateConflictValidation();
}

void URA4SkirmishSetupWidget::HandleAllianceNameChanged(const FText& Text)
{
    (void)Text;
    for (UComboBoxString* TeamCombo : SlotTeamCombos)
    {
        const int32 SelectedTeam = TeamCombo->GetSelectedIndex();
        TeamCombo->ClearOptions();
        TeamCombo->AddOption(TEXT("Без союза"));
        for (int32 TeamIndex = 0; TeamIndex < AllianceNameEdits.Num(); ++TeamIndex)
        {
            const FString Name = AllianceNameEdits[TeamIndex]->GetText().ToString();
            TeamCombo->AddOption(Name.IsEmpty()
                ? FString::Printf(TEXT("Союз %c"), TCHAR('A' + TeamIndex))
                : Name);
        }
        TeamCombo->SetSelectedIndex(SelectedTeam);
    }
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

    TSet<int32> UsedSpots;
    TSet<int32> HostileSides;
    int32 ActiveSlots = 0;
    for (int32 Slot = 0; Slot < SlotStatusCombos.Num(); ++Slot)
    {
        const bool bActive = Slot == 0 || SlotStatusCombos[Slot]->GetSelectedIndex() == 0;
        if (!bActive)
        {
            continue;
        }
        ++ActiveSlots;
        const int32 Team = SlotTeamCombos[Slot]->GetSelectedIndex();
        HostileSides.Add(Team == 0 ? 100 + Slot : Team);
        const int32 Spot = SlotSpotCombos[Slot]->GetSelectedIndex();
        if (UsedSpots.Contains(Spot))
        {
            bHasConflict = true;
            Message = LOCTEXT("SpotConflict", "ОШИБКА: активные участники не могут занимать одну стартовую позицию.");
            break;
        }
        UsedSpots.Add(Spot);
    }
    if (!bHasConflict && ActiveSlots < 2)
    {
        bHasConflict = true;
        Message = LOCTEXT("OpponentRequired", "ОШИБКА: откройте хотя бы одну вакансию ИИ.");
    }
    if (!bHasConflict && HostileSides.Num() < 2)
    {
        bHasConflict = true;
        Message = LOCTEXT("HostileSideRequired", "ОШИБКА: все участники находятся в одном союзе; назначьте хотя бы две стороны.");
    }
    if (!bHasConflict)
    {
        for (int32 TeamIndex = 0; TeamIndex < AllianceNameEdits.Num(); ++TeamIndex)
        {
            if (AllianceNameEdits[TeamIndex]->GetText().IsEmpty())
            {
                bHasConflict = true;
                Message = LOCTEXT("AllianceNameRequired", "ОШИБКА: названия союзов не могут быть пустыми.");
                break;
            }
        }
    }
    if (!bHasConflict)
    {
        Message = FText::Format(LOCTEXT("ValidationOkCount", "Готово: {0} участников, включая {1} противников."),
            FText::AsNumber(ActiveSlots), FText::AsNumber(ActiveSlots - 1));
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
        FString Options = FString::Printf(TEXT("?Difficulty=%d?Credits=%d"), DifficultyIndex, CreditsIndex);
        for (int32 Slot = 0; Slot < SlotStatusCombos.Num(); ++Slot)
        {
            const int32 bActive = Slot == 0 || SlotStatusCombos[Slot]->GetSelectedIndex() == 0 ? 1 : 0;
            Options += FString::Printf(TEXT("?Slot%d=%d,%d,%d,%d"), Slot, bActive,
                SlotFactionCombos[Slot]->GetSelectedIndex(),
                SlotTeamCombos[Slot]->GetSelectedIndex(),
                SlotSpotCombos[Slot]->GetSelectedIndex());
        }
        for (int32 TeamIndex = 0; TeamIndex < AllianceNameEdits.Num(); ++TeamIndex)
        {
            Options += FString::Printf(TEXT("?Alliance%d=%s"), TeamIndex + 1,
                *FBase64::Encode(AllianceNameEdits[TeamIndex]->GetText().ToString()));
        }

        UGameplayStatics::OpenLevel(World, TEXT("/Game/Maps/RA4_Skirmish_Production"), true, Options);
    }
}

#undef LOCTEXT_NAMESPACE
