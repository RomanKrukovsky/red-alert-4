// Copyright (c) Red Alert 4 project.

#include "RA4MissionFlowWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "RA4AngularPanelWidget.h"
#include "RA4CampaignScreenWidget.h"

#define LOCTEXT_NAMESPACE "RA4MissionFlowWidgets"

namespace
{
constexpr FLinearColor MissionRed(0.92f, 0.035f, 0.04f, 1.0f);
constexpr FLinearColor MissionBlue(0.14f, 0.54f, 1.0f, 1.0f);
constexpr FLinearColor MissionText(0.88f, 0.84f, 0.78f, 1.0f);
constexpr FLinearColor MissionMuted(0.58f, 0.56f, 0.54f, 1.0f);
constexpr FLinearColor MissionPanel(0.012f, 0.008f, 0.010f, 0.94f);

void PlaceMissionWidget(
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

UTextBlock* MakeMissionText(
    UWidgetTree* Tree,
    const FText& Text,
    const int32 Size,
    const FLinearColor& Color,
    const FName Name,
    const bool bBold = true)
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
        FontInfo.LetterSpacing = bBold ? 45 : 14;
        Label->SetFont(FontInfo);
    }
    Label->SetShadowOffset(FVector2D(1.0f, 2.0f));
    Label->SetShadowColorAndOpacity(FLinearColor::Black);
    return Label;
}

URA4AngularPanelWidget* MakeMissionPanel(
    UWidgetTree* Tree,
    UWidget* Content,
    const FName Name,
    const FLinearColor& Color = MissionPanel,
    const ERA4PanelRole Role = ERA4PanelRole::Standard)
{
    URA4AngularPanelWidget* Panel = Tree->ConstructWidget<URA4AngularPanelWidget>(
        URA4AngularPanelWidget::StaticClass(), Name);
    Panel->SetPanelRole(Role);
    FLinearColor Outline = (Color * 4.0f).GetClamped();
    if (FMath::Max3(Outline.R, Outline.G, Outline.B) < 0.20f)
    {
        Outline = MissionRed * 0.72f;
    }
    Outline.A = 1.0f;
    Panel->SetBrush(FSlateRoundedBoxBrush(Color, 0.0f, Outline, 1.4f));
    Panel->SetContent(Content);
    return Panel;
}

FButtonStyle MakeMissionButtonStyle(
    const FLinearColor& Accent = MissionRed,
    const bool bSelected = false)
{
    FButtonStyle Style;
    Style.SetNormal(FSlateColorBrush(bSelected
        ? FLinearColor(Accent.R * 0.30f, Accent.G * 0.30f, Accent.B * 0.30f, 0.98f)
        : FLinearColor(0.018f, 0.012f, 0.014f, 0.96f)));
    Style.SetHovered(FSlateColorBrush(FLinearColor(
        Accent.R * 0.50f, Accent.G * 0.50f, Accent.B * 0.50f, 1.0f)));
    Style.SetPressed(FSlateColorBrush(Accent));
    Style.SetDisabled(FSlateColorBrush(FLinearColor(0.01f, 0.01f, 0.012f, 0.42f)));
    return Style;
}

UImage* MakeImage(UWidgetTree* Tree, const TCHAR* TexturePath, const FName Name)
{
    UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
    if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TexturePath))
    {
        Image->SetBrushFromTexture(Texture, false);
    }
    Image->SetVisibility(ESlateVisibility::HitTestInvisible);
    return Image;
}

void SetRootBackground(URA4ScreenRootWidget* Screen, const TCHAR* TexturePath, const FLinearColor Tint)
{
    if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TexturePath))
    {
        Screen->GetBackgroundLayer()->SetBrushFromTexture(Texture, false);
        Screen->GetBackgroundLayer()->SetColorAndOpacity(Tint);
    }
}

UCanvasPanel* AddCanvas(URA4ScreenRootWidget* Screen, UWidgetTree* Tree, const FName Name)
{
    UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), Name);
    UOverlaySlot* CanvasSlot = Screen->GetContentLayer()->AddChildToOverlay(Canvas);
    CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
    CanvasSlot->SetVerticalAlignment(VAlign_Fill);
    return Canvas;
}
} // namespace

void URA4MissionNodeButton::InitializeMissionNode(
    URA4MissionMapScreenWidget* InOwner,
    const FName InMissionId)
{
    MissionOwner = InOwner;
    MissionId = InMissionId;
    OnClicked.AddDynamic(this, &URA4MissionNodeButton::HandleMissionClicked);
}

void URA4MissionNodeButton::HandleMissionClicked()
{
    if (MissionOwner)
    {
        MissionOwner->SelectMissionById(MissionId);
    }
}

URA4MissionMapScreenWidget::URA4MissionMapScreenWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetScreenIdentity(ERA4UIScreenId::MissionMap);
}

TSharedRef<SWidget> URA4MissionMapScreenWidget::RebuildWidget()
{
    SetScreenIdentity(ERA4UIScreenId::MissionMap);
    const TSharedRef<SWidget> RootWidget = Super::RebuildWidget();
    if (!WidgetTree || !GetContentLayer())
    {
        return RootWidget;
    }

    CampaignViewModel = NewObject<URA4CampaignViewModel>(this);
    CampaignViewModel->SelectFaction(ERA4FactionTheme::USSR);
    CampaignViewModel->SelectMission(TEXT("ussr_operation_molot"));
    MissionButtons.Reset();
    SetRootBackground(
        this,
        TEXT("/Game/RA4UI/Art/T_RA4_USSR_MissionMap.T_RA4_USSR_MissionMap"),
        FLinearColor(0.82f, 0.82f, 0.82f, 1.0f));

    UCanvasPanel* Canvas = AddCanvas(this, WidgetTree, TEXT("MissionMapCanvas"));

    UImage* Logo = MakeImage(
        WidgetTree, TEXT("/Game/RA4UI/Art/T_RA4_Logo.T_RA4_Logo"), TEXT("MissionMapLogo"));
    PlaceMissionWidget(Canvas, Logo, FVector2D(610.0f, 4.0f), FVector2D(700.0f, 180.0f), 3);

    UVerticalBox* CampaignTitle = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("MissionCampaignTitle"));
    CampaignTitle->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("MissionCampaign", "★  КАМПАНИЯ: СССР"), 28,
        MissionRed, TEXT("MissionCampaignHeading")));
    CampaignTitle->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("MissionChapter", "ГЛАВА 3: КРАСНЫЙ ШТОРМ"), 17,
        MissionText, TEXT("MissionChapterHeading"), false));
    PlaceMissionWidget(Canvas, CampaignTitle, FVector2D(24.0f, 130.0f), FVector2D(520.0f, 90.0f), 4);

    UBorder* TacticalMap = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("TacticalMapSurface"));
    TacticalMap->SetBrushColor(FLinearColor(0.04f, 0.002f, 0.004f, 0.16f));
    PlaceMissionWidget(Canvas, TacticalMap, FVector2D(24.0f, 220.0f), FVector2D(1325.0f, 690.0f), 2);

    const FVector2D NodePositions[] = {
        FVector2D(210.0f, 360.0f), FVector2D(280.0f, 520.0f),
        FVector2D(545.0f, 285.0f), FVector2D(560.0f, 460.0f),
        FVector2D(840.0f, 245.0f), FVector2D(1040.0f, 430.0f),
        FVector2D(970.0f, 620.0f), FVector2D(900.0f, 780.0f),
        FVector2D(690.0f, 610.0f), FVector2D(1160.0f, 690.0f)
    };
    const TArray<FRA4MissionNodeView>& Missions = CampaignViewModel->GetMissionNodes();
    for (int32 Index = 0; Index < Missions.Num(); ++Index)
    {
        const FRA4MissionNodeView& Mission = Missions[Index];
        const bool bSelected = Mission.ContentId == CampaignViewModel->GetSelectedMissionId();
        URA4MissionNodeButton* Button = WidgetTree->ConstructWidget<URA4MissionNodeButton>(
            URA4MissionNodeButton::StaticClass(),
            FName(*FString::Printf(TEXT("MissionNode_%d"), Index)));
        Button->InitializeMissionNode(this, Mission.ContentId);
        Button->SetStyle(MakeMissionButtonStyle(MissionRed, bSelected));
        Button->SetIsEnabled(!Mission.bLocked);
        UTextBlock* Label = MakeMissionText(
            WidgetTree,
            FText::FromString(FString::Printf(TEXT("★  %02d  %s"), Mission.MissionNumber, *Mission.Location.ToString())),
            bSelected ? 17 : 14,
            bSelected ? FLinearColor::White : MissionText,
            FName(*FString::Printf(TEXT("MissionNodeLabel_%d"), Index)));
        Button->AddChild(Label);
        MissionButtons.Add(Button);
        PlaceMissionWidget(
            Canvas, Button, NodePositions[Index % UE_ARRAY_COUNT(NodePositions)],
            bSelected ? FVector2D(285.0f, 72.0f) : FVector2D(210.0f, 56.0f), 6);
    }

    UVerticalBox* ChapterProgress = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ChapterProgress"));
    ChapterProgress->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("ChapterProgressTitle", "ПРОГРЕСС ГЛАВЫ"), 18,
        MissionRed, TEXT("ChapterProgressTitle")));
    UProgressBar* Progress = WidgetTree->ConstructWidget<UProgressBar>(
        UProgressBar::StaticClass(), TEXT("ChapterProgressBar"));
    Progress->SetPercent(8.0f / 12.0f);
    Progress->SetFillColorAndOpacity(MissionRed);
    ChapterProgress->AddChildToVerticalBox(Progress)->SetPadding(FMargin(0.0f, 12.0f));
    ChapterProgress->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("CompletedMissions", "ВЫПОЛНЕНО МИССИЙ                         8 / 12"),
        14, MissionText, TEXT("CompletedMissions"), false));
    PlaceMissionWidget(Canvas, MakeMissionPanel(
        WidgetTree, ChapterProgress, TEXT("ChapterProgressPanel"), MissionPanel, ERA4PanelRole::Compact),
        FVector2D(24.0f, 705.0f), FVector2D(390.0f, 185.0f), 7);

    UVerticalBox* Details = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("MissionDetails"));
    MissionTitleText = MakeMissionText(
        WidgetTree, FText::GetEmpty(), 25, MissionRed, TEXT("SelectedMissionTitle"));
    Details->AddChildToVerticalBox(MissionTitleText)->SetPadding(FMargin(4.0f, 4.0f, 4.0f, 12.0f));
    UImage* MissionPreview = MakeImage(
        WidgetTree,
        TEXT("/Game/RA4UI/Art/T_RA4_USSR_CommandCenter.T_RA4_USSR_CommandCenter"),
        TEXT("MissionPreview"));
    Details->AddChildToVerticalBox(MissionPreview)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 16.0f));
    Details->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("MissionObjectiveHeading", "ЦЕЛЬ МИССИИ"), 17,
        MissionRed, TEXT("MissionObjectiveHeading")));
    MissionObjectiveText = MakeMissionText(
        WidgetTree, FText::GetEmpty(), 16, MissionText, TEXT("SelectedMissionObjective"), false);
    Details->AddChildToVerticalBox(MissionObjectiveText)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 18.0f));
    Details->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("Rewards", "НАГРАДЫ\n★  1 500     ▣  10 000     НОВАЯ ТЕХНОЛОГИЯ"),
        16, MissionText, TEXT("MissionRewards"), false))->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 18.0f));
    Details->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("MissionDifficulty", "СЛОЖНОСТЬ                                  ВЕТЕРАН"),
        16, MissionRed, TEXT("MissionDifficulty"), false));
    UButton* StartButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("StartMissionButton"));
    StartButton->SetStyle(MakeMissionButtonStyle(MissionRed, true));
    UTextBlock* StartLabel = MakeMissionText(
        WidgetTree, LOCTEXT("StartMission", "★   НАЧАТЬ МИССИЮ"), 26,
        FLinearColor::White, TEXT("StartMissionLabel"));
    StartLabel->SetJustification(ETextJustify::Center);
    StartButton->AddChild(StartLabel);
    StartButton->OnClicked.AddDynamic(this, &URA4MissionMapScreenWidget::StartSelectedMission);
    Details->AddChildToVerticalBox(StartButton)->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 4.0f));
    PlaceMissionWidget(Canvas, MakeMissionPanel(
        WidgetTree, Details, TEXT("MissionDetailsPanel"), MissionPanel, ERA4PanelRole::Standard),
        FVector2D(1370.0f, 190.0f), FVector2D(520.0f, 720.0f), 8);

    UButton* BackButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("MissionMapBackButton"));
    BackButton->SetStyle(MakeMissionButtonStyle());
    UTextBlock* BackLabel = MakeMissionText(
        WidgetTree, LOCTEXT("Back", "‹   НАЗАД"), 18, MissionText, TEXT("MissionMapBackLabel"));
    BackLabel->SetJustification(ETextJustify::Center);
    BackButton->AddChild(BackLabel);
    BackButton->OnClicked.AddDynamic(this, &URA4MissionMapScreenWidget::GoBack);
    PlaceMissionWidget(Canvas, BackButton, FVector2D(24.0f, 952.0f), FVector2D(230.0f, 78.0f), 9);

    RefreshMissionDetails();
    return RootWidget;
}

void URA4MissionMapScreenWidget::SelectMissionById(const FName MissionId)
{
    if (CampaignViewModel && CampaignViewModel->SelectMission(MissionId))
    {
        RefreshMissionDetails();
    }
}

void URA4MissionMapScreenWidget::RefreshMissionDetails()
{
    if (!CampaignViewModel)
    {
        return;
    }
    const FRA4MissionNodeView* Mission = CampaignViewModel->FindSelectedMission();
    if (!Mission)
    {
        return;
    }
    if (MissionTitleText)
    {
        MissionTitleText->SetText(FText::FromString(
            FString::Printf(TEXT("%02d. %s"), Mission->MissionNumber, *Mission->DisplayName.ToString())));
    }
    if (MissionObjectiveText)
    {
        MissionObjectiveText->SetText(Mission->Objective);
    }
}

void URA4MissionMapScreenWidget::StartSelectedMission()
{
    if (!CampaignViewModel || !CampaignViewModel->StartMission())
    {
        return;
    }
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4BriefingScreenWidget* Briefing = CreateWidget<URA4BriefingScreenWidget>(
            PlayerController, URA4BriefingScreenWidget::StaticClass()))
        {
            Briefing->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

void URA4MissionMapScreenWidget::GoBack()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4CampaignScreenWidget* Campaign = CreateWidget<URA4CampaignScreenWidget>(
            PlayerController, URA4CampaignScreenWidget::StaticClass()))
        {
            Campaign->ConfigureCampaign(ERA4FactionTheme::USSR);
            Campaign->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

URA4BriefingScreenWidget::URA4BriefingScreenWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetScreenIdentity(ERA4UIScreenId::Briefing);
}

TSharedRef<SWidget> URA4BriefingScreenWidget::RebuildWidget()
{
    SetScreenIdentity(ERA4UIScreenId::Briefing);
    const TSharedRef<SWidget> RootWidget = Super::RebuildWidget();
    if (!WidgetTree || !GetContentLayer())
    {
        return RootWidget;
    }
    SetRootBackground(
        this,
        TEXT("/Game/RA4UI/Art/T_RA4_USSR_CampaignCommander.T_RA4_USSR_CampaignCommander"),
        FLinearColor(0.64f, 0.56f, 0.56f, 1.0f));
    UCanvasPanel* Canvas = AddCanvas(this, WidgetTree, TEXT("BriefingCanvas"));

    UImage* Logo = MakeImage(
        WidgetTree, TEXT("/Game/RA4UI/Art/T_RA4_Logo.T_RA4_Logo"), TEXT("BriefingLogo"));
    PlaceMissionWidget(Canvas, Logo, FVector2D(20.0f, 8.0f), FVector2D(430.0f, 125.0f), 3);
    UTextBlock* Header = MakeMissionText(
        WidgetTree, LOCTEXT("BriefingHeader", "БРИФИНГ ОПЕРАЦИИ"), 27,
        MissionText, TEXT("BriefingHeader"));
    Header->SetJustification(ETextJustify::Center);
    PlaceMissionWidget(Canvas, Header, FVector2D(650.0f, 20.0f), FVector2D(620.0f, 56.0f), 4);

    UVerticalBox* Intel = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("BriefingIntel"));
    Intel->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("Operation", "ОПЕРАЦИЯ"), 15, MissionRed, TEXT("OperationLabel")));
    Intel->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("RedDawn", "КРАСНЫЙ РАССВЕТ"), 38, MissionRed, TEXT("OperationName")))
        ->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 12.0f));
    Intel->AddChildToVerticalBox(MakeMissionText(
        WidgetTree,
        LOCTEXT("OperationSummary", "Альянс стягивает войска к нашим границам. Нанесите упреждающий удар и сломайте сопротивление до прибытия подкреплений."),
        17, MissionText, TEXT("OperationSummary"), false))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
    Intel->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("ObjectivesHeading", "ЦЕЛИ ОПЕРАЦИИ"), 17,
        MissionRed, TEXT("ObjectivesHeading")));
    Intel->AddChildToVerticalBox(MakeMissionText(
        WidgetTree,
        LOCTEXT("Objectives", "★  Уничтожить командный центр Альянса\n★  Вывести из строя спутниковую связь\n★  Обеспечить контроль над мостом\n★  Эвакуировать войска в зону сбора"),
        16, MissionText, TEXT("Objectives"), false))->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 18.0f));
    Intel->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("IntelHeading", "ДАННЫЕ РАЗВЕДКИ"), 17,
        MissionRed, TEXT("IntelHeading")));
    Intel->AddChildToVerticalBox(MakeMissionText(
        WidgetTree,
        LOCTEXT("Intel", "•  Противник переправляет силы через реку.\n•  Замечены тяжёлая техника и авиация.\n•  Координация подразделений нарушена."),
        15, MissionMuted, TEXT("IntelText"), false));
    PlaceMissionWidget(Canvas, MakeMissionPanel(
        WidgetTree, Intel, TEXT("BriefingIntelPanel"), MissionPanel, ERA4PanelRole::Hero),
        FVector2D(22.0f, 145.0f), FVector2D(620.0f, 745.0f), 6);

    UVerticalBox* Commander = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("BriefingCommander"));
    Commander->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("SovietUnion", "СОВЕТСКИЙ СОЮЗ"), 17,
        MissionRed, TEXT("SovietUnion")));
    UImage* CommanderImage = MakeImage(
        WidgetTree,
        TEXT("/Game/RA4UI/Art/T_RA4_USSR_CampaignCommander.T_RA4_USSR_CampaignCommander"),
        TEXT("BriefingCommanderImage"));
    UVerticalBoxSlot* CommanderImageSlot = Commander->AddChildToVerticalBox(CommanderImage);
    CommanderImageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    CommanderImageSlot->SetPadding(FMargin(0.0f, 12.0f));
    UTextBlock* CommanderName = MakeMissionText(
        WidgetTree, LOCTEXT("MarshalSokolov", "МАРШАЛ\nВИКТОР СОКОЛОВ"), 24,
        MissionText, TEXT("MarshalSokolov"));
    CommanderName->SetJustification(ETextJustify::Center);
    Commander->AddChildToVerticalBox(CommanderName)->SetPadding(FMargin(0.0f, 10.0f));
    PlaceMissionWidget(Canvas, MakeMissionPanel(
        WidgetTree, Commander, TEXT("BriefingCommanderPanel"), MissionPanel, ERA4PanelRole::Standard),
        FVector2D(665.0f, 145.0f), FVector2D(860.0f, 745.0f), 6);

    UVerticalBox* Signals = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("BriefingSignals"));
    Signals->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("EnemyDeployment", "РАССТАНОВКА СИЛ ПРОТИВНИКА"), 16,
        MissionRed, TEXT("EnemyDeployment")));
    Signals->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("DeploymentData", "△  △  △     ◆  ◆\n\nПЕРЕХВАЧЕННЫЕ ПЕРЕГОВОРЫ\n〰〰〰〰〰〰〰\n\nКОДОВОЕ СЛОВО ОПЕРАЦИИ\n\nГРОМ"),
        18, MissionText, TEXT("DeploymentData"), false))->SetPadding(FMargin(0.0f, 16.0f));
    PlaceMissionWidget(Canvas, MakeMissionPanel(
        WidgetTree, Signals, TEXT("BriefingSignalsPanel"), MissionPanel, ERA4PanelRole::Standard),
        FVector2D(1548.0f, 145.0f), FVector2D(345.0f, 745.0f), 6);

    UButton* BackButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("BriefingBackButton"));
    BackButton->SetStyle(MakeMissionButtonStyle());
    UTextBlock* BackLabel = MakeMissionText(
        WidgetTree, LOCTEXT("BriefingBack", "‹   НАЗАД"), 19,
        MissionText, TEXT("BriefingBackLabel"));
    BackLabel->SetJustification(ETextJustify::Center);
    BackButton->AddChild(BackLabel);
    BackButton->OnClicked.AddDynamic(this, &URA4BriefingScreenWidget::GoBack);
    PlaceMissionWidget(Canvas, BackButton, FVector2D(24.0f, 942.0f), FVector2D(250.0f, 82.0f), 8);

    ContinueButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("BriefingContinueButton"));
    ContinueButton->SetStyle(MakeMissionButtonStyle(MissionRed, true));
    UTextBlock* ContinueLabel = MakeMissionText(
        WidgetTree, LOCTEXT("BriefingContinue", "ПРОДОЛЖИТЬ"), 29,
        FLinearColor::White, TEXT("BriefingContinueLabel"));
    ContinueLabel->SetJustification(ETextJustify::Center);
    ContinueButton->AddChild(ContinueLabel);
    ContinueButton->OnClicked.AddDynamic(this, &URA4BriefingScreenWidget::ContinueToComms);
    PlaceMissionWidget(Canvas, ContinueButton, FVector2D(720.0f, 930.0f), FVector2D(480.0f, 96.0f), 9);
    return RootWidget;
}

void URA4BriefingScreenWidget::ContinueToComms()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4VideoCommsScreenWidget* Comms = CreateWidget<URA4VideoCommsScreenWidget>(
            PlayerController, URA4VideoCommsScreenWidget::StaticClass()))
        {
            Comms->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

void URA4BriefingScreenWidget::GoBack()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4MissionMapScreenWidget* MissionMap = CreateWidget<URA4MissionMapScreenWidget>(
            PlayerController, URA4MissionMapScreenWidget::StaticClass()))
        {
            MissionMap->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

URA4VideoCommsScreenWidget::URA4VideoCommsScreenWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetScreenIdentity(ERA4UIScreenId::VideoComms);
}

TSharedRef<SWidget> URA4VideoCommsScreenWidget::RebuildWidget()
{
    SetScreenIdentity(ERA4UIScreenId::VideoComms);
    const TSharedRef<SWidget> RootWidget = Super::RebuildWidget();
    if (!WidgetTree || !GetContentLayer())
    {
        return RootWidget;
    }

    GetBackgroundLayer()->SetColorAndOpacity(FLinearColor(0.005f, 0.007f, 0.012f, 1.0f));
    UCanvasPanel* Canvas = AddCanvas(this, WidgetTree, TEXT("VideoCommsCanvas"));
    UImage* Logo = MakeImage(
        WidgetTree, TEXT("/Game/RA4UI/Art/T_RA4_Logo.T_RA4_Logo"), TEXT("VideoCommsLogo"));
    PlaceMissionWidget(Canvas, Logo, FVector2D(680.0f, 0.0f), FVector2D(560.0f, 150.0f), 3);

    UTextBlock* SecureHeader = MakeMissionText(
        WidgetTree, LOCTEXT("SecureLine", "▣  ЗАЩИЩЁННАЯ ЛИНИЯ СВЯЗИ"), 27,
        MissionText, TEXT("SecureLine"));
    SecureHeader->SetJustification(ETextJustify::Center);
    PlaceMissionWidget(Canvas, SecureHeader, FVector2D(640.0f, 126.0f), FVector2D(640.0f, 52.0f), 4);

    UVerticalBox* SovietChannel = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("SovietChannel"));
    SovietChannel->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("MarshalChannel", "★  МАРШАЛ ВИКТОР СОКОЛОВ\nВЕРХОВНОЕ КОМАНДОВАНИЕ СССР"),
        19, MissionRed, TEXT("MarshalChannel")));
    UImage* SovietImage = MakeImage(
        WidgetTree,
        TEXT("/Game/RA4UI/Art/T_RA4_USSR_CampaignCommander.T_RA4_USSR_CampaignCommander"),
        TEXT("SovietChannelImage"));
    UVerticalBoxSlot* SovietImageSlot = SovietChannel->AddChildToVerticalBox(SovietImage);
    SovietImageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    SovietImageSlot->SetPadding(FMargin(0.0f, 12.0f));
    SovietChannel->AddChildToVerticalBox(MakeMissionText(
        WidgetTree, LOCTEXT("SovietStatus", "КАНАЛ ЗАЩИЩЁН   •   ЗАДЕРЖКА 17 МС   •   ШИФРОВАНИЕ АКТИВНО"),
        14, FLinearColor(0.32f, 0.90f, 0.46f, 1.0f), TEXT("SovietStatus"), false));
    PlaceMissionWidget(Canvas, MakeMissionPanel(
        WidgetTree, SovietChannel, TEXT("SovietChannelPanel"),
        FLinearColor(0.16f, 0.008f, 0.012f, 0.96f), ERA4PanelRole::Standard),
        FVector2D(42.0f, 190.0f), FVector2D(880.0f, 640.0f), 5);

    UVerticalBox* AlliesChannel = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("AlliesChannel"));
    UTextBlock* AlliesHeading = MakeMissionText(
        WidgetTree, LOCTEXT("PresidentChannel", "ПРЕЗИДЕНТ ЭЛЕАНОР УОРД  ◆\nСОЕДИНЁННЫЕ ШТАТЫ АМЕРИКИ"),
        19, MissionBlue, TEXT("PresidentChannel"));
    AlliesHeading->SetJustification(ETextJustify::Right);
    AlliesChannel->AddChildToVerticalBox(AlliesHeading);
    UImage* AlliesImage = MakeImage(
        WidgetTree,
        TEXT("/Game/RA4UI/Art/T_RA4_Allies_CampaignCommander.T_RA4_Allies_CampaignCommander"),
        TEXT("AlliesChannelImage"));
    UVerticalBoxSlot* AlliesImageSlot = AlliesChannel->AddChildToVerticalBox(AlliesImage);
    AlliesImageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    AlliesImageSlot->SetPadding(FMargin(0.0f, 12.0f));
    UTextBlock* AlliesStatus = MakeMissionText(
        WidgetTree, LOCTEXT("AlliesStatus", "КАНАЛ ЗАЩИЩЁН   •   ЗАДЕРЖКА 18 МС   •   ШИФРОВАНИЕ АКТИВНО"),
        14, FLinearColor(0.32f, 0.90f, 0.46f, 1.0f), TEXT("AlliesStatus"), false);
    AlliesStatus->SetJustification(ETextJustify::Right);
    AlliesChannel->AddChildToVerticalBox(AlliesStatus);
    PlaceMissionWidget(Canvas, MakeMissionPanel(
        WidgetTree, AlliesChannel, TEXT("AlliesChannelPanel"),
        FLinearColor(0.008f, 0.055f, 0.14f, 0.96f), ERA4PanelRole::Standard),
        FVector2D(998.0f, 190.0f), FVector2D(880.0f, 640.0f), 5);

    UTextBlock* Subtitle = MakeMissionText(
        WidgetTree,
        LOCTEXT("CommsSubtitle", "СОКОЛОВ: Госпожа президент… у вас есть один шанс избежать войны."),
        22, MissionText, TEXT("CommsSubtitle"));
    Subtitle->SetJustification(ETextJustify::Center);
    PlaceMissionWidget(Canvas, Subtitle, FVector2D(260.0f, 845.0f), FVector2D(1400.0f, 52.0f), 7);

    EndSessionButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("EndSessionButton"));
    EndSessionButton->SetStyle(MakeMissionButtonStyle(MissionRed, true));
    UTextBlock* EndLabel = MakeMissionText(
        WidgetTree, LOCTEXT("EndSession", "☎   ЗАВЕРШИТЬ СЕАНС"), 21,
        FLinearColor::White, TEXT("EndSessionLabel"));
    EndLabel->SetJustification(ETextJustify::Center);
    EndSessionButton->AddChild(EndLabel);
    EndSessionButton->OnClicked.AddDynamic(this, &URA4VideoCommsScreenWidget::EndSession);
    PlaceMissionWidget(Canvas, EndSessionButton, FVector2D(760.0f, 930.0f), FVector2D(400.0f, 86.0f), 8);
    return RootWidget;
}

void URA4VideoCommsScreenWidget::EndSession()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4LoadingScreenWidget* Loading = CreateWidget<URA4LoadingScreenWidget>(
            PlayerController, URA4LoadingScreenWidget::StaticClass()))
        {
            Loading->SetLoadingVariant(ERA4UIScreenVariant::Default);
            Loading->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

URA4LoadingScreenWidget::URA4LoadingScreenWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetScreenIdentity(ERA4UIScreenId::Loading);
}

void URA4LoadingScreenWidget::SetLoadingProgress(const float InProgress)
{
    LoadingProgress = FMath::Clamp(InProgress, 0.0f, 1.0f);
    RefreshProgressVisuals();
}

void URA4LoadingScreenWidget::SetLoadingVariant(const ERA4UIScreenVariant InVariant)
{
    LoadingVariant = InVariant == ERA4UIScreenVariant::LoadingBriefing
        ? InVariant
        : ERA4UIScreenVariant::Default;
    SetScreenIdentity(ERA4UIScreenId::Loading, LoadingVariant);
}

TSharedRef<SWidget> URA4LoadingScreenWidget::RebuildWidget()
{
    SetScreenIdentity(ERA4UIScreenId::Loading, LoadingVariant);
    const TSharedRef<SWidget> RootWidget = Super::RebuildWidget();
    if (!WidgetTree || !GetContentLayer())
    {
        return RootWidget;
    }

    SetRootBackground(
        this,
        TEXT("/Game/RA4UI/Art/T_RA4_USSR_LoadingKyiv.T_RA4_USSR_LoadingKyiv"),
        FLinearColor(0.82f, 0.72f, 0.72f, 1.0f));
    UCanvasPanel* Canvas = AddCanvas(this, WidgetTree, TEXT("LoadingCanvas"));

    UImage* Logo = MakeImage(
        WidgetTree, TEXT("/Game/RA4UI/Art/T_RA4_Logo.T_RA4_Logo"), TEXT("LoadingLogo"));
    PlaceMissionWidget(
        Canvas, Logo,
        LoadingVariant == ERA4UIScreenVariant::LoadingBriefing
            ? FVector2D(35.0f, 12.0f)
            : FVector2D(670.0f, 20.0f),
        LoadingVariant == ERA4UIScreenVariant::LoadingBriefing
            ? FVector2D(520.0f, 150.0f)
            : FVector2D(580.0f, 170.0f), 3);

    UTextBlock* LoadingTitle = MakeMissionText(
        WidgetTree, LOCTEXT("LoadingMission", "ЗАГРУЗКА МИССИИ"), 38,
        MissionText, TEXT("LoadingMission"));
    LoadingTitle->SetJustification(ETextJustify::Center);
    PlaceMissionWidget(
        Canvas, LoadingTitle,
        LoadingVariant == ERA4UIScreenVariant::LoadingBriefing
            ? FVector2D(1390.0f, 34.0f)
            : FVector2D(600.0f, 164.0f),
        LoadingVariant == ERA4UIScreenVariant::LoadingBriefing
            ? FVector2D(450.0f, 56.0f)
            : FVector2D(720.0f, 62.0f), 4);

    UTextBlock* OperationTitle = MakeMissionText(
        WidgetTree, LOCTEXT("LoadingOperation", "ОПЕРАЦИЯ «КИЕВ-86»"), 24,
        MissionRed, TEXT("LoadingOperation"));
    OperationTitle->SetJustification(ETextJustify::Center);
    PlaceMissionWidget(
        Canvas, OperationTitle,
        LoadingVariant == ERA4UIScreenVariant::LoadingBriefing
            ? FVector2D(1390.0f, 94.0f)
            : FVector2D(660.0f, 226.0f),
        LoadingVariant == ERA4UIScreenVariant::LoadingBriefing
            ? FVector2D(450.0f, 42.0f)
            : FVector2D(600.0f, 48.0f), 4);

    if (LoadingVariant == ERA4UIScreenVariant::LoadingBriefing)
    {
        UVerticalBox* Summary = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), TEXT("LoadingBriefingSummary"));
        Summary->AddChildToVerticalBox(MakeMissionText(
            WidgetTree, LOCTEXT("SummaryHeading", "СВОДКА"), 18,
            MissionRed, TEXT("SummaryHeading")));
        Summary->AddChildToVerticalBox(MakeMissionText(
            WidgetTree,
            LOCTEXT("SummaryText", "Американцы укрепили позиции в Киеве. Подготовьте город к наступлению и уничтожьте командный центр противника."),
            19, MissionText, TEXT("SummaryText"), false))->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 34.0f));
        Summary->AddChildToVerticalBox(MakeMissionText(
            WidgetTree, LOCTEXT("GoalsHeading", "ЦЕЛИ"), 18,
            MissionRed, TEXT("GoalsHeading")));
        Summary->AddChildToVerticalBox(MakeMissionText(
            WidgetTree,
            LOCTEXT("LoadingGoals", "☆  Уничтожить командный центр США\n☆  Ликвидировать генерала Хейса\n☆  Захватить центральный район Киева\n☆  Эвакуировать инженеров"),
            17, MissionText, TEXT("LoadingGoals"), false))->SetPadding(FMargin(0.0f, 14.0f));
        PlaceMissionWidget(Canvas, MakeMissionPanel(
            WidgetTree, Summary, TEXT("LoadingBriefingPanel"), MissionPanel, ERA4PanelRole::Hero),
            FVector2D(38.0f, 190.0f), FVector2D(430.0f, 665.0f), 6);
    }

    LoadingProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
        UProgressBar::StaticClass(), TEXT("LoadingProgressBar"));
    LoadingProgressBar->SetFillColorAndOpacity(MissionRed);
    PlaceMissionWidget(Canvas, MakeMissionPanel(
        WidgetTree, LoadingProgressBar, TEXT("LoadingProgressPanel"), MissionPanel, ERA4PanelRole::Compact),
        FVector2D(350.0f, 900.0f), FVector2D(1220.0f, 58.0f), 7);

    LoadingPercentText = MakeMissionText(
        WidgetTree, FText::GetEmpty(), 28, FLinearColor::White, TEXT("LoadingPercent"));
    LoadingPercentText->SetJustification(ETextJustify::Center);
    PlaceMissionWidget(Canvas, LoadingPercentText, FVector2D(1560.0f, 900.0f), FVector2D(150.0f, 58.0f), 8);

    UTextBlock* Tip = MakeMissionText(
        WidgetTree,
        LoadingVariant == ERA4UIScreenVariant::LoadingBriefing
            ? LOCTEXT("LoadingBriefingTip", "Подсказка: Используйте инженеров для захвата вражеских зданий.")
            : LOCTEXT("LoadingTip", "Совет: Используйте маскировку и диверсии, чтобы ослабить базу противника."),
        17, MissionText, TEXT("LoadingTip"), false);
    Tip->SetJustification(ETextJustify::Center);
    PlaceMissionWidget(Canvas, Tip, FVector2D(350.0f, 980.0f), FVector2D(1360.0f, 42.0f), 8);
    RefreshProgressVisuals();
    return RootWidget;
}

void URA4LoadingScreenWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (LoadingProgress < 1.0f)
    {
        SetLoadingProgress(LoadingProgress + InDeltaTime * 0.035f);
    }
}

void URA4LoadingScreenWidget::RefreshProgressVisuals()
{
    if (LoadingProgressBar)
    {
        LoadingProgressBar->SetPercent(LoadingProgress);
    }
    if (LoadingPercentText)
    {
        LoadingPercentText->SetText(FText::Format(
            LOCTEXT("LoadingPercentFormat", "{0}%"),
            FMath::RoundToInt(LoadingProgress * 100.0f)));
    }
}

#undef LOCTEXT_NAMESPACE
