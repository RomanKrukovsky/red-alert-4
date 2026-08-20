// Copyright (c) Red Alert 4 project.

#include "RA4HUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RA4AngularPanelWidget.h"
#include "RA4HUDViewModel.h"
#include "RA4UIDataProviderSubsystem.h"

#define LOCTEXT_NAMESPACE "RA4HUDWidget"

namespace
{
constexpr FLinearColor HUDRed(0.92f, 0.05f, 0.035f, 1.0f);
constexpr FLinearColor HUDText(0.90f, 0.88f, 0.83f, 1.0f);
constexpr FLinearColor HUDMuted(0.54f, 0.53f, 0.49f, 1.0f);
constexpr FLinearColor HUDPanel(0.005f, 0.007f, 0.009f, 0.94f);
constexpr FLinearColor HUDGreen(0.22f, 0.92f, 0.25f, 1.0f);

void PlaceHUDWidget(
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

UTextBlock* MakeHUDText(
    UWidgetTree* WidgetTree,
    const FText& Text,
    const int32 Size,
    const FLinearColor& Color,
    const FName Name,
    const bool bBold = false)
{
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    Label->SetText(Text);
    Label->SetColorAndOpacity(FSlateColor(Color));
    Label->SetAutoWrapText(true);
    FSlateFontInfo Font = Label->GetFont();
    Font.Size = Size;
    Font.OutlineSettings.OutlineSize = bBold ? 1 : 0;
    Font.OutlineSettings.OutlineColor = FLinearColor::Black;
    Label->SetFont(Font);
    return Label;
}

URA4AngularPanelWidget* MakeHUDPanel(
    UWidgetTree* WidgetTree,
    UWidget* Content,
    const FName Name)
{
    URA4AngularPanelWidget* Panel = WidgetTree->ConstructWidget<URA4AngularPanelWidget>(
        URA4AngularPanelWidget::StaticClass(), Name);
    Panel->SetPanelRole(ERA4PanelRole::DenseHUD);
    Panel->SetBrushColor(HUDPanel);
    Panel->SetContent(Content);
    return Panel;
}

FButtonStyle MakeHUDButtonStyle(const bool bSelected = false)
{
    const FLinearColor Normal = bSelected
        ? FLinearColor(0.36f, 0.025f, 0.018f, 0.98f)
        : FLinearColor(0.035f, 0.025f, 0.022f, 0.98f);
    FButtonStyle Style;
    Style.SetNormal(FSlateColorBrush(Normal));
    Style.SetHovered(FSlateColorBrush(FLinearColor(0.56f, 0.045f, 0.03f, 1.0f)));
    Style.SetPressed(FSlateColorBrush(HUDRed));
    Style.SetDisabled(FSlateColorBrush(FLinearColor(0.02f, 0.02f, 0.02f, 0.8f)));
    Style.SetNormalPadding(FMargin(5.0f));
    Style.SetPressedPadding(FMargin(6.0f, 7.0f, 4.0f, 3.0f));
    return Style;
}

UButton* MakeHUDButton(
    UWidgetTree* WidgetTree,
    const FText& Label,
    const FName Name,
    const bool bSelected = false)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
    Button->SetStyle(MakeHUDButtonStyle(bSelected));
    UTextBlock* Text = MakeHUDText(
        WidgetTree, Label, 14, bSelected ? FLinearColor::White : HUDText,
        FName(*FString::Printf(TEXT("%s_Label"), *Name.ToString())), bSelected);
    Text->SetJustification(ETextJustify::Center);
    Button->AddChild(Text);
    return Button;
}

FRA4HUDSnapshotView MakeShowcaseSnapshot()
{
    FRA4HUDSnapshotView Snapshot;
    Snapshot.Credits = 23450;
    Snapshot.CreditsDelta = 850;
    Snapshot.PowerProduced = 17820;
    Snapshot.PowerConsumed = 9680;
    Snapshot.SupplyUsed = 88;
    Snapshot.SupplyCap = 200;
    Snapshot.MatchElapsedSeconds = 754;
    Snapshot.SelectionKind = ERA4SelectionKind::SingleBuilding;
    Snapshot.SelectionCount = 1;
    Snapshot.PrimaryEntityName = TEXT("ГЛАВНЫЙ ШТАБ");
    Snapshot.SelectionHealthRatio = 1.0f;
    Snapshot.bPrimaryOwned = true;

    FRA4HUDObjective Primary;
    Primary.Label = LOCTEXT("ShowcaseObjectivePrimary", "Уничтожить базу противника");
    Snapshot.Objectives.Add(Primary);
    FRA4HUDObjective Secondary;
    Secondary.Label = LOCTEXT("ShowcaseObjectiveSecondary", "Захватить хранилище ресурсов");
    Secondary.Current = 1;
    Secondary.Target = 3;
    Snapshot.Objectives.Add(Secondary);

    const FText BuildNames[] = {
        LOCTEXT("BuildPower", "ЭЛЕКТРОСТАНЦИЯ"),
        LOCTEXT("BuildBarracks", "КАЗАРМЫ"),
        LOCTEXT("BuildFactory", "ВОЕННЫЙ ЗАВОД"),
        LOCTEXT("BuildRefinery", "НЕФТЕБАЗА"),
        LOCTEXT("BuildTurret", "ЗЕНИТНАЯ ПУШКА"),
        LOCTEXT("BuildRadar", "РАКЕТНАЯ ШАХТА"),
        LOCTEXT("BuildLab", "ОБСЕРВАТОРИЯ"),
        LOCTEXT("BuildWall", "СТЕНА")
    };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(BuildNames); ++Index)
    {
        FRA4BuildOption Option;
        Option.ContentId = Index + 1;
        Option.DisplayName = BuildNames[Index];
        Option.Cost = 300 + Index * 250;
        Option.Category = Index < 4 ? 0 : 1;
        Option.bAvailable = Index != 5;
        Option.BlockReason = Option.bAvailable
            ? ERA4BuildBlockReason::None
            : ERA4BuildBlockReason::MissingPrerequisite;
        Snapshot.BuildOptions.Add(Option);
    }

    FRA4ProductionEntry Tank;
    Tank.ContentId = 101;
    Tank.DisplayName = LOCTEXT("QueueTank", "ТАНК Т-34");
    Tank.ProgressPercent = 68;
    Tank.RemainingSeconds = 12.0f;
    Snapshot.ProductionQueue.Add(Tank);
    FRA4ProductionEntry Infantry;
    Infantry.ContentId = 102;
    Infantry.DisplayName = LOCTEXT("QueueInfantry", "ШТУРМОВИКИ");
    Infantry.ProgressPercent = 39;
    Infantry.RemainingSeconds = 8.0f;
    Snapshot.ProductionQueue.Add(Infantry);

    FRA4Alert Alert;
    Alert.Message = LOCTEXT("ShowcaseAlert", "НАША БАЗА АТАКОВАНА!");
    Alert.Severity = ERA4AlertSeverity::Critical;
    Snapshot.Alerts.Add(Alert);
    return Snapshot;
}
} // namespace

URA4HUDWidget::URA4HUDWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetScreenIdentity(ERA4UIScreenId::SovietHud);
}

TSharedRef<SWidget> URA4HUDWidget::RebuildWidget()
{
    const TSharedRef<SWidget> RootWidget = Super::RebuildWidget();
    if (!WidgetTree || !GetContentLayer())
    {
        return RootWidget;
    }

    int32 ShowcaseScreen = 0;
    const bool bShowcaseMode = FParse::Value(
        FCommandLine::Get(), TEXT("RA4Screen="), ShowcaseScreen) && ShowcaseScreen >= 13;
    if (bShowcaseMode)
    {
        if (UTexture2D* Background = LoadObject<UTexture2D>(
            nullptr, TEXT("/Game/RA4UI/Art/T_RA4_USSR_CommandCenter.T_RA4_USSR_CommandCenter")))
        {
            GetBackgroundLayer()->SetBrushFromTexture(Background, false);
            GetBackgroundLayer()->SetColorAndOpacity(FLinearColor(0.72f, 0.72f, 0.72f, 1.0f));
        }
    }
    else
    {
        GetBackgroundLayer()->SetBrushFromTexture(nullptr);
        GetBackgroundLayer()->SetColorAndOpacity(FLinearColor::Transparent);
    }
    HUDViewModel = NewObject<URA4HUDViewModel>(this);
    InteractiveRegions.Reset();

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("HUDCanvas"));
    UOverlaySlot* CanvasSlot = GetContentLayer()->AddChildToOverlay(Canvas);
    CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
    CanvasSlot->SetVerticalAlignment(VAlign_Fill);

    UVerticalBox* Objectives = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ObjectivesPanelContent"));
    Objectives->AddChildToVerticalBox(MakeHUDText(
        WidgetTree, LOCTEXT("Commander", "ТОВАРИЩ КОМАНДИР  •  УРОВЕНЬ 45"),
        16, HUDText, TEXT("CommanderTitle"), true));
    Objectives->AddChildToVerticalBox(MakeHUDText(
        WidgetTree, LOCTEXT("ObjectivesHeading", "ОСНОВНЫЕ ЗАДАЧИ"),
        15, HUDRed, TEXT("ObjectivesHeading"), true))->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 5.0f));
    ObjectivesList = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ObjectivesList"));
    Objectives->AddChildToVerticalBox(ObjectivesList);
    const FVector2D ObjectivesPosition(16.0f, 18.0f);
    const FVector2D ObjectivesSize(370.0f, 205.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Objectives, TEXT("ObjectivesPanel")), ObjectivesPosition, ObjectivesSize, 10);
    AddInteractiveRegion(ObjectivesPosition, ObjectivesSize);

    ResourceText = MakeHUDText(
        WidgetTree, FText::GetEmpty(), 17, HUDText, TEXT("ResourceText"), true);
    ResourceText->SetJustification(ETextJustify::Center);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, ResourceText, TEXT("ResourceBarPanel")),
        FVector2D(1110.0f, 12.0f), FVector2D(790.0f, 55.0f), 10);

    UOverlay* Minimap = WidgetTree->ConstructWidget<UOverlay>(
        UOverlay::StaticClass(), TEXT("MinimapPlaceholder"));
    UImage* MinimapImage = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), TEXT("MinimapImage"));
    if (UTexture2D* Texture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_USSR_CommandCenter.T_RA4_USSR_CommandCenter")))
    {
        MinimapImage->SetBrushFromTexture(Texture, false);
    }
    MinimapImage->SetColorAndOpacity(FLinearColor(0.36f, 0.38f, 0.40f, 1.0f));
    UOverlaySlot* MinimapImageSlot = Minimap->AddChildToOverlay(MinimapImage);
    MinimapImageSlot->SetHorizontalAlignment(HAlign_Fill);
    MinimapImageSlot->SetVerticalAlignment(VAlign_Fill);
    UTextBlock* RadarLabel = MakeHUDText(
        WidgetTree, LOCTEXT("RadarOnline", "РАДАР: СЕТЬ АКТИВНА"),
        13, HUDGreen, TEXT("RadarStatus"), true);
    UOverlaySlot* RadarSlot = Minimap->AddChildToOverlay(RadarLabel);
    RadarSlot->SetHorizontalAlignment(HAlign_Left);
    RadarSlot->SetVerticalAlignment(VAlign_Bottom);
    RadarSlot->SetPadding(FMargin(10.0f));
    const FVector2D MinimapPosition(1590.0f, 80.0f);
    const FVector2D MinimapSize(310.0f, 285.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Minimap, TEXT("MinimapPanel")), MinimapPosition, MinimapSize, 10);
    AddInteractiveRegion(MinimapPosition, MinimapSize);

    UVerticalBox* Sidebar = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ProductionSidebar"));
    UButton* TabButton = MakeHUDButton(
        WidgetTree, LOCTEXT("ProductionTabs", "СТРОИТЬ   |   ВОЙСКА   |   УЛУЧШЕНИЯ   |   ДОКТРИНЫ"),
        TEXT("ProductionTabButton"), true);
    TabButton->OnClicked.AddDynamic(this, &URA4HUDWidget::CycleProductionTab);
    Sidebar->AddChildToVerticalBox(TabButton)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    BuildGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(
        UUniformGridPanel::StaticClass(), TEXT("BuildGrid"));
    UVerticalBoxSlot* BuildGridSlot = Sidebar->AddChildToVerticalBox(BuildGrid);
    BuildGridSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    const FVector2D SidebarPosition(1450.0f, 380.0f);
    const FVector2D SidebarSize(450.0f, 500.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Sidebar, TEXT("ProductionPanel")), SidebarPosition, SidebarSize, 10);
    AddInteractiveRegion(SidebarPosition, SidebarSize);

    UVerticalBox* Selection = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("SelectionContent"));
    SelectionTitleText = MakeHUDText(
        WidgetTree, FText::GetEmpty(), 18, HUDText, TEXT("SelectionTitle"), true);
    SelectionDetailText = MakeHUDText(
        WidgetTree, FText::GetEmpty(), 14, HUDMuted, TEXT("SelectionDetails"));
    Selection->AddChildToVerticalBox(SelectionTitleText);
    Selection->AddChildToVerticalBox(SelectionDetailText)->SetPadding(FMargin(0.0f, 8.0f));
    const FVector2D SelectionPosition(16.0f, 820.0f);
    const FVector2D SelectionSize(420.0f, 240.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Selection, TEXT("SelectionPanel")), SelectionPosition, SelectionSize, 10);
    AddInteractiveRegion(SelectionPosition, SelectionSize);

    UVerticalBox* Queue = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ProductionQueueContent"));
    Queue->AddChildToVerticalBox(MakeHUDText(
        WidgetTree, LOCTEXT("QueueHeading", "ОЧЕРЕДЬ ПОСТРОЙКИ"),
        14, HUDText, TEXT("QueueHeading"), true));
    ProductionQueueList = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ProductionQueueList"));
    Queue->AddChildToVerticalBox(ProductionQueueList)->SetPadding(FMargin(0.0f, 7.0f));
    const FVector2D QueuePosition(455.0f, 875.0f);
    const FVector2D QueueSize(560.0f, 185.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Queue, TEXT("ProductionQueuePanel")), QueuePosition, QueueSize, 10);
    AddInteractiveRegion(QueuePosition, QueueSize);

    UVerticalBox* Commands = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CommandGridContent"));
    UHorizontalBox* CommandButtons = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CommandButtons"));
    const FText CommandLabels[] = {
        LOCTEXT("MoveCommand", "ДВИЖЕНИЕ"),
        LOCTEXT("AttackCommand", "АТАКА"),
        LOCTEXT("GuardCommand", "ОХРАНА"),
        LOCTEXT("StopCommand", "СТОП")
    };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(CommandLabels); ++Index)
    {
        UButton* Command = MakeHUDButton(
            WidgetTree, CommandLabels[Index],
            FName(*FString::Printf(TEXT("Command_%d"), Index)), Index == 1);
        Command->OnClicked.AddDynamic(this, &URA4HUDWidget::IssuePrimaryCommand);
        UHorizontalBoxSlot* CommandSlot = CommandButtons->AddChildToHorizontalBox(Command);
        CommandSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        CommandSlot->SetPadding(FMargin(2.0f));
    }
    Commands->AddChildToVerticalBox(CommandButtons);
    CommandStatusText = MakeHUDText(
        WidgetTree, LOCTEXT("CommandReady", "КОМАНДНАЯ СЕТЬ ГОТОВА"),
        13, HUDGreen, TEXT("CommandStatus"));
    Commands->AddChildToVerticalBox(CommandStatusText)->SetPadding(FMargin(4.0f, 10.0f));
    const FVector2D CommandPosition(1450.0f, 900.0f);
    const FVector2D CommandSize(450.0f, 160.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Commands, TEXT("CommandGridPanel")), CommandPosition, CommandSize, 10);
    AddInteractiveRegion(CommandPosition, CommandSize);

    AlertText = MakeHUDText(
        WidgetTree, FText::GetEmpty(), 16, HUDRed, TEXT("AlertText"), true);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, AlertText, TEXT("AlertPanel")),
        FVector2D(16.0f, 250.0f), FVector2D(330.0f, 54.0f), 20);

    ApplyShowcaseSnapshot();
    return RootWidget;
}

void URA4HUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    int32 ShowcaseScreen = 0;
    if (FParse::Value(FCommandLine::Get(), TEXT("RA4Screen="), ShowcaseScreen) && ShowcaseScreen >= 13)
    {
        return;
    }
    if (UWorld* World = GetWorld())
    {
        if (URA4UIDataProviderSubsystem* Provider = World->GetSubsystem<URA4UIDataProviderSubsystem>())
        {
            if (URA4HUDViewModel* ProviderViewModel = Provider->GetHUDViewModel())
            {
                SetHUDViewModel(ProviderViewModel);
            }
        }
    }
}

void URA4HUDWidget::NativeDestruct()
{
    if (HUDViewModel)
    {
        HUDViewModel->OnHUDChanged.RemoveAll(this);
    }
    Super::NativeDestruct();
}

void URA4HUDWidget::SetHUDViewModel(URA4HUDViewModel* InViewModel)
{
    if (HUDViewModel == InViewModel)
    {
        return;
    }
    if (HUDViewModel)
    {
        HUDViewModel->OnHUDChanged.RemoveAll(this);
    }
    HUDViewModel = InViewModel;
    if (HUDViewModel)
    {
        HUDViewModel->OnHUDChanged.AddUObject(this, &URA4HUDWidget::HandleHUDChanged);
    }
    HandleHUDChanged(
        ERA4HUDChangeFlags::Resources | ERA4HUDChangeFlags::Selection |
        ERA4HUDChangeFlags::Production | ERA4HUDChangeFlags::Objectives |
        ERA4HUDChangeFlags::Alerts);
}

void URA4HUDWidget::ApplyShowcaseSnapshot()
{
    if (HUDViewModel)
    {
        HUDViewModel->OnHUDChanged.AddUObject(this, &URA4HUDWidget::HandleHUDChanged);
        HUDViewModel->ApplySnapshot(MakeShowcaseSnapshot());
    }
}

void URA4HUDWidget::HandleHUDChanged(const ERA4HUDChangeFlags Changes)
{
    if (EnumHasAnyFlags(Changes, ERA4HUDChangeFlags::Resources))
    {
        RefreshResources();
    }
    if (EnumHasAnyFlags(Changes, ERA4HUDChangeFlags::Selection))
    {
        RefreshSelection();
    }
    if (EnumHasAnyFlags(Changes, ERA4HUDChangeFlags::Production))
    {
        RefreshProduction();
    }
    if (EnumHasAnyFlags(Changes, ERA4HUDChangeFlags::Objectives))
    {
        RefreshObjectives();
    }
    if (EnumHasAnyFlags(Changes, ERA4HUDChangeFlags::Alerts))
    {
        RefreshAlerts();
    }
}

void URA4HUDWidget::RefreshResources()
{
    if (!HUDViewModel || !ResourceText)
    {
        return;
    }
    const int32 TotalSeconds = HUDViewModel->GetMatchElapsedSeconds();
    ResourceText->SetText(FText::Format(
        LOCTEXT("ResourceFormat", "КРЕДИТЫ  {0}  (+{1})     ЭНЕРГИЯ  {2} / {3}     ЛИМИТ  {4} / {5}     {6}:{7}"),
        FText::AsNumber(HUDViewModel->GetCredits()),
        FText::AsNumber(HUDViewModel->GetCreditsDelta()),
        FText::AsNumber(HUDViewModel->GetPowerProduced()),
        FText::AsNumber(HUDViewModel->GetPowerConsumed()),
        FText::AsNumber(HUDViewModel->GetCommandLimitUsed()),
        FText::AsNumber(HUDViewModel->GetCommandLimitMax()),
        FText::AsNumber(TotalSeconds / 60),
        FText::FromString(FString::Printf(TEXT("%02d"), TotalSeconds % 60))));
    ResourceText->SetColorAndOpacity(FSlateColor(
        HUDViewModel->IsPowerShortage() ? HUDRed : HUDText));
}

void URA4HUDWidget::RefreshSelection()
{
    if (!HUDViewModel || !SelectionTitleText || !SelectionDetailText)
    {
        return;
    }
    const FString Name = HUDViewModel->GetPrimaryEntityName().IsEmpty()
        ? LOCTEXT("NoSelection", "НЕТ ВЫБРАННЫХ ОБЪЕКТОВ").ToString()
        : HUDViewModel->GetPrimaryEntityName();
    SelectionTitleText->SetText(FText::FromString(Name));
    SelectionDetailText->SetText(FText::Format(
        LOCTEXT("SelectionFormat", "ВЫБРАНО: {0}\nПРОЧНОСТЬ: {1}%\nГРУППЫ: {2}\nГРУЗ: {3} / {4}"),
        FText::AsNumber(HUDViewModel->GetSelectionCount()),
        FText::AsNumber(FMath::RoundToInt(HUDViewModel->GetSelectionHealthRatio() * 100.0f)),
        FText::AsNumber(HUDViewModel->GetSelectionGroups().Num()),
        FText::AsNumber(HUDViewModel->GetHarvesterCargo()),
        FText::AsNumber(HUDViewModel->GetHarvesterCapacity())));
}

void URA4HUDWidget::RefreshProduction()
{
    if (!HUDViewModel || !ProductionQueueList || !BuildGrid || !WidgetTree)
    {
        return;
    }
    ProductionQueueList->ClearChildren();
    const TArray<FRA4ProductionEntry>& Queue = HUDViewModel->GetDetailedProductionQueue();
    for (int32 Index = 0; Index < Queue.Num(); ++Index)
    {
        const FRA4ProductionEntry& Entry = Queue[Index];
        ProductionQueueList->AddChildToVerticalBox(MakeHUDText(
            WidgetTree,
            FText::Format(LOCTEXT("QueueRow", "{0}. {1}     {2}%     {3} СЕК."),
                FText::AsNumber(Index + 1), Entry.DisplayName,
                FText::AsNumber(Entry.ProgressPercent),
                FText::AsNumber(FMath::CeilToInt(Entry.RemainingSeconds))),
            14, HUDText,
            FName(*FString::Printf(TEXT("QueueEntry_%d"), Index))));
    }

    BuildGrid->ClearChildren();
    int32 VisibleIndex = 0;
    for (const FRA4BuildOption& Option : HUDViewModel->GetBuildOptions())
    {
        if (Option.Category != ActiveProductionTab)
        {
            continue;
        }
        const FText CardLabel = FText::Format(
            LOCTEXT("BuildCard", "{0}\n{1}"), Option.DisplayName, FText::AsNumber(Option.Cost));
        UButton* Card = MakeHUDButton(
            WidgetTree, CardLabel,
            FName(*FString::Printf(TEXT("BuildOption_%d"), VisibleIndex)));
        Card->SetIsEnabled(Option.bAvailable);
        Card->OnClicked.AddDynamic(this, &URA4HUDWidget::QueueSelectedProduction);
        UUniformGridSlot* CardSlot = BuildGrid->AddChildToUniformGrid(
            Card, VisibleIndex / 3, VisibleIndex % 3);
        CardSlot->SetHorizontalAlignment(HAlign_Fill);
        CardSlot->SetVerticalAlignment(VAlign_Fill);
        ++VisibleIndex;
    }
}

void URA4HUDWidget::RefreshObjectives()
{
    if (!HUDViewModel || !ObjectivesList || !WidgetTree)
    {
        return;
    }
    ObjectivesList->ClearChildren();
    for (int32 Index = 0; Index < HUDViewModel->GetObjectives().Num(); ++Index)
    {
        const FRA4HUDObjective& Objective = HUDViewModel->GetObjectives()[Index];
        const FString Prefix = Objective.bCompleted ? TEXT("✓") : TEXT("□");
        const FString Progress = Objective.Target > 0
            ? FString::Printf(TEXT("  (%d/%d)"), Objective.Current, Objective.Target)
            : FString();
        ObjectivesList->AddChildToVerticalBox(MakeHUDText(
            WidgetTree,
            FText::FromString(Prefix + TEXT("  ") + Objective.Label.ToString() + Progress),
            14, Objective.bCompleted ? HUDGreen : HUDText,
            FName(*FString::Printf(TEXT("Objective_%d"), Index))));
    }
}

void URA4HUDWidget::RefreshAlerts()
{
    if (!HUDViewModel || !AlertText)
    {
        return;
    }
    const TArray<FRA4Alert>& Alerts = HUDViewModel->GetAlerts();
    AlertText->SetText(Alerts.IsEmpty() ? FText::GetEmpty() : Alerts.Last().Message);
    AlertText->SetVisibility(Alerts.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void URA4HUDWidget::AddInteractiveRegion(const FVector2D Position, const FVector2D Size)
{
    InteractiveRegions.Emplace(Position, Position + Size);
}

bool URA4HUDWidget::IsWorldInputBlockedAtReferencePoint(const FVector2D Point) const
{
    for (const FBox2D& Region : InteractiveRegions)
    {
        if (Region.IsInside(Point))
        {
            return true;
        }
    }
    return false;
}

void URA4HUDWidget::CycleProductionTab()
{
    ActiveProductionTab = (ActiveProductionTab + 1) % 2;
    RefreshProduction();
    if (CommandStatusText)
    {
        CommandStatusText->SetText(ActiveProductionTab == 0
            ? LOCTEXT("StructuresTab", "КАТЕГОРИЯ: СТРОЕНИЯ")
            : LOCTEXT("DefencesTab", "КАТЕГОРИЯ: ОБОРОНА"));
    }
}

void URA4HUDWidget::QueueSelectedProduction()
{
    if (CommandStatusText)
    {
        CommandStatusText->SetText(LOCTEXT("OrderAccepted", "ЗАКАЗ ПРИНЯТ В ПРОИЗВОДСТВО"));
    }
}

void URA4HUDWidget::IssuePrimaryCommand()
{
    if (CommandStatusText)
    {
        CommandStatusText->SetText(LOCTEXT("CommandIssued", "ПРИКАЗ ПЕРЕДАН ПОДРАЗДЕЛЕНИЮ"));
    }
}

#undef LOCTEXT_NAMESPACE
