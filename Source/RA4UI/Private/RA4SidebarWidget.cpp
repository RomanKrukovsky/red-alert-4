// Copyright (c) Red Alert 4 project.
#include "RA4SidebarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

#include "RA4UIDataProviderSubsystem.h"

namespace
{
constexpr float kRadarDesiredSize = 208.0f;
}

class SRA4RadarSlate final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SRA4RadarSlate) {}
    SLATE_END_ARGS()

    void Construct(const FArguments&, URA4RadarWidget* InOwner)
    {
        Owner = InOwner;
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(kRadarDesiredSize, kRadarDesiredSize);
    }

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                          const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                          int32 LayerId, const FWidgetStyle& InWidgetStyle,
                          bool bParentEnabled) const override
    {
        const FVector2D Size = AllottedGeometry.GetLocalSize();
        if (Size.X <= 0.0f || Size.Y <= 0.0f)
        {
            return LayerId;
        }

        const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId,
            AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform()),
            WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.018f, 0.028f, 0.040f, 1.0f));

        const FLinearColor GridColour(0.12f, 0.19f, 0.22f, 0.65f);
        for (int32 Division = 1; Division < 4; ++Division)
        {
            const float X = Size.X * float(Division) / 4.0f;
            const float Y = Size.Y * float(Division) / 4.0f;
            TArray<FVector2D> Vertical{FVector2D(X, 0.0f), FVector2D(X, Size.Y)};
            TArray<FVector2D> Horizontal{FVector2D(0.0f, Y), FVector2D(Size.X, Y)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
                                         Vertical, ESlateDrawEffect::None, GridColour, true, 1.0f);
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
                                         Horizontal, ESlateDrawEffect::None, GridColour, true, 1.0f);
        }

        const URA4RadarWidget* RadarOwner = Owner.Get();
        if (RadarOwner == nullptr)
        {
            return LayerId + 1;
        }

        const FVector2D MapSize = RadarOwner->GetMapSize();
        if (MapSize.X <= 0.0f || MapSize.Y <= 0.0f)
        {
            return LayerId + 1;
        }

        const int32 LocalPlayer = RadarOwner->GetLocalPlayer();
        for (const FRA4RadarMarker& Marker : RadarOwner->GetMarkers())
        {
            const float NormalizedX = FMath::Clamp(float(Marker.WorldPosition.X / MapSize.X), 0.0f, 1.0f);
            const float NormalizedY = FMath::Clamp(float(Marker.WorldPosition.Y / MapSize.Y), 0.0f, 1.0f);
            const FVector2D Centre(NormalizedX * Size.X, (1.0f - NormalizedY) * Size.Y);

            float MarkerExtent = 4.0f;
            FLinearColor Colour;
            if (Marker.Kind == ERA4RadarMarkerKind::Resource)
            {
                MarkerExtent = 3.0f;
                Colour = FLinearColor(0.96f, 0.78f, 0.20f, 1.0f);
            }
            else if (Marker.Owner == LocalPlayer)
            {
                MarkerExtent = Marker.Kind == ERA4RadarMarkerKind::Building ? 6.0f : 4.0f;
                Colour = FLinearColor(0.20f, 0.92f, 0.38f, 1.0f);
            }
            else
            {
                MarkerExtent = Marker.Kind == ERA4RadarMarkerKind::Building ? 6.0f : 4.0f;
                Colour = FLinearColor(0.96f, 0.22f, 0.16f, 1.0f);
            }

            if (Marker.bSelected)
            {
                const FVector2D OutlineSize(MarkerExtent + 4.0f, MarkerExtent + 4.0f);
                FSlateDrawElement::MakeBox(
                    OutDrawElements, LayerId + 2,
                    AllottedGeometry.ToPaintGeometry(
                        OutlineSize, FSlateLayoutTransform(Centre - OutlineSize * 0.5f)),
                    WhiteBrush, ESlateDrawEffect::None, FLinearColor::White);
            }

            const FVector2D MarkerSize(MarkerExtent, MarkerExtent);
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 3,
                AllottedGeometry.ToPaintGeometry(
                    MarkerSize, FSlateLayoutTransform(Centre - MarkerSize * 0.5f)),
                WhiteBrush, ESlateDrawEffect::None, Colour);
        }

        return LayerId + 3;
    }

    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry,
                                     const FPointerEvent& MouseEvent) override
    {
        if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
        {
            const FVector2D Size = MyGeometry.GetLocalSize();
            if (Size.X > 0.0f && Size.Y > 0.0f)
            {
                const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
                if (URA4RadarWidget* RadarOwner = Owner.Get())
                {
                    RadarOwner->HandleSlateClick(FVector2D(
                        FMath::Clamp(Local.X / Size.X, 0.0f, 1.0f),
                        FMath::Clamp(Local.Y / Size.Y, 0.0f, 1.0f)));
                }
            }
        }

        // Both mouse buttons belong to the radar while the pointer is over it.
        return FReply::Handled();
    }

private:
    TWeakObjectPtr<URA4RadarWidget> Owner;
};

namespace
{
// The sidebar is a fixed-width column, as in the originals: it does not reflow with
// resolution, it stays the same slice of screen so the cards keep their positions.
constexpr float kSidebarWidth = URA4SidebarWidget::SidebarWidth;
constexpr float kMinimapHeight = kRadarDesiredSize;
constexpr int32 kCardColumns = 2;

const FLinearColor kPanel(0.055f, 0.065f, 0.080f, 0.94f);
const FLinearColor kPanelDeep(0.030f, 0.036f, 0.046f, 0.96f);
const FLinearColor kTabIdle(0.10f, 0.12f, 0.15f, 1.0f);
const FLinearColor kTabActive(0.20f, 0.34f, 0.24f, 1.0f);
const FLinearColor kCardOk(0.13f, 0.17f, 0.14f, 1.0f);
const FLinearColor kCardBlocked(0.13f, 0.10f, 0.10f, 1.0f);
const FLinearColor kTextNormal(0.86f, 0.89f, 0.93f);
const FLinearColor kTextDim(0.50f, 0.55f, 0.61f);
const FLinearColor kCredits(0.94f, 0.80f, 0.32f);
const FLinearColor kPowerOk(0.42f, 0.82f, 0.48f);
const FLinearColor kPowerLow(0.94f, 0.36f, 0.28f);

// Mirrors ProductionCategory. Naval and Ability are omitted until the content has
// entries for them -- an empty tab is worse than no tab.
struct TabDef
{
    int32 Category;
    const TCHAR* Caption;
};
const TabDef kTabs[] = {
    {0, TEXT("СТР")},   // Structure
    {1, TEXT("ОБО")},   // Defense
    {2, TEXT("ПЕХ")},   // Infantry
    {3, TEXT("ТЕХ")},   // Vehicle
    {4, TEXT("АВИ")},   // Aircraft
};

UTextBlock* MakeLabel(UWidgetTree* Tree, FName Name, const FLinearColor& Colour, int32 Size, bool bBold)
{
    UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = Size;
    Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
    Text->SetFont(Font);
    Text->SetColorAndOpacity(FSlateColor(Colour));
    return Text;
}

void StyleButton(UButton* Button, const FLinearColor& Base)
{
    FButtonStyle Style = Button->GetStyle();
    Style.Normal.TintColor = FSlateColor(Base);
    Style.Hovered.TintColor = FSlateColor(Base * 1.35f);
    Style.Pressed.TintColor = FSlateColor(Base * 0.75f);
    Style.Disabled.TintColor = FSlateColor(Base * 0.55f);
    Button->SetStyle(Style);
}

FText BlockReasonText(ERA4BuildBlockReason Reason)
{
    switch (Reason)
    {
    case ERA4BuildBlockReason::InsufficientCredits:
        return NSLOCTEXT("RA4", "Block_Credits", "нет средств");
    case ERA4BuildBlockReason::MissingPrerequisite:
        return NSLOCTEXT("RA4", "Block_Prereq", "нет здания");
    case ERA4BuildBlockReason::NoProducer:
        return NSLOCTEXT("RA4", "Block_Producer", "нет завода");
    case ERA4BuildBlockReason::QueueFull:
        return NSLOCTEXT("RA4", "Block_Queue", "очередь полна");
    case ERA4BuildBlockReason::MatchOver:
        return NSLOCTEXT("RA4", "Block_Over", "матч окончен");
    default:
        return FText::GetEmpty();
    }
}
} // namespace

// ---------------------------------------------------------------------------
// URA4RadarWidget
// ---------------------------------------------------------------------------

TSharedRef<SWidget> URA4RadarWidget::RebuildWidget()
{
    RadarSlate = SNew(SRA4RadarSlate, this);
    return RadarSlate.ToSharedRef();
}

void URA4RadarWidget::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    RadarSlate.Reset();
}

URA4UIDataProviderSubsystem* URA4RadarWidget::GetProvider() const
{
    UWorld* World = GetWorld();
    return World != nullptr ? World->GetSubsystem<URA4UIDataProviderSubsystem>() : nullptr;
}

const TArray<FRA4RadarMarker>& URA4RadarWidget::GetMarkers() const
{
    static const TArray<FRA4RadarMarker> Empty;
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    return Provider != nullptr ? Provider->GetRadarMarkers() : Empty;
}

FVector2D URA4RadarWidget::GetMapSize() const
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    return Provider != nullptr ? Provider->GetRadarMapSize() : FVector2D::ZeroVector;
}

int32 URA4RadarWidget::GetLocalPlayer() const
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    return Provider != nullptr ? Provider->GetRadarLocalPlayer() : 0;
}

void URA4RadarWidget::HandleSlateClick(const FVector2D& NormalizedPosition)
{
    const FVector2D MapSize = GetMapSize();
    if (MapSize.X <= 0.0f || MapSize.Y <= 0.0f)
    {
        return;
    }

    OnRadarClicked.Broadcast(FVector2D(
        NormalizedPosition.X * MapSize.X,
        (1.0f - NormalizedPosition.Y) * MapSize.Y));
}

// ---------------------------------------------------------------------------
// URA4IndexedButton
// ---------------------------------------------------------------------------

void URA4IndexedButton::BindForwarding()
{
    OnClicked.AddDynamic(this, &URA4IndexedButton::HandleClicked);
}

void URA4IndexedButton::HandleClicked()
{
    OnIndexedClicked.Broadcast(Index);
}

// ---------------------------------------------------------------------------
// URA4SidebarWidget
// ---------------------------------------------------------------------------

TSharedRef<SWidget> URA4SidebarWidget::RebuildWidget()
{
    if (WidgetTree == nullptr || WidgetTree->RootWidget != nullptr)
    {
        return Super::RebuildWidget();
    }

    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));

    auto AddRow = [&](UWidget* Child, float Padding) -> UVerticalBoxSlot*
    {
        UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Child);
        if (Slot != nullptr)
        {
            Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, Padding));
            Slot->SetHorizontalAlignment(HAlign_Fill);
        }
        return Slot;
    };

    // --- minimap ------------------------------------------------------------
    {
        UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MinimapFrame"));
        Frame->SetBrushColor(kPanelDeep);
        Frame->SetPadding(FMargin(6.0f));

        RadarWidget = WidgetTree->ConstructWidget<URA4RadarWidget>(
            URA4RadarWidget::StaticClass(), TEXT("Radar"));
        RadarWidget->OnRadarClicked.AddUObject(this, &URA4SidebarWidget::HandleRadarClicked);
        Frame->AddChild(RadarWidget);

        USizeBox* Sizer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MinimapSizer"));
        Sizer->SetHeightOverride(kMinimapHeight);
        Sizer->AddChild(Frame);
        AddRow(Sizer, 6.0f);
    }

    // --- credits and power --------------------------------------------------
    {
        UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResourceFrame"));
        Frame->SetBrushColor(kPanelDeep);
        Frame->SetPadding(FMargin(8.0f, 6.0f));

        UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResStack"));

        CreditsText = MakeLabel(WidgetTree, TEXT("SidebarCredits"), kCredits, 18, true);
        CreditsText->SetText(FText::AsNumber(0));
        Stack->AddChildToVerticalBox(CreditsText);

        PowerText = MakeLabel(WidgetTree, TEXT("SidebarPower"), kPowerOk, 11, false);
        Stack->AddChildToVerticalBox(PowerText);

        Frame->AddChild(Stack);
        AddRow(Frame, 6.0f);
    }

    // --- selected object info card ("СПРАВКА ОБ ОБЪЕКТЕ") -------------------
    {
        UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SelectionFrame"));
        Frame->SetBrushColor(kPanelDeep);
        Frame->SetPadding(FMargin(8.0f, 6.0f));

        UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SelStack"));

        UTextBlock* Header = MakeLabel(WidgetTree, TEXT("SelHeader"), kTextDim, 9, true);
        Header->SetText(NSLOCTEXT("RA4", "Sidebar_SelHeader", "СПРАВКА ОБ ОБЪЕКТЕ"));
        Stack->AddChildToVerticalBox(Header);

        SelectionNameText = MakeLabel(WidgetTree, TEXT("SelName"), kTextNormal, 12, true);
        SelectionNameText->SetText(NSLOCTEXT("RA4", "Sidebar_NoSelection", "ОБЪЕКТ НЕ ВЫБРАН"));
        Stack->AddChildToVerticalBox(SelectionNameText);

        SelectionHealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("SelHealthBar"));
        SelectionHealthBar->SetPercent(1.0f);
        SelectionHealthBar->SetFillColorAndOpacity(kPowerOk);
        if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(SelectionHealthBar))
        {
            Slot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 2.0f));
        }

        SelectionHealthText = MakeLabel(WidgetTree, TEXT("SelHealthText"), kTextDim, 10, false);
        SelectionHealthText->SetText(FText::GetEmpty());
        Stack->AddChildToVerticalBox(SelectionHealthText);

        SelectionDetailsText = MakeLabel(WidgetTree, TEXT("SelDetails"), kTextDim, 9, false);
        SelectionDetailsText->SetText(NSLOCTEXT("RA4", "Sidebar_SelectionHint", "Кликните по юниту или зданию"));
        Stack->AddChildToVerticalBox(SelectionDetailsText);

        Frame->AddChild(Stack);
        AddRow(Frame, 6.0f);
    }

    // --- category tabs ------------------------------------------------------
    {
        UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),
                                                                             TEXT("TabRow"));
        TabButtons.Reset();
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(kTabs); ++Index)
        {
            URA4IndexedButton* Button = WidgetTree->ConstructWidget<URA4IndexedButton>(
                URA4IndexedButton::StaticClass(), *FString::Printf(TEXT("Tab%d"), Index));
            Button->SetIndex(Index);
            Button->BindForwarding();
            Button->OnIndexedClicked.AddUObject(this, &URA4SidebarWidget::HandleTabClicked);
            StyleButton(Button, kTabs[Index].Category == ActiveCategory ? kTabActive : kTabIdle);

            UTextBlock* Caption = MakeLabel(WidgetTree, *FString::Printf(TEXT("TabText%d"), Index), kTextNormal, 10,
                                            true);
            Caption->SetText(FText::FromString(kTabs[Index].Caption));
            Caption->SetJustification(ETextJustify::Center);
            Button->AddChild(Caption);

            if (UHorizontalBoxSlot* Slot = TabRow->AddChildToHorizontalBox(Button))
            {
                Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                Slot->SetPadding(FMargin(0.0f, 0.0f, 2.0f, 0.0f));
            }
            TabButtons.Add(Button);
        }
        AddRow(TabRow, 6.0f);
    }

    // --- build cards --------------------------------------------------------
    {
        CardGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("CardGrid"));
        CardGrid->SetSlotPadding(FMargin(2.0f));
        UVerticalBoxSlot* Slot = AddRow(CardGrid, 6.0f);
        if (Slot != nullptr)
        {
            // The card grid takes the leftover height so the queue stays pinned to the
            // bottom of the column instead of floating under a short list.
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }

    // --- production queue ---------------------------------------------------
    {
        UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QueueFrame"));
        Frame->SetBrushColor(kPanelDeep);
        Frame->SetPadding(FMargin(6.0f));

        QueueBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("QueueBox"));
        Frame->AddChild(QueueBox);
        AddRow(Frame, 0.0f);
    }

    UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SidebarBackground"));
    Background->SetBrushColor(kPanel);
    Background->SetPadding(FMargin(8.0f));
    Background->AddChild(Column);

    USizeBox* WidthBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SidebarWidth"));
    WidthBox->SetWidthOverride(kSidebarWidth);
    WidthBox->AddChild(Background);

    WidgetTree->RootWidget = WidthBox;
    return Super::RebuildWidget();
}

void URA4SidebarWidget::HandleRadarClicked(FVector2D WorldPosition)
{
    OnRadarClicked.Broadcast(WorldPosition);
}

void URA4SidebarWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (URA4UIDataProviderSubsystem* Provider = GetProvider())
    {
        ResourceChangeHandle = Provider->OnResourcesChanged.AddUObject(this, &URA4SidebarWidget::RefreshResources);
        ProductionChangeHandle = Provider->OnProductionChanged.AddUObject(this, &URA4SidebarWidget::RefreshCards);
        SelectionChangeHandle = Provider->OnSelectionChanged.AddUObject(this, &URA4SidebarWidget::RefreshSelection);
        RefreshResources();
        RefreshCards();
        RefreshSelection();
    }
}

void URA4SidebarWidget::NativeDestruct()
{
    if (URA4UIDataProviderSubsystem* Provider = GetProvider())
    {
        Provider->OnResourcesChanged.Remove(ResourceChangeHandle);
        Provider->OnProductionChanged.Remove(ProductionChangeHandle);
        Provider->OnSelectionChanged.Remove(SelectionChangeHandle);
    }
    ResourceChangeHandle.Reset();
    ProductionChangeHandle.Reset();
    SelectionChangeHandle.Reset();
    Super::NativeDestruct();
}

#include "RA4HUDViewModel.h"

void URA4SidebarWidget::RefreshSelection()
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    if (Provider == nullptr || SelectionNameText == nullptr)
    {
        return;
    }

    const URA4HUDViewModel* VM = Provider->GetHUDViewModel();
    if (VM == nullptr || VM->GetSelectionCount() == 0)
    {
        SelectionNameText->SetText(NSLOCTEXT("RA4", "Sidebar_NoSelection", "ОБЪЕКТ НЕ ВЫБРАН"));
        if (SelectionHealthText) SelectionHealthText->SetText(FText::GetEmpty());
        if (SelectionHealthBar) SelectionHealthBar->SetPercent(0.0f);
        if (SelectionDetailsText) SelectionDetailsText->SetText(NSLOCTEXT("RA4", "Sidebar_SelectionHint", "Кликните по юниту или зданию"));
    }
    else
    {
        SelectionNameText->SetText(FText::FromString(VM->GetPrimaryEntityName()));
        float HP = VM->GetSelectionHealthRatio();
        if (SelectionHealthBar)
        {
            SelectionHealthBar->SetPercent(HP);
            SelectionHealthBar->SetFillColorAndOpacity(HP > 0.5f ? kPowerOk : (HP > 0.2f ? kCredits : kPowerLow));
        }
        if (SelectionHealthText)
        {
            SelectionHealthText->SetText(FText::Format(NSLOCTEXT("RA4", "Sidebar_HPFormat", "ЗДОРОВЬЕ: {0}%"), FText::AsNumber(int32(HP * 100.0f))));
        }
        if (SelectionDetailsText)
        {
            if (VM->GetSelectionCount() > 1)
            {
                SelectionDetailsText->SetText(FText::Format(NSLOCTEXT("RA4", "Sidebar_MultiSelFormat", "Выбрано объектов: {0}"), FText::AsNumber(VM->GetSelectionCount())));
            }
            else
            {
                SelectionDetailsText->SetText(VM->IsPrimaryOwned() ? NSLOCTEXT("RA4", "Sidebar_Owned", "Союзный объект") : NSLOCTEXT("RA4", "Sidebar_Enemy", "Вражеский объект"));
            }
        }
    }
}

URA4UIDataProviderSubsystem* URA4SidebarWidget::GetProvider() const
{
    UWorld* World = GetWorld();
    return World != nullptr ? World->GetSubsystem<URA4UIDataProviderSubsystem>() : nullptr;
}

void URA4SidebarWidget::SetActiveCategory(int32 Category)
{
    if (ActiveCategory == Category)
    {
        return;
    }
    ActiveCategory = Category;

    for (int32 Index = 0; Index < TabButtons.Num() && Index < UE_ARRAY_COUNT(kTabs); ++Index)
    {
        if (TabButtons[Index] != nullptr)
        {
            StyleButton(TabButtons[Index], kTabs[Index].Category == ActiveCategory ? kTabActive : kTabIdle);
        }
    }
    RefreshCards();
}

void URA4SidebarWidget::HandleTabClicked(int32 TabIndex)
{
    if (TabButtons.IsValidIndex(TabIndex) && TabIndex < UE_ARRAY_COUNT(kTabs))
    {
        SetActiveCategory(kTabs[TabIndex].Category);
    }
}

void URA4SidebarWidget::HandleCardClicked(int32 CardIndex)
{
    if (CardContentIds.IsValidIndex(CardIndex))
    {
        OnBuildCardClicked.Broadcast(CardContentIds[CardIndex]);
    }
}

void URA4SidebarWidget::RefreshResources()
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    if (Provider == nullptr)
    {
        return;
    }

    if (CreditsText != nullptr)
    {
        CreditsText->SetText(FText::AsNumber(Provider->GetCredits()));
    }
    if (PowerText != nullptr)
    {
        PowerText->SetText(FText::Format(NSLOCTEXT("RA4", "Sidebar_PowerFormat", "ЭНЕРГИЯ  {0} / {1}"),
                                         FText::AsNumber(Provider->GetPowerProduced()),
                                         FText::AsNumber(Provider->GetPowerConsumed())));
        PowerText->SetColorAndOpacity(FSlateColor(Provider->IsPowerShortage() ? kPowerLow : kPowerOk));
    }
}

void URA4SidebarWidget::RefreshCards()
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    if (Provider == nullptr || CardGrid == nullptr || WidgetTree == nullptr)
    {
        return;
    }

    const TArray<FRA4BuildOption> Options = Provider->GetBuildOptionsForCategory(ActiveCategory);

    // Queue progress fires the same change event as availability, and rebuilding two
    // dozen buttons every time a bar advances one percent is real work for no visible
    // difference. Rebuild the grid only when what it shows actually changed.
    uint32 Signature = ::GetTypeHash(ActiveCategory);
    for (const FRA4BuildOption& Option : Options)
    {
        Signature = HashCombine(Signature, ::GetTypeHash(Option.ContentId));
        Signature = HashCombine(Signature, ::GetTypeHash(Option.bAvailable));
        Signature = HashCombine(Signature, ::GetTypeHash(uint8(Option.BlockReason)));
    }
    if (Signature == CardsSignature && CardButtons.Num() == Options.Num())
    {
        RefreshQueue();
        return;
    }
    CardsSignature = Signature;

    CardGrid->ClearChildren();
    CardButtons.Reset();
    CardContentIds.Reset();

    for (int32 Index = 0; Index < Options.Num(); ++Index)
    {
        const FRA4BuildOption& Option = Options[Index];

        URA4IndexedButton* Button = WidgetTree->ConstructWidget<URA4IndexedButton>(
            URA4IndexedButton::StaticClass(), *FString::Printf(TEXT("Card%d"), Index));
        Button->SetIndex(Index);
        Button->BindForwarding();
        Button->OnIndexedClicked.AddUObject(this, &URA4SidebarWidget::HandleCardClicked);
        StyleButton(Button, Option.bAvailable ? kCardOk : kCardBlocked);
        // Blocked cards stay clickable-looking but inert, and say why underneath --
        // a greyed-out card that gives no reason is the classic sidebar's one real
        // usability failure and there is no reason to reproduce it.
        Button->SetIsEnabled(Option.bAvailable);

        UVerticalBox* CardStack = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), *FString::Printf(TEXT("CardStack%d"), Index));

        UTextBlock* Name = MakeLabel(WidgetTree, *FString::Printf(TEXT("CardName%d"), Index), kTextNormal, 10, true);
        Name->SetText(Option.DisplayName);
        Name->SetJustification(ETextJustify::Center);
        Name->SetAutoWrapText(true);
        CardStack->AddChildToVerticalBox(Name);

        UTextBlock* Cost = MakeLabel(WidgetTree, *FString::Printf(TEXT("CardCost%d"), Index), kCredits, 10, false);
        Cost->SetText(FText::AsNumber(Option.Cost));
        Cost->SetJustification(ETextJustify::Center);
        CardStack->AddChildToVerticalBox(Cost);

        if (!Option.bAvailable)
        {
            UTextBlock* Reason =
                MakeLabel(WidgetTree, *FString::Printf(TEXT("CardWhy%d"), Index), kTextDim, 8, false);
            Reason->SetText(BlockReasonText(Option.BlockReason));
            Reason->SetJustification(ETextJustify::Center);
            Reason->SetAutoWrapText(true);
            CardStack->AddChildToVerticalBox(Reason);
        }

        Button->AddChild(CardStack);

        if (UUniformGridSlot* Slot = CardGrid->AddChildToUniformGrid(Button, Index / kCardColumns,
                                                                    Index % kCardColumns))
        {
            Slot->SetHorizontalAlignment(HAlign_Fill);
            Slot->SetVerticalAlignment(VAlign_Fill);
        }

        CardButtons.Add(Button);
        CardContentIds.Add(Option.ContentId);
    }

    RefreshQueue();
}

void URA4SidebarWidget::RefreshQueue()
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    if (Provider == nullptr || QueueBox == nullptr || WidgetTree == nullptr)
    {
        return;
    }

    QueueBox->ClearChildren();

    const TArray<FRA4ProductionEntry>& Queue = Provider->GetProductionQueue();
    if (Queue.Num() == 0)
    {
        UTextBlock* Idle = MakeLabel(WidgetTree, TEXT("QueueIdle"), kTextDim, 9, false);
        Idle->SetText(NSLOCTEXT("RA4", "Sidebar_QueueIdle", "ОЧЕРЕДЬ ПУСТА"));
        QueueBox->AddChildToVerticalBox(Idle);
        return;
    }

    for (int32 Index = 0; Index < Queue.Num(); ++Index)
    {
        const FRA4ProductionEntry& Entry = Queue[Index];

        UTextBlock* Line = MakeLabel(WidgetTree, *FString::Printf(TEXT("QueueLine%d"), Index), kTextNormal, 9, false);
        if (Entry.bAwaitingPlacement)
        {
            // The one queue state that needs the player to act, so it says so instead
            // of showing a finished bar and waiting to be understood.
            Line->SetText(FText::Format(NSLOCTEXT("RA4", "Sidebar_QueuePlace", "{0} — ВЫБЕРИТЕ МЕСТО"),
                                        Entry.DisplayName));
            Line->SetColorAndOpacity(FSlateColor(kPowerOk));
        }
        else if (Entry.bPaused)
        {
            Line->SetText(FText::Format(NSLOCTEXT("RA4", "Sidebar_QueuePaused", "{0} — ПАУЗА"), Entry.DisplayName));
            Line->SetColorAndOpacity(FSlateColor(kTextDim));
        }
        else
        {
            Line->SetText(FText::Format(NSLOCTEXT("RA4", "Sidebar_QueueLine", "{0}  {1}%"), Entry.DisplayName,
                                        FText::AsNumber(Entry.ProgressPercent)));
        }
        QueueBox->AddChildToVerticalBox(Line);

        UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(),
                                                                      *FString::Printf(TEXT("QueueBar%d"), Index));
        Bar->SetPercent(float(Entry.ProgressPercent) / 100.0f);
        Bar->SetFillColorAndOpacity(Entry.bPaused ? kTextDim : kPowerOk);
        if (UVerticalBoxSlot* Slot = QueueBox->AddChildToVerticalBox(Bar))
        {
            Slot->SetPadding(FMargin(0.0f, 1.0f, 0.0f, 4.0f));
        }
    }
}
