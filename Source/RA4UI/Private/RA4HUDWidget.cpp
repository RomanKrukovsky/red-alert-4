// Copyright (c) Red Alert 4 project.

#include "RA4HUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/ProgressBar.h"
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
    case ERA4FactionTheme::EurasianPact:
        return {
            FLinearColor(0.68f, 0.28f, 0.88f, 1.0f),
            FLinearColor(0.016f, 0.008f, 0.024f, 0.95f),
            FLinearColor(0.92f, 0.86f, 0.96f, 1.0f),
            TEXT("/Game/RA4UI/Art/T_RA4_Theatre_Eurasian.T_RA4_Theatre_Eurasian"),
            LOCTEXT("EurasianCommander", "ЕВРАЗИЙСКИЙ ПАКТ  •  РОССИЯ  •  РЭБ И БРОНЕГРУППА")};
    case ERA4FactionTheme::AtlanticAlliance:
        return {
            FLinearColor(0.35f, 0.70f, 0.98f, 1.0f),
            FLinearColor(0.006f, 0.018f, 0.038f, 0.95f),
            FLinearColor(0.85f, 0.93f, 1.0f, 1.0f),
            TEXT("/Game/RA4UI/Art/T_RA4_Theatre_Atlantic.T_RA4_Theatre_Atlantic"),
            LOCTEXT("AtlanticCommander", "АТЛАНТИЧЕСКИЙ АЛЬЯНС  •  США  •  СЕТЕЦЕНТРИЧЕСКИЙ ШТАБ")};
    case ERA4FactionTheme::EasternCoalition:
        return {
            FLinearColor(0.88f, 0.72f, 0.22f, 1.0f),
            FLinearColor(0.008f, 0.028f, 0.016f, 0.95f),
            FLinearColor(0.95f, 0.92f, 0.80f, 1.0f),
            TEXT("/Game/RA4UI/Art/T_RA4_Theatre_Eastern.T_RA4_Theatre_Eastern"),
            LOCTEXT("EasternCommander", "ВОСТОЧНАЯ КОАЛИЦИЯ  •  КИТАЙ  •  ИНДУСТРИАЛЬНЫЙ КОМПЛЕКС")};
    case ERA4FactionTheme::PacificPact:
        return {
            FLinearColor(0.20f, 0.80f, 0.90f, 1.0f),
            FLinearColor(0.005f, 0.022f, 0.032f, 0.95f),
            FLinearColor(0.80f, 0.96f, 1.0f, 1.0f),
            TEXT("/Game/RA4UI/Art/T_RA4_Theatre_Pacific.T_RA4_Theatre_Pacific"),
            LOCTEXT("PacificCommander", "ТИХООКЕАНСКИЙ ПАКТ  •  ЯПОНИЯ  •  РОБОТИЗИРОВАННЫЙ КОРПУС")};
    case ERA4FactionTheme::Independent:
        return {
            FLinearColor(0.78f, 0.52f, 0.18f, 1.0f),
            FLinearColor(0.024f, 0.016f, 0.008f, 0.95f),
            FLinearColor(0.95f, 0.90f, 0.80f, 1.0f),
            TEXT("/Game/RA4UI/Art/T_RA4_Theatre_Independent.T_RA4_Theatre_Independent"),
            LOCTEXT("IndepCommander", "НЕЗАВИСИМЫЕ ДЕРЖАВЫ  •  ИРАН  •  АСИММЕТРИЧНЫЙ РУБЕЖ")};
    case ERA4FactionTheme::Chronolegion:
        return {
            FLinearColor(0.66f, 0.20f, 1.0f, 1.0f),
            FLinearColor(0.025f, 0.004f, 0.045f, 0.95f),
            FLinearColor(0.92f, 0.80f, 1.0f, 1.0f),
            TEXT("/Game/RA4UI/Art/T_RA4_Chrono_TemporalCitadel.T_RA4_Chrono_TemporalCitadel"),
            LOCTEXT("ChronoCommander", "ХРОНОЛЕГИОН (LEGACY)  •  ВРЕМЕННОЙ УЗЕЛ")};
    default:
        return ResolveHUDVisualStyle(ERA4FactionTheme::EurasianPact);
    }
}

/**
 * Remaster reference plates 12–18 are keyed by faction AND combat variant.
 * The one-faction-per-HUD shells (Eurasian/Atlantic/Eastern/Pacific/Independent)
 * each have a signature reference; Eurasian and Pacific also have a secondary
 * base-defence plate (17 and 18). Default variants fall back to the signature.
 */
const TCHAR* ResolveRemasterHudPlate(
    const ERA4FactionTheme Theme,
    const ERA4UIScreenVariant Variant)
{
    switch (Theme)
    {
    case ERA4FactionTheme::EurasianPact:
        if (Variant == ERA4UIScreenVariant::BaseDefense)
        {
            return TEXT("/Game/RA4UI/Art/Remaster/T_SH_17_HudEurasianBase.T_SH_17_HudEurasianBase");
        }
        return TEXT("/Game/RA4UI/Art/Remaster/T_SH_12_HudEurasianGround.T_SH_12_HudEurasianGround");
    case ERA4FactionTheme::AtlanticAlliance:
        return TEXT("/Game/RA4UI/Art/Remaster/T_SH_13_HudAtlanticNaval.T_SH_13_HudAtlanticNaval");
    case ERA4FactionTheme::EasternCoalition:
        return TEXT("/Game/RA4UI/Art/Remaster/T_SH_14_HudEasternBase.T_SH_14_HudEasternBase");
    case ERA4FactionTheme::PacificPact:
        if (Variant == ERA4UIScreenVariant::BaseDefense)
        {
            return TEXT("/Game/RA4UI/Art/Remaster/T_SH_18_HudPacificBase.T_SH_18_HudPacificBase");
        }
        return TEXT("/Game/RA4UI/Art/Remaster/T_SH_15_HudPacificAir.T_SH_15_HudPacificAir");
    case ERA4FactionTheme::Independent:
        return TEXT("/Game/RA4UI/Art/Remaster/T_SH_16_HudIndependentFront.T_SH_16_HudIndependentFront");
    default:
        return TEXT("/Game/RA4UI/Art/Remaster/T_SH_12_HudEurasianGround.T_SH_12_HudEurasianGround");
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
    const bool bBold = false,
    const bool bWrap = true)
{
    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    Label->SetText(Text);
    Label->SetColorAndOpacity(FSlateColor(Color));
    // Wrapping suits multi-line panels but breaks one-line chrome: the tab strip
    // and the resource strip folded onto a second line.
    Label->SetAutoWrapText(bWrap);
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
            TextureBrush.DrawAs = ESlateBrushDrawType::Box;
            TextureBrush.Margin = FMargin(0.26f);
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
        WidgetTree, Label, 12, bSelected ? FLinearColor::White : HUDText,
        FName(*FString::Printf(TEXT("%s_Label"), *Name.ToString())), bSelected, false);
    Text->SetJustification(ETextJustify::Center);
    Text->SetMinDesiredWidth(0.0f);
    Button->AddChild(Text);
    return Button;
}

const TCHAR* ResolveProductionIconPath(const ERA4FactionTheme Theme, const int32 Index)
{
    // The cards used photoreal renders from the retired roster, so a barracks
    // card showed a rifleman and a refinery card showed a tank. Schematic icons
    // match the reference, read at card size and describe the structure that the
    // card actually builds. The order follows the build list of every direction:
    // headquarters, power, refinery, barracks, factory, radar, special, strategic.
    static const TCHAR* StructureIcons[] = {
        TEXT("/Game/RA4UI/Art/T_RA4_Icon_HQ.T_RA4_Icon_HQ"),
        TEXT("/Game/RA4UI/Art/T_RA4_Icon_Power.T_RA4_Icon_Power"),
        TEXT("/Game/RA4UI/Art/T_RA4_Icon_Refinery.T_RA4_Icon_Refinery"),
        TEXT("/Game/RA4UI/Art/T_RA4_Icon_Barracks.T_RA4_Icon_Barracks"),
        TEXT("/Game/RA4UI/Art/T_RA4_Icon_Factory.T_RA4_Icon_Factory"),
        TEXT("/Game/RA4UI/Art/T_RA4_Icon_Radar.T_RA4_Icon_Radar"),
        TEXT("/Game/RA4UI/Art/T_RA4_Icon_Emp.T_RA4_Icon_Emp"),
        TEXT("/Game/RA4UI/Art/T_RA4_Icon_Silo.T_RA4_Icon_Silo")
    };

    // Every direction builds the same kinds of structure, so one neutral sheet
    // serves all of them; the accent tint carries the identity.
    (void)Theme;
    return StructureIcons[FMath::Clamp(Index, 0, int32(UE_ARRAY_COUNT(StructureIcons)) - 1)];
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
    IconBox->SetHeightOverride(64.0f);
    UImage* Icon = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), FName(Name.ToString() + TEXT("_Icon")));
    if (UTexture2D* IconTexture = LoadObject<UTexture2D>(nullptr, IconPath))
    {
        Icon->SetBrushFromTexture(IconTexture, false);
    }
    Icon->SetColorAndOpacity(Accent);
    Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
    IconBox->SetContent(Icon);
    Content->AddChildToVerticalBox(IconBox);
    UTextBlock* Text = MakeHUDText(
        WidgetTree, Label, 10, HUDText,
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
    case ERA4FactionTheme::EurasianPact:
        Snapshot.PrimaryEntityName = Variant == ERA4UIScreenVariant::GroundAssault
            ? TEXT("ОБТ «ГРАНИТ»")
            : Variant == ERA4UIScreenVariant::BaseDefense
                ? TEXT("КОМПЛЕКС РЭБ «ГРОМОБОЙ»")
                : TEXT("КОМАНДНЫЙ ЦЕНТР «ЗАСЛОН»");
        break;
    case ERA4FactionTheme::AtlanticAlliance:
        Snapshot.PrimaryEntityName = Variant == ERA4UIScreenVariant::NavalWarfare
            ? TEXT("ЭСМИНЕЦ «СВОБОДА»")
            : Variant == ERA4UIScreenVariant::AirWarfare
                ? TEXT("ИСТРЕБИТЕЛЬ F-35C")
                : TEXT("СЕТЕЦЕНТРИЧЕСКИЙ КП");
        break;
    case ERA4FactionTheme::EasternCoalition:
        Snapshot.PrimaryEntityName = TEXT("ИНДУСТРИАЛЬНЫЙ ШТАБ «ТИП-99B»");
        break;
    case ERA4FactionTheme::PacificPact:
        Snapshot.PrimaryEntityName = Variant == ERA4UIScreenVariant::AirWarfare
            ? TEXT("ИСТРЕБИТЕЛЬ «СУСАНОО»")
            : TEXT("ШАГОХОД «КАЙГАН»");
        break;
    case ERA4FactionTheme::Independent:
        Snapshot.PrimaryEntityName = TEXT("МОБИЛЬНЫЙ СПУ «ХЕЙБАР»");
        break;
    case ERA4FactionTheme::Chronolegion:
        Snapshot.PrimaryEntityName = TEXT("ГЛАВНЫЙ ХРОНОРЕАКТОР");
        break;
    default:
        Snapshot.PrimaryEntityName = TEXT("КОМАНДНЫЙ ЦЕНТР");
        break;
    }
    Snapshot.SelectionHealthRatio = 1.0f;
    Snapshot.bPrimaryOwned = true;

    FRA4HUDObjective Primary;
    Primary.Label = Variant == ERA4UIScreenVariant::NavalWarfare
        ? LOCTEXT("ObjectiveNaval", "Уничтожить вражеский флот")
        : Variant == ERA4UIScreenVariant::AirWarfare
            ? LOCTEXT("ObjectiveAir", "Захватить передовые аэродромы")
            : Variant == ERA4UIScreenVariant::InsurgentFront
                ? LOCTEXT("ObjectiveInsurgent", "Продержаться до завершения насыщающего удара")
                : LOCTEXT("ShowcaseObjectivePrimary", "Уничтожить базу противника");
    Snapshot.Objectives.Add(Primary);
    FRA4HUDObjective Secondary;
    Secondary.Label = LOCTEXT("ShowcaseObjectiveSecondary", "Захватить хранилище ресурсов");
    Secondary.Current = 1;
    Secondary.Target = 3;
    Snapshot.Objectives.Add(Secondary);

    // Names and prices come from the building tables of the design bible rather
    // than being invented: the showcase previously offered "НЕФТЕБАЗА",
    // "ОБСЕРВАТОРИЯ" and "СТЕНА", none of which exist in this world, at costs
    // generated from the loop index.
    struct FShowcaseBuildEntry
    {
        FText Name;
        int32 Cost;
    };
    TArray<FShowcaseBuildEntry> BuildEntries;

    if (Variant == ERA4UIScreenVariant::NavalWarfare)
    {
        BuildEntries = {
            {LOCTEXT("NavalDock", "ОКЕАНИЧЕСКИЙ ДОК"), 2100},
            {LOCTEXT("NavalFrigate", "ФРЕГАТ"), 900},
            {LOCTEXT("NavalDestroyer", "ЭСМИНЕЦ"), 1400},
            {LOCTEXT("NavalCruiser", "КРЕЙСЕР"), 2200},
            {LOCTEXT("NavalMissile", "РАКЕТНЫЙ КАТЕР"), 700},
            {LOCTEXT("NavalSub", "ПОДЛОДКА"), 1600},
            {LOCTEXT("NavalHarvester", "M88 «PIONEER»"), 1200},
            {LOCTEXT("NavalShield", "АКТИВНАЯ ЗАЩИТА"), 1800}};
    }
    else if (Variant == ERA4UIScreenVariant::AirWarfare || Theme == ERA4FactionTheme::AtlanticAlliance)
    {
        BuildEntries = {
            {LOCTEXT("AtlHQ", "СЕТЕВОЙ ШТАБ"), 5000},
            {LOCTEXT("AtlReactor", "КОМПАКТНЫЙ РЕАКТОР"), 900},
            {LOCTEXT("AtlRefinery", "ПЕРЕРАБОТЧИК"), 2500},
            {LOCTEXT("AtlBarracks", "ТАКТИЧЕСКАЯ КАЗАРМА"), 750},
            {LOCTEXT("AtlFactory", "МОДУЛЬНЫЙ ЗАВОД"), 2200},
            {LOCTEXT("AtlAirbase", "АВИАБАЗА"), 1850},
            {LOCTEXT("AtlRecon", "РАЗВЕДЦЕНТР"), 1450},
            {LOCTEXT("AtlStrike", "ГИПЕРЗВУКОВОЙ КОМПЛЕКС"), 7200}};
    }
    else if (Theme == ERA4FactionTheme::EasternCoalition)
    {
        BuildEntries = {
            {LOCTEXT("CnHQ", "КОМАНДНЫЙ ЦЕНТР"), 5000},
            {LOCTEXT("CnSolar", "СОЛНЕЧНАЯ СТАНЦИЯ"), 850},
            {LOCTEXT("CnRefinery", "ПЕРЕРАБОТЧИК"), 2450},
            {LOCTEXT("CnTraining", "УЧЕБНЫЙ ЦЕНТР"), 720},
            {LOCTEXT("CnRobotics", "ЗАВОД РОБОТОТЕХНИКИ"), 2250},
            {LOCTEXT("CnDroneBase", "АВИАБАЗА БПЛА"), 1900},
            {LOCTEXT("CnNetwork", "ЦЕНТР УПРАВЛЕНИЯ"), 1500},
            {LOCTEXT("CnSeismic", "СЕЙСМИЧЕСКИЙ КОМПЛЕКС"), 7000}};
    }
    else if (Theme == ERA4FactionTheme::PacificPact)
    {
        BuildEntries = {
            {LOCTEXT("JpHQ", "КОМАНДНЫЙ ТЕРМИНАЛ"), 5000},
            {LOCTEXT("JpPower", "ЭЛЕКТРОСТАНЦИЯ"), 880},
            {LOCTEXT("JpRefinery", "ПЕРЕРАБОТЧИК"), 2450},
            {LOCTEXT("JpTraining", "УЧЕБНЫЙ ЦЕНТР"), 740},
            {LOCTEXT("JpRobotics", "ЦЕХ «КАЙГАН»"), 2300},
            {LOCTEXT("JpRadar", "БЕРЕГОВОЙ РАДАР"), 1500},
            {LOCTEXT("JpLaser", "ЛАЗЕРНЫЙ ПЕРЕХВАТ"), 1050},
            {LOCTEXT("JpMatrix", "МАТРИЦА ПЕРЕХВАТА"), 6100}};
    }
    else if (Theme == ERA4FactionTheme::Independent)
    {
        BuildEntries = {
            {LOCTEXT("IrHQ", "МОБИЛЬНЫЙ УЗЕЛ «МИРАЖ»"), 5000},
            {LOCTEXT("IrPower", "ЭЛЕКТРОСТАНЦИЯ"), 820},
            {LOCTEXT("IrRefinery", "ПЕРЕРАБОТЧИК"), 2400},
            {LOCTEXT("IrBarracks", "КАЗАРМА"), 700},
            {LOCTEXT("IrLaunchers", "ПУСКОВЫЕ УСТАНОВКИ"), 1600},
            {LOCTEXT("IrDrones", "АНГАР БПЛА"), 1450},
            {LOCTEXT("IrJammer", "КОМПЛЕКС РЭБ"), 1900},
            {LOCTEXT("IrDecoy", "ЛОЖНЫЕ ПОЗИЦИИ"), 600}};
    }
    else if (Theme == ERA4FactionTheme::Chronolegion)
    {
        BuildEntries = {
            {LOCTEXT("ChCausality", "ЦЕНТР ПРИЧИННОСТИ"), 5200},
            {LOCTEXT("ChReactor", "РЕАКТОР РАСПАДА"), 950},
            {LOCTEXT("ChRefinery", "КВАНТОВЫЙ ПЕРЕРАБОТЧИК"), 2600},
            {LOCTEXT("ChBarracks", "КАЗАРМА ЭХА"), 800},
            {LOCTEXT("ChFactory", "ФАБРИКА КОНТИНУУМА"), 2400},
            {LOCTEXT("ChObserver", "НАБЛЮДАТЕЛЬ"), 1650},
            {LOCTEXT("ChArchive", "АРХИВ БУДУЩЕГО"), 3900},
            {LOCTEXT("ChCollapser", "СИНГУЛЯРНЫЙ КОЛЛАПСЕР"), 7500}};
    }
    else
    {
        BuildEntries = {
            {LOCTEXT("RuHQ", "ШТАБ"), 5000},
            {LOCTEXT("RuPower", "ЭЛЕКТРОСТАНЦИЯ"), 800},
            {LOCTEXT("RuRefinery", "ПЕРЕРАБОТЧИК"), 2400},
            {LOCTEXT("RuBarracks", "КАЗАРМА"), 700},
            {LOCTEXT("RuFactory", "ЗАВОД БРОНЕТЕХНИКИ"), 2300},
            {LOCTEXT("RuRadar", "РАДАРНЫЙ УЗЕЛ"), 1500},
            {LOCTEXT("RuEmp", "КОМПЛЕКС ЭМИ «ПЕРУН»"), 1900},
            {LOCTEXT("RuSilo", "РАКЕТНАЯ ШАХТА «КАРАТЕЛЬ»"), 7000}};
    }

    for (int32 Index = 0; Index < BuildEntries.Num(); ++Index)
    {
        FRA4BuildOption Option;
        Option.ContentId = Index + 1;
        Option.DisplayName = BuildEntries[Index].Name;
        Option.Cost = BuildEntries[Index].Cost;
        Option.Category = ActiveCategory;
        Option.bAvailable = Index != BuildEntries.Num() - 1;
        Option.BlockReason = Option.bAvailable
            ? ERA4BuildBlockReason::None
            : ERA4BuildBlockReason::MissingPrerequisite;
        Snapshot.BuildOptions.Add(Option);
    }

    FRA4ProductionEntry Tank;
    Tank.ContentId = 101;
    Tank.DisplayName = BuildEntries[1].Name;
    Tank.ProgressPercent = 68;
    Tank.RemainingSeconds = 12.0f;
    Snapshot.ProductionQueue.Add(Tank);
    FRA4ProductionEntry Infantry;
    Infantry.ContentId = 102;
    Infantry.DisplayName = BuildEntries[3].Name;
    Infantry.ProgressPercent = 39;
    Infantry.RemainingSeconds = 8.0f;
    Snapshot.ProductionQueue.Add(Infantry);

    FRA4Alert Alert;
    Alert.Message = Variant == ERA4UIScreenVariant::InsurgentFront
        ? LOCTEXT("SaturationStrikeAlert", "САТУРАЦИОННЫЙ УДАР НАЧАТ — УКРЫТИЯ ЗАЩИЩЕНЫ")
        : Variant == ERA4UIScreenVariant::BaseDefense
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
    SetScreenIdentity(ERA4UIScreenId::EurasianHud);
}

void URA4HUDWidget::ConfigureHUD(
    const ERA4FactionTheme InFactionTheme,
    const ERA4UIScreenVariant InVariant,
    const int32 InActiveProductionTab)
{
    FactionTheme = InFactionTheme;
    HUDVariant = InVariant;
    ActiveProductionTab = FMath::Clamp(InActiveProductionTab, 0, 4);

    ERA4UIScreenId HUDScreen = ERA4UIScreenId::EurasianHud;
    switch (FactionTheme)
    {
    case ERA4FactionTheme::EurasianPact:
        HUDScreen = ERA4UIScreenId::EurasianHud;
        break;
    case ERA4FactionTheme::AtlanticAlliance:
        HUDScreen = ERA4UIScreenId::AtlanticHud;
        break;
    case ERA4FactionTheme::EasternCoalition:
        HUDScreen = ERA4UIScreenId::EasternHud;
        break;
    case ERA4FactionTheme::PacificPact:
        HUDScreen = ERA4UIScreenId::PacificHud;
        break;
    case ERA4FactionTheme::Independent:
        HUDScreen = ERA4UIScreenId::IndependentHud;
        break;
    default:
        // Retired directions fall back to the shared Eurasian shell.
        HUDScreen = ERA4UIScreenId::EurasianHud;
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
        FCommandLine::Get(), TEXT("RA4Screen="), ShowcaseScreen) && ShowcaseScreen >= 12;
    if (bShowcaseMode)
    {
        // Showcase mode (RA4Screen=12..18) renders the canonical remaster HUD
        // plate as the backdrop so the live HUD chrome can be reviewed against
        // its reference composition. The plate is selected by faction AND
        // variant, matching the 12–18 reference set one-to-one.
        const TCHAR* RemasterPlate = ResolveRemasterHudPlate(FactionTheme, HUDVariant);
        if (UTexture2D* Background = LoadObject<UTexture2D>(nullptr, RemasterPlate))
        {
            GetBackgroundLayer()->SetBrushFromTexture(Background, false);
            // Light tint keeps the painted HUD plate readable while the live
            // chrome above it owns the readable pixels.
            GetBackgroundLayer()->SetColorAndOpacity(FLinearColor(0.84f, 0.86f, 0.90f, 1.0f));
        }
        else if (UTexture2D* Fallback = LoadObject<UTexture2D>(nullptr, ThemeStyle.BackgroundPath))
        {
            GetBackgroundLayer()->SetBrushFromTexture(Fallback, false);
            GetBackgroundLayer()->SetColorAndOpacity(FLinearColor(0.72f, 0.72f, 0.72f, 1.0f));
        }
    }
    else
    {
        GetBackgroundLayer()->SetBrushFromTexture(nullptr);
        GetBackgroundLayer()->SetColorAndOpacity(FLinearColor::Transparent);
    }
    HUDViewModel = NewObject<URA4HUDViewModel>(this);
    Occlusion.Reset();

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("HUDCanvas"));
    UOverlaySlot* CanvasSlot = GetContentLayer()->AddChildToOverlay(Canvas);
    CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
    CanvasSlot->SetVerticalAlignment(VAlign_Fill);

    UVerticalBox* Objectives = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ObjectivesPanelContent"));
    Objectives->AddChildToVerticalBox(MakeHUDText(
        WidgetTree, ThemeStyle.Commander,
        16, ThemeStyle.Text, TEXT("CommanderTitle"), true, false));
    Objectives->AddChildToVerticalBox(MakeHUDText(
        WidgetTree, LOCTEXT("ObjectivesHeading", "ОСНОВНЫЕ ЗАДАЧИ"),
        15, ThemeStyle.Accent, TEXT("ObjectivesHeading"), true))->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 5.0f));
    ObjectivesList = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ObjectivesList"));
    Objectives->AddChildToVerticalBox(ObjectivesList);
    const FVector2D ObjectivesPosition(16.0f, 18.0f);
    const FVector2D ObjectivesSize(370.0f, 205.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Objectives, TEXT("ObjectivesPanel"), ThemeStyle.Panel, ThemeStyle.Accent), ObjectivesPosition, ObjectivesSize, 10);
    AddInteractiveRegion(ObjectivesPosition, ObjectivesSize);

    ResourceText = MakeHUDText(
        WidgetTree, FText::GetEmpty(), 17, ThemeStyle.Text, TEXT("ResourceText"), true, false);
    ResourceText->SetJustification(ETextJustify::Center);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, ResourceText, TEXT("ResourceBarPanel"), ThemeStyle.Panel, ThemeStyle.Accent),
        FVector2D(1046.0f, 12.0f), FVector2D(846.0f, 55.0f), 10);

    URA4MinimapWidget* Minimap = WidgetTree->ConstructWidget<URA4MinimapWidget>(
        URA4MinimapWidget::StaticClass(), TEXT("TacticalMinimap"));
    Minimap->SetSnapshot(MakeShowcaseRadarMarkers(), FVector2D(100.0f, 100.0f), 0);
    Minimap->SetViewportWorldBounds(FVector2D(18.0f, 54.0f), FVector2D(46.0f, 82.0f));
    const FVector2D MinimapPosition(1590.0f, 80.0f);
    const FVector2D MinimapSize(310.0f, 285.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Minimap, TEXT("MinimapPanel"), ThemeStyle.Panel, ThemeStyle.Accent), MinimapPosition, MinimapSize, 10);
    AddInteractiveRegion(MinimapPosition, MinimapSize);

    UVerticalBox* Sidebar = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ProductionSidebar"));
    // One button carrying every caption could not fit the sidebar and clipped at
    // both ends. The reference has separate tabs, so each category gets its own
    // button and the row divides the width between them.
    TArray<FText> TabCaptions;
    if (FactionTheme == ERA4FactionTheme::AtlanticAlliance)
    {
        TabCaptions = HUDVariant == ERA4UIScreenVariant::NavalWarfare
            ? TArray<FText>{LOCTEXT("Tab_Str", "СТРОИТЬ"), LOCTEXT("Tab_Inf", "ПЕХОТА"),
                            LOCTEXT("Tab_Veh", "ТЕХНИКА"), LOCTEXT("Tab_Air", "АВИАЦИЯ"),
                            LOCTEXT("Tab_Sea", "ФЛОТ")}
            : TArray<FText>{LOCTEXT("Tab_Str", "СТРОИТЬ"), LOCTEXT("Tab_Inf", "ПЕХОТА"),
                            LOCTEXT("Tab_Veh", "ТЕХНИКА"), LOCTEXT("Tab_Air", "АВИАЦИЯ")};
    }
    else if (FactionTheme == ERA4FactionTheme::Chronolegion)
    {
        TabCaptions = {LOCTEXT("Tab_Str", "СТРОИТЬ"), LOCTEXT("Tab_Units", "ВОЙСКА"),
                       LOCTEXT("Tab_Support", "ПОДДЕРЖКА"), LOCTEXT("Tab_Special", "ОСОБОЕ")};
    }
    else
    {
        TabCaptions = {LOCTEXT("Tab_Str", "СТРОИТЬ"), LOCTEXT("Tab_Units", "ВОЙСКА"),
                       LOCTEXT("Tab_Upg", "УЛУЧШЕНИЯ"), LOCTEXT("Tab_Doc", "ДОКТРИНА")};
    }

    UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("ProductionTabRow"));
    for (int32 TabIndex = 0; TabIndex < TabCaptions.Num(); ++TabIndex)
    {
        UButton* Tab = MakeHUDButton(
            WidgetTree, TabCaptions[TabIndex],
            FName(*FString::Printf(TEXT("ProductionTab_%d"), TabIndex)),
            ThemeStyle.Accent, TabIndex == ActiveProductionTab);
        Tab->SetClipping(EWidgetClipping::ClipToBounds);
        Tab->OnClicked.AddDynamic(this, &URA4HUDWidget::CycleProductionTab);
        if (UHorizontalBoxSlot* TabSlot = TabRow->AddChildToHorizontalBox(Tab))
        {
            TabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            TabSlot->SetPadding(FMargin(0.0f, 0.0f, 3.0f, 0.0f));
        }
    }
    Sidebar->AddChildToVerticalBox(TabRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
    BuildGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(
        UUniformGridPanel::StaticClass(), TEXT("BuildGrid"));
    UVerticalBoxSlot* BuildGridSlot = Sidebar->AddChildToVerticalBox(BuildGrid);
    BuildGridSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    const FVector2D SidebarPosition(1450.0f, 380.0f);
    const FVector2D SidebarSize(450.0f, 500.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Sidebar, TEXT("ProductionPanel"), ThemeStyle.Panel, ThemeStyle.Accent), SidebarPosition, SidebarSize, 10);
    AddInteractiveRegion(SidebarPosition, SidebarSize);

    // Reference 12_battle_hud_eurasian_ground.png: the selection card carries a
    // portrait beside the name, then armour and structure read as bars rather
    // than as another line of text.
    UHorizontalBox* Selection = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("SelectionContent"));

    USizeBox* PortraitBox = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("SelectionPortraitBox"));
    PortraitBox->SetWidthOverride(112.0f);
    PortraitBox->SetHeightOverride(112.0f);
    UImage* Portrait = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SelectionPortrait"));
    if (UTexture2D* PortraitTexture = LoadObject<UTexture2D>(nullptr, ThemeStyle.BackgroundPath))
    {
        Portrait->SetBrushFromTexture(PortraitTexture, false);
    }
    Portrait->SetColorAndOpacity(FLinearColor(0.75f, 0.78f, 0.86f, 1.0f));
    PortraitBox->AddChild(Portrait);
    UHorizontalBoxSlot* PortraitSlot = Selection->AddChildToHorizontalBox(
        MakeHUDPanel(WidgetTree, PortraitBox, TEXT("SelectionPortraitFrame"),
            ThemeStyle.Panel, ThemeStyle.Accent, nullptr));
    PortraitSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
    PortraitSlot->SetVerticalAlignment(VAlign_Top);

    UVerticalBox* SelectionInfo = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("SelectionInfo"));
    SelectionTitleText = MakeHUDText(
        WidgetTree, FText::GetEmpty(), 18, ThemeStyle.Text, TEXT("SelectionTitle"), true, false);
    SelectionDetailText = MakeHUDText(
        WidgetTree, FText::GetEmpty(), 13, HUDMuted, TEXT("SelectionDetails"));
    SelectionInfo->AddChildToVerticalBox(SelectionTitleText);
    SelectionInfo->AddChildToVerticalBox(SelectionDetailText)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 8.0f));

    const auto AddStatusBar = [this, SelectionInfo, &ThemeStyle](
        const FText& Caption, const FLinearColor& Fill, const FName Name) -> UProgressBar*
    {
        SelectionInfo->AddChildToVerticalBox(MakeHUDText(
            WidgetTree, Caption, 11, HUDMuted, FName(Name.ToString() + TEXT("_Cap")), false, false));
        UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), Name);
        Bar->SetFillColorAndOpacity(Fill);
        SelectionInfo->AddChildToVerticalBox(Bar)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 6.0f));
        return Bar;
    };
    SelectionArmourBar = AddStatusBar(
        LOCTEXT("SelArmour", "БРОНЯ"), ThemeStyle.Accent, TEXT("SelectionArmourBar"));
    SelectionHealthBar = AddStatusBar(
        LOCTEXT("SelHealth", "ПРОЧНОСТЬ"), FLinearColor(0.30f, 0.82f, 0.42f, 1.0f), TEXT("SelectionHealthBar"));

    UHorizontalBoxSlot* InfoSlot = Selection->AddChildToHorizontalBox(SelectionInfo);
    InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    const FVector2D SelectionPosition(16.0f, 820.0f);
    const FVector2D SelectionSize(420.0f, 240.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Selection, TEXT("SelectionPanel"), ThemeStyle.Panel, ThemeStyle.Accent), SelectionPosition, SelectionSize, 10);
    AddInteractiveRegion(SelectionPosition, SelectionSize);

    UVerticalBox* Queue = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ProductionQueueContent"));
    const FText QueueHeading = HUDVariant == ERA4UIScreenVariant::InsurgentFront
        ? LOCTEXT("LauncherQueueHeading", "ОЧЕРЕДЬ ПУСКОВЫХ")
        : HUDVariant == ERA4UIScreenVariant::NavalWarfare
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
        WidgetTree, Queue, TEXT("ProductionQueuePanel"), ThemeStyle.Panel, ThemeStyle.Accent), QueuePosition, QueueSize, 10);
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
    const FText InitialCommandStatus = HUDVariant == ERA4UIScreenVariant::InsurgentFront
        ? LOCTEXT("SaturationStrikeActive", "САТУРАЦИОННЫЙ УДАР НАЧАТ  •  УКРЫТИЯ ЗАЩИЩЕНЫ")
        : HUDVariant == ERA4UIScreenVariant::BaseDefense
            ? LOCTEXT("BaseDefenceActive", "ПРОТОКОЛ ОБОРОНЫ БАЗЫ АКТИВЕН")
            : LOCTEXT("CommandReady", "КОМАНДНАЯ СЕТЬ ГОТОВА");
    CommandStatusText = MakeHUDText(
        WidgetTree, InitialCommandStatus,
        13, HUDGreen, TEXT("CommandStatus"));
    Commands->AddChildToVerticalBox(CommandStatusText)->SetPadding(FMargin(4.0f, 10.0f));
    const FVector2D CommandPosition(1450.0f, 900.0f);
    const FVector2D CommandSize(450.0f, 160.0f);
    PlaceHUDWidget(Canvas, MakeHUDPanel(
        WidgetTree, Commands, TEXT("CommandGridPanel"), ThemeStyle.Panel, ThemeStyle.Accent), CommandPosition, CommandSize, 10);
    AddInteractiveRegion(CommandPosition, CommandSize);

    const bool bProminentAlert = HUDVariant == ERA4UIScreenVariant::BaseDefense ||
        HUDVariant == ERA4UIScreenVariant::InsurgentFront;
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
    if (FParse::Value(FCommandLine::Get(), TEXT("RA4Screen="), ShowcaseScreen) && ShowcaseScreen >= 12)
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
    // Structure follows the snapshot; armour is a derived readout of how much of
    // that structure is still intact, so a fresh unit reads full on both bars.
    const float Health = FMath::Clamp(HUDViewModel->GetSelectionHealthRatio(), 0.0f, 1.0f);
    if (SelectionHealthBar)
    {
        SelectionHealthBar->SetPercent(Health);
    }
    if (SelectionArmourBar)
    {
        SelectionArmourBar->SetPercent(FMath::Clamp(0.18f + Health * 0.82f, 0.0f, 1.0f));
    }
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
        // Long canon names such as "ПЕРЕРАБАТЫВАЮЩИЙ КОМПЛЕКС" must stay inside
        // their own card instead of running across the neighbouring one.
        Card->SetClipping(EWidgetClipping::ClipToBounds);
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
    Occlusion.Add(Position, Size);
}

float URA4HUDWidget::GetBattlefieldViewFraction() const
{
    return Occlusion.BattlefieldShare();
}

bool URA4HUDWidget::IsWorldInputBlockedAtReferencePoint(const FVector2D Point) const
{
    return Occlusion.IsBlocked(Point);
}

void URA4HUDWidget::CycleProductionTab()
{
    const int32 TabCount = HUDVariant == ERA4UIScreenVariant::NavalWarfare ? 5 : 4;
    ActiveProductionTab = (ActiveProductionTab + 1) % TabCount;
    int32 ShowcaseScreen = 0;
    if (FParse::Value(FCommandLine::Get(), TEXT("RA4Screen="), ShowcaseScreen) && ShowcaseScreen >= 12)
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
