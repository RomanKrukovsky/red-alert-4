// Copyright (c) Red Alert 4 project.

#include "RA4HUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
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
#include "RA4MinimapWidget.h"
#include "RA4UIDataProviderSubsystem.h"

#define LOCTEXT_NAMESPACE "RA4HUDWidget"

namespace
{
constexpr FLinearColor HUDRed(0.92f, 0.05f, 0.035f, 1.0f);
constexpr FLinearColor HUDText(0.90f, 0.88f, 0.83f, 1.0f);
constexpr FLinearColor HUDMuted(0.54f, 0.53f, 0.49f, 1.0f);
constexpr FLinearColor HUDPanel(0.005f, 0.007f, 0.009f, 0.94f);
constexpr FLinearColor HUDGreen(0.22f, 0.92f, 0.25f, 1.0f);

struct FRA4HUDVisualStyle
{
    FLinearColor Accent;
    FLinearColor Panel;
    FLinearColor Text;
    const TCHAR* BackgroundPath;
    FText Commander;
};

FRA4HUDVisualStyle ResolveHUDVisualStyle(const ERA4FactionTheme Theme)
{
    switch (Theme)
    {
    case ERA4FactionTheme::USSR:
        return {
            HUDRed, HUDPanel, HUDText,
            TEXT("/Game/RA4UI/Art/T_RA4_USSR_CommandCenter.T_RA4_USSR_CommandCenter"),
            LOCTEXT("SovietCommander", "ТОВАРИЩ КОМАНДИР  •  УРОВЕНЬ 45")};
    case ERA4FactionTheme::Allies:
        return {
            FLinearColor(0.08f, 0.54f, 1.0f, 1.0f),
            FLinearColor(0.004f, 0.018f, 0.038f, 0.95f),
            FLinearColor(0.78f, 0.90f, 1.0f, 1.0f),
            TEXT("/Game/RA4UI/Art/T_RA4_Allies_ArcticFleet.T_RA4_Allies_ArcticFleet"),
            LOCTEXT("AlliesCommander", "ПРЕЗИДЕНТ ЭЛЕАНОР УОРД  •  АЛЬЯНС")};
    case ERA4FactionTheme::EasternCoalition:
        return {
            FLinearColor(0.26f, 0.92f, 0.24f, 1.0f),
            FLinearColor(0.006f, 0.030f, 0.012f, 0.95f),
            FLinearColor(0.86f, 0.94f, 0.78f, 1.0f),
            TEXT("/Game/RA4UI/Art/T_RA4_Eastern_CommandFortress.T_RA4_Eastern_CommandFortress"),
            LOCTEXT("EasternCommander", "ВОСТОЧНАЯ КОАЛИЦИЯ  •  БОЕВАЯ МОЩЬ 12 450")};
    case ERA4FactionTheme::Chronolegion:
        return {
            FLinearColor(0.66f, 0.20f, 1.0f, 1.0f),
            FLinearColor(0.025f, 0.004f, 0.045f, 0.95f),
            FLinearColor(0.92f, 0.80f, 1.0f, 1.0f),
            TEXT("/Game/RA4UI/Art/T_RA4_Chrono_TemporalCitadel.T_RA4_Chrono_TemporalCitadel"),
            LOCTEXT("ChronoCommander", "ХРОНОЛЕГИОН  •  ГЛАВНОКОМАНДУЮЩИЙ АЛЕКСЕЙ")};
    default:
        checkNoEntry();
        return ResolveHUDVisualStyle(ERA4FactionTheme::USSR);
    }
}

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
    const FName Name,
    const FLinearColor& PanelColor,
    const FLinearColor& Accent,
    const TCHAR* TexturePath = nullptr)
{
    URA4AngularPanelWidget* Panel = WidgetTree->ConstructWidget<URA4AngularPanelWidget>(
        URA4AngularPanelWidget::StaticClass(), Name);
    Panel->SetPanelRole(ERA4PanelRole::DenseHUD);
    if (TexturePath)
    {
        if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TexturePath))
        {
            FSlateBrush TextureBrush;
            TextureBrush.SetResourceObject(Texture);
            TextureBrush.DrawAs = ESlateBrushDrawType::Image;
            Panel->SetBrush(TextureBrush);
        }
        else
        {
            Panel->SetBrush(FSlateRoundedBoxBrush(PanelColor, 0.0f, Accent, 1.35f));
        }
    }
    else
    {
        Panel->SetBrush(FSlateRoundedBoxBrush(PanelColor, 0.0f, Accent, 1.35f));
    }
    Panel->SetContent(Content);
    return Panel;
}

FButtonStyle MakeHUDButtonStyle(
    const FLinearColor& Accent,
    const bool bSelected = false)
{
    const FLinearColor Normal = bSelected
        ? FLinearColor(Accent.R * 0.42f, Accent.G * 0.42f, Accent.B * 0.42f, 0.98f)
        : FLinearColor(0.035f, 0.025f, 0.022f, 0.98f);
    FButtonStyle Style;
    Style.SetNormal(FSlateColorBrush(Normal));
    Style.SetHovered(FSlateColorBrush(FLinearColor(Accent.R * 0.70f, Accent.G * 0.70f, Accent.B * 0.70f, 1.0f)));
    Style.SetPressed(FSlateColorBrush(Accent));
    Style.SetDisabled(FSlateColorBrush(FLinearColor(0.02f, 0.02f, 0.02f, 0.8f)));
    Style.SetNormalPadding(FMargin(5.0f));
    Style.SetPressedPadding(FMargin(6.0f, 7.0f, 4.0f, 3.0f));
    return Style;
}

UButton* MakeHUDButton(
    UWidgetTree* WidgetTree,
    const FText& Label,
    const FName Name,
    const FLinearColor& Accent,
    const bool bSelected = false)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
    Button->SetStyle(MakeHUDButtonStyle(Accent, bSelected));
    UTextBlock* Text = MakeHUDText(
        WidgetTree, Label, 14, bSelected ? FLinearColor::White : HUDText,
        FName(*FString::Printf(TEXT("%s_Label"), *Name.ToString())), bSelected);
    Text->SetJustification(ETextJustify::Center);
    Button->AddChild(Text);
    return Button;
}

const TCHAR* ResolveProductionIconPath(const ERA4FactionTheme Theme, const int32 Index)
{
    static const TCHAR* SovietIcons[] = {
        TEXT("/Game/RA4UI/Art/T_RA4_SU_Conscript.T_RA4_SU_Conscript"),
        TEXT("/Game/RA4UI/Art/T_RA4_SU_FlakTrooper.T_RA4_SU_FlakTrooper"),
        TEXT("/Game/RA4UI/Art/T_RA4_SU_HammerTank.T_RA4_SU_HammerTank"),
        TEXT("/Game/RA4UI/Art/T_RA4_SU_SickleScout.T_RA4_SU_SickleScout"),
        TEXT("/Game/RA4UI/Art/T_RA4_SU_MiG41.T_RA4_SU_MiG41"),
        TEXT("/Game/RA4UI/Art/T_RA4_SU_TyphoonSub.T_RA4_SU_TyphoonSub")
    };
    static const TCHAR* AlliedIcons[] = {
        TEXT("/Game/RA4UI/Art/T_RA4_AL_Peacekeeper.T_RA4_AL_Peacekeeper"),
        TEXT("/Game/RA4UI/Art/T_RA4_AL_Javelin.T_RA4_AL_Javelin"),
        TEXT("/Game/RA4UI/Art/T_RA4_AL_Guardian.T_RA4_AL_Guardian"),
        TEXT("/Game/RA4UI/Art/T_RA4_AL_Mirage.T_RA4_AL_Mirage"),
        TEXT("/Game/RA4UI/Art/T_RA4_AL_Harrier.T_RA4_AL_Harrier"),
        TEXT("/Game/RA4UI/Art/T_RA4_AL_Poseidon.T_RA4_AL_Poseidon")
    };

    switch (Theme)
    {
    case ERA4FactionTheme::USSR:
        return SovietIcons[Index % UE_ARRAY_COUNT(SovietIcons)];
    case ERA4FactionTheme::Allies:
        return AlliedIcons[Index % UE_ARRAY_COUNT(AlliedIcons)];
    case ERA4FactionTheme::EasternCoalition:
    case ERA4FactionTheme::Chronolegion:
        return nullptr;
    default:
        checkNoEntry();
        return nullptr;
    }
}

UButton* MakeHUDProductionCard(
    UWidgetTree* WidgetTree,
    const FText& Label,
    const FName Name,
    const FLinearColor& Accent,
    const ERA4FactionTheme Theme,
    const int32 Index)
{
    const TCHAR* IconPath = ResolveProductionIconPath(Theme, Index);
    if (!IconPath)
    {
        return MakeHUDButton(WidgetTree, Label, Name, Accent);
    }

    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
    Button->SetStyle(MakeHUDButtonStyle(Accent));
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), FName(Name.ToString() + TEXT("_Content")));
    USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), FName(Name.ToString() + TEXT("_IconBox")));
    IconBox->SetHeightOverride(82.0f);
    UImage* Icon = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), FName(Name.ToString() + TEXT("_Icon")));
    if (UTexture2D* IconTexture = LoadObject<UTexture2D>(nullptr, IconPath))
    {
        Icon->SetBrushFromTexture(IconTexture, false);
    }
    Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
    IconBox->SetContent(Icon);
    Content->AddChildToVerticalBox(IconBox);
    UTextBlock* Text = MakeHUDText(
        WidgetTree, Label, 11, HUDText,
        FName(Name.ToString() + TEXT("_Label")), true);
    Text->SetJustification(ETextJustify::Center);
    Content->AddChildToVerticalBox(Text)->SetPadding(FMargin(2.0f, 1.0f, 2.0f, 3.0f));
    Button->AddChild(Content);
    return Button;
}

FRA4HUDSnapshotView MakeShowcaseSnapshot(
    const ERA4FactionTheme Theme,
    const ERA4UIScreenVariant Variant,
    const int32 ActiveCategory)
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
    switch (Theme)
    {
    case ERA4FactionTheme::USSR:
        Snapshot.PrimaryEntityName = Variant == ERA4UIScreenVariant::SovietBattle
            ? TEXT("ТЯЖЁЛЫЙ ТАНК КВ-3")
            : Variant == ERA4UIScreenVariant::SovietAlert
                ? TEXT("КОМАНДНЫЙ ЦЕНТР")
                : TEXT("ГЛАВНЫЙ ШТАБ");
        break;
    case ERA4FactionTheme::Allies:
        Snapshot.PrimaryEntityName = Variant == ERA4UIScreenVariant::AlliesNaval
            ? TEXT("ЭСМИНЕЦ «СВОБОДА»")
            : Variant == ERA4UIScreenVariant::AlliesAir
                ? TEXT("ИСТРЕБИТЕЛЬ «ОРЁЛ»")
                : TEXT("АЭРОДРОМ АЛЬЯНСА");
        break;
    case ERA4FactionTheme::EasternCoalition:
        Snapshot.PrimaryEntityName = TEXT("ЦЕНТРАЛЬНЫЙ КОМПЛЕКС");
        break;
    case ERA4FactionTheme::Chronolegion:
        Snapshot.PrimaryEntityName = Variant == ERA4UIScreenVariant::ChronoSuperweapon
            ? TEXT("ХРОНОКОЛЛАПС «ВЕЧНОСТЬ»")
            : TEXT("ГЛАВНЫЙ ХРОНОРЕАКТОР");
        break;
    default:
        checkNoEntry();
        break;
    }
    Snapshot.SelectionHealthRatio = 1.0f;
    Snapshot.bPrimaryOwned = true;

    FRA4HUDObjective Primary;
    Primary.Label = Variant == ERA4UIScreenVariant::AlliesNaval
        ? LOCTEXT("ObjectiveNaval", "Уничтожить вражеский флот")
        : Variant == ERA4UIScreenVariant::AlliesAir
            ? LOCTEXT("ObjectiveAir", "Захватить передовые аэродромы")
            : Variant == ERA4UIScreenVariant::ChronoSuperweapon
                ? LOCTEXT("ObjectiveChrono", "Уничтожить командный центр противника")
                : LOCTEXT("ShowcaseObjectivePrimary", "Уничтожить базу противника");
    Snapshot.Objectives.Add(Primary);
    FRA4HUDObjective Secondary;
    Secondary.Label = LOCTEXT("ShowcaseObjectiveSecondary", "Захватить хранилище ресурсов");
    Secondary.Current = 1;
    Secondary.Target = 3;
    Snapshot.Objectives.Add(Secondary);

    TArray<FText> BuildNames;
    if (Variant == ERA4UIScreenVariant::AlliesNaval)
    {
        BuildNames = {
            LOCTEXT("NavalFrigate", "ФРЕГАТ"), LOCTEXT("NavalDestroyer", "ЭСМИНЕЦ"),
            LOCTEXT("NavalCruiser", "КРЕЙСЕР"), LOCTEXT("NavalMissile", "РАКЕТНЫЙ КАТЕР"),
            LOCTEXT("NavalSub", "ПОДЛОДКА"), LOCTEXT("NavalCarrier", "АВИАНОСЕЦ"),
            LOCTEXT("NavalPlatform", "МОРСКАЯ ПЛАТФОРМА"), LOCTEXT("NavalRepair", "РЕМОНТНЫЙ КОРАБЛЬ")};
    }
    else if (Variant == ERA4UIScreenVariant::AlliesAir || Theme == ERA4FactionTheme::Allies)
    {
        BuildNames = {
            LOCTEXT("AirEagle", "ОРЁЛ"), LOCTEXT("AirPredator", "ХИЩНИК"),
            LOCTEXT("AirAvenger", "МСТИТЕЛЬ"), LOCTEXT("AirHarpy", "ГАРПИЯ"),
            LOCTEXT("AirStorm", "ШТОРМ"), LOCTEXT("AirLightning", "МОЛНИЯ"),
            LOCTEXT("AirAurora", "АВРОРА"), LOCTEXT("AirAngel", "АНГЕЛ")};
    }
    else if (Theme == ERA4FactionTheme::EasternCoalition)
    {
        BuildNames = {
            LOCTEXT("EastDragon", "ДРАКОН"), LOCTEXT("EastQilin", "ЦИЛИНЬ"),
            LOCTEXT("EastLotus", "ЛОТОС"), LOCTEXT("EastTiger", "БЕЛЫЙ ТИГР"),
            LOCTEXT("EastJade", "НЕФРИТОВЫЙ СТРАЖ"), LOCTEXT("EastRocket", "ОГНЕННАЯ СТРЕЛА"),
            LOCTEXT("EastCrane", "ЖУРАВЛЬ"), LOCTEXT("EastCitadel", "ЦИТАДЕЛЬ")};
    }
    else if (Theme == ERA4FactionTheme::Chronolegion)
    {
        BuildNames = {
            LOCTEXT("ChronoNode", "ХРОНОУЗЕЛ"), LOCTEXT("ChronoReactor", "ХРОНОРЕАКТОР"),
            LOCTEXT("ChronoGate", "ВРАТА ВРЕМЕНИ"), LOCTEXT("ChronoTank", "ПАРАДОКС"),
            LOCTEXT("ChronoSpire", "СИНХРОНИЗАТОР"), LOCTEXT("ChronoDome", "КУПОЛ ВРЕМЕНИ"),
            LOCTEXT("ChronoFrigate", "ТЕМПОРАЛЬНЫЙ ФРЕГАТ"), LOCTEXT("ChronoCollapse", "ХРОНОКОЛЛАПС")};
    }
    else
    {
        BuildNames = {
            LOCTEXT("BuildPower", "ЭЛЕКТРОСТАНЦИЯ"), LOCTEXT("BuildBarracks", "КАЗАРМЫ"),
            LOCTEXT("BuildFactory", "ВОЕННЫЙ ЗАВОД"), LOCTEXT("BuildRefinery", "НЕФТЕБАЗА"),
            LOCTEXT("BuildTurret", "ЗЕНИТНАЯ ПУШКА"), LOCTEXT("BuildRadar", "РАКЕТНАЯ ШАХТА"),
            LOCTEXT("BuildLab", "ОБСЕРВАТОРИЯ"), LOCTEXT("BuildWall", "СТЕНА")};
    }
    for (int32 Index = 0; Index < BuildNames.Num(); ++Index)
    {
        FRA4BuildOption Option;
        Option.ContentId = Index + 1;
        Option.DisplayName = BuildNames[Index];
        Option.Cost = 300 + Index * 250;
        Option.Category = ActiveCategory;
        Option.bAvailable = Index != 5;
        Option.BlockReason = Option.bAvailable
            ? ERA4BuildBlockReason::None
            : ERA4BuildBlockReason::MissingPrerequisite;
        Snapshot.BuildOptions.Add(Option);
    }

    FRA4ProductionEntry Tank;
    Tank.ContentId = 101;
    Tank.DisplayName = BuildNames[0];
    Tank.ProgressPercent = 68;
    Tank.RemainingSeconds = 12.0f;
    Snapshot.ProductionQueue.Add(Tank);
    FRA4ProductionEntry Infantry;
    Infantry.ContentId = 102;
    Infantry.DisplayName = BuildNames[1];
    Infantry.ProgressPercent = 39;
    Infantry.RemainingSeconds = 8.0f;
    Snapshot.ProductionQueue.Add(Infantry);

    FRA4Alert Alert;
    Alert.Message = Variant == ERA4UIScreenVariant::ChronoSuperweapon
        ? LOCTEXT("ChronoWeaponAlert", "СУПЕРОРУЖИЕ АКТИВИРОВАНО — ЦЕЛЬ ПОРАЖЕНА")
        : Variant == ERA4UIScreenVariant::SovietAlert
            ? LOCTEXT("BaseAlert", "ТРЕВОГА! БАЗА ПОДВЕРГАЕТСЯ АТАКЕ!")
            : LOCTEXT("ShowcaseAlert", "НАША БАЗА АТАКОВАНА!");
    Alert.Severity = ERA4AlertSeverity::Critical;
    Snapshot.Alerts.Add(Alert);
    return Snapshot;
}

TArray<FRA4RadarMarker> MakeShowcaseRadarMarkers()
{
    TArray<FRA4RadarMarker> Markers;
    int32 MarkerCount = 74;
    FParse::Value(FCommandLine::Get(), TEXT("RA4MarkerCount="), MarkerCount);
    MarkerCount = FMath::Clamp(MarkerCount, 0, 5000);
    for (int32 Index = 0; Index < MarkerCount; ++Index)
    {
        FRA4RadarMarker Marker;
        Marker.WorldPosition = FVector2D(
            10.0f + float((Index * 23) % 82),
            8.0f + float((Index * 37) % 84));
        Marker.Owner = Index < 46 ? 0 : 1;
        Marker.Kind = Index % 9 == 0
            ? ERA4RadarMarkerKind::Building
            : ERA4RadarMarkerKind::Unit;
        Marker.bSelected = Index >= 12 && Index <= 18;
        Markers.Add(Marker);
    }
    return Markers;
}
} // namespace

URA4HUDWidget::URA4HUDWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetScreenIdentity(ERA4UIScreenId::SovietHud);
}

void URA4HUDWidget::ConfigureHUD(
    const ERA4FactionTheme InFactionTheme,
    const ERA4UIScreenVariant InVariant,
    const int32 InActiveProductionTab)
{
    FactionTheme = InFactionTheme;
    HUDVariant = InVariant;
    ActiveProductionTab = FMath::Clamp(InActiveProductionTab, 0, 4);

    ERA4UIScreenId HUDScreen = ERA4UIScreenId::SovietHud;
    switch (FactionTheme)
    {
    case ERA4FactionTheme::USSR:
        HUDScreen = ERA4UIScreenId::SovietHud;
        break;
    case ERA4FactionTheme::Allies:
        HUDScreen = ERA4UIScreenId::AlliesHud;
        break;
    case ERA4FactionTheme::EasternCoalition:
        HUDScreen = ERA4UIScreenId::EasternHud;
        break;
    case ERA4FactionTheme::Chronolegion:
        HUDScreen = ERA4UIScreenId::ChronoHud;
        break;
    default:
        checkNoEntry();
        break;
    }
    SetScreenIdentity(HUDScreen, HUDVariant);
}

TSharedRef<SWidget> URA4HUDWidget::RebuildWidget()
{
    const TSharedRef<SWidget> RootWidget = Super::RebuildWidget();
    if (!WidgetTree || !GetContentLayer())
    {
        return RootWidget;
    }

    const FRA4HUDVisualStyle ThemeStyle = ResolveHUDVisualStyle(FactionTheme);
    int32 ShowcaseScreen = 0;
    const bool bShowcaseMode = FParse::Value(
        FCommandLine::Get(), TEXT("RA4Screen="), ShowcaseScreen) && ShowcaseScreen >= 13;
    if (bShowcaseMode)
    {
        if (UTexture2D* Background = LoadObject<UTexture2D>(
            nullptr, ThemeStyle.BackgroundPath))
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
        WidgetTree, ThemeStyle.Commander,
        16, ThemeStyle.Text, TEXT("CommanderTitle"), true));
    Objectives->AddChildToVerticalBox(MakeHUDText(
        WidgetTree, LOCTEXT("ObjectivesHeading", "ОСНОВНЫЕ ЗАДАЧИ"),
        15, ThemeStyle.Accent, TEXT("ObjectivesHeading"), true))->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 5.0f));
    ObjectivesList = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ObjectivesList"));
    Objectives->AddChildToVerticalBox(ObjectivesList);
    const FVector2D ObjectivesPosition(16.0f, 18.0f);
    const FVector2D ObjectivesSize(370.0f, 205.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Objectives, TEXT("ObjectivesPanel"), ThemeStyle.Panel, ThemeStyle.Accent,
        TEXT("/Game/RA4UI/Art/T_RA4_UI_ObjectivesFrame.T_RA4_UI_ObjectivesFrame")), ObjectivesPosition, ObjectivesSize, 10);
    AddInteractiveRegion(ObjectivesPosition, ObjectivesSize);

    ResourceText = MakeHUDText(
        WidgetTree, FText::GetEmpty(), 17, ThemeStyle.Text, TEXT("ResourceText"), true);
    ResourceText->SetJustification(ETextJustify::Center);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, ResourceText, TEXT("ResourceBarPanel"), ThemeStyle.Panel, ThemeStyle.Accent,
        TEXT("/Game/RA4UI/Art/T_RA4_Frame_ResourceBarV2.T_RA4_Frame_ResourceBarV2")),
        FVector2D(1110.0f, 12.0f), FVector2D(790.0f, 55.0f), 10);

    URA4MinimapWidget* Minimap = WidgetTree->ConstructWidget<URA4MinimapWidget>(
        URA4MinimapWidget::StaticClass(), TEXT("TacticalMinimap"));
    Minimap->SetSnapshot(MakeShowcaseRadarMarkers(), FVector2D(100.0f, 100.0f), 0);
    Minimap->SetViewportWorldBounds(FVector2D(18.0f, 54.0f), FVector2D(46.0f, 82.0f));
    const FVector2D MinimapPosition(1590.0f, 80.0f);
    const FVector2D MinimapSize(310.0f, 285.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Minimap, TEXT("MinimapPanel"), ThemeStyle.Panel, ThemeStyle.Accent,
        TEXT("/Game/RA4UI/Art/T_RA4_Frame_MinimapV2.T_RA4_Frame_MinimapV2")), MinimapPosition, MinimapSize, 10);
    AddInteractiveRegion(MinimapPosition, MinimapSize);

    UVerticalBox* Sidebar = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ProductionSidebar"));
    FText TabLabel = LOCTEXT("ProductionTabs", "СТРОИТЬ   |   ВОЙСКА   |   УЛУЧШЕНИЯ   |   ДОКТРИНЫ");
    if (FactionTheme == ERA4FactionTheme::Allies)
    {
        TabLabel = HUDVariant == ERA4UIScreenVariant::AlliesNaval
            ? LOCTEXT("AlliesNavalTabs", "СТРОЕНИЯ   |   ПЕХОТА   |   ТЕХНИКА   |   АВИАЦИЯ   |   ФЛОТ")
            : LOCTEXT("AlliesTabs", "СТРОЕНИЯ   |   ПЕХОТА   |   ТЕХНИКА   |   АВИАЦИЯ");
    }
    else if (FactionTheme == ERA4FactionTheme::Chronolegion)
    {
        TabLabel = LOCTEXT("ChronoTabs", "СТРОЕНИЯ   |   БОЕВЫЕ ЕДИНИЦЫ   |   ПОДДЕРЖКА   |   ОСОБОЕ");
    }
    else if (FactionTheme == ERA4FactionTheme::EasternCoalition)
    {
        TabLabel = LOCTEXT("EasternTabs", "СТРОЕНИЯ   |   БОЕВЫЕ ЕД.   |   УЛУЧШЕНИЯ   |   ДОКТРИНЫ");
    }
    UButton* TabButton = MakeHUDButton(
        WidgetTree, TabLabel, TEXT("ProductionTabButton"), ThemeStyle.Accent, true);
    TabButton->OnClicked.AddDynamic(this, &URA4HUDWidget::CycleProductionTab);
    Sidebar->AddChildToVerticalBox(TabButton)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    BuildGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(
        UUniformGridPanel::StaticClass(), TEXT("BuildGrid"));
    UVerticalBoxSlot* BuildGridSlot = Sidebar->AddChildToVerticalBox(BuildGrid);
    BuildGridSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    const FVector2D SidebarPosition(1450.0f, 380.0f);
    const FVector2D SidebarSize(450.0f, 500.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Sidebar, TEXT("ProductionPanel"), ThemeStyle.Panel, ThemeStyle.Accent,
        TEXT("/Game/RA4UI/Art/T_RA4_UI_PanelTall.T_RA4_UI_PanelTall")), SidebarPosition, SidebarSize, 10);
    AddInteractiveRegion(SidebarPosition, SidebarSize);

    UVerticalBox* Selection = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("SelectionContent"));
    SelectionTitleText = MakeHUDText(
        WidgetTree, FText::GetEmpty(), 18, ThemeStyle.Text, TEXT("SelectionTitle"), true);
    SelectionDetailText = MakeHUDText(
        WidgetTree, FText::GetEmpty(), 14, HUDMuted, TEXT("SelectionDetails"));
    Selection->AddChildToVerticalBox(SelectionTitleText);
    Selection->AddChildToVerticalBox(SelectionDetailText)->SetPadding(FMargin(0.0f, 8.0f));
    const FVector2D SelectionPosition(16.0f, 820.0f);
    const FVector2D SelectionSize(420.0f, 240.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Selection, TEXT("SelectionPanel"), ThemeStyle.Panel, ThemeStyle.Accent,
        TEXT("/Game/RA4UI/Art/T_RA4_Frame_UnitCardV2.T_RA4_Frame_UnitCardV2")), SelectionPosition, SelectionSize, 10);
    AddInteractiveRegion(SelectionPosition, SelectionSize);

    UVerticalBox* Queue = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ProductionQueueContent"));
    const FText QueueHeading = HUDVariant == ERA4UIScreenVariant::ChronoSuperweapon
        ? LOCTEXT("SuperweaponHeading", "СУПЕРОРУЖИЕ: ХРОНОКОЛЛАПС")
        : HUDVariant == ERA4UIScreenVariant::AlliesNaval
            ? LOCTEXT("FleetQueueHeading", "ОЧЕРЕДЬ ВЕРФИ")
            : LOCTEXT("QueueHeading", "ОЧЕРЕДЬ ПОСТРОЙКИ");
    Queue->AddChildToVerticalBox(MakeHUDText(
        WidgetTree, QueueHeading,
        14, ThemeStyle.Text, TEXT("QueueHeading"), true));
    ProductionQueueList = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ProductionQueueList"));
    Queue->AddChildToVerticalBox(ProductionQueueList)->SetPadding(FMargin(0.0f, 7.0f));
    const FVector2D QueuePosition(455.0f, 875.0f);
    const FVector2D QueueSize(560.0f, 185.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Queue, TEXT("ProductionQueuePanel"), ThemeStyle.Panel, ThemeStyle.Accent,
        TEXT("/Game/RA4UI/Art/T_RA4_UI_QueueFrame.T_RA4_UI_QueueFrame")), QueuePosition, QueueSize, 10);
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
            FName(*FString::Printf(TEXT("Command_%d"), Index)), ThemeStyle.Accent, Index == 1);
        Command->OnClicked.AddDynamic(this, &URA4HUDWidget::IssuePrimaryCommand);
        UHorizontalBoxSlot* CommandSlot = CommandButtons->AddChildToHorizontalBox(Command);
        CommandSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        CommandSlot->SetPadding(FMargin(2.0f));
    }
    Commands->AddChildToVerticalBox(CommandButtons);
    const FText InitialCommandStatus = HUDVariant == ERA4UIScreenVariant::ChronoSuperweapon
        ? LOCTEXT("ChronoTargetLocked", "ЦЕЛЬ ЗАФИКСИРОВАНА  •  СИНХРОНИЗАЦИЯ 100%")
        : HUDVariant == ERA4UIScreenVariant::SovietAlert
            ? LOCTEXT("SovietDefenceActive", "ПРОТОКОЛ ОБОРОНЫ БАЗЫ АКТИВЕН")
            : LOCTEXT("CommandReady", "КОМАНДНАЯ СЕТЬ ГОТОВА");
    CommandStatusText = MakeHUDText(
        WidgetTree, InitialCommandStatus,
        13, HUDGreen, TEXT("CommandStatus"));
    Commands->AddChildToVerticalBox(CommandStatusText)->SetPadding(FMargin(4.0f, 10.0f));
    const FVector2D CommandPosition(1450.0f, 900.0f);
    const FVector2D CommandSize(450.0f, 160.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Commands, TEXT("CommandGridPanel"), ThemeStyle.Panel, ThemeStyle.Accent,
        TEXT("/Game/RA4UI/Art/T_RA4_UI_CommandBar.T_RA4_UI_CommandBar")), CommandPosition, CommandSize, 10);
    AddInteractiveRegion(CommandPosition, CommandSize);

    const bool bProminentAlert = HUDVariant == ERA4UIScreenVariant::SovietAlert ||
        HUDVariant == ERA4UIScreenVariant::ChronoSuperweapon;
    const FVector2D AlertPosition = bProminentAlert
        ? FVector2D(555.0f, 92.0f)
        : FVector2D(16.0f, 250.0f);
    const FVector2D AlertSize = bProminentAlert
        ? FVector2D(850.0f, 74.0f)
        : FVector2D(330.0f, 54.0f);
    AlertText = MakeHUDText(
        WidgetTree, FText::GetEmpty(), bProminentAlert ? 22 : 16,
        ThemeStyle.Accent, TEXT("AlertText"), true);
    AlertText->SetJustification(ETextJustify::Center);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, AlertText, TEXT("AlertPanel"), ThemeStyle.Panel, ThemeStyle.Accent),
        AlertPosition, AlertSize, 20);

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
        HUDViewModel->ApplySnapshot(MakeShowcaseSnapshot(
            FactionTheme, HUDVariant, ActiveProductionTab));
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
        UButton* Card = MakeHUDProductionCard(
            WidgetTree, CardLabel,
            FName(*FString::Printf(TEXT("BuildOption_%d"), VisibleIndex)),
            ResolveHUDVisualStyle(FactionTheme).Accent,
            FactionTheme,
            VisibleIndex);
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
    const int32 TabCount = HUDVariant == ERA4UIScreenVariant::AlliesNaval ? 5 : 4;
    ActiveProductionTab = (ActiveProductionTab + 1) % TabCount;
    int32 ShowcaseScreen = 0;
    if (FParse::Value(FCommandLine::Get(), TEXT("RA4Screen="), ShowcaseScreen) && ShowcaseScreen >= 13)
    {
        HUDViewModel->ApplySnapshot(MakeShowcaseSnapshot(
            FactionTheme, HUDVariant, ActiveProductionTab));
    }
    else
    {
        RefreshProduction();
    }
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
