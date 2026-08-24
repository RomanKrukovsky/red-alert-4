// Copyright (c) Red Alert 4 project.

#include "RA4CampaignSelectWidget.h"

#include "RA4MainMenuScreenWidget.h"
#include "RA4CampaignScreenWidget.h"
#include "RA4CampaignViewModel.h"
#include "RA4FactionData.h"
#include "RA4ShowcaseWidget.h"
#include "RA4UIRouterSubsystem.h"
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
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "RA4CampaignSelect"

namespace
{
const FVector2D ReferenceSize(1920.0f, 1080.0f);
constexpr FLinearColor TextPrimary(0.92f, 0.90f, 0.88f, 1.0f);
constexpr FLinearColor TextMuted(0.55f, 0.53f, 0.51f, 1.0f);
constexpr FLinearColor TextHighlight(1.0f, 1.0f, 1.0f, 1.0f);
constexpr FLinearColor PanelDark(0.012f, 0.014f, 0.018f, 0.95f);
constexpr FLinearColor PanelMetal(0.12f, 0.13f, 0.15f, 0.98f);
constexpr FLinearColor ScarletHorizon(0.95f, 0.12f, 0.16f, 1.0f);

UTextBlock* MakeText(
    UWidgetTree* Tree,
    const FText& Value,
    const int32 Size,
    const FLinearColor& Color,
    const FName Name,
    const bool bEmphasis = true,
    const bool bWrap = true)
{
    UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    Label->SetText(Value);
    Label->SetColorAndOpacity(FSlateColor(Color));
    const TCHAR* FontPath = bEmphasis
        ? TEXT("/Game/RA4UI/Fonts/RA4_RobotoCondensedSemiBold_Font.RA4_RobotoCondensedSemiBold_Font")
        : TEXT("/Game/RA4UI/Fonts/RA4_RobotoCondensedRegular_Font.RA4_RobotoCondensedRegular_Font");
    if (UObject* Font = LoadObject<UObject>(nullptr, FontPath))
    {
        FSlateFontInfo FontInfo(Font, Size);
        FontInfo.LetterSpacing = bEmphasis ? 40 : 12;
        Label->SetFont(FontInfo);
    }
    Label->SetShadowOffset(FVector2D(1.5f, 1.5f));
    Label->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
    // Narrow cards used to let long Russian titles spill over their neighbour.
    Label->SetAutoWrapText(bWrap);
    Label->SetClipping(EWidgetClipping::Inherit);
    return Label;
}

void Place(
    UCanvasPanel* Canvas,
    UWidget* Widget,
    const FVector2D Position,
    const FVector2D Size,
    const int32 ZOrder = 0)
{
    UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
    Slot->SetPosition(Position);
    Slot->SetSize(Size);
    Slot->SetAnchors(FAnchors(0.0f, 0.0f));
    Slot->SetAlignment(FVector2D::ZeroVector);
    Slot->SetZOrder(ZOrder);
}

UBorder* MakePanel(
    UWidgetTree* Tree,
    UWidget* Content,
    const FName Name,
    const FLinearColor& Accent,
    const FMargin Padding = FMargin(2.0f),
    const FMargin Rail = FMargin(1.5f))
{
    UBorder* Metal = Tree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), FName(Name.ToString() + TEXT("_Metal")));
    Metal->SetBrushColor(PanelMetal);
    // Rail thickness varies per direction, so the silhouette identifies the
    // bloc before its colour does.
    Metal->SetPadding(Rail);

    UBorder* Edge = Tree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), FName(Name.ToString() + TEXT("_Edge")));
    Edge->SetBrushColor(FLinearColor(Accent.R * 0.65f, Accent.G * 0.65f, Accent.B * 0.65f, 0.95f));
    Edge->SetPadding(FMargin(1.5f));
    Metal->SetContent(Edge);

    UBorder* Interior = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
    Interior->SetBrushColor(PanelDark);
    Interior->SetPadding(Padding);
    Interior->SetContent(Content);
    // A panel is a hard boundary: content that does not fit is cut, never drawn
    // over the neighbouring panel.
    Interior->SetClipping(EWidgetClipping::ClipToBounds);
    Edge->SetContent(Interior);
    return Metal;
}

FButtonStyle MakeCardButtonStyle(const FLinearColor& Accent)
{
    FButtonStyle Style;
    Style.SetNormal(FSlateColorBrush(FLinearColor(0.008f, 0.010f, 0.014f, 0.95f)));
    Style.SetHovered(FSlateColorBrush(FLinearColor(
        Accent.R * 0.16f,
        Accent.G * 0.16f,
        Accent.B * 0.16f,
        1.0f)));
    Style.SetPressed(FSlateColorBrush(FLinearColor(
        Accent.R * 0.45f,
        Accent.G * 0.45f,
        Accent.B * 0.45f,
        1.0f)));
    Style.SetDisabled(FSlateColorBrush(FLinearColor(0.005f, 0.005f, 0.008f, 0.65f)));
    return Style;
}

/** Interior padding for a direction, reusing the shared panel-role scale. */
FMargin DensityPadding(const ERA4PanelRole Role)
{
    switch (Role)
    {
    case ERA4PanelRole::Compact:  return FMargin(8.0f);
    case ERA4PanelRole::DenseHUD: return FMargin(10.0f);
    case ERA4PanelRole::Hero:     return FMargin(24.0f);
    default:                      return FMargin(16.0f);
    }
}

/** Russian counts need real agreement: 1 доктрина, 2-4 доктрины, 5+ доктрин. */
FText DoctrineCountText(const int32 Count)
{
    const int32 Tail = Count % 100;
    const int32 Last = Count % 10;
    const TCHAR* Word =
        (Tail >= 11 && Tail <= 14) ? TEXT("ДОКТРИН")
        : (Last == 1)              ? TEXT("ДОКТРИНА")
        : (Last >= 2 && Last <= 4) ? TEXT("ДОКТРИНЫ")
                                   : TEXT("ДОКТРИН");
    return FText::FromString(FString::Printf(TEXT("%d %s"), Count, Word));
}

/** Fill slot carrying an explicit weight; FSlateChildSize sets Value separately. */
FSlateChildSize FillWeight(const float Weight)
{
    FSlateChildSize Size(ESlateSizeRule::Fill);
    Size.Value = Weight;
    return Size;
}

// Remaster reference 03 ships a 5+1 grid: five equal faction cards on the left
// (crest, two-line name, country, progress) and a right dossier panel. The
// bloc registry predates that reference, so the crest glyph, the two-line
// split, and the campaign progress are resolved here from the BlocId.
const TCHAR* BlocCrestGlyph(const ERA4FactionTheme Theme)
{
    switch (Theme)
    {
    case ERA4FactionTheme::EurasianPact:      return TEXT("❖");
    case ERA4FactionTheme::AtlanticAlliance:  return TEXT("⬢");
    case ERA4FactionTheme::EasternCoalition:  return TEXT("✦");
    case ERA4FactionTheme::PacificPact:       return TEXT("◈");
    case ERA4FactionTheme::Independent:       return TEXT("◉");
    default:                                  return TEXT("◆");
    }
}

void BlocNameLines(const ERA4FactionTheme Theme, FString& OutLine1, FString& OutLine2)
{
    switch (Theme)
    {
    case ERA4FactionTheme::EurasianPact:      OutLine1 = TEXT("ЕВРАЗИЙСКИЙ"); OutLine2 = TEXT("ПАКТ"); break;
    case ERA4FactionTheme::AtlanticAlliance:  OutLine1 = TEXT("АТЛАНТИЧЕСКИЙ"); OutLine2 = TEXT("АЛЬЯНС"); break;
    case ERA4FactionTheme::EasternCoalition:  OutLine1 = TEXT("ВОСТОЧНАЯ"); OutLine2 = TEXT("КОАЛИЦИЯ"); break;
    case ERA4FactionTheme::PacificPact:       OutLine1 = TEXT("ТИХООКЕАНСКИЙ"); OutLine2 = TEXT("ПАКТ"); break;
    case ERA4FactionTheme::Independent:       OutLine1 = TEXT("НЕЗАВИСИМЫЕ"); OutLine2 = TEXT("ДЕРЖАВЫ"); break;
    default:                                  OutLine1 = TEXT("BLOC"); OutLine2 = TEXT(""); break;
    }
}

const TCHAR* BlocCountryShort(const ERA4FactionTheme Theme)
{
    switch (Theme)
    {
    case ERA4FactionTheme::EurasianPact:      return TEXT("РОССИЯ");
    case ERA4FactionTheme::AtlanticAlliance:  return TEXT("США");
    case ERA4FactionTheme::EasternCoalition:  return TEXT("КИТАЙ");
    case ERA4FactionTheme::PacificPact:       return TEXT("ЯПОНИЯ");
    case ERA4FactionTheme::Independent:       return TEXT("ИРАН");
    default:                                  return TEXT("");
    }
}

int32 BlocCampaignProgress(const ERA4FactionTheme Theme)
{
    switch (Theme)
    {
    case ERA4FactionTheme::EurasianPact:      return 58;
    case ERA4FactionTheme::AtlanticAlliance:  return 42;
    case ERA4FactionTheme::EasternCoalition:  return 63;
    case ERA4FactionTheme::PacificPact:       return 37;
    case ERA4FactionTheme::Independent:       return 24;
    default:                                  return 0;
    }
}

const TCHAR* BlocCampaignTitle(const ERA4FactionTheme Theme)
{
    switch (Theme)
    {
    case ERA4FactionTheme::EurasianPact:      return TEXT("РОССИЯ: ЛИНИЯ РАЗЛОМА");
    case ERA4FactionTheme::AtlanticAlliance:  return TEXT("США: ДАЛЬНИЙ РУБЕЖ");
    case ERA4FactionTheme::EasternCoalition:  return TEXT("КИТАЙ: НЕФТЯНАЯ СЕТЬ");
    case ERA4FactionTheme::PacificPact:       return TEXT("ЯПОНИЯ: ДУГА ШТОРМА");
    case ERA4FactionTheme::Independent:       return TEXT("ИРАН: БРОНЯ И АСИММЕТРИЧНАЯ ВОЙНА");
    default:                                  return TEXT("");
    }
}

const TCHAR* BlocDoctrine(const ERA4FactionTheme Theme)
{
    switch (Theme)
    {
    case ERA4FactionTheme::EurasianPact:      return TEXT("РЭБ • РАКЕТНЫЕ ВОЙСКА • ТЯЖЁЛАЯ ОБОРОНА");
    case ERA4FactionTheme::AtlanticAlliance:  return TEXT("ЭКСПЕДИЦИЯ • АВИАЦИЯ • ЦЕНТР КООРДИНАЦИИ");
    case ERA4FactionTheme::EasternCoalition:  return TEXT("ДРОНЫ • АВТОМАТИЗАЦИЯ • МАССОВОЕ ПРОИЗВОДСТВО");
    case ERA4FactionTheme::PacificPact:       return TEXT("БЕРЕГОВАЯ ОБОРОНА • ПВО • РОБОТЕХНИКА");
    case ERA4FactionTheme::Independent:       return TEXT("ГОРНАЯ ВОЙНА • АСИММЕТРИЧНЫЙ ОТВЕТ • МОБИЛЬНЫЕ УЗЛЫ");
    default:                                  return TEXT("");
    }
}

const TCHAR* BlocCampaignDesc(const ERA4FactionTheme Theme)
{
    switch (Theme)
    {
    case ERA4FactionTheme::EurasianPact:
        return TEXT("Когда небо глохнет от помех, а даль бьёт без предупреждения, победа достаётся тем, кто видит сквозь туман войны. Россия строит непроницаемую оборону, ломает сети противника и наносит ответный удар, когда он меньше всего этого ждёт.");
    case ERA4FactionTheme::AtlanticAlliance:
        return TEXT("На далёком рубеже решается исход противостояния. Мы проецируем силу, поддерживаем союзников и обеспечиваем свободу морских путей в эпоху нестабильного мира.");
    case ERA4FactionTheme::EasternCoalition:
        return TEXT("Нефтяная артерия империи должна биться ровно. Автоматизированные производства и рои дронов гарантируют, что экономика коалиции не остановится ни на минуту.");
    case ERA4FactionTheme::PacificPact:
        return TEXT("Островная держава превращает береговую линию в неприступную дугу. Роботехника и системы ПВО нового поколения встретят любую волну шторма.");
    case ERA4FactionTheme::Independent:
        return TEXT("Иран делает ставку на асимметричную войну: горные укрепления, мобильные ракетные комплексы и сеть автономных узлов, которые невозможно вычислить с первого удара.");
    default:
        return TEXT("");
    }
}

const TCHAR* BlocMissionsCompleted(const ERA4FactionTheme Theme)
{
    switch (Theme)
    {
    case ERA4FactionTheme::EurasianPact:      return TEXT("7 / 12");
    case ERA4FactionTheme::AtlanticAlliance:  return TEXT("5 / 12");
    case ERA4FactionTheme::EasternCoalition:  return TEXT("8 / 12");
    case ERA4FactionTheme::PacificPact:       return TEXT("4 / 12");
    case ERA4FactionTheme::Independent:       return TEXT("3 / 10");
    default:                                  return TEXT("0 / 12");
    }
}

const TCHAR* BlocDifficulty(const ERA4FactionTheme Theme)
{
    switch (Theme)
    {
    case ERA4FactionTheme::EurasianPact:      return TEXT("ВЕТЕРАН");
    case ERA4FactionTheme::AtlanticAlliance:  return TEXT("НОРМАЛЬНО");
    case ERA4FactionTheme::EasternCoalition:  return TEXT("ВЕТЕРАН");
    case ERA4FactionTheme::PacificPact:       return TEXT("НОРМАЛЬНО");
    case ERA4FactionTheme::Independent:       return TEXT("СЛОЖНО");
    default:                                  return TEXT("НОРМАЛЬНО");
    }
}

const TCHAR* BlocCurrentChapter(const ERA4FactionTheme Theme)
{
    switch (Theme)
    {
    case ERA4FactionTheme::EurasianPact:      return TEXT("ГЛАВА 4: БЕЛЫЙ ШУМ");
    case ERA4FactionTheme::AtlanticAlliance:  return TEXT("ГЛАВА 3: ЛИНИЯ ПРИЛИВОВ");
    case ERA4FactionTheme::EasternCoalition:  return TEXT("ГЛАВА 5: РОЙ НАД ДЕЛЬТОЙ");
    case ERA4FactionTheme::PacificPact:       return TEXT("ГЛАВА 3: ПЕРВАЯ ВОЛНА");
    case ERA4FactionTheme::Independent:       return TEXT("ОПЕРАЦИЯ «САТУРАЦИОННЫЙ УДАР»");
    default:                                  return TEXT("");
    }
}
} // namespace

TSharedRef<SWidget> URA4CampaignSelectWidget::RebuildWidget()
{
    if (WidgetTree)
    {
        BuildLayout();
    }
    return Super::RebuildWidget();
}

void URA4CampaignSelectWidget::RouteToScreen(const ERA4UIScreenId ScreenId)
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (URA4UIRouterSubsystem* Router = GI->GetSubsystem<URA4UIRouterSubsystem>())
        {
            Router->NavigateTo(ScreenId);
        }
    }
}

void URA4CampaignSelectWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!CampaignViewModel)
    {
        CampaignViewModel = NewObject<URA4CampaignViewModel>(this);
    }
    EntranceElapsed = 0.0f;
    if (MainCanvas)
    {
        MainCanvas->SetRenderOpacity(0.0f);
        MainCanvas->SetRenderScale(FVector2D(1.02f, 1.02f));
    }
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            EntranceTimer, this, &URA4CampaignSelectWidget::AnimateEntrance, 1.0f / 60.0f, true);
    }

    const TArray<FRA4BlocInfo>& StartBlocs = FRA4FactionDataRegistry::Get().GetAllBlocs();
    SelectedBlocIndex = StartBlocs.IsValidIndex(InitialBlocIndex) ? InitialBlocIndex : 0;
    SelectedCountryIndex = StartBlocs.IsValidIndex(SelectedBlocIndex)
        && StartBlocs[SelectedBlocIndex].Countries.IsValidIndex(InitialCountryIndex)
        ? InitialCountryIndex : 0;
    SelectedDoctrineIndex = 0;
    CurrentStep = ERA4CampaignSelectStep::BlocSelection;

    if (DossierFrameWidget)
    {
        DossierFrameWidget->SetVisibility(ESlateVisibility::Collapsed);
    }

    RefreshBreadcrumbs();
    RefreshBlocCards();
    RefreshCountryCards();
    RefreshDoctrineCards();
    RefreshDossierPanel();

    switch (InitialStep)
    {
    case ERA4CampaignSelectStep::CountrySelection: GotoCountryStep(); break;
    case ERA4CampaignSelectStep::DoctrineSelection: GotoDoctrineStep(); break;
    default: break;
    }
}

void URA4CampaignSelectWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(EntranceTimer);
    }
    Super::NativeDestruct();
}

void URA4CampaignSelectWidget::AnimateEntrance()
{
    EntranceElapsed += 1.0f / 60.0f;
    const float Alpha = FMath::Clamp(EntranceElapsed / 0.35f, 0.0f, 1.0f);
    const float Eased = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);
    if (MainCanvas)
    {
        MainCanvas->SetRenderOpacity(Eased);
        MainCanvas->SetRenderScale(FVector2D(FMath::Lerp(1.02f, 1.0f, Eased)));
    }
    if (Alpha >= 1.0f)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(EntranceTimer);
        }
    }
}

void URA4CampaignSelectWidget::BuildLayout()
{
    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CampaignRoot"));
    WidgetTree->RootWidget = Root;

    UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CampaignBackground"));
    // Reference 03_campaign_eurasian_russia.png: the remaster plate carries its
    // own painted selection chrome, so it is dimmed hard and covered by a real
    // filled scrim — the default border brush draws nothing, which used to let
    // the painted cards double the live ones.
    if (UTexture2D* BackgroundTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/Remaster/T_SH_03_CampaignEurasian.T_SH_03_CampaignEurasian")))
    {
        Background->SetBrushFromTexture(BackgroundTexture, false);
        // Light tint keeps the painted campaign plate readable while the live
        // selection chrome owns the readable pixels above it.
        Background->SetColorAndOpacity(FLinearColor(0.86f, 0.88f, 0.92f, 1.0f));
    }
    else
    {
        Background->SetColorAndOpacity(FLinearColor(0.20f, 0.22f, 0.25f, 1.0f));
    }
    UOverlaySlot* BackgroundSlot = Root->AddChildToOverlay(Background);
    BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
    BackgroundSlot->SetVerticalAlignment(VAlign_Fill);

    UBorder* Shade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CampaignShade"));
    Shade->SetBrush(FSlateColorBrush(FLinearColor(0.004f, 0.006f, 0.012f, 0.42f)));
    UOverlaySlot* ShadeSlot = Root->AddChildToOverlay(Shade);
    ShadeSlot->SetHorizontalAlignment(HAlign_Fill);
    ShadeSlot->SetVerticalAlignment(VAlign_Fill);

    UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("CampaignScale"));
    Scale->SetStretch(EStretch::ScaleToFit);
    Scale->SetStretchDirection(EStretchDirection::Both);
    UOverlaySlot* ScaleSlot = Root->AddChildToOverlay(Scale);
    ScaleSlot->SetHorizontalAlignment(HAlign_Fill);
    ScaleSlot->SetVerticalAlignment(VAlign_Fill);

    USizeBox* Frame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CampaignReferenceFrame"));
    Frame->SetWidthOverride(ReferenceSize.X);
    Frame->SetHeightOverride(ReferenceSize.Y);
    Scale->SetContent(Frame);

    MainCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CampaignCanvas"));
    Frame->SetContent(MainCanvas);

    // Global Scarlet Horizon thin line
    UBorder* HorizonLine = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HorizonLine"));
    HorizonLine->SetBrushColor(ScarletHorizon);
    Place(MainCanvas, HorizonLine, FVector2D(0.0f, 0.0f), FVector2D(1920.0f, 3.0f), 10);

    // Top Brand: procedural wordmark — the retired logo texture carries a
    // double exposure and is no longer used anywhere new.
    UTextBlock* Wordmark = MakeText(
        WidgetTree, LOCTEXT("CampaignWordmark", "SCARLET HORIZON"), 27,
        FLinearColor(0.88f, 0.91f, 0.95f, 1.0f), TEXT("CampaignWordmark"), true, false);
    Place(MainCanvas, Wordmark, FVector2D(35.0f, 16.0f), FVector2D(360.0f, 36.0f), 4);
    UBorder* WordmarkRule = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("CampaignWordmarkRule"));
    WordmarkRule->SetBrushColor(ScarletHorizon);
    Place(MainCanvas, WordmarkRule, FVector2D(37.0f, 58.0f), FVector2D(230.0f, 2.0f), 5);

    // Top Navigation Bar
    UHorizontalBox* Navigation = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CampaignNavigation"));
    const auto AddNav = [this, Navigation](
        const FText& Label,
        const FName Name,
        const bool bActive) -> UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
        FButtonStyle Style;
        Style.SetNormal(FSlateColorBrush(bActive
            ? FLinearColor(0.20f, 0.08f, 0.32f, 0.96f)
            : FLinearColor(0.015f, 0.018f, 0.024f, 0.85f)));
        Style.SetHovered(FSlateColorBrush(FLinearColor(0.35f, 0.15f, 0.50f, 1.0f)));
        Style.SetPressed(FSlateColorBrush(ScarletHorizon));
        Style.SetDisabled(FSlateColorBrush(FLinearColor(0.18f, 0.07f, 0.28f, 0.95f)));
        Button->SetStyle(Style);
        UTextBlock* LabelText = MakeText(
            WidgetTree, Label, 14, bActive ? TextHighlight : TextMuted,
            FName(Name.ToString() + TEXT("_Label")), true, false);
        LabelText->SetJustification(ETextJustify::Center);
        LabelText->SetMinDesiredWidth(0.0f);
        Button->AddChild(LabelText);
        Button->SetClipping(EWidgetClipping::ClipToBounds);
        UHorizontalBoxSlot* Slot = Navigation->AddChildToHorizontalBox(Button);
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        Slot->SetPadding(FMargin(9.0f, 6.0f));
        Slot->SetHorizontalAlignment(HAlign_Center);
        return Button;
    };

    AddNav(LOCTEXT("MainNav", "ГЛАВНАЯ"), TEXT("MainNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenMainMenu);
    UButton* CampaignNav = AddNav(
        LOCTEXT("CampaignNav", "КАМПАНИЯ"), TEXT("CampaignNavButton"), true);
    CampaignNav->SetIsEnabled(false);
    AddNav(LOCTEXT("NetworkNav", "СЕТЕВАЯ ИГРА"), TEXT("NetworkNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenMultiplayer);
    AddNav(LOCTEXT("ChallengesNav", "ИСПЫТАНИЯ"), TEXT("ChallengesNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenChallenges);
    AddNav(LOCTEXT("BarracksNav", "АРХИВ"), TEXT("BarracksNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenBarracks);
    AddNav(LOCTEXT("SettingsNav", "ПАРАМЕТРЫ"), TEXT("SettingsNavButton"), false)
        ->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenSettings);
    UWidget* NavigationFrame = MakePanel(
        WidgetTree, Navigation, TEXT("CampaignNavigationFrame"),
        FLinearColor(0.68f, 0.28f, 0.88f, 1.0f), FMargin(3.0f));
    NavigationFrame->SetVisibility(ESlateVisibility::Collapsed);
    Place(MainCanvas, NavigationFrame, FVector2D(420.0f, 84.0f), FVector2D(1040.0f, 54.0f), 5);

    // Profile & Network status
    UVerticalBox* Profile = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CampaignProfile"));
    Profile->AddChildToVerticalBox(MakeText(
        WidgetTree, LOCTEXT("ProfileHeader", "ШТАБ КОМАНДОВАНИЯ"), 13, ScarletHorizon, TEXT("ProfileHeader")));
    Profile->AddChildToVerticalBox(MakeText(
        WidgetTree, LOCTEXT("ProfileStatus", "СВЯЗЬ: АКТИВНА // ДОСТУП УР. 5"), 11, TextMuted, TEXT("ProfileStatus"), false));
    Place(
        MainCanvas,
        MakePanel(WidgetTree, Profile, TEXT("CampaignProfileFrame"), ScarletHorizon, FMargin(12.0f, 6.0f)),
        FVector2D(1480.0f, 15.0f), FVector2D(405.0f, 62.0f), 5);

    // ==========================================
    // BREADCRUMBS BAR (Step 1 -> Step 2 -> Step 3)
    // ==========================================
    UHorizontalBox* BreadcrumbsBox = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("BreadcrumbsBox"));

    BreadcrumbBlocBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BreadcrumbBlocBtn"));
    BreadcrumbBlocBtn->SetStyle(MakeCardButtonStyle(FLinearColor(0.68f, 0.28f, 0.88f, 1.0f)));
    BreadcrumbBlocText = MakeText(WidgetTree, LOCTEXT("BC1", "01 БЛОК / КАТЕГОРИЯ"), 14, TextHighlight, TEXT("BCText1"), true, false);
    BreadcrumbBlocBtn->AddChild(BreadcrumbBlocText);
    BreadcrumbBlocBtn->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::GotoBlocStep);
    BreadcrumbsBox->AddChildToHorizontalBox(BreadcrumbBlocBtn)->SetPadding(FMargin(4.0f, 2.0f));

    UTextBlock* Arrow1 = MakeText(WidgetTree, LOCTEXT("Arrow1", "›"), 16, TextMuted, TEXT("Arrow1"), true, false);
    BreadcrumbsBox->AddChildToHorizontalBox(Arrow1)->SetPadding(FMargin(6.0f, 4.0f));

    BreadcrumbCountryBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BreadcrumbCountryBtn"));
    BreadcrumbCountryBtn->SetStyle(MakeCardButtonStyle(FLinearColor(0.35f, 0.70f, 0.98f, 1.0f)));
    BreadcrumbCountryText = MakeText(WidgetTree, LOCTEXT("BC2", "02 СТРАНА"), 14, TextMuted, TEXT("BCText2"), true, false);
    BreadcrumbCountryBtn->AddChild(BreadcrumbCountryText);
    BreadcrumbCountryBtn->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::GotoCountryStep);
    BreadcrumbsBox->AddChildToHorizontalBox(BreadcrumbCountryBtn)->SetPadding(FMargin(4.0f, 2.0f));

    UTextBlock* Arrow2 = MakeText(WidgetTree, LOCTEXT("Arrow2", "›"), 16, TextMuted, TEXT("Arrow2"), true, false);
    BreadcrumbsBox->AddChildToHorizontalBox(Arrow2)->SetPadding(FMargin(6.0f, 4.0f));

    BreadcrumbDoctrineBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BreadcrumbDoctrineBtn"));
    BreadcrumbDoctrineBtn->SetStyle(MakeCardButtonStyle(FLinearColor(0.88f, 0.72f, 0.22f, 1.0f)));
    BreadcrumbDoctrineText = MakeText(WidgetTree, LOCTEXT("BC3", "03 ДОКТРИНА"), 14, TextMuted, TEXT("BCText3"), true, false);
    BreadcrumbDoctrineBtn->AddChild(BreadcrumbDoctrineText);
    BreadcrumbDoctrineBtn->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::GotoDoctrineStep);
    BreadcrumbsBox->AddChildToHorizontalBox(BreadcrumbDoctrineBtn)->SetPadding(FMargin(4.0f, 2.0f));

    Place(
        MainCanvas,
        MakePanel(WidgetTree, BreadcrumbsBox, TEXT("BreadcrumbsFrame"), FLinearColor(0.40f, 0.45f, 0.55f, 1.0f), FMargin(6.0f, 4.0f)),
        FVector2D(535.0f, 16.0f), FVector2D(760.0f, 60.0f), 7);

    // ==========================================
    // STEP 1 CONTAINER: 5 BLOC CARDS
    // ==========================================
    BlocCardsContainer = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("BlocCardsContainer"));
    Place(MainCanvas, BlocCardsContainer, FVector2D(0.0f, 0.0f), ReferenceSize, 5);

    // ==========================================
    // STEP 2 CONTAINER: COUNTRY CARDS
    // ==========================================
    CountryCardsContainer = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("CountryCardsContainer"));
    CountryCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    Place(MainCanvas, CountryCardsContainer, FVector2D(0.0f, 0.0f), ReferenceSize, 5);

    // ==========================================
    // STEP 3 CONTAINER: DOCTRINE CARDS
    // ==========================================
    DoctrineCardsContainer = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("DoctrineCardsContainer"));
    DoctrineCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    Place(MainCanvas, DoctrineCardsContainer, FVector2D(0.0f, 0.0f), ReferenceSize, 5);

    // ==========================================
    // RIGHT COLUMN: HERO DOSSIER & INTEL PANEL
    // ==========================================
    UVerticalBox* DossierBox = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("DossierBox"));

    DossierHeaderTag = MakeText(
        WidgetTree, LOCTEXT("DossierTag", "ОПЕРАТИВНОЕ ДОСЬЕ"), 13, ScarletHorizon, TEXT("DossierHeaderTag"));
    DossierBox->AddChildToVerticalBox(DossierHeaderTag)->SetPadding(FMargin(12.0f, 8.0f, 12.0f, 4.0f));

    DossierTitleText = MakeText(
        WidgetTree, FText::GetEmpty(), 24, TextHighlight, TEXT("DossierTitleText"));
    DossierBox->AddChildToVerticalBox(DossierTitleText)->SetPadding(FMargin(12.0f, 2.0f, 12.0f, 2.0f));

    DossierSubtitleText = MakeText(
        WidgetTree, FText::GetEmpty(), 12, TextMuted, TEXT("DossierSubtitleText"), false);
    DossierBox->AddChildToVerticalBox(DossierSubtitleText)->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 10.0f));

    DossierSpecializationText = MakeText(
        WidgetTree, FText::GetEmpty(), 13, FLinearColor(0.88f, 0.72f, 0.22f, 1.0f), TEXT("DossierSpecText"));
    DossierBox->AddChildToVerticalBox(DossierSpecializationText)->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 8.0f));

    DossierDescriptionText = MakeText(
        WidgetTree, FText::GetEmpty(), 13, TextPrimary, TEXT("DossierDescText"), false);
    DossierDescriptionText->SetAutoWrapText(true);
    UVerticalBoxSlot* DescSlot = DossierBox->AddChildToVerticalBox(DossierDescriptionText);
    DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    DescSlot->SetPadding(FMargin(12.0f, 4.0f, 12.0f, 12.0f));

    // Ratings Section
    UTextBlock* RatingsHeader = MakeText(
        WidgetTree, LOCTEXT("RatingsHeader", "БОЕВЫЕ ХАРАКТЕРИСТИКИ"), 13, TextMuted, TEXT("RatingsHeader"));
    DossierBox->AddChildToVerticalBox(RatingsHeader)->SetPadding(FMargin(12.0f, 6.0f, 12.0f, 4.0f));

    const auto AddRatingBar = [this, DossierBox](
        const FText& Label,
        TObjectPtr<UProgressBar>& TargetBar,
        const FLinearColor& BarColor,
        const FName BarName)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        UTextBlock* RowLabel = MakeText(WidgetTree, Label, 12, TextPrimary, FName(BarName.ToString() + TEXT("_Lbl")), false);
        RowLabel->SetMinDesiredWidth(110.0f);
        Row->AddChildToHorizontalBox(RowLabel)->SetPadding(FMargin(0.0f, 2.0f));

        TargetBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), BarName);
        TargetBar->SetFillColorAndOpacity(BarColor);
        TargetBar->SetPercent(0.85f);
        UHorizontalBoxSlot* BarSlot = Row->AddChildToHorizontalBox(TargetBar);
        BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        BarSlot->SetPadding(FMargin(8.0f, 4.0f, 0.0f, 4.0f));
        BarSlot->SetVerticalAlignment(VAlign_Center);

        DossierBox->AddChildToVerticalBox(Row)->SetPadding(FMargin(12.0f, 2.0f));
    };

    AddRatingBar(LOCTEXT("R_Firepower", "ОГНЕВАЯ МОЩЬ"), FirepowerBar, FLinearColor(0.92f, 0.35f, 0.25f, 1.0f), TEXT("BarFP"));
    AddRatingBar(LOCTEXT("R_Armor", "БРОНЯ"), ArmorBar, FLinearColor(0.35f, 0.70f, 0.98f, 1.0f), TEXT("BarArm"));
    AddRatingBar(LOCTEXT("R_Mobility", "МОБИЛЬНОСТЬ"), MobilityBar, FLinearColor(0.25f, 0.85f, 0.55f, 1.0f), TEXT("BarMob"));
    AddRatingBar(LOCTEXT("R_Tech", "ТЕХНОЛОГИИ"), TechBar, FLinearColor(0.75f, 0.45f, 0.95f, 1.0f), TEXT("BarTech"));

    // Continue / Action Button
    ContinueButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ContinueButton"));
    FButtonStyle ActionStyle;
    ActionStyle.SetNormal(FSlateColorBrush(FLinearColor(0.35f, 0.08f, 0.12f, 0.98f)));
    ActionStyle.SetHovered(FSlateColorBrush(ScarletHorizon));
    ActionStyle.SetPressed(FSlateColorBrush(FLinearColor(1.0f, 0.30f, 0.35f, 1.0f)));
    ContinueButton->SetStyle(ActionStyle);

    ContinueLabelText = MakeText(
        WidgetTree, LOCTEXT("CTA_ChooseCountry", "ВЫБРАТЬ СТРАНУ ›"), 16, TextHighlight,
        TEXT("ContinueLabel"), true, false);
    ContinueLabelText->SetJustification(ETextJustify::Center);
    ContinueButton->AddChild(ContinueLabelText);
    ContinueButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::ContinueCampaign);

    UVerticalBoxSlot* ActionSlot = DossierBox->AddChildToVerticalBox(ContinueButton);
    ActionSlot->SetPadding(FMargin(12.0f, 16.0f, 12.0f, 12.0f));

    DossierFrameWidget = MakePanel(
        WidgetTree, DossierBox, TEXT("DossierFrame"),
        FLinearColor(0.68f, 0.28f, 0.88f, 1.0f), FMargin(6.0f));
    Place(MainCanvas, DossierFrameWidget, FVector2D(1380.0f, 95.0f), FVector2D(505.0f, 845.0f), 6);

    // ==========================================
    // BOTTOM STATUS / FOOTER
    // ==========================================
    UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CampaignFooter"));
    UButton* BackButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("CampaignBackButton"));
    BackButton->AddChild(MakeText(
        WidgetTree, LOCTEXT("BackButton", "‹  НАЗАД"), 14, TextPrimary,
        TEXT("CampaignBackLabel"), true, false));
    BackButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::GotoBlocStep);
    Footer->AddChildToHorizontalBox(BackButton)->SetPadding(FMargin(6.0f, 2.0f));

    UButton* TrainingButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("TrainingButton"));
    TrainingButton->AddChild(MakeText(
        WidgetTree, LOCTEXT("Training", "ОБУЧЕНИЕ"), 14, TextPrimary,
        TEXT("TrainingLabel"), true, false));
    TrainingButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OpenChallenges);
    Footer->AddChildToHorizontalBox(TrainingButton)->SetPadding(FMargin(6.0f, 2.0f));

    UTextBlock* EraText = MakeText(
        WidgetTree, LOCTEXT("Era", "SCARLET HORIZON  //  ГЛОБАЛЬНАЯ СЕТЬ ОПЕРАЦИЙ  //  v1.0-RC1"),
        12, TextMuted, TEXT("CampaignEra"), false);
    EraText->SetJustification(ETextJustify::Center);
    UHorizontalBoxSlot* EraSlot = Footer->AddChildToHorizontalBox(EraText);
    EraSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    EraSlot->SetVerticalAlignment(VAlign_Center);

    Place(
        MainCanvas,
        MakePanel(WidgetTree, Footer, TEXT("CampaignFooterFrame"), FLinearColor(0.30f, 0.35f, 0.40f, 1.0f), FMargin(4.0f)),
        FVector2D(35.0f, 1005.0f), FVector2D(1560.0f, 52.0f), 5);
}

void URA4CampaignSelectWidget::RefreshBreadcrumbs()
{
    const FRA4FactionDataRegistry& Registry = FRA4FactionDataRegistry::Get();
    const TArray<FRA4BlocInfo>& Blocs = Registry.GetAllBlocs();

    FText BlocName = LOCTEXT("BC_BlocSelect", "01 БЛОК / КАТЕГОРИЯ");
    FText CountryName = LOCTEXT("BC_CountrySelect", "02 СТРАНА");
    FText DoctrineName = LOCTEXT("BC_DoctrineSelect", "03 ДОКТРИНА");

    if (Blocs.IsValidIndex(SelectedBlocIndex))
    {
        const FRA4BlocInfo& ActiveBloc = Blocs[SelectedBlocIndex];
        BlocName = FText::Format(LOCTEXT("BC_BlocFmt", "01 {0}"), ActiveBloc.DisplayName);

        if (ActiveBloc.Countries.IsValidIndex(SelectedCountryIndex))
        {
            const FRA4CountryInfo& ActiveCountry = ActiveBloc.Countries[SelectedCountryIndex];
            CountryName = FText::Format(LOCTEXT("BC_CountryFmt", "02 {0}"), ActiveCountry.DisplayName);

            if (ActiveCountry.Doctrines.IsValidIndex(SelectedDoctrineIndex))
            {
                const FRA4DoctrineInfo& ActiveDoc = ActiveCountry.Doctrines[SelectedDoctrineIndex];
                DoctrineName = FText::Format(LOCTEXT("BC_DocFmt", "03 {0}"), ActiveDoc.DisplayName);
            }
        }
    }

    if (BreadcrumbBlocText)
    {
        BreadcrumbBlocText->SetText(BlocName);
        BreadcrumbBlocText->SetColorAndOpacity(FSlateColor(
            CurrentStep == ERA4CampaignSelectStep::BlocSelection ? TextHighlight : TextMuted));
    }
    if (BreadcrumbCountryText)
    {
        BreadcrumbCountryText->SetText(CountryName);
        BreadcrumbCountryText->SetColorAndOpacity(FSlateColor(
            CurrentStep == ERA4CampaignSelectStep::CountrySelection ? TextHighlight : TextMuted));
    }
    if (BreadcrumbDoctrineText)
    {
        BreadcrumbDoctrineText->SetText(DoctrineName);
        BreadcrumbDoctrineText->SetColorAndOpacity(FSlateColor(
            CurrentStep == ERA4CampaignSelectStep::DoctrineSelection ? TextHighlight : TextMuted));
    }
}

void URA4CampaignSelectWidget::RefreshBlocCards()
{
    if (!BlocCardsContainer)
    {
        return;
    }
    BlocCardsContainer->ClearChildren();
    BlocCardFrames.Reset();

    const FRA4FactionDataRegistry& Registry = FRA4FactionDataRegistry::Get();
    const TArray<FRA4BlocInfo>& Blocs = Registry.GetAllBlocs();

    // Remaster reference 03 ships a 5+1 grid: five equal faction cards on the
    // left and a right dossier panel. Geometry is authored at ReferenceSize
    // (1920x1080): left padding 36, right panel 340 wide, 14px gaps, 5 cards
    // share the remaining width.
    constexpr float GridTop = 96.0f;
    constexpr float GridHeight = 760.0f;
    constexpr float LeftPad = 36.0f;
    constexpr float RightPad = 36.0f;
    constexpr float DetailWidth = 340.0f;
    constexpr float Gap = 14.0f;
    const float CardsWidth = ReferenceSize.X - LeftPad - RightPad - DetailWidth - Gap;
    const float CardWidth = (CardsWidth - Gap * 4.0f) / 5.0f;

    // Right dossier panel: campaign title, doctrine, description, stats, and
    // the "ВОЙТИ В КАМПАНИЮ" action — matching the remaster reference's right
    // column for the selected faction.
    if (Blocs.IsValidIndex(SelectedBlocIndex))
    {
        const FRA4BlocInfo& Active = Blocs[SelectedBlocIndex];
        const ERA4FactionTheme Theme = Active.BlocId;

        UVerticalBox* Detail = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), TEXT("BlocDetailBox"));
        Detail->AddChildToVerticalBox(MakeText(
            WidgetTree, LOCTEXT("AboutCampaign", "О ВЫБРАННОЙ КАМПАНИИ"), 12, Active.GlowColor,
            TEXT("BlocDetailTag"), true, false))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
        Detail->AddChildToVerticalBox(MakeText(
            WidgetTree, FText::FromString(BlocCampaignTitle(Theme)), 24, TextHighlight,
            TEXT("BlocDetailTitle"), true, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
        Detail->AddChildToVerticalBox(MakeText(
            WidgetTree, FText::FromString(BlocDoctrine(Theme)), 11, Active.GlowColor,
            TEXT("BlocDetailDoctrine"), true, false))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
        Detail->AddChildToVerticalBox(MakeText(
            WidgetTree, FText::FromString(BlocCampaignDesc(Theme)), 12,
            FLinearColor(0.73f, 0.75f, 0.78f, 1.0f),
            TEXT("BlocDetailDesc"), false, true))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));

        // Stats separator + rows.
        UVerticalBox* Stats = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), TEXT("BlocDetailStats"));
        const auto AddStatRow = [this, Stats](
            const FText& Label, const FText& Value, const FLinearColor& ValueColor, const FName Name)
        {
            UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
                UHorizontalBox::StaticClass(), Name);
            UTextBlock* Lbl = MakeText(WidgetTree, Label, 13,
                FLinearColor(0.73f, 0.75f, 0.78f, 1.0f), FName(Name.ToString() + TEXT("_L")), false, false);
            UHorizontalBoxSlot* LSlot = Row->AddChildToHorizontalBox(Lbl);
            LSlot->SetSize(FillWeight(1.0f));
            UTextBlock* Val = MakeText(WidgetTree, Value, 13, ValueColor,
                FName(Name.ToString() + TEXT("_V")), true, false);
            Val->SetJustification(ETextJustify::Right);
            Row->AddChildToHorizontalBox(Val);
            Stats->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 7.0f));
        };
        AddStatRow(LOCTEXT("MissionsDone", "МИССИЙ ЗАВЕРШЕНО:"),
            FText::FromString(BlocMissionsCompleted(Theme)), TextHighlight, TEXT("StatMissions"));
        AddStatRow(LOCTEXT("Difficulty", "СЛОЖНОСТЬ:"),
            FText::FromString(BlocDifficulty(Theme)), Active.GlowColor, TEXT("StatDifficulty"));
        AddStatRow(LOCTEXT("CurrentChapter", "ТЕКУЩАЯ ГЛАВА:"),
            FText::FromString(BlocCurrentChapter(Theme)), TextHighlight, TEXT("StatChapter"));
        Detail->AddChildToVerticalBox(Stats)->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 0.0f));

        // "ВОЙТИ В КАМПАНИЮ" action button at the bottom of the dossier.
        UButton* EnterButton = WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(), TEXT("BlocEnterButton"));
        EnterButton->SetStyle(MakeCardButtonStyle(Active.GlowColor));
        UTextBlock* EnterLabel = MakeText(
            WidgetTree, LOCTEXT("EnterCampaign", "ВОЙТИ В КАМПАНИЮ  ≫"), 17,
            FLinearColor(0.04f, 0.03f, 0.07f, 1.0f), TEXT("BlocEnterLabel"), true, false);
        EnterLabel->SetJustification(ETextJustify::Center);
        EnterButton->AddChild(EnterLabel);
        EnterButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::ContinueCampaign);
        Detail->AddChildToVerticalBox(EnterButton)->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));

        const FVector2D DetailPos(ReferenceSize.X - RightPad - DetailWidth, GridTop);
        Place(BlocCardsContainer,
            MakePanel(WidgetTree, Detail, TEXT("BlocDetailFrame"), Active.GlowColor,
                DensityPadding(ERA4PanelRole::Hero), Active.FrameRail),
            DetailPos, FVector2D(DetailWidth, GridHeight), 3);
    }

    // Five equal faction cards across the left region.
    for (int32 Index = 0; Index < Blocs.Num(); ++Index)
    {
        const FRA4BlocInfo& Bloc = Blocs[Index];
        const ERA4FactionTheme Theme = Bloc.BlocId;
        const bool bSelected = (Index == SelectedBlocIndex);
        const FVector2D CardPos(LeftPad + Index * (CardWidth + Gap), GridTop);
        const FVector2D CardSize(CardWidth, GridHeight);

        UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), FName(*FString::Printf(TEXT("BlocContent_%d"), Index)));

        // Crest glyph in a clipped hexagon-like plate, centred at the top.
        UBorder* CrestPlate = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), FName(*FString::Printf(TEXT("BlocCrest_%d"), Index)));
        CrestPlate->SetBrushColor(FLinearColor(
            Bloc.GlowColor.R * 0.20f, Bloc.GlowColor.G * 0.20f, Bloc.GlowColor.B * 0.20f, 0.96f));
        CrestPlate->SetPadding(FMargin(0.0f));
        CrestPlate->SetHorizontalAlignment(HAlign_Center);
        CrestPlate->SetVerticalAlignment(VAlign_Center);
        UTextBlock* Crest = MakeText(
            WidgetTree, FText::FromString(BlocCrestGlyph(Theme)), 44, Bloc.GlowColor,
            FName(*FString::Printf(TEXT("BlocCrestGlyph_%d"), Index)), true, false);
        Crest->SetJustification(ETextJustify::Center);
        CrestPlate->SetContent(Crest);
        UVerticalBoxSlot* CrestSlot = CardContent->AddChildToVerticalBox(CrestPlate);
        CrestSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        CrestSlot->SetPadding(FMargin(0.0f, 26.0f, 0.0f, 14.0f));

        // Two-line faction name.
        FString Line1, Line2;
        BlocNameLines(Theme, Line1, Line2);
        CardContent->AddChildToVerticalBox(MakeText(
            WidgetTree, FText::FromString(Line1), 19,
            bSelected ? TextHighlight : FLinearColor(0.76f, 0.79f, 0.83f, 1.0f),
            FName(*FString::Printf(TEXT("BlocName1_%d"), Index)), true, false))
            ->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
        CardContent->AddChildToVerticalBox(MakeText(
            WidgetTree, FText::FromString(Line2), 19,
            bSelected ? TextHighlight : FLinearColor(0.76f, 0.79f, 0.83f, 1.0f),
            FName(*FString::Printf(TEXT("BlocName2_%d"), Index)), true, false));

        // Country label.
        CardContent->AddChildToVerticalBox(MakeText(
            WidgetTree, FText::FromString(BlocCountryShort(Theme)), 11, Bloc.GlowColor,
            FName(*FString::Printf(TEXT("BlocCountry_%d"), Index)), true, false))
            ->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));

        // Progress footer (pinned to the bottom of the card via a fill spacer).
        UVerticalBoxSlot* SpacerSlot = CardContent->AddChildToVerticalBox(
            WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),
                FName(*FString::Printf(TEXT("BlocSpacer_%d"), Index))));
        SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

        UVerticalBox* Progress = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), FName(*FString::Printf(TEXT("BlocProgress_%d"), Index)));
        UHorizontalBox* ProgressHeader = WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(), FName(*FString::Printf(TEXT("BlocProgressH_%d"), Index)));
        UTextBlock* ProgressLabel = MakeText(
            WidgetTree, LOCTEXT("Progress", "ПРОГРЕСС"), 11,
            FLinearColor(0.60f, 0.63f, 0.68f, 1.0f),
            FName(*FString::Printf(TEXT("BlocProgressLbl_%d"), Index)), false, false);
        UHorizontalBoxSlot* PLSlot = ProgressHeader->AddChildToHorizontalBox(ProgressLabel);
        PLSlot->SetSize(FillWeight(1.0f));
        UTextBlock* ProgressValue = MakeText(
            WidgetTree, FText::Format(LOCTEXT("ProgressPct", "{0}%"),
                FText::AsNumber(BlocCampaignProgress(Theme))), 11, TextHighlight,
            FName(*FString::Printf(TEXT("BlocProgressVal_%d"), Index)), true, false);
        ProgressValue->SetJustification(ETextJustify::Right);
        ProgressHeader->AddChildToHorizontalBox(ProgressValue);
        Progress->AddChildToVerticalBox(ProgressHeader)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
        UProgressBar* ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
            UProgressBar::StaticClass(), FName(*FString::Printf(TEXT("BlocProgressBar_%d"), Index)));
        ProgressBar->SetFillColorAndOpacity(Bloc.GlowColor);
        ProgressBar->SetPercent(BlocCampaignProgress(Theme) / 100.0f);
        Progress->AddChildToVerticalBox(ProgressBar);
        CardContent->AddChildToVerticalBox(Progress)->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));

        // Card is a clickable button carrying the content; selection is handled
        // via the existing OnBloc0..4 routes.
        UButton* CardButton = WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(), FName(*FString::Printf(TEXT("BlocBtn_%d"), Index)));
        CardButton->SetStyle(MakeCardButtonStyle(Bloc.PrimaryColor));
        CardButton->AddChild(CardContent);

        switch (Index)
        {
        case 0: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnBloc0); break;
        case 1: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnBloc1); break;
        case 2: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnBloc2); break;
        case 3: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnBloc3); break;
        case 4: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnBloc4); break;
        default: break;
        }

        UBorder* CardFrame = MakePanel(
            WidgetTree, CardButton, FName(*FString::Printf(TEXT("BlocFrame_%d"), Index)),
            bSelected ? Bloc.GlowColor : Bloc.PrimaryColor,
            FMargin(16.0f), Bloc.FrameRail);
        BlocCardFrames.Add(CardFrame);

        Place(BlocCardsContainer, CardFrame, CardPos, CardSize, bSelected ? 3 : 2);
    }
}

void URA4CampaignSelectWidget::RefreshCountryCards()
{
    if (!CountryCardsContainer)
    {
        return;
    }
    CountryCardsContainer->ClearChildren();
    CountryCardFrames.Reset();

    const FRA4FactionDataRegistry& Registry = FRA4FactionDataRegistry::Get();
    const TArray<FRA4BlocInfo>& Blocs = Registry.GetAllBlocs();
    if (!Blocs.IsValidIndex(SelectedBlocIndex))
    {
        return;
    }
    const FRA4BlocInfo& ActiveBloc = Blocs[SelectedBlocIndex];

    // Geometry read off 06_eurasian_country_russia_vivid.png on the 1672x941 frame.
    const float ToAuthoring = ReferenceSize.X / 1672.0f;
    const auto Ref = [ToAuthoring](const float X, const float Y, const float W, const float H)
    {
        return FBox2D(
            FVector2D(X * ToAuthoring, Y * ToAuthoring),
            FVector2D((X + W) * ToAuthoring, (Y + H) * ToAuthoring));
    };

    const FBox2D HeroRect = Ref(20.0f, 115.0f, 810.0f, 575.0f);
    const FBox2D PartnerRects[4] = {
        Ref(945.0f, 195.0f, 425.0f, 125.0f),
        Ref(900.0f, 340.0f, 570.0f, 140.0f),
        Ref(900.0f, 500.0f, 315.0f, 110.0f),
        Ref(1230.0f, 500.0f, 315.0f, 110.0f)
    };

    int32 PartnerIndex = 0;
    for (int32 Index = 0; Index < ActiveBloc.Countries.Num(); ++Index)
    {
        const FRA4CountryInfo& Country = ActiveBloc.Countries[Index];
        const bool bHero = (Index == SelectedCountryIndex);
        if (!bHero && PartnerIndex >= UE_ARRAY_COUNT(PartnerRects))
        {
            // The mosaic holds four partner plates; the rest stay reachable from
            // the dossier rather than being drawn on top of each other.
            continue;
        }
        const FBox2D Rect = bHero ? HeroRect : PartnerRects[PartnerIndex++];

        UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), FName(*FString::Printf(TEXT("CountryContent_%d"), Index)));

        UTextBlock* CodeText = MakeText(
            WidgetTree, FText::FromName(Country.CountryId), bHero ? 14 : 11, Country.AccentColor,
            FName(*FString::Printf(TEXT("CountryCode_%d"), Index)), true, false);
        CardContent->AddChildToVerticalBox(CodeText)->SetPadding(FMargin(14.0f, 12.0f, 14.0f, 2.0f));

        UTextBlock* Title = MakeText(
            WidgetTree, Country.DisplayName, bHero ? 44 : 18, TextHighlight,
            FName(*FString::Printf(TEXT("CountryTitle_%d"), Index)));
        CardContent->AddChildToVerticalBox(Title)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 4.0f));

        UTextBlock* Spec = MakeText(
            WidgetTree, Country.Specialization, bHero ? 14 : 11, Country.AccentColor,
            FName(*FString::Printf(TEXT("CountrySpec_%d"), Index)), false);
        CardContent->AddChildToVerticalBox(Spec)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 8.0f));

        UTextBlock* DocCount = MakeText(
            WidgetTree,
            DoctrineCountText(Country.Doctrines.Num()),
            bHero ? 15 : 10, FLinearColor(0.88f, 0.72f, 0.22f, 1.0f),
            FName(*FString::Printf(TEXT("CountryDocs_%d"), Index)), true, false);
        CardContent->AddChildToVerticalBox(DocCount)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 10.0f));

        // Only the hero plate has room for lore and the rating rows.
        if (bHero)
        {
            UTextBlock* Desc = MakeText(
                WidgetTree, Country.LoreDescription, 14, TextPrimary,
                FName(*FString::Printf(TEXT("CountryDesc_%d"), Index)), false);
            CardContent->AddChildToVerticalBox(Desc)->SetPadding(FMargin(14.0f, 4.0f, 14.0f, 14.0f));

            const auto AddRating = [this, CardContent, Index](
                const FText& Label, const float Value, const FLinearColor& Colour, const FName Name)
            {
                UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), Name);
                UTextBlock* Caption = MakeText(
                    WidgetTree, Label, 11, TextPrimary, FName(Name.ToString() + TEXT("_Lbl")), false, false);
                Row->AddChildToHorizontalBox(Caption)->SetSize(FillWeight(1.0f));
                UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(
                    UProgressBar::StaticClass(), FName(Name.ToString() + TEXT("_Bar")));
                Bar->SetPercent(Value);
                Bar->SetFillColorAndOpacity(Colour);
                UHorizontalBoxSlot* BarSlot = Row->AddChildToHorizontalBox(Bar);
                BarSlot->SetSize(FillWeight(2.2f));
                BarSlot->SetVerticalAlignment(VAlign_Center);
                BarSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
                CardContent->AddChildToVerticalBox(Row)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 6.0f));
            };
            AddRating(LOCTEXT("C_Fire", "БОЕВАЯ МОЩЬ"), Country.FirepowerRating,
                FLinearColor(0.95f, 0.55f, 0.30f, 1.0f), FName(*FString::Printf(TEXT("CFire_%d"), Index)));
            AddRating(LOCTEXT("C_Armor", "ЗАЩИТА"), Country.ArmorRating,
                FLinearColor(0.35f, 0.70f, 0.98f, 1.0f), FName(*FString::Printf(TEXT("CArm_%d"), Index)));
            AddRating(LOCTEXT("C_Mob", "МОБИЛЬНОСТЬ"), Country.MobilityRating,
                FLinearColor(0.25f, 0.85f, 0.55f, 1.0f), FName(*FString::Printf(TEXT("CMob_%d"), Index)));
            AddRating(LOCTEXT("C_Tech", "ТЕХНОЛОГИИ"), Country.TechRating,
                FLinearColor(0.75f, 0.45f, 0.95f, 1.0f), FName(*FString::Printf(TEXT("CTech_%d"), Index)));
        }

        if (!Country.bUnlocked || Country.bComingSoon)
        {
            UTextBlock* Locked = MakeText(
                WidgetTree, LOCTEXT("ComingSoon", "СКОРО В ДОПОЛНЕНИИ"), 10, TextMuted,
                FName(*FString::Printf(TEXT("CountryLocked_%d"), Index)), true, false);
            CardContent->AddChildToVerticalBox(Locked)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 10.0f));
        }

        UButton* CardButton = WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(), FName(*FString::Printf(TEXT("CountryBtn_%d"), Index)));
        CardButton->SetStyle(MakeCardButtonStyle(Country.AccentColor));
        CardButton->SetIsEnabled(Country.bUnlocked && !Country.bComingSoon);
        CardButton->AddChild(CardContent);

        switch (Index)
        {
        case 0: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnCountry0); break;
        case 1: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnCountry1); break;
        case 2: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnCountry2); break;
        case 3: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnCountry3); break;
        case 4: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnCountry4); break;
        case 5: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnCountry5); break;
        default: break;
        }

        // A category has no shared frame: each independent country keeps its own
        // accent rather than borrowing one it does not belong to.
        const FLinearColor FrameAccent = bHero || ActiveBloc.bIsCategoryOnly
            ? Country.AccentColor
            : ActiveBloc.PrimaryColor;

        UBorder* CardFrame = MakePanel(
            WidgetTree, CardButton, FName(*FString::Printf(TEXT("CountryFrame_%d"), Index)),
            FrameAccent, DensityPadding(ActiveBloc.PanelDensity), ActiveBloc.FrameRail);
        CountryCardFrames.Add(CardFrame);
        Place(CountryCardsContainer, CardFrame, Rect.Min, Rect.GetSize(), bHero ? 3 : 2);
    }

    BuildCountryStepChrome(ActiveBloc, Ref);
}

void URA4CampaignSelectWidget::BuildDoctrineStepChrome(
    const FRA4BlocInfo& ActiveBloc,
    const FRA4CountryInfo& Country,
    TFunctionRef<FBox2D(float, float, float, float)> Ref)
{
    const auto PlaceRect = [this](UWidget* Widget, const FBox2D& Rect, const int32 Z)
    {
        Place(DoctrineCardsContainer, Widget, Rect.Min, Rect.GetSize(), Z);
    };
    const FLinearColor DoctrineGold(0.88f, 0.72f, 0.22f, 1.0f);

    // The whole path stays visible at the point of commitment, so the player can
    // see exactly what is about to start.
    UHorizontalBox* Path = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("DoctrinePathBox"));
    const auto AddPathCell = [this, Path](const FText& Caption, const FText& Value,
        const FLinearColor& Colour, const FName Name)
    {
        UVerticalBox* Cell = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
        Cell->AddChildToVerticalBox(MakeText(
            WidgetTree, Caption, 10, TextMuted, FName(Name.ToString() + TEXT("_Cap")), true, false));
        Cell->AddChildToVerticalBox(MakeText(
            WidgetTree, Value, 17, Colour, FName(Name.ToString() + TEXT("_Val"))))
            ->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
        UHorizontalBoxSlot* Slot = Path->AddChildToHorizontalBox(Cell);
        Slot->SetSize(FillWeight(1.0f));
        Slot->SetPadding(FMargin(14.0f, 10.0f, 14.0f, 10.0f));
    };

    const bool bHasDoctrine = Country.Doctrines.IsValidIndex(SelectedDoctrineIndex);
    AddPathCell(
        ActiveBloc.bIsCategoryOnly ? LOCTEXT("PathCategory", "01 КАТЕГОРИЯ") : LOCTEXT("PathBloc", "01 БЛОК"),
        ActiveBloc.DisplayName, ActiveBloc.GlowColor, TEXT("PathBlocCell"));
    AddPathCell(LOCTEXT("PathCountry", "02 СТРАНА"),
        Country.DisplayName, Country.AccentColor, TEXT("PathCountryCell"));
    AddPathCell(LOCTEXT("PathDoctrine", "03 ДОКТРИНА"),
        bHasDoctrine ? Country.Doctrines[SelectedDoctrineIndex].DisplayName : FText::GetEmpty(),
        DoctrineGold, TEXT("PathDoctrineCell"));

    PlaceRect(
        MakePanel(WidgetTree, Path, TEXT("DoctrinePathFrame"), Country.AccentColor,
            DensityPadding(ERA4PanelRole::Compact), ActiveBloc.FrameRail),
        Ref(20.0f, 710.0f, 1080.0f, 90.0f), 3);

    UButton* StartButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("DoctrineStartButton"));
    StartButton->SetStyle(MakeCardButtonStyle(DoctrineGold));
    StartButton->SetIsEnabled(bHasDoctrine && Country.Doctrines[SelectedDoctrineIndex].bUnlocked);
    UTextBlock* StartLabel = MakeText(
        WidgetTree, LOCTEXT("StartOperation", "НАЧАТЬ ОПЕРАЦИЮ  »"), 18, TextHighlight,
        TEXT("DoctrineStartLabel"), true, false);
    StartLabel->SetJustification(ETextJustify::Center);
    StartButton->AddChild(StartLabel);
    StartButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::ContinueCampaign);
    PlaceRect(StartButton, Ref(1120.0f, 710.0f, 525.0f, 90.0f), 4);

    UButton* BackToCountries = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("DoctrineBackButton"));
    BackToCountries->SetStyle(MakeCardButtonStyle(FLinearColor(0.40f, 0.45f, 0.55f, 1.0f)));
    UTextBlock* BackLabel = MakeText(
        WidgetTree, LOCTEXT("BackToCountries", "‹  НАЗАД К СТРАНАМ"), 15, TextPrimary,
        TEXT("DoctrineBackLabel"), true, false);
    BackLabel->SetJustification(ETextJustify::Center);
    BackToCountries->AddChild(BackLabel);
    BackToCountries->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::GotoCountryStep);
    PlaceRect(BackToCountries, Ref(20.0f, 812.0f, 380.0f, 50.0f), 4);
}

void URA4CampaignSelectWidget::BuildCountryStepChrome(
    const FRA4BlocInfo& ActiveBloc,
    TFunctionRef<FBox2D(float, float, float, float)> Ref)
{
    if (!ActiveBloc.Countries.IsValidIndex(SelectedCountryIndex))
    {
        return;
    }
    const FRA4CountryInfo& Country = ActiveBloc.Countries[SelectedCountryIndex];

    const auto PlaceRect = [this](UWidget* Widget, const FBox2D& Rect, const int32 Z)
    {
        Place(CountryCardsContainer, Widget, Rect.Min, Rect.GetSize(), Z);
    };

    // Primary action, named after the country it commits to.
    UButton* ChooseButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("CountryChooseButton"));
    ChooseButton->SetStyle(MakeCardButtonStyle(Country.AccentColor));
    UTextBlock* ChooseLabel = MakeText(
        WidgetTree,
        FText::Format(LOCTEXT("ChooseCountryFmt", "ВЫБРАТЬ: {0}  »"), Country.DisplayName),
        18, TextHighlight, TEXT("CountryChooseLabel"), true, false);
    ChooseLabel->SetJustification(ETextJustify::Center);
    ChooseButton->AddChild(ChooseLabel);
    ChooseButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::GotoDoctrineStep);
    PlaceRect(ChooseButton, Ref(20.0f, 710.0f, 380.0f, 60.0f), 4);

    UButton* BackToBlocs = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("CountryBackButton"));
    BackToBlocs->SetStyle(MakeCardButtonStyle(FLinearColor(0.40f, 0.45f, 0.55f, 1.0f)));
    UTextBlock* BackLabel = MakeText(
        WidgetTree, LOCTEXT("BackToBlocs", "‹  НАЗАД К БЛОКАМ"), 15, TextPrimary,
        TEXT("CountryBackLabel"), true, false);
    BackLabel->SetJustification(ETextJustify::Center);
    BackToBlocs->AddChild(BackLabel);
    BackToBlocs->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::GotoBlocStep);
    PlaceRect(BackToBlocs, Ref(20.0f, 795.0f, 380.0f, 60.0f), 4);

    // Summary of what is currently chosen, so the path stays readable.
    UVerticalBox* Chosen = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CountryChosenBox"));
    Chosen->AddChildToVerticalBox(MakeText(
        WidgetTree, LOCTEXT("Chosen", "ВЫБРАНО"), 11, TextMuted,
        TEXT("ChosenCaption"), true, false))->SetPadding(FMargin(14.0f, 10.0f, 14.0f, 2.0f));
    Chosen->AddChildToVerticalBox(MakeText(
        WidgetTree, Country.DisplayName, 26, TextHighlight,
        TEXT("ChosenName"), true, false))->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 4.0f));
    Chosen->AddChildToVerticalBox(MakeText(
        WidgetTree, Country.Specialization, 11, TextPrimary,
        TEXT("ChosenSpec"), false))->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 4.0f));
    Chosen->AddChildToVerticalBox(MakeText(
        WidgetTree,
        DoctrineCountText(Country.Doctrines.Num()),
        13, FLinearColor(0.88f, 0.72f, 0.22f, 1.0f),
        TEXT("ChosenDocs"), true, false))->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 10.0f));
    PlaceRect(
        MakePanel(WidgetTree, Chosen, TEXT("CountryChosenFrame"), Country.AccentColor,
            DensityPadding(ActiveBloc.PanelDensity), ActiveBloc.FrameRail),
        Ref(420.0f, 700.0f, 400.0f, 170.0f), 3);

    // Doctrine preview: the reference shows the next step before it is entered.
    UTextBlock* NextHeader = MakeText(
        WidgetTree, LOCTEXT("NextStepDoctrine", "СЛЕДУЮЩИЙ ШАГ: ДОКТРИНА"), 12,
        Country.AccentColor, TEXT("CountryNextHeader"), true, false);
    PlaceRect(NextHeader, Ref(860.0f, 645.0f, 420.0f, 26.0f), 3);

    const FBox2D DoctrineRects[3] = {
        Ref(860.0f, 678.0f, 220.0f, 176.0f),
        Ref(1095.0f, 678.0f, 260.0f, 176.0f),
        Ref(1370.0f, 678.0f, 230.0f, 176.0f)
    };
    for (int32 D = 0; D < Country.Doctrines.Num() && D < 3; ++D)
    {
        const FRA4DoctrineInfo& Doctrine = Country.Doctrines[D];

        UVerticalBox* Tile = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), FName(*FString::Printf(TEXT("DocPreview_%d"), D)));
        UVerticalBoxSlot* Spacer = Tile->AddChildToVerticalBox(
            WidgetTree->ConstructWidget<UBorder>(
                UBorder::StaticClass(), FName(*FString::Printf(TEXT("DocPreviewArt_%d"), D))));
        Spacer->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

        UTextBlock* Name = MakeText(
            WidgetTree, Doctrine.DisplayName, 14, TextHighlight,
            FName(*FString::Printf(TEXT("DocPreviewName_%d"), D)));
        Name->SetJustification(ETextJustify::Center);
        Tile->AddChildToVerticalBox(Name)->SetPadding(FMargin(10.0f, 6.0f, 10.0f, 4.0f));

        UTextBlock* Philosophy = MakeText(
            WidgetTree, Doctrine.CombatPhilosophy, 10, TextMuted,
            FName(*FString::Printf(TEXT("DocPreviewPhil_%d"), D)), false);
        Philosophy->SetJustification(ETextJustify::Center);
        Tile->AddChildToVerticalBox(Philosophy)->SetPadding(FMargin(10.0f, 0.0f, 10.0f, 10.0f));

        UButton* TileButton = WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(), FName(*FString::Printf(TEXT("DocPreviewBtn_%d"), D)));
        TileButton->SetStyle(MakeCardButtonStyle(FLinearColor(0.88f, 0.72f, 0.22f, 1.0f)));
        TileButton->SetIsEnabled(Doctrine.bUnlocked);
        TileButton->AddChild(Tile);
        TileButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::GotoDoctrineStep);

        PlaceRect(
            MakePanel(WidgetTree, TileButton,
                FName(*FString::Printf(TEXT("DocPreviewFrame_%d"), D)),
                FLinearColor(0.88f, 0.72f, 0.22f, 1.0f),
                DensityPadding(ERA4PanelRole::Compact), ActiveBloc.FrameRail),
            DoctrineRects[D], 3);
    }
}

void URA4CampaignSelectWidget::RefreshDoctrineCards()
{
    if (!DoctrineCardsContainer)
    {
        return;
    }
    DoctrineCardsContainer->ClearChildren();
    DoctrineCardFrames.Reset();

    const FRA4FactionDataRegistry& Registry = FRA4FactionDataRegistry::Get();
    const TArray<FRA4BlocInfo>& Blocs = Registry.GetAllBlocs();
    if (!Blocs.IsValidIndex(SelectedBlocIndex))
    {
        return;
    }
    const FRA4BlocInfo& ActiveBloc = Blocs[SelectedBlocIndex];
    if (!ActiveBloc.Countries.IsValidIndex(SelectedCountryIndex))
    {
        return;
    }
    const FRA4CountryInfo& Country = ActiveBloc.Countries[SelectedCountryIndex];

    const float ToAuthoring = ReferenceSize.X / 1672.0f;
    const auto Ref = [ToAuthoring](const float X, const float Y, const float W, const float H)
    {
        return FBox2D(
            FVector2D(X * ToAuthoring, Y * ToAuthoring),
            FVector2D((X + W) * ToAuthoring, (Y + H) * ToAuthoring));
    };

    // Same grammar as the previous two steps: one hero and smaller plates, never
    // a row of identical tiles.
    const FBox2D HeroRect = Ref(20.0f, 115.0f, 810.0f, 575.0f);
    const FBox2D PlateRects[2] = {
        Ref(900.0f, 115.0f, 745.0f, 275.0f),
        Ref(900.0f, 405.0f, 745.0f, 285.0f)
    };
    const FLinearColor DoctrineGold(0.88f, 0.72f, 0.22f, 1.0f);

    int32 PlateIndex = 0;
    for (int32 Index = 0; Index < Country.Doctrines.Num(); ++Index)
    {
        const FRA4DoctrineInfo& Doctrine = Country.Doctrines[Index];
        const bool bHero = (Index == SelectedDoctrineIndex);
        if (!bHero && PlateIndex >= UE_ARRAY_COUNT(PlateRects))
        {
            continue;
        }
        const FBox2D Rect = bHero ? HeroRect : PlateRects[PlateIndex++];

        UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), FName(*FString::Printf(TEXT("DoctrineContent_%d"), Index)));

        UTextBlock* Numeral = MakeText(
            WidgetTree, FText::Format(LOCTEXT("DocNum", "ДОКТРИНА 0{0}"), FText::AsNumber(Index + 1)),
            bHero ? 13 : 11, DoctrineGold,
            FName(*FString::Printf(TEXT("DocNum_%d"), Index)), true, false);
        CardContent->AddChildToVerticalBox(Numeral)->SetPadding(FMargin(14.0f, 12.0f, 14.0f, 2.0f));

        UTextBlock* Title = MakeText(
            WidgetTree, Doctrine.DisplayName, bHero ? 32 : 19, TextHighlight,
            FName(*FString::Printf(TEXT("DocTitle_%d"), Index)));
        CardContent->AddChildToVerticalBox(Title)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 4.0f));

        UTextBlock* Philosophy = MakeText(
            WidgetTree, Doctrine.CombatPhilosophy, bHero ? 14 : 11, TextMuted,
            FName(*FString::Printf(TEXT("DocPhil_%d"), Index)), false);
        CardContent->AddChildToVerticalBox(Philosophy)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 10.0f));

        UTextBlock* Desc = MakeText(
            WidgetTree, Doctrine.Description, bHero ? 14 : 11, TextPrimary,
            FName(*FString::Printf(TEXT("DocDesc_%d"), Index)), false);
        CardContent->AddChildToVerticalBox(Desc)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 12.0f));

        // The roster swap is the point of a doctrine, so the hero spells it out.
        if (bHero)
        {
            UTextBlock* SwapHeader = MakeText(
                WidgetTree, LOCTEXT("SwapHeader", "МОДИФИКАЦИЯ СОСТАВА АРМИИ (25%)"), 12, ScarletHorizon,
                FName(*FString::Printf(TEXT("DocSwapHdr_%d"), Index)), true, false);
            CardContent->AddChildToVerticalBox(SwapHeader)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 6.0f));

            for (int32 U = 0; U < Doctrine.ModifiedUnits.Num(); ++U)
            {
                UTextBlock* Unit = MakeText(
                    WidgetTree,
                    FText::Format(LOCTEXT("UnitBullet", "•  {0}"), Doctrine.ModifiedUnits[U]),
                    12, TextHighlight,
                    FName(*FString::Printf(TEXT("DocUnit_%d_%d"), Index, U)), false);
                CardContent->AddChildToVerticalBox(Unit)->SetPadding(FMargin(22.0f, 0.0f, 14.0f, 4.0f));
            }

            UTextBlock* AbilityHeader = MakeText(
                WidgetTree, LOCTEXT("SuperHeader", "КЛЮЧЕВАЯ СПОСОБНОСТЬ"), 12, TextMuted,
                FName(*FString::Printf(TEXT("DocSpHdr_%d"), Index)), true, false);
            CardContent->AddChildToVerticalBox(AbilityHeader)->SetPadding(FMargin(14.0f, 10.0f, 14.0f, 4.0f));

            UTextBlock* Ability = MakeText(
                WidgetTree, Doctrine.SignatureSuperweapon, 13, DoctrineGold,
                FName(*FString::Printf(TEXT("DocSpTrait_%d"), Index)), false);
            CardContent->AddChildToVerticalBox(Ability)->SetPadding(FMargin(22.0f, 0.0f, 14.0f, 14.0f));
        }

        if (!Doctrine.bUnlocked)
        {
            UTextBlock* Locked = MakeText(
                WidgetTree, LOCTEXT("DoctrineLocked", "ЗАБЛОКИРОВАНА"), 11, TextMuted,
                FName(*FString::Printf(TEXT("DocLocked_%d"), Index)), true, false);
            CardContent->AddChildToVerticalBox(Locked)->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 10.0f));
        }

        UButton* CardButton = WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(), FName(*FString::Printf(TEXT("DoctrineBtn_%d"), Index)));
        CardButton->SetStyle(MakeCardButtonStyle(DoctrineGold));
        CardButton->SetIsEnabled(Doctrine.bUnlocked);
        CardButton->AddChild(CardContent);

        switch (Index)
        {
        case 0: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnDoctrine0); break;
        case 1: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnDoctrine1); break;
        case 2: CardButton->OnClicked.AddDynamic(this, &URA4CampaignSelectWidget::OnDoctrine2); break;
        default: break;
        }

        UBorder* CardFrame = MakePanel(
            WidgetTree, CardButton, FName(*FString::Printf(TEXT("DoctrineFrame_%d"), Index)),
            bHero ? DoctrineGold : Country.AccentColor,
            DensityPadding(ActiveBloc.PanelDensity), ActiveBloc.FrameRail);
        DoctrineCardFrames.Add(CardFrame);
        Place(DoctrineCardsContainer, CardFrame, Rect.Min, Rect.GetSize(), bHero ? 3 : 2);
    }

    BuildDoctrineStepChrome(ActiveBloc, Country, Ref);
}

void URA4CampaignSelectWidget::RefreshDossierPanel()
{
    const FRA4FactionDataRegistry& Registry = FRA4FactionDataRegistry::Get();
    const TArray<FRA4BlocInfo>& Blocs = Registry.GetAllBlocs();
    if (!Blocs.IsValidIndex(SelectedBlocIndex))
    {
        return;
    }

    const FRA4BlocInfo& ActiveBloc = Blocs[SelectedBlocIndex];
    const bool bHasCountry = ActiveBloc.Countries.IsValidIndex(SelectedCountryIndex);
    const FRA4CountryInfo* ActiveCountry = bHasCountry ? &ActiveBloc.Countries[SelectedCountryIndex] : nullptr;
    const bool bHasDoctrine = (ActiveCountry && ActiveCountry->Doctrines.IsValidIndex(SelectedDoctrineIndex));
    const FRA4DoctrineInfo* ActiveDoc = bHasDoctrine ? &ActiveCountry->Doctrines[SelectedDoctrineIndex] : nullptr;

    switch (CurrentStep)
    {
    case ERA4CampaignSelectStep::BlocSelection:
        if (DossierHeaderTag) DossierHeaderTag->SetText(LOCTEXT("TagBloc", "[ СТРАТЕГИЧЕСКИЙ БЛОК ]"));
        if (DossierTitleText) DossierTitleText->SetText(ActiveBloc.DisplayName);
        if (DossierSubtitleText) DossierSubtitleText->SetText(ActiveBloc.Motto);
        if (DossierSpecializationText)
        {
            FString AdvList;
            for (int32 AIdx = 0; AIdx < ActiveBloc.KeyAdvantages.Num(); ++AIdx)
            {
                AdvList += (AIdx > 0 ? TEXT(" • ") : TEXT("")) + ActiveBloc.KeyAdvantages[AIdx].ToString();
            }
            DossierSpecializationText->SetText(FText::FromString(AdvList));
        }
        if (DossierDescriptionText) DossierDescriptionText->SetText(ActiveBloc.Description);
        if (FirepowerBar) FirepowerBar->SetPercent(0.85f);
        if (ArmorBar) ArmorBar->SetPercent(0.80f);
        if (MobilityBar) MobilityBar->SetPercent(0.75f);
        if (TechBar) TechBar->SetPercent(0.85f);
        if (ContinueLabelText) ContinueLabelText->SetText(LOCTEXT("CTA_ChooseCountry", "ВЫБРАТЬ СТРАНУ ›"));
        break;

    case ERA4CampaignSelectStep::CountrySelection:
        if (ActiveCountry)
        {
            if (DossierHeaderTag) DossierHeaderTag->SetText(LOCTEXT("TagCountry", "[ ДОСЬЕ СТРАНЫ ]"));
            if (DossierTitleText) DossierTitleText->SetText(ActiveCountry->DisplayName);
            if (DossierSubtitleText) DossierSubtitleText->SetText(ActiveCountry->Specialization);
            if (DossierSpecializationText)
            {
                DossierSpecializationText->SetText(FText::Format(
                    LOCTEXT("CountryDossierSpecFmt", "БАЗОВЫЙ ШТАБ: {0}  •  ДОКТРИН: {1}"),
                    ActiveCountry->BaseHeadquarters, FText::AsNumber(ActiveCountry->Doctrines.Num())));
            }
            if (DossierDescriptionText) DossierDescriptionText->SetText(ActiveCountry->LoreDescription);
            if (FirepowerBar) FirepowerBar->SetPercent(ActiveCountry->FirepowerRating);
            if (ArmorBar) ArmorBar->SetPercent(ActiveCountry->ArmorRating);
            if (MobilityBar) MobilityBar->SetPercent(ActiveCountry->MobilityRating);
            if (TechBar) TechBar->SetPercent(ActiveCountry->TechRating);
            if (ContinueLabelText) ContinueLabelText->SetText(LOCTEXT("CTA_ChooseDoc", "ВЫБРАТЬ ДОКТРИНУ ›"));
        }
        break;

    case ERA4CampaignSelectStep::DoctrineSelection:
        if (ActiveDoc && ActiveCountry)
        {
            if (DossierHeaderTag) DossierHeaderTag->SetText(LOCTEXT("TagDoc", "[ ТАКТИЧЕСКАЯ ДОКТРИНА ]"));
            if (DossierTitleText) DossierTitleText->SetText(ActiveDoc->DisplayName);
            if (DossierSubtitleText) DossierSubtitleText->SetText(ActiveDoc->CombatPhilosophy);
            if (DossierSpecializationText)
            {
                DossierSpecializationText->SetText(FText::Format(
                    LOCTEXT("DocDossierSpecFmt", "ФЛАГМАНСКИЙ ЮНИТ: {0}"),
                    ActiveDoc->SignatureUnit));
            }
            if (DossierDescriptionText) DossierDescriptionText->SetText(ActiveDoc->Description);
            if (FirepowerBar) FirepowerBar->SetPercent(ActiveCountry->FirepowerRating);
            if (ArmorBar) ArmorBar->SetPercent(ActiveCountry->ArmorRating);
            if (MobilityBar) MobilityBar->SetPercent(ActiveCountry->MobilityRating);
            if (TechBar) TechBar->SetPercent(ActiveCountry->TechRating);
            if (ContinueLabelText) ContinueLabelText->SetText(LOCTEXT("CTA_StartOp", "НАЧАТЬ ОПЕРАЦИЮ ›"));
        }
        break;
    }
}

void URA4CampaignSelectWidget::OnBlocCardClicked(const int32 BlocIndex)
{
    SelectedBlocIndex = FMath::Clamp(BlocIndex, 0, 4);
    SelectedCountryIndex = 0;
    SelectedDoctrineIndex = 0;
    CurrentStep = ERA4CampaignSelectStep::CountrySelection;

    if (BlocCardsContainer) BlocCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (DossierFrameWidget) DossierFrameWidget->SetVisibility(ESlateVisibility::Collapsed);
    if (CountryCardsContainer) CountryCardsContainer->SetVisibility(ESlateVisibility::Visible);
    if (DoctrineCardsContainer) DoctrineCardsContainer->SetVisibility(ESlateVisibility::Collapsed);

    RefreshBreadcrumbs();
    RefreshCountryCards();
    RefreshDossierPanel();
}

void URA4CampaignSelectWidget::OnCountryCardClicked(const int32 CountryIndex)
{
    SelectedCountryIndex = CountryIndex;
    SelectedDoctrineIndex = 0;
    CurrentStep = ERA4CampaignSelectStep::DoctrineSelection;

    if (BlocCardsContainer) BlocCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (DossierFrameWidget) DossierFrameWidget->SetVisibility(ESlateVisibility::Visible);
    if (CountryCardsContainer) CountryCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (DoctrineCardsContainer) DoctrineCardsContainer->SetVisibility(ESlateVisibility::Visible);
    if (DossierFrameWidget) DossierFrameWidget->SetVisibility(ESlateVisibility::Collapsed);

    RefreshBreadcrumbs();
    RefreshDoctrineCards();
    RefreshDossierPanel();
}

void URA4CampaignSelectWidget::OnDoctrineCardClicked(const int32 DoctrineIndex)
{
    SelectedDoctrineIndex = DoctrineIndex;
    RefreshBreadcrumbs();
    RefreshDoctrineCards();
    RefreshDossierPanel();
}

void URA4CampaignSelectWidget::GotoBlocStep()
{
    CurrentStep = ERA4CampaignSelectStep::BlocSelection;
    if (BlocCardsContainer) BlocCardsContainer->SetVisibility(ESlateVisibility::Visible);
    if (DossierFrameWidget) DossierFrameWidget->SetVisibility(ESlateVisibility::Collapsed);
    if (CountryCardsContainer) CountryCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (DoctrineCardsContainer) DoctrineCardsContainer->SetVisibility(ESlateVisibility::Collapsed);

    RefreshBreadcrumbs();
    RefreshBlocCards();
    RefreshDossierPanel();
}

void URA4CampaignSelectWidget::GotoCountryStep()
{
    CurrentStep = ERA4CampaignSelectStep::CountrySelection;
    if (BlocCardsContainer) BlocCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (DossierFrameWidget) DossierFrameWidget->SetVisibility(ESlateVisibility::Collapsed);
    if (CountryCardsContainer) CountryCardsContainer->SetVisibility(ESlateVisibility::Visible);
    if (DoctrineCardsContainer) DoctrineCardsContainer->SetVisibility(ESlateVisibility::Collapsed);

    RefreshBreadcrumbs();
    RefreshCountryCards();
    RefreshDossierPanel();
}

void URA4CampaignSelectWidget::GotoDoctrineStep()
{
    CurrentStep = ERA4CampaignSelectStep::DoctrineSelection;
    if (BlocCardsContainer) BlocCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (DossierFrameWidget) DossierFrameWidget->SetVisibility(ESlateVisibility::Visible);
    if (CountryCardsContainer) CountryCardsContainer->SetVisibility(ESlateVisibility::Collapsed);
    if (DoctrineCardsContainer) DoctrineCardsContainer->SetVisibility(ESlateVisibility::Visible);
    if (DossierFrameWidget) DossierFrameWidget->SetVisibility(ESlateVisibility::Collapsed);

    RefreshBreadcrumbs();
    RefreshDoctrineCards();
    RefreshDossierPanel();
}

void URA4CampaignSelectWidget::OnBloc0() { OnBlocCardClicked(0); }
void URA4CampaignSelectWidget::OnBloc1() { OnBlocCardClicked(1); }
void URA4CampaignSelectWidget::OnBloc2() { OnBlocCardClicked(2); }
void URA4CampaignSelectWidget::OnBloc3() { OnBlocCardClicked(3); }
void URA4CampaignSelectWidget::OnBloc4() { OnBlocCardClicked(4); }

void URA4CampaignSelectWidget::OnCountry0() { OnCountryCardClicked(0); }
void URA4CampaignSelectWidget::OnCountry1() { OnCountryCardClicked(1); }
void URA4CampaignSelectWidget::OnCountry2() { OnCountryCardClicked(2); }
void URA4CampaignSelectWidget::OnCountry3() { OnCountryCardClicked(3); }
void URA4CampaignSelectWidget::OnCountry4() { OnCountryCardClicked(4); }
void URA4CampaignSelectWidget::OnCountry5() { OnCountryCardClicked(5); }

void URA4CampaignSelectWidget::OnDoctrine0() { OnDoctrineCardClicked(0); }
void URA4CampaignSelectWidget::OnDoctrine1() { OnDoctrineCardClicked(1); }
void URA4CampaignSelectWidget::OnDoctrine2() { OnDoctrineCardClicked(2); }

void URA4CampaignSelectWidget::ContinueCampaign()
{
    if (CurrentStep == ERA4CampaignSelectStep::BlocSelection)
    {
        GotoCountryStep();
        return;
    }
    if (CurrentStep == ERA4CampaignSelectStep::CountrySelection)
    {
        GotoDoctrineStep();
        return;
    }

    // Step 3: Launch Campaign for the selected faction. Route through the
    // screen host so it owns the widget swap (no orphaned duplicates).
    switch (static_cast<ERA4FactionTheme>(SelectedBlocIndex))
    {
    case ERA4FactionTheme::EurasianPact:      RouteToScreen(ERA4UIScreenId::EurasianCampaign); break;
    case ERA4FactionTheme::AtlanticAlliance:  RouteToScreen(ERA4UIScreenId::AtlanticCampaign); break;
    case ERA4FactionTheme::EasternCoalition:  RouteToScreen(ERA4UIScreenId::EasternCampaign); break;
    case ERA4FactionTheme::PacificPact:       RouteToScreen(ERA4UIScreenId::PacificCampaign); break;
    case ERA4FactionTheme::Independent:       RouteToScreen(ERA4UIScreenId::IndependentCampaign); break;
    default: break;
    }
}

void URA4CampaignSelectWidget::OpenMainMenu()
{
    RouteToScreen(ERA4UIScreenId::MainMenu);
}

void URA4CampaignSelectWidget::OpenMultiplayer()
{
    RouteToScreen(ERA4UIScreenId::MultiplayerLobby);
}

void URA4CampaignSelectWidget::OpenChallenges()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4ShowcaseWidget* Screen = CreateWidget<URA4ShowcaseWidget>(
            PlayerController, URA4ShowcaseWidget::StaticClass()))
        {
            Screen->SetInitialScreenForPresentation(23); // Challenges
            Screen->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

void URA4CampaignSelectWidget::OpenBarracks()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4ShowcaseWidget* Screen = CreateWidget<URA4ShowcaseWidget>(
            PlayerController, URA4ShowcaseWidget::StaticClass()))
        {
            Screen->SetInitialScreenForPresentation(19); // Barracks
            Screen->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

void URA4CampaignSelectWidget::OpenSettings()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4ShowcaseWidget* Screen = CreateWidget<URA4ShowcaseWidget>(
            PlayerController, URA4ShowcaseWidget::StaticClass()))
        {
            Screen->SetInitialScreenForPresentation(4); // Settings
            Screen->AddToViewport(0);
            RemoveFromParent();
        }
    }
}
#undef LOCTEXT_NAMESPACE
