// Copyright (c) Red Alert 4 project.

#include "RA4MainMenuScreenWidget.h"

#include "RA4FactionData.h"

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
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "RA4AngularPanelWidget.h"
#include "RA4SkirmishSetupWidget.h"

#define LOCTEXT_NAMESPACE "RA4MainMenuScreenWidget"

namespace
{
// The command centre is neutral ground: the frame carries the shared Scarlet
// horizon line, while bloc colour arrives only with the selected direction.
// The command centre is neutral ground: cold steel chrome, with scarlet kept for
// the horizon line under the title and for alarm states only.
const FLinearColor MenuAccent(0.32f, 0.42f, 0.56f, 1.0f);
const FLinearColor MenuChromeTint(0.42f, 0.52f, 0.66f, 1.0f);
const FLinearColor MenuSelectedTint(0.45f, 0.62f, 0.95f, 1.0f);
const FLinearColor MenuPressedTint(0.65f, 0.80f, 1.00f, 1.0f);
const FLinearColor MenuHorizon = FRA4FactionDataRegistry::GetHorizonScarletColor();
constexpr FLinearColor MenuText(0.87f, 0.89f, 0.94f, 1.0f);
constexpr FLinearColor MutedText(0.55f, 0.58f, 0.64f, 1.0f);

void PlaceMenuWidget(
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

UTextBlock* MakeMenuText(
    UWidgetTree* Tree,
    const FText& Text,
    const int32 Size,
    const FLinearColor& Color,
    const FName Name,
    const bool bBold = false)
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
        FontInfo.LetterSpacing = bBold ? 40 : 18;
        Label->SetFont(FontInfo);
    }
    return Label;
}

FButtonStyle MakeMenuButtonStyle(const bool bSelected)
{
    // The supplied frame textures are warm red and carry their own pixel size, so
    // drawing them stretched both fought the palette and made rows overlap. The
    // menu plate is drawn procedurally instead: it takes the row height exactly,
    // scales to any resolution and reads its colour from the theme.
    FButtonStyle Style;

    const FLinearColor Edge = bSelected ? MenuSelectedTint : MenuAccent;
    Style.SetNormal(FSlateRoundedBoxBrush(
        bSelected ? FLinearColor(0.055f, 0.105f, 0.200f, 0.98f)
                  : FLinearColor(0.016f, 0.022f, 0.034f, 0.94f),
        3.0f, Edge, bSelected ? 2.0f : 1.0f));
    Style.SetHovered(FSlateRoundedBoxBrush(
        FLinearColor(0.085f, 0.155f, 0.280f, 0.98f), 3.0f, MenuSelectedTint, 2.0f));
    Style.SetPressed(FSlateRoundedBoxBrush(
        FLinearColor(0.140f, 0.245f, 0.420f, 1.0f), 3.0f, MenuPressedTint, 2.0f));
    Style.SetDisabled(FSlateRoundedBoxBrush(
        FLinearColor(0.012f, 0.014f, 0.018f, 0.55f), 3.0f,
        FLinearColor(0.22f, 0.24f, 0.28f, 0.8f), 1.0f));

    Style.NormalPadding = FMargin(0.0f);
    Style.PressedPadding = FMargin(2.0f, 2.0f, 0.0f, 0.0f);
    return Style;
}
} // namespace

URA4MainMenuScreenWidget::URA4MainMenuScreenWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetScreenIdentity(ERA4UIScreenId::MainMenu);
}

TSharedRef<SWidget> URA4MainMenuScreenWidget::RebuildWidget()
{
    const TSharedRef<SWidget> RootWidget = Super::RebuildWidget();
    if (!WidgetTree || !GetContentLayer())
    {
        return RootWidget;
    }

    MainMenuViewModel = NewObject<URA4MainMenuViewModel>(this);
    MenuButtons.Reset();

    if (UTexture2D* BackgroundTexture = LoadObject<UTexture2D>(
        nullptr,
        TEXT("/Game/RA4UI/Art/T_RA4_USSR_MainMenuBackground.T_RA4_USSR_MainMenuBackground")))
    {
        GetBackgroundLayer()->SetBrushFromTexture(BackgroundTexture, false);
        GetBackgroundLayer()->SetColorAndOpacity(FLinearColor(0.20f, 0.34f, 0.60f, 1.0f));
    }

    UBorder* Grade = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("CommandCentreGrade"));
    Grade->SetBrushColor(FLinearColor(0.008f, 0.016f, 0.038f, 0.58f));
    Grade->SetVisibility(ESlateVisibility::HitTestInvisible);
    UOverlaySlot* GradeSlot = GetContentLayer()->AddChildToOverlay(Grade);
    GradeSlot->SetHorizontalAlignment(HAlign_Fill);
    GradeSlot->SetVerticalAlignment(VAlign_Fill);

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("MainMenuCanvas"));
    UOverlaySlot* CanvasSlot = GetContentLayer()->AddChildToOverlay(Canvas);
    CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
    CanvasSlot->SetVerticalAlignment(VAlign_Fill);

    UImage* MechanicalChrome = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), TEXT("MainMenuMechanicalChrome"));
    if (UTexture2D* ChromeTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_USSR_MainMenuChrome.T_RA4_USSR_MainMenuChrome")))
    {
        MechanicalChrome->SetBrushFromTexture(ChromeTexture, false);
    }
    MechanicalChrome->SetVisibility(ESlateVisibility::HitTestInvisible);
    PlaceMenuWidget(Canvas, MechanicalChrome, FVector2D::ZeroVector, FVector2D(1920.0f, 1080.0f), 3);

    LogoImage = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), TEXT("MainMenuLogo"));
    if (UTexture2D* LogoTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_Logo.T_RA4_Logo")))
    {
        LogoImage->SetBrushFromTexture(LogoTexture, false);
    }
    LogoImage->SetVisibility(ESlateVisibility::HitTestInvisible);
    // Reference 02_main_menu.png: wordmark centred over the holographic table,
    // with the scarlet horizon line and the screen's own tagline beneath it.
    PlaceMenuWidget(Canvas, LogoImage, FVector2D(637.0f, 26.0f), FVector2D(718.0f, 188.0f), 4);

    UBorder* HorizonRule = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("MainMenuHorizonRule"));
    HorizonRule->SetBrushColor(MenuHorizon);
    HorizonRule->SetVisibility(ESlateVisibility::HitTestInvisible);
    PlaceMenuWidget(Canvas, HorizonRule, FVector2D(720.0f, 196.0f), FVector2D(552.0f, 2.0f), 5);

    UTextBlock* Tagline = MakeMenuText(
        WidgetTree, LOCTEXT("MainMenuTagline", "ГЛОБАЛЬНЫЙ КОМАНДНЫЙ ЦЕНТР"), 19,
        MutedText, TEXT("MainMenuTagline"), true);
    Tagline->SetJustification(ETextJustify::Center);
    Tagline->SetVisibility(ESlateVisibility::HitTestInvisible);
    PlaceMenuWidget(Canvas, Tagline, FVector2D(637.0f, 206.0f), FVector2D(718.0f, 30.0f), 5);

    UVerticalBox* MenuList = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("MainMenuEntries"));
    const TArray<FRA4MainMenuEntry>& Entries = MainMenuViewModel->GetMenuEntries();
    for (int32 Index = 0; Index < Entries.Num(); ++Index)
    {
        CreateMenuButton(MenuList, Entries[Index], Index);
    }

    URA4AngularPanelWidget* MenuPanel = WidgetTree->ConstructWidget<URA4AngularPanelWidget>(
        URA4AngularPanelWidget::StaticClass(), TEXT("MainMenuPanel"));
    MenuPanel->SetPanelRole(ERA4PanelRole::Compact);
    MenuPanel->SetBrush(FSlateRoundedBoxBrush(
        FLinearColor(0.005f, 0.005f, 0.008f, 0.88f), 0.0f, MenuAccent, 1.5f));
    MenuPanel->SetContent(MenuList);
    PlaceMenuWidget(Canvas, MenuPanel, FVector2D(52.0f, 100.0f), FVector2D(348.0f, 600.0f), 2);

    BuildInformationCard(
        Canvas,
        LOCTEXT("CommanderHeading", "КОМАНДИР  ·  УРОВЕНЬ 24"),
        LOCTEXT("CommanderBody", "ОПЫТ  34 750 / 48 000 ОП\nДОПУСК: ОПЕРАТИВНАЯ СЕТЬ · УРОВЕНЬ A"),
        FVector2D(28.0f, 878.0f),
        FVector2D(532.0f, 170.0f),
        TEXT("CommanderCard"));
    BuildInformationCard(
        Canvas,
        LOCTEXT("NewsHeading", "СВОДКА ФРОНТОВ"),
        LOCTEXT("NewsBody", "Евразийский пакт · Атлантический альянс\nВосточная коалиция · Тихоокеанский пакт\nНезависимые державы"),
        FVector2D(579.0f, 878.0f),
        FVector2D(496.0f, 170.0f),
        TEXT("NewsCard"));
    BuildInformationCard(
        Canvas,
        LOCTEXT("OperationsHeading", "ТЕКУЩАЯ ОПЕРАЦИЯ"),
        LOCTEXT("OperationsBody", "Барьер «Тифон»  ·  фаза 2 / 4\nВосстановить контроль над защищённым маршрутом."),
        FVector2D(1086.0f, 878.0f),
        FVector2D(523.0f, 170.0f),
        TEXT("OperationsCard"));
    BuildInformationCard(
        Canvas,
        LOCTEXT("EmblemHeading", "СОСТОЯНИЕ СЕТИ"),
        LOCTEXT("EmblemBody", "СЕТЬ ПОДКЛЮЧЕНА  //  КАНАЛ 01"),
        FVector2D(1620.0f, 878.0f),
        FVector2D(274.0f, 170.0f),
        TEXT("EmblemCard"));

    UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("MainMenuFooter"));
    UTextBlock* Connection = MakeMenuText(
        WidgetTree,
        LOCTEXT("Connection", "◉  СЕТЬ: ПОДКЛЮЧЕНО     СЕРВИСЫ БЛОКОВ: ДОСТУПНЫ"),
        14,
        FLinearColor(0.34f, 0.78f, 0.44f, 1.0f),
        TEXT("ConnectionStatus"));
    Connection->SetAutoWrapText(false);
    Footer->AddChildToHorizontalBox(Connection)->SetPadding(FMargin(16.0f, 7.0f));
    UTextBlock* Version = MakeMenuText(
        WidgetTree,
        LOCTEXT("Version", "SCARLET HORIZON v1.0-RC1"),
        13,
        MutedText,
        TEXT("VersionText"));
    UHorizontalBoxSlot* VersionSlot = Footer->AddChildToHorizontalBox(Version);
    VersionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    VersionSlot->SetHorizontalAlignment(HAlign_Right);
    VersionSlot->SetPadding(FMargin(16.0f, 7.0f));
    URA4AngularPanelWidget* FooterPanel = WidgetTree->ConstructWidget<URA4AngularPanelWidget>(
        URA4AngularPanelWidget::StaticClass(), TEXT("FooterPanel"));
    FooterPanel->SetPanelRole(ERA4PanelRole::Compact);
    FooterPanel->SetBrush(FSlateRoundedBoxBrush(
        FLinearColor(0.005f, 0.005f, 0.007f, 0.94f), 0.0f, MenuAccent, 1.25f));
    FooterPanel->SetContent(Footer);
    PlaceMenuWidget(Canvas, FooterPanel, FVector2D(18.0f, 1024.0f), FVector2D(1884.0f, 40.0f), 6);
    return RootWidget;
}

UButton* URA4MainMenuScreenWidget::CreateMenuButton(
    UVerticalBox* Menu,
    const FRA4MainMenuEntry& Entry,
    const int32 Index)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), FName(*FString::Printf(TEXT("MainMenuButton_%d"), Index)));
    Button->SetStyle(MakeMenuButtonStyle(Entry.bSelected));

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), FName(*FString::Printf(TEXT("MainMenuRow_%d"), Index)));
    UTextBlock* Icon = MakeMenuText(
        WidgetTree,
        FText::FromString(Index == 0 ? TEXT("★") : TEXT("◆")),
        24,
        Entry.bSelected ? FLinearColor::White : MutedText,
        FName(*FString::Printf(TEXT("MainMenuIcon_%d"), Index)),
        true);
    UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(Icon);
    IconSlot->SetPadding(FMargin(22.0f, 0.0f, 20.0f, 0.0f));
    IconSlot->SetVerticalAlignment(VAlign_Center);

    UTextBlock* Label = MakeMenuText(
        WidgetTree,
        Entry.Label,
        24,
        MenuText,
        FName(*FString::Printf(TEXT("MainMenuLabel_%d"), Index)),
        true);
    Label->SetAutoWrapText(false);
    UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
    LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LabelSlot->SetVerticalAlignment(VAlign_Center);
    Button->AddChild(Row);

    switch (Entry.Action)
    {
    case ERA4MainMenuAction::Campaign:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::OpenCampaign);
        break;
    case ERA4MainMenuAction::Multiplayer:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::OpenMultiplayer);
        break;
    case ERA4MainMenuAction::Skirmish:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::OpenSkirmish);
        break;
    case ERA4MainMenuAction::Editor:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::OpenEditor);
        break;
    case ERA4MainMenuAction::Encyclopedia:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::OpenEncyclopedia);
        break;
    case ERA4MainMenuAction::Modifications:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::OpenModifications);
        break;
    case ERA4MainMenuAction::Settings:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::OpenSettings);
        break;
    case ERA4MainMenuAction::Exit:
        Button->OnClicked.AddDynamic(this, &URA4MainMenuScreenWidget::ExitToSplash);
        break;
    default:
        checkNoEntry();
        break;
    }

    USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), FName(*FString::Printf(TEXT("MainMenuButtonSize_%d"), Index)));
    ButtonSize->SetHeightOverride(62.0f);
    ButtonSize->SetContent(Button);
    Menu->AddChildToVerticalBox(ButtonSize)->SetPadding(FMargin(4.0f, 4.0f));
    MenuButtons.Add(Button);
    return Button;
}

void URA4MainMenuScreenWidget::BuildInformationCard(
    UCanvasPanel* Canvas,
    const FText& Heading,
    const FText& Body,
    const FVector2D& Position,
    const FVector2D& Size,
    const FName Name)
{
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), FName(Name.ToString() + TEXT("_Content")));
    Content->AddChildToVerticalBox(MakeMenuText(
        WidgetTree,
        Heading,
        17,
        MenuAccent,
        FName(Name.ToString() + TEXT("_Heading")),
        true))->SetPadding(FMargin(34.0f, 10.0f, 34.0f, 4.0f));
    Content->AddChildToVerticalBox(MakeMenuText(
        WidgetTree,
        Body,
        14,
        MenuText,
        FName(Name.ToString() + TEXT("_Body"))))->SetPadding(FMargin(34.0f, 4.0f, 34.0f, 10.0f));

    URA4AngularPanelWidget* Panel = WidgetTree->ConstructWidget<URA4AngularPanelWidget>(
        URA4AngularPanelWidget::StaticClass(), Name);
    Panel->SetPanelRole(ERA4PanelRole::Compact);
    Panel->SetBrush(FSlateRoundedBoxBrush(
        FLinearColor(0.006f, 0.006f, 0.009f, 0.92f), 0.0f, MenuAccent, 1.25f));
    Panel->SetContent(Content);
    PlaceMenuWidget(Canvas, Panel, Position, Size, 2);
}

int32 URA4MainMenuScreenWidget::GetSelectedMenuIndex() const
{
    return MainMenuViewModel ? MainMenuViewModel->GetSelectedMenuIndex() : INDEX_NONE;
}

void URA4MainMenuScreenWidget::OpenCampaign()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Campaign);
    NavigateToScreen(ERA4UIScreenId::CampaignSelect);
}

void URA4MainMenuScreenWidget::OpenMultiplayer()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Multiplayer);
    NavigateToScreen(ERA4UIScreenId::MultiplayerLobby);
}

void URA4MainMenuScreenWidget::OpenSkirmish()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Skirmish);
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4SkirmishSetupWidget* Skirmish = CreateWidget<URA4SkirmishSetupWidget>(
            PlayerController, URA4SkirmishSetupWidget::StaticClass()))
        {
            Skirmish->AddToViewport();
            RemoveFromParent();
        }
    }
}

void URA4MainMenuScreenWidget::OpenEditor()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Editor);
    NavigateToScreen(ERA4UIScreenId::TechTree);
}

void URA4MainMenuScreenWidget::OpenEncyclopedia()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Encyclopedia);
    NavigateToScreen(ERA4UIScreenId::Encyclopedia);
}

void URA4MainMenuScreenWidget::OpenModifications()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Modifications);
    NavigateToScreen(ERA4UIScreenId::Mods);
}

void URA4MainMenuScreenWidget::OpenSettings()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Settings);
    NavigateToScreen(ERA4UIScreenId::Settings);
}

void URA4MainMenuScreenWidget::ExitToSplash()
{
    MainMenuViewModel->ExecuteAction(ERA4MainMenuAction::Exit);
    NavigateToScreen(ERA4UIScreenId::Splash);
}

#undef LOCTEXT_NAMESPACE
