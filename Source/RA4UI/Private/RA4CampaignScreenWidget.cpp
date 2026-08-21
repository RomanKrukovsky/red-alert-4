// Copyright (c) Red Alert 4 project.

#include "RA4CampaignScreenWidget.h"

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
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "RA4AngularPanelWidget.h"
#include "RA4CampaignSelectWidget.h"
#include "RA4MissionFlowWidgets.h"

#define LOCTEXT_NAMESPACE "RA4CampaignScreenWidget"

namespace
{
struct FFactionVisual
{
    ERA4UIScreenId Screen;
    FLinearColor Accent;
    FLinearColor DarkAccent;
    const TCHAR* BackgroundPath;
    FText Heading;
    FText Chapter;
    FText Bulletin;
};

FFactionVisual ResolveFactionVisual(const ERA4FactionTheme Faction)
{
    switch (Faction)
    {
    case ERA4FactionTheme::EurasianPact:
        return {
            ERA4UIScreenId::SovietCampaign,
            FLinearColor(0.68f, 0.28f, 0.88f, 1.0f),
            FLinearColor(0.04f, 0.015f, 0.06f, 0.96f),
            TEXT("/Game/RA4UI/Art/T_RA4_USSR_CampaignCommander.T_RA4_USSR_CampaignCommander"),
            LOCTEXT("EurasianHeading", "КАМПАНИЯ ЕВРАЗИЙСКОГО ПАКТА"),
            LOCTEXT("EurasianChapter", "РОССИЯ: ЛИНИЯ РАЗЛОМА"),
            LOCTEXT("EurasianEvent", "ФРОНТОВАЯ СВОДКА: ГРУППИРОВКА РЭБ «СЕВЕР» РАЗВЁРНУТА НА ПОЗИЦИЯХ.")
        };
    case ERA4FactionTheme::AtlanticAlliance:
        return {
            ERA4UIScreenId::AlliesCampaign,
            FLinearColor(0.35f, 0.70f, 0.98f, 1.0f),
            FLinearColor(0.015f, 0.05f, 0.12f, 0.96f),
            TEXT("/Game/RA4UI/Art/T_RA4_Allies_CampaignCommander.T_RA4_Allies_CampaignCommander"),
            LOCTEXT("AtlanticHeading", "КАМПАНИЯ АТЛАНТИЧЕСКОГО АЛЬЯНСА"),
            LOCTEXT("AtlanticChapter", "США: ДАЛЬНИЙ РУБЕЖ"),
            LOCTEXT("AtlanticEvent", "ОПЕРАТИВНАЯ СВОДКА: АВИАНОСНАЯ ГРУППА ВЫШЛА НА ПЕРЕДОВОЙ РУБЕЖ.")
        };
    case ERA4FactionTheme::EasternCoalition:
        return {
            ERA4UIScreenId::EasternCampaign,
            FLinearColor(0.88f, 0.72f, 0.22f, 1.0f),
            FLinearColor(0.02f, 0.08f, 0.04f, 0.96f),
            TEXT("/Game/RA4UI/Art/T_RA4_Eastern_CampaignCommander.T_RA4_Eastern_CampaignCommander"),
            LOCTEXT("EasternHeading", "КАМПАНИЯ ВОСТОЧНОЙ КОАЛИЦИИ"),
            LOCTEXT("EasternChapter", "КИТАЙ: НЕФРИТОВАЯ СЕТЬ"),
            LOCTEXT("EasternEvent", "СВОДКА КОАЛИЦИИ: АВТОМАТИЗИРОВАННЫЙ ЗАВОД ВЫПУСТИЛ ПЕРВУЮ СЕРИЮ БПЛА.")
        };
    case ERA4FactionTheme::PacificPact:
        return {
            ERA4UIScreenId::ChronoCampaign,
            FLinearColor(0.20f, 0.80f, 0.90f, 1.0f),
            FLinearColor(0.01f, 0.06f, 0.08f, 0.96f),
            TEXT("/Game/RA4UI/Art/T_RA4_Chrono_CampaignCommander.T_RA4_Chrono_CampaignCommander"),
            LOCTEXT("PacificHeading", "КАМПАНИЯ ТИХООКЕАНСКОГО ПАКТА"),
            LOCTEXT("PacificChapter", "ЯПОНИЯ: ДУГА ШТОРМА"),
            LOCTEXT("PacificEvent", "ОБОРОНА ОСТРОВОВ: БЕРЕГОВОЙ ЛАЗЕРНЫЙ КОМПЛЕКС «КАГАМИ» АКТИВИРОВАН.")
        };
    case ERA4FactionTheme::Independent:
        return {
            ERA4UIScreenId::CampaignSelect,
            FLinearColor(0.78f, 0.52f, 0.18f, 1.0f),
            FLinearColor(0.08f, 0.05f, 0.02f, 0.96f),
            TEXT("/Game/RA4UI/Art/T_RA4_USSR_CampaignCommander.T_RA4_USSR_CampaignCommander"),
            LOCTEXT("IndepHeading", "НЕЗАВИСИМЫЕ ДЕРЖАВЫ"),
            LOCTEXT("IndepChapter", "ИРАН: ТЕНЬ НАД ХРЕБТОМ"),
            LOCTEXT("IndepEvent", "АСИММЕТРИЧНЫЙ УДАР: МОБИЛЬНЫЕ РАКЕТНЫЕ ПУСКОВЫЕ «ХЕЙБАР» ВЫШЛИ НА МАРШ.")
        };
    case ERA4FactionTheme::Chronolegion:
        return {
            ERA4UIScreenId::ChronoCampaign,
            FLinearColor(0.70f, 0.30f, 1.0f, 1.0f),
            FLinearColor(0.10f, 0.025f, 0.16f, 0.96f),
            TEXT("/Game/RA4UI/Art/T_RA4_Chrono_CampaignCommander.T_RA4_Chrono_CampaignCommander"),
            LOCTEXT("ChronoHeading", "ХРОНОЛЕГИОН (LEGACY)"),
            LOCTEXT("ChronoChapter", "ГЛАВА 1: ВРЕМЕННАЯ АНОМАЛИЯ"),
            LOCTEXT("ChronoEvent", "ХРОНОПРОТОКОЛ АКТИВЕН: АНОМАЛИЯ ЗАФИКСИРОВАНА.")
        };
    default:
        return ResolveFactionVisual(ERA4FactionTheme::EurasianPact);
    }
}

void PlaceCampaignWidget(
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

UTextBlock* MakeCampaignText(
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
        FontInfo.LetterSpacing = bBold ? 55 : 18;
        Label->SetFont(FontInfo);
    }
    Label->SetShadowOffset(FVector2D(1.0f, 2.0f));
    Label->SetShadowColorAndOpacity(FLinearColor::Black);
    return Label;
}

URA4AngularPanelWidget* MakeCampaignPanel(
    UWidgetTree* Tree,
    UWidget* Content,
    const FName Name,
    const FLinearColor& DarkAccent,
    const ERA4PanelRole Role = ERA4PanelRole::Standard)
{
    URA4AngularPanelWidget* Panel = Tree->ConstructWidget<URA4AngularPanelWidget>(
        URA4AngularPanelWidget::StaticClass(), Name);
    Panel->SetPanelRole(Role);
    FLinearColor Outline = (DarkAccent * 4.0f).GetClamped();
    Outline.A = 1.0f;
    FLinearColor Fill = DarkAccent * 0.18f;
    Fill.A = 0.92f;
    Panel->SetBrush(FSlateRoundedBoxBrush(Fill, 0.0f, Outline, 1.5f));
    Panel->SetContent(Content);
    return Panel;
}

FButtonStyle MakeCampaignButtonStyle(const FLinearColor& Accent, const FLinearColor& DarkAccent)
{
    FButtonStyle Style;
    Style.SetNormal(FSlateColorBrush(DarkAccent));
    Style.SetHovered(FSlateColorBrush(FLinearColor(
        Accent.R * 0.42f, Accent.G * 0.42f, Accent.B * 0.42f, 1.0f)));
    Style.SetPressed(FSlateColorBrush(Accent));
    Style.SetDisabled(FSlateColorBrush(FLinearColor(0.02f, 0.02f, 0.025f, 0.52f)));
    Style.NormalPadding = FMargin(2.0f);
    Style.PressedPadding = FMargin(4.0f, 4.0f, 0.0f, 0.0f);
    return Style;
}
} // namespace

URA4CampaignScreenWidget::URA4CampaignScreenWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetScreenIdentity(ERA4UIScreenId::SovietCampaign);
}

void URA4CampaignScreenWidget::ConfigureCampaign(
    const ERA4FactionTheme InFaction,
    const ERA4UIScreenVariant InVariant)
{
    FactionTheme = InFaction;
    CampaignVariant = InVariant;
    const FFactionVisual Visual = ResolveFactionVisual(FactionTheme);
    SetScreenIdentity(Visual.Screen, CampaignVariant);
}

TSharedRef<SWidget> URA4CampaignScreenWidget::RebuildWidget()
{
    const FFactionVisual Visual = ResolveFactionVisual(FactionTheme);
    SetScreenIdentity(Visual.Screen, CampaignVariant);
    const TSharedRef<SWidget> RootWidget = Super::RebuildWidget();
    if (!WidgetTree || !GetContentLayer())
    {
        return RootWidget;
    }

    CampaignViewModel = NewObject<URA4CampaignViewModel>(this);
    CampaignViewModel->SelectFaction(FactionTheme);
    ActionButtons.Reset();

    if (UTexture2D* BackgroundTexture = LoadObject<UTexture2D>(nullptr, Visual.BackgroundPath))
    {
        GetBackgroundLayer()->SetBrushFromTexture(BackgroundTexture, false);
        GetBackgroundLayer()->SetColorAndOpacity(FLinearColor(0.72f, 0.72f, 0.72f, 1.0f));
    }

    UBorder* Shade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CampaignShade"));
    Shade->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.34f));
    Shade->SetVisibility(ESlateVisibility::HitTestInvisible);
    UOverlaySlot* ShadeSlot = GetContentLayer()->AddChildToOverlay(Shade);
    ShadeSlot->SetHorizontalAlignment(HAlign_Fill);
    ShadeSlot->SetVerticalAlignment(VAlign_Fill);

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("FactionCampaignCanvas"));
    UOverlaySlot* CanvasSlot = GetContentLayer()->AddChildToOverlay(Canvas);
    CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
    CanvasSlot->SetVerticalAlignment(VAlign_Fill);

    UTextBlock* ScreenHeader = MakeCampaignText(
        WidgetTree, Visual.Heading, 36,
        FLinearColor(0.91f, 0.86f, 0.77f, 1.0f), TEXT("CampaignScreenHeader"));
    ScreenHeader->SetJustification(ETextJustify::Center);
    PlaceCampaignWidget(Canvas, MakeCampaignPanel(
        WidgetTree, ScreenHeader, TEXT("CampaignScreenHeaderPanel"), Visual.DarkAccent,
        ERA4PanelRole::Compact),
        FVector2D(405.0f, 18.0f), FVector2D(1080.0f, 82.0f), 3);

    UVerticalBox* Profile = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CampaignProfile"));
    Profile->AddChildToVerticalBox(MakeCampaignText(
        WidgetTree, LOCTEXT("Commander", "КАМПАНИЯ"), 17,
        FLinearColor(0.88f, 0.84f, 0.78f, 1.0f), TEXT("CommanderName")));
    Profile->AddChildToVerticalBox(MakeCampaignText(
        WidgetTree, Visual.Chapter, 14,
        Visual.Accent, TEXT("CommanderLevel"), false));
    PlaceCampaignWidget(Canvas, MakeCampaignPanel(
        WidgetTree, Profile, TEXT("CampaignProfilePanel"), Visual.DarkAccent, ERA4PanelRole::Compact),
        FVector2D(18.0f, 18.0f), FVector2D(370.0f, 92.0f), 4);

    UHorizontalBox* SystemButtons = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CampaignSystemButtons"));
    for (int32 Index = 0; Index < 5; ++Index)
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(), FName(*FString::Printf(TEXT("SystemButton_%d"), Index)));
        Button->SetStyle(MakeCampaignButtonStyle(Visual.Accent, Visual.DarkAccent));
        UTextBlock* Icon = MakeCampaignText(
            WidgetTree, FText::FromString(Index == 4 ? TEXT("⏻") : TEXT("◆")), 20,
            Visual.Accent, FName(*FString::Printf(TEXT("SystemIcon_%d"), Index)));
        Icon->SetJustification(ETextJustify::Center);
        Button->AddChild(Icon);
        UHorizontalBoxSlot* Slot = SystemButtons->AddChildToHorizontalBox(Button);
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Slot->SetPadding(FMargin(3.0f));
    }
    PlaceCampaignWidget(Canvas, MakeCampaignPanel(
        WidgetTree, SystemButtons, TEXT("CampaignSystemPanel"), Visual.DarkAccent, ERA4PanelRole::Compact),
        FVector2D(1515.0f, 18.0f), FVector2D(387.0f, 78.0f), 4);

    const FRA4FactionCardView* Faction = CampaignViewModel->FindFaction(FactionTheme);
    check(Faction != nullptr);

    UVerticalBox* LeftRail = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CampaignFactionRail"));
    LeftRail->AddChildToVerticalBox(MakeCampaignText(
        WidgetTree, Visual.Heading, 28,
        FLinearColor(0.91f, 0.86f, 0.77f, 1.0f), TEXT("CampaignRailTitle")))
        ->SetPadding(FMargin(12.0f, 10.0f, 12.0f, 4.0f));
    LeftRail->AddChildToVerticalBox(MakeCampaignText(
        WidgetTree, Faction->Motto, 16, Visual.Accent, TEXT("CampaignRailMotto")))
        ->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 18.0f));
    LeftRail->AddChildToVerticalBox(MakeCampaignText(
        WidgetTree, Faction->Description, 16,
        FLinearColor(0.76f, 0.74f, 0.70f, 1.0f), TEXT("CampaignRailDescription"), false))
        ->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 22.0f));
    LeftRail->AddChildToVerticalBox(MakeCampaignText(
        WidgetTree, LOCTEXT("FactionFeatures", "ОСОБЕННОСТИ ФРАКЦИИ"), 17,
        Visual.Accent, TEXT("FactionFeatures")))
        ->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 10.0f));
    LeftRail->AddChildToVerticalBox(MakeCampaignText(
        WidgetTree,
        LOCTEXT("FactionFeatureList", "◆  ГАРМОНИЯ РЕСУРСОВ\n\n◆  УНИКАЛЬНЫЕ БОЕВЫЕ ЕДИНИЦЫ\n\n◆  ПРОДВИНУТЫЕ ТЕХНОЛОГИИ\n\n◆  ОСОБАЯ ДОКТРИНА КОМАНДОВАНИЯ"),
        15, FLinearColor(0.82f, 0.80f, 0.75f, 1.0f), TEXT("FactionFeatureList"), false))
        ->SetPadding(FMargin(12.0f));
    PlaceCampaignWidget(Canvas, MakeCampaignPanel(
        WidgetTree, LeftRail, TEXT("CampaignLeftRailPanel"), Visual.DarkAccent, ERA4PanelRole::Compact),
        FVector2D(24.0f, 132.0f), FVector2D(430.0f, 650.0f), 5);

    const bool bEasternDetail = CampaignVariant == ERA4UIScreenVariant::EasternDetail;

    UVerticalBox* Details = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CampaignDetails"));
    UTextBlock* Heading = MakeCampaignText(
        WidgetTree, Visual.Heading, bEasternDetail ? 42 : 50,
        FLinearColor(0.91f, 0.86f, 0.77f, 1.0f), TEXT("FactionHeading"));
    Heading->SetJustification(ETextJustify::Center);
    Details->AddChildToVerticalBox(Heading)->SetPadding(FMargin(8.0f, 8.0f, 8.0f, 2.0f));
    UTextBlock* Motto = MakeCampaignText(
        WidgetTree, Faction->Motto, 17, Visual.Accent, TEXT("FactionMotto"));
    Motto->SetJustification(ETextJustify::Center);
    Details->AddChildToVerticalBox(Motto)->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 16.0f));
    Details->AddChildToVerticalBox(MakeCampaignText(
        WidgetTree, Faction->Description, 17,
        FLinearColor(0.75f, 0.75f, 0.72f, 1.0f), TEXT("FactionDescription"), false))
        ->SetPadding(FMargin(28.0f, 4.0f, 28.0f, 22.0f));

    UTextBlock* ProgressHeading = MakeCampaignText(
        WidgetTree, LOCTEXT("ProgressHeading", "ПРОГРЕСС КАМПАНИИ"), 17,
        Visual.Accent, TEXT("ProgressHeading"));
    Details->AddChildToVerticalBox(ProgressHeading)->SetPadding(FMargin(28.0f, 4.0f, 28.0f, 6.0f));
    UProgressBar* Progress = WidgetTree->ConstructWidget<UProgressBar>(
        UProgressBar::StaticClass(), TEXT("CampaignProgress"));
    Progress->SetPercent(Faction->Progress);
    Progress->SetFillColorAndOpacity(Visual.Accent);
    Details->AddChildToVerticalBox(Progress)->SetPadding(FMargin(28.0f, 0.0f, 28.0f, 8.0f));
    const FText ProgressText = FText::Format(
        LOCTEXT("ProgressFormat", "МИССИЙ ЗАВЕРШЕНО    {0} / {1}                         {2}%"),
        Faction->CompletedMissions,
        Faction->TotalMissions,
        FMath::RoundToInt(Faction->Progress * 100.0f));
    Details->AddChildToVerticalBox(MakeCampaignText(
        WidgetTree, ProgressText, 15,
        FLinearColor(0.78f, 0.76f, 0.72f, 1.0f), TEXT("CampaignProgressText"), false))
        ->SetPadding(FMargin(28.0f, 0.0f, 28.0f, 16.0f));

    Details->AddChildToVerticalBox(MakeCampaignText(
        WidgetTree, Visual.Chapter, 18, Visual.Accent, TEXT("CampaignChapter")))
        ->SetPadding(FMargin(28.0f, 8.0f));
    UHorizontalBox* Chapters = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CampaignChapters"));
    for (int32 Index = 1; Index <= 8; ++Index)
    {
        const bool bActive = Index == 3 || (bEasternDetail && Index == 2);
        UBorder* Chapter = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), FName(*FString::Printf(TEXT("Chapter_%d"), Index)));
        Chapter->SetBrushColor(bActive
            ? Visual.Accent
            : FLinearColor(0.025f, 0.03f, 0.035f, 0.94f));
        UTextBlock* ChapterLabel = MakeCampaignText(
            WidgetTree, FText::AsNumber(Index), 17,
            bActive ? FLinearColor::White : FLinearColor(0.65f, 0.65f, 0.63f, 1.0f),
            FName(*FString::Printf(TEXT("ChapterLabel_%d"), Index)));
        ChapterLabel->SetJustification(ETextJustify::Center);
        Chapter->SetContent(ChapterLabel);
        UHorizontalBoxSlot* ChapterSlot = Chapters->AddChildToHorizontalBox(Chapter);
        ChapterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ChapterSlot->SetPadding(FMargin(5.0f));
    }
    Details->AddChildToVerticalBox(Chapters)->SetPadding(FMargin(22.0f, 6.0f, 22.0f, 18.0f));

    Details->AddChildToVerticalBox(MakeCampaignText(
        WidgetTree,
        bEasternDetail
            ? LOCTEXT("EasternFeatures", "ГАРМОНИЯ РЕСУРСОВ  •  ДРОН-СЕТИ  •  РАСПРЕДЕЛЁННОЕ ПРОИЗВОДСТВО  •  ТЕХНОЛОГИИ БУДУЩЕГО")
            : LOCTEXT("Difficulty", "УРОВЕНЬ СЛОЖНОСТИ     ВЕТЕРАН"),
        16, Visual.Accent, TEXT("CampaignDifficulty"), false))
        ->SetPadding(FMargin(28.0f, 8.0f, 28.0f, 8.0f));

    PlaceCampaignWidget(Canvas, MakeCampaignPanel(
        WidgetTree, Details, TEXT("CampaignDetailsPanel"), Visual.DarkAccent, ERA4PanelRole::Hero),
        FVector2D(1190.0f, 150.0f),
        FVector2D(690.0f, 640.0f), 6);

    UTextBlock* Commander = MakeCampaignText(
        WidgetTree, Faction->CommanderName, 23,
        FLinearColor(0.88f, 0.84f, 0.77f, 1.0f), TEXT("CampaignCommander"));
    Commander->SetJustification(ETextJustify::Center);
    PlaceCampaignWidget(Canvas, MakeCampaignPanel(
        WidgetTree, Commander, TEXT("CampaignCommanderPanel"), Visual.DarkAccent, ERA4PanelRole::Compact),
        FVector2D(495.0f, 720.0f), FVector2D(620.0f, 62.0f), 6);

    UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CampaignActions"));
    const FText ActionLabels[] = {
        LOCTEXT("NewCampaign", "НАЧАТЬ КАМПАНИЮ"),
        LOCTEXT("ContinueCampaign", "ПРОДОЛЖИТЬ"),
        LOCTEXT("ChapterSelect", "ВЫБРАТЬ ГЛАВУ")
    };
    for (int32 Index = 0; Index < 3; ++Index)
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(), FName(*FString::Printf(TEXT("CampaignAction_%d"), Index)));
        Button->SetStyle(MakeCampaignButtonStyle(Visual.Accent, Visual.DarkAccent));
        UTextBlock* Label = MakeCampaignText(
            WidgetTree, ActionLabels[Index], 22,
            FLinearColor(0.91f, 0.87f, 0.80f, 1.0f),
            FName(*FString::Printf(TEXT("CampaignActionLabel_%d"), Index)));
        Label->SetJustification(ETextJustify::Center);
        Button->AddChild(Label);
        UHorizontalBoxSlot* ButtonSlot = Actions->AddChildToHorizontalBox(Button);
        ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ButtonSlot->SetPadding(FMargin(7.0f));
        ActionButtons.Add(Button);
    }
    ActionButtons[0]->OnClicked.AddDynamic(this, &URA4CampaignScreenWidget::StartNewCampaign);
    ActionButtons[1]->OnClicked.AddDynamic(this, &URA4CampaignScreenWidget::ContinueCampaign);
    ActionButtons[2]->OnClicked.AddDynamic(this, &URA4CampaignScreenWidget::OpenChapterMap);
    PlaceCampaignWidget(Canvas, MakeCampaignPanel(
        WidgetTree, Actions, TEXT("CampaignActionsPanel"), Visual.DarkAccent, ERA4PanelRole::Compact),
        FVector2D(24.0f, 806.0f), FVector2D(1090.0f, 104.0f), 8);

    UButton* BackButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("CampaignBackButton"));
    BackButton->SetStyle(MakeCampaignButtonStyle(Visual.Accent, Visual.DarkAccent));
    UTextBlock* BackLabel = MakeCampaignText(
        WidgetTree, LOCTEXT("Back", "‹   НАЗАД"), 19,
        FLinearColor(0.88f, 0.84f, 0.78f, 1.0f), TEXT("CampaignBackLabel"));
    BackLabel->SetJustification(ETextJustify::Center);
    BackButton->AddChild(BackLabel);
    BackButton->OnClicked.AddDynamic(this, &URA4CampaignScreenWidget::GoBack);
    PlaceCampaignWidget(Canvas, BackButton, FVector2D(18.0f, 952.0f), FVector2D(250.0f, 86.0f), 9);

    UTextBlock* Bulletin = MakeCampaignText(
        WidgetTree, Visual.Bulletin, 15,
        FLinearColor(0.62f, 0.76f, 0.68f, 1.0f), TEXT("CampaignBulletin"), false);
    PlaceCampaignWidget(Canvas, MakeCampaignPanel(
        WidgetTree, Bulletin, TEXT("CampaignBulletinPanel"), Visual.DarkAccent, ERA4PanelRole::Compact),
        FVector2D(350.0f, 952.0f), FVector2D(1530.0f, 86.0f), 9);
    return RootWidget;
}

void URA4CampaignScreenWidget::StartNewCampaign()
{
    if (CampaignViewModel)
    {
        const TArray<FRA4MissionNodeView>& Missions = CampaignViewModel->GetMissionNodes();
        if (!Missions.IsEmpty())
        {
            CampaignViewModel->SelectMission(Missions[0].ContentId);
        }
    }
    OpenMissionMap();
}

void URA4CampaignScreenWidget::ContinueCampaign()
{
    OpenMissionMap();
}

void URA4CampaignScreenWidget::OpenChapterMap()
{
    OpenMissionMap();
}

void URA4CampaignScreenWidget::OpenMissionMap()
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

void URA4CampaignScreenWidget::GoBack()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4CampaignSelectWidget* CampaignSelect = CreateWidget<URA4CampaignSelectWidget>(
            PlayerController, URA4CampaignSelectWidget::StaticClass()))
        {
            CampaignSelect->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

#undef LOCTEXT_NAMESPACE
