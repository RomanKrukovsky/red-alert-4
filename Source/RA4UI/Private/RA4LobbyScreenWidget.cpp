// Copyright (c) Red Alert 4 project.

#include "RA4LobbyScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "RA4AngularPanelWidget.h"
#include "RA4MainMenuScreenWidget.h"

#define LOCTEXT_NAMESPACE "RA4LobbyScreenWidget"

namespace
{
constexpr FLinearColor LobbyAccent(0.32f, 0.42f, 0.56f, 1.0f);
constexpr FLinearColor LobbyAlarm(0.92f, 0.035f, 0.04f, 1.0f);
constexpr FLinearColor LobbyText(0.86f, 0.82f, 0.78f, 1.0f);
constexpr FLinearColor LobbyMuted(0.54f, 0.52f, 0.50f, 1.0f);
constexpr FLinearColor LobbyPanel(0.008f, 0.008f, 0.011f, 0.95f);
constexpr FLinearColor LobbyGreen(0.38f, 0.92f, 0.25f, 1.0f);

void PlaceLobbyWidget(
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

UTextBlock* MakeLobbyText(
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
        FontInfo.LetterSpacing = bBold ? 34 : 10;
        Label->SetFont(FontInfo);
    }
    return Label;
}

URA4AngularPanelWidget* MakeLobbyPanel(
    UWidgetTree* Tree,
    UWidget* Content,
    const FName Name,
    const ERA4PanelRole Role = ERA4PanelRole::Standard,
    const TCHAR* TexturePath = nullptr)
{
    URA4AngularPanelWidget* Panel = Tree->ConstructWidget<URA4AngularPanelWidget>(
        URA4AngularPanelWidget::StaticClass(), Name);
    Panel->SetPanelRole(Role);
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
            Panel->SetBrush(FSlateRoundedBoxBrush(LobbyPanel, 0.0f, LobbyAccent, 1.35f));
        }
    }
    else
    {
        Panel->SetBrush(FSlateRoundedBoxBrush(LobbyPanel, 0.0f, LobbyAccent, 1.35f));
    }
    Panel->SetContent(Content);
    return Panel;
}

FButtonStyle MakeLobbyButtonStyle(const bool bPrimary = false)
{
    FButtonStyle Style;
    UTexture2D* NormalTexture = LoadObject<UTexture2D>(nullptr, bPrimary
        ? TEXT("/Game/RA4UI/Art/T_RA4_UI_ButtonPrimary.T_RA4_UI_ButtonPrimary")
        : TEXT("/Game/RA4UI/Art/T_RA4_UI_ButtonSecondary.T_RA4_UI_ButtonSecondary"));
    UTexture2D* HoveredTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_Frame_ButtonHoveredV2.T_RA4_Frame_ButtonHoveredV2"));
    UTexture2D* PressedTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_UI_ButtonPressed.T_RA4_UI_ButtonPressed"));
    UTexture2D* DisabledTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_UI_ButtonDisabled.T_RA4_UI_ButtonDisabled"));
    if (NormalTexture && HoveredTexture && PressedTexture && DisabledTexture)
    {
        FSlateBrush Normal;
        Normal.SetResourceObject(NormalTexture);
        Normal.DrawAs = ESlateBrushDrawType::Image;
        FSlateBrush Hovered = Normal;
        Hovered.SetResourceObject(HoveredTexture);
        FSlateBrush Pressed = Normal;
        Pressed.SetResourceObject(PressedTexture);
        FSlateBrush Disabled = Normal;
        Disabled.SetResourceObject(DisabledTexture);
        Style.SetNormal(Normal);
        Style.SetHovered(Hovered);
        Style.SetPressed(Pressed);
        Style.SetDisabled(Disabled);
    }
    else
    {
        Style.SetNormal(FSlateColorBrush(bPrimary
            ? FLinearColor(0.28f, 0.012f, 0.018f, 0.98f)
            : FLinearColor(0.025f, 0.022f, 0.024f, 0.96f)));
        Style.SetHovered(FSlateColorBrush(FLinearColor(0.48f, 0.018f, 0.026f, 1.0f)));
        Style.SetPressed(FSlateColorBrush(LobbyAlarm));
        Style.SetDisabled(FSlateColorBrush(FLinearColor(0.02f, 0.02f, 0.022f, 0.45f)));
    }
    return Style;
}

FText GetFactionName(const ERA4FactionTheme Faction)
{
    switch (Faction)
    {
    case ERA4FactionTheme::EurasianPact:
        return LOCTEXT("FactionEurasia", "◆  ЕВРАЗИЙСКИЙ ПАКТ");
    case ERA4FactionTheme::AtlanticAlliance:
        return LOCTEXT("FactionAtlantic", "▲  АТЛАНТИЧЕСКИЙ АЛЬЯНС");
    case ERA4FactionTheme::EasternCoalition:
        return LOCTEXT("FactionEastern", "■  ВОСТОЧНАЯ КОАЛИЦИЯ");
    case ERA4FactionTheme::PacificPact:
        return LOCTEXT("FactionPacific", "◇  ТИХООКЕАНСКИЙ ПАКТ");
    case ERA4FactionTheme::Independent:
        return LOCTEXT("FactionIndep", "○  НЕЗАВИСИМЫЕ ДЕРЖАВЫ");
    case ERA4FactionTheme::Chronolegion:
        return LOCTEXT("FactionChrono", "△  EXPERIMENTAL / LEGACY");
    default:
        return LOCTEXT("FactionEurasia", "◆  ЕВРАЗИЙСКИЙ ПАКТ");
    }
}

FLinearColor GetPlayerColor(const int32 ColorIndex)
{
    const FLinearColor Colors[] = {
        FLinearColor(0.68f, 0.28f, 0.88f, 1.0f), // Purple
        FLinearColor(0.35f, 0.70f, 0.98f, 1.0f), // Blue
        FLinearColor(0.88f, 0.72f, 0.22f, 1.0f), // Gold
        FLinearColor(0.20f, 0.80f, 0.90f, 1.0f), // Turquoise
        FLinearColor(0.78f, 0.52f, 0.18f, 1.0f), // Amber
        FLinearColor(0.22f, 0.75f, 0.35f, 1.0f), // Green
        FLinearColor(0.45f, 0.75f, 1.0f, 1.0f),  // Ice
        FLinearColor(0.95f, 0.45f, 0.35f, 1.0f)  // Coral
    };
    return Colors[FMath::Clamp(ColorIndex, 0, UE_ARRAY_COUNT(Colors) - 1)];
}
} // namespace

TSharedRef<SWidget> URA4LobbyPlayerRowWidget::RebuildWidget()
{
    if (WidgetTree)
    {
        UBorder* Background = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), TEXT("LobbyPlayerRowBackground"));
        if (UTexture2D* SlotTexture = LoadObject<UTexture2D>(
            nullptr, TEXT("/Game/RA4UI/Art/T_RA4_UI_LobbySlot.T_RA4_UI_LobbySlot")))
        {
            FSlateBrush SlotBrush;
            SlotBrush.SetResourceObject(SlotTexture);
            SlotBrush.DrawAs = ESlateBrushDrawType::Image;
            Background->SetBrush(SlotBrush);
        }
        else
        {
            Background->SetBrushColor(FLinearColor(0.018f, 0.016f, 0.018f, 0.96f));
        }
        Background->SetPadding(FMargin(8.0f, 5.0f));
        WidgetTree->RootWidget = Background;

        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(), TEXT("LobbyPlayerRow"));
        Background->SetContent(Row);

        const auto AddColumn = [this, Row](
            TObjectPtr<UTextBlock>& Target,
            const float Fill,
            const FName Name)
        {
            Target = MakeLobbyText(WidgetTree, FText::GetEmpty(), 14, LobbyText, Name, false);
            UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Target.Get());
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            Slot->SetPadding(FMargin(4.0f));
            Slot->SetHorizontalAlignment(HAlign_Left);
            Target->SetMinDesiredWidth(Fill);
        };

        AddColumn(IndexText, 32.0f, TEXT("PlayerIndex"));
        AddColumn(PlayerText, 160.0f, TEXT("PlayerName"));
        AddColumn(TeamText, 55.0f, TEXT("PlayerTeam"));
        AddColumn(FactionText, 180.0f, TEXT("PlayerFaction"));
        AddColumn(CountryText, 130.0f, TEXT("PlayerCountry"));
        AddColumn(DoctrineText, 180.0f, TEXT("PlayerDoctrine"));
        AddColumn(ReadyText, 95.0f, TEXT("PlayerReady"));
    }
    return Super::RebuildWidget();
}

void URA4LobbyPlayerRowWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
    const URA4LobbyPlayerListItem* Item = Cast<URA4LobbyPlayerListItem>(ListItemObject);
    if (!Item)
    {
        return;
    }
    const FRA4LobbyPlayerView& Player = Item->GetData();
    if (IndexText)
    {
        IndexText->SetText(FText::AsNumber(Player.ColorIndex + 1));
        IndexText->SetColorAndOpacity(FSlateColor(GetPlayerColor(Player.ColorIndex)));
    }
    if (PlayerText)
    {
        PlayerText->SetText(Player.PlayerName);
    }
    if (TeamText)
    {
        TeamText->SetText(FText::AsNumber(Player.Team));
    }
    if (FactionText)
    {
        FactionText->SetText(GetFactionName(Player.Faction));
        FactionText->SetColorAndOpacity(FSlateColor(GetPlayerColor(Player.ColorIndex)));
    }
    if (CountryText)
    {
        CountryText->SetText(Player.CountryName);
    }
    if (DoctrineText)
    {
        DoctrineText->SetText(Player.DoctrineName);
    }
    if (ReadyText)
    {
        ReadyText->SetText(Player.bReady
            ? LOCTEXT("Ready", "✔  ГОТОВ")
            : LOCTEXT("NotReady", "○  ОЖИДАНИЕ"));
        ReadyText->SetColorAndOpacity(FSlateColor(Player.bReady ? LobbyGreen : LobbyMuted));
    }
}

URA4LobbyScreenWidget::URA4LobbyScreenWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetScreenIdentity(ERA4UIScreenId::MultiplayerLobby);
}

TSharedRef<SWidget> URA4LobbyScreenWidget::RebuildWidget()
{
    SetScreenIdentity(ERA4UIScreenId::MultiplayerLobby);
    const TSharedRef<SWidget> RootWidget = Super::RebuildWidget();
    if (!WidgetTree || !GetContentLayer())
    {
        return RootWidget;
    }

    LobbyViewModel = NewObject<URA4LobbyViewModel>(this);
    // Remaster screen 11 is the canonical multiplayer lobby reference; its
    // composition (eight-slot roster, match parameters, map preview, chat) is
    // baked into the plate, with the live widgets layered on top.
    if (UTexture2D* Background = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/Remaster/T_SH_11_MultiplayerLobby.T_SH_11_MultiplayerLobby")))
    {
        GetBackgroundLayer()->SetBrushFromTexture(Background, false);
        GetBackgroundLayer()->SetColorAndOpacity(FLinearColor(0.78f, 0.80f, 0.84f, 1.0f));
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("LobbyCanvas"));
    UOverlaySlot* CanvasSlot = GetContentLayer()->AddChildToOverlay(Canvas);
    CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
    CanvasSlot->SetVerticalAlignment(VAlign_Fill);

    UImage* Logo = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LobbyLogo"));
    if (UTexture2D* LogoTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_Logo.T_RA4_Logo")))
    {
        Logo->SetBrushFromTexture(LogoTexture, false);
    }
    Logo->SetVisibility(ESlateVisibility::HitTestInvisible);
    PlaceLobbyWidget(Canvas, Logo, FVector2D(690.0f, 0.0f), FVector2D(540.0f, 150.0f), 3);

    UVerticalBox* LobbyInfo = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("LobbyInfo"));
    LobbyInfo->AddChildToVerticalBox(MakeLobbyText(
        WidgetTree, LOCTEXT("LobbyTitle", "ЛОББИ СЕТЕВОГО МАТЧА"), 26,
        LobbyText, TEXT("LobbyTitle")));
    ReadyStatusText = MakeLobbyText(
        WidgetTree, LobbyViewModel->GetValidationMessage(), 16,
        LobbyGreen, TEXT("LobbyReadyStatus"), false);
    LobbyInfo->AddChildToVerticalBox(ReadyStatusText)->SetPadding(FMargin(0.0f, 12.0f));

    USizeBox* EmblemFrame = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("LobbyEmblemFrame"));
    EmblemFrame->SetHeightOverride(210.0f);
    UImage* Emblem = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), TEXT("LobbyEmblem"));
    if (UTexture2D* EmblemTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_TitleBackdrop.T_RA4_TitleBackdrop")))
    {
        Emblem->SetBrushFromTexture(EmblemTexture, false);
    }
    EmblemFrame->AddChild(Emblem);
    LobbyInfo->AddChildToVerticalBox(EmblemFrame)->SetPadding(FMargin(0.0f, 12.0f));
    LobbyInfo->AddChildToVerticalBox(MakeLobbyText(
        WidgetTree, LOCTEXT("GameMode", "РЕЖИМ ИГРЫ\nСХВАТКА\n\nПОБЕДНЫЕ УСЛОВИЯ\nУНИЧТОЖИТЬ ВСЕХ ПРОТИВНИКОВ\n\nНАСТРОЙКИ ЛОББИ\nДРУЖЕСКИЙ ОГОНЬ       ВЫКЛ.\nОГРАНИЧЕНИЕ ВРЕМЕНИ   60 МИН.\nНАБЛЮДАТЕЛИ            ВКЛ."),
        15, LobbyMuted, TEXT("LobbyRules"), false))->SetPadding(FMargin(0.0f, 12.0f));
    PlaceLobbyWidget(Canvas, MakeLobbyPanel(
        WidgetTree, LobbyInfo, TEXT("LobbyInfoPanel"), ERA4PanelRole::Standard),
        FVector2D(18.0f, 28.0f), FVector2D(330.0f, 820.0f), 5);

    UHorizontalBox* ListHeader = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("PlayerListHeader"));
    const FText Headers[] = {
        LOCTEXT("HeaderSlot", "№"), LOCTEXT("HeaderPlayer", "ИГРОК"),
        LOCTEXT("HeaderTeam", "КОМАНДА"), LOCTEXT("HeaderBloc", "БЛОК / КАТЕГОРИЯ"),
        LOCTEXT("HeaderCountry", "СТРАНА"), LOCTEXT("HeaderDoctrine", "ДОКТРИНА"),
        LOCTEXT("HeaderReady", "ГОТОВНОСТЬ")
    };
    const float Widths[] = {32.0f, 160.0f, 55.0f, 180.0f, 130.0f, 180.0f, 95.0f};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Headers); ++Index)
    {
        UTextBlock* Header = MakeLobbyText(
            WidgetTree, Headers[Index], 13, LobbyMuted,
            FName(*FString::Printf(TEXT("PlayerHeader_%d"), Index)), false);
        Header->SetMinDesiredWidth(Widths[Index]);
        ListHeader->AddChildToHorizontalBox(Header)->SetPadding(FMargin(4.0f));
    }
    PlaceLobbyWidget(Canvas, ListHeader, FVector2D(380.0f, 130.0f), FVector2D(960.0f, 42.0f), 6);

    PlayerList = WidgetTree->ConstructWidget<URA4LobbyPlayerListView>(
        URA4LobbyPlayerListView::StaticClass(), TEXT("LobbyPlayerList"));
    // UListView only accepts a Blueprint-generated entry class. Handing it the
    // raw C++ class made the engine reject every row, so the list rendered empty
    // while still reporting eight items. WBP_RA4_LobbyPlayerRow is a thin
    // Blueprint over that same C++ row: layout and binding stay in C++.
    if (UClass* RowClass = LoadClass<UUserWidget>(
        nullptr,
        TEXT("/Game/RA4UI/Widgets/WBP_RA4_LobbyPlayerRow.WBP_RA4_LobbyPlayerRow_C")))
    {
        PlayerList->ConfigureEntryWidgetClass(RowClass);
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("RA4 lobby: WBP_RA4_LobbyPlayerRow is missing, the player list will be empty."));
    }
    PlayerList->SetSelectionMode(ESelectionMode::Single);
    PlayerList->SetScrollbarVisibility(ESlateVisibility::Collapsed);
    PlaceLobbyWidget(Canvas, MakeLobbyPanel(
        WidgetTree, PlayerList, TEXT("LobbyPlayerListPanel"), ERA4PanelRole::Compact,
        TEXT("/Game/RA4UI/Art/T_RA4_Frame_PanelV2.T_RA4_Frame_PanelV2")),
        FVector2D(365.0f, 168.0f), FVector2D(980.0f, 520.0f), 6);
    PopulatePlayerList();

    UVerticalBox* Chat = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("LobbyChat"));
    UScrollBox* ChatHistory = WidgetTree->ConstructWidget<UScrollBox>(
        UScrollBox::StaticClass(), TEXT("LobbyChatHistory"));
    for (int32 Index = 0; Index < LobbyViewModel->GetChatMessages().Num(); ++Index)
    {
        const FRA4LobbyChatMessageView& Message = LobbyViewModel->GetChatMessages()[Index];
        UTextBlock* ChatLine = MakeLobbyText(
            WidgetTree,
            FText::Format(LOCTEXT("ChatLine", "{0}: {1}"), Message.Author, Message.Message),
            14, Message.AuthorColor,
            FName(*FString::Printf(TEXT("ChatLine_%d"), Index)), false);
        ChatHistory->AddChild(ChatLine);
    }
    UVerticalBoxSlot* HistorySlot = Chat->AddChildToVerticalBox(ChatHistory);
    HistorySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UHorizontalBox* ChatComposer = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("ChatComposer"));
    ChatInput = WidgetTree->ConstructWidget<UEditableTextBox>(
        UEditableTextBox::StaticClass(), TEXT("ChatInput"));
    ChatInput->SetHintText(LOCTEXT("ChatHint", "НАПИСАТЬ СООБЩЕНИЕ…"));
    UHorizontalBoxSlot* ChatInputSlot = ChatComposer->AddChildToHorizontalBox(ChatInput);
    ChatInputSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UButton* SendButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("SendChatButton"));
    SendButton->SetStyle(MakeLobbyButtonStyle());
    SendButton->AddChild(MakeLobbyText(
        WidgetTree, LOCTEXT("Send", "ОТПРАВИТЬ"), 15,
        LobbyText, TEXT("SendChatLabel")));
    SendButton->OnClicked.AddDynamic(this, &URA4LobbyScreenWidget::SendChat);
    ChatComposer->AddChildToHorizontalBox(SendButton)->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
    Chat->AddChildToVerticalBox(ChatComposer)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
    PlaceLobbyWidget(Canvas, MakeLobbyPanel(
        WidgetTree, Chat, TEXT("LobbyChatPanel"), ERA4PanelRole::Compact,
        TEXT("/Game/RA4UI/Art/T_RA4_UI_ChatFrame.T_RA4_UI_ChatFrame")),
        FVector2D(365.0f, 700.0f), FVector2D(980.0f, 215.0f), 6);

    UVerticalBox* MapAndSettings = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("LobbyMapAndSettings"));
    MapAndSettings->AddChildToVerticalBox(MakeLobbyText(
        WidgetTree, LOCTEXT("MapHeading", "КАРТА\nАРХИПЕЛАГ «ТИФОН»"), 18,
        LobbyText, TEXT("MapHeading")));
    UImage* MapPreview = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), TEXT("LobbyMapPreview"));
    if (UTexture2D* MapTexture = LoadObject<UTexture2D>(
        nullptr, TEXT("/Game/RA4UI/Art/T_RA4_Allies_ArcticFleet.T_RA4_Allies_ArcticFleet")))
    {
        MapPreview->SetBrushFromTexture(MapTexture, false);
    }
    USizeBox* MapPreviewFrame = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("LobbyMapPreviewFrame"));
    MapPreviewFrame->SetHeightOverride(250.0f);
    MapPreviewFrame->AddChild(MapPreview);
    MapAndSettings->AddChildToVerticalBox(MapPreviewFrame)->SetPadding(FMargin(0.0f, 12.0f));
    MapAndSettings->AddChildToVerticalBox(MakeLobbyText(
        WidgetTree, LOCTEXT("MapMeta", "РАЗМЕР КАРТЫ                 БОЛЬШАЯ (8 ИГРОКОВ)\nТИП ЛАНДШАФТА               ЗИМНИЙ"),
        14, LobbyMuted, TEXT("MapMeta"), false));
    MapAndSettings->AddChildToVerticalBox(MakeLobbyText(
        WidgetTree, LOCTEXT("MatchSettings", "\nНАСТРОЙКИ МАТЧА\n\nНАЧАЛЬНЫЕ РЕСУРСЫ       СРЕДНИЕ\nДОХОД                    СРЕДНИЙ\nСКОРОСТЬ ИГРЫ            НОРМАЛЬНО\nТЕХНОЛОГИИ               ВСЕ ВКЛ.\nСУПЕРОРУЖИЕ              ВКЛ.\nРЕЖИМ ИГРЫ               СХВАТКА\n\nНАБЛЮДАТЕЛИ (1)\n●  Observer_01          ПИНГ: 48"),
        15, LobbyText, TEXT("MatchSettings"), false));
    PlaceLobbyWidget(Canvas, MakeLobbyPanel(
        WidgetTree, MapAndSettings, TEXT("LobbyMapPanel"), ERA4PanelRole::Standard,
        TEXT("/Game/RA4UI/Art/T_RA4_UI_MapPreviewFrame.T_RA4_UI_MapPreviewFrame")),
        FVector2D(1370.0f, 55.0f), FVector2D(520.0f, 795.0f), 6);

    UButton* LeaveButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("LeaveLobbyButton"));
    LeaveButton->SetStyle(MakeLobbyButtonStyle());
    UTextBlock* LeaveLabel = MakeLobbyText(
        WidgetTree, LOCTEXT("LeaveLobby", "ПОКИНУТЬ ЛОББИ"), 17,
        LobbyText, TEXT("LeaveLobbyLabel"));
    LeaveLabel->SetJustification(ETextJustify::Center);
    LeaveButton->AddChild(LeaveLabel);
    LeaveButton->OnClicked.AddDynamic(this, &URA4LobbyScreenWidget::LeaveLobby);
    PlaceLobbyWidget(Canvas, LeaveButton, FVector2D(25.0f, 885.0f), FVector2D(310.0f, 78.0f), 8);

    StartButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("StartBattleButton"));
    StartButton->SetStyle(MakeLobbyButtonStyle(true));
    UTextBlock* StartLabel = MakeLobbyText(
        WidgetTree, LOCTEXT("StartBattle", "НАЧАТЬ БИТВУ"), 30,
        FLinearColor::White, TEXT("StartBattleLabel"));
    StartLabel->SetJustification(ETextJustify::Center);
    StartButton->AddChild(StartLabel);
    StartButton->OnClicked.AddDynamic(this, &URA4LobbyScreenWidget::StartMatch);
    PlaceLobbyWidget(Canvas, StartButton, FVector2D(720.0f, 935.0f), FVector2D(480.0f, 90.0f), 9);
    RefreshStartState();
    return RootWidget;
}

void URA4LobbyScreenWidget::PopulatePlayerList()
{
    if (!PlayerList || !LobbyViewModel)
    {
        return;
    }
    PlayerItems.Reset();
    PlayerList->ClearListItems();
    for (const FRA4LobbyPlayerView& Player : LobbyViewModel->GetPlayers())
    {
        URA4LobbyPlayerListItem* Item = NewObject<URA4LobbyPlayerListItem>(this);
        Item->SetData(Player);
        PlayerItems.Add(Item);
        PlayerList->AddItem(Item);
    }
}

void URA4LobbyScreenWidget::SendChat()
{
    if (LobbyViewModel && ChatInput && LobbyViewModel->SendChat(ChatInput->GetText().ToString()))
    {
        ChatInput->SetText(FText::GetEmpty());
    }
}

void URA4LobbyScreenWidget::StartMatch()
{
    if (LobbyViewModel && LobbyViewModel->StartMatch() && ReadyStatusText)
    {
        ReadyStatusText->SetText(LOCTEXT("Launching", "МАТЧ ЗАПУСКАЕТСЯ…"));
    }
}

void URA4LobbyScreenWidget::LeaveLobby()
{
    if (LobbyViewModel)
    {
        LobbyViewModel->LeaveLobby();
    }
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URA4MainMenuScreenWidget* MainMenu = CreateWidget<URA4MainMenuScreenWidget>(
            PlayerController, URA4MainMenuScreenWidget::StaticClass()))
        {
            MainMenu->AddToViewport(0);
            RemoveFromParent();
        }
    }
}

void URA4LobbyScreenWidget::RefreshStartState()
{
    if (ReadyStatusText && LobbyViewModel)
    {
        ReadyStatusText->SetText(LobbyViewModel->GetValidationMessage());
        ReadyStatusText->SetColorAndOpacity(FSlateColor(
            LobbyViewModel->CanStartMatch() ? LobbyGreen : LobbyAccent));
    }
    if (StartButton && LobbyViewModel)
    {
        StartButton->SetIsEnabled(LobbyViewModel->CanStartMatch());
    }
}

#undef LOCTEXT_NAMESPACE
