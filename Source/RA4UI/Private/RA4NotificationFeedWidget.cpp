// Copyright (c) Red Alert 4 project.
//
// EVA alert feed from the SC-20 reference: a stack of severity-coloured rows in
// the top-left area under the resource bar. Each row shows the alert message and,
// when the same alert fires repeatedly, a xN counter instead of a duplicate row.
// The newest alert briefly flashes to pull the eye without a full-screen effect.
//
// Data comes from URA4UIDataProviderSubsystem::GetAlerts(), which is already
// fog-filtered and content-compared upstream, so this widget rebuilds only when
// OnAlertsChanged actually fires.
#include "RA4HUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "RA4UIDataProviderSubsystem.h"

namespace
{
constexpr int32 kMaxVisibleAlerts = 5;
constexpr float kFlashDuration = 1.2f;

const FLinearColor kRowPanel(0.03f, 0.04f, 0.05f, 0.90f);
const FLinearColor kTextNormal(0.85f, 0.88f, 0.92f);
const FLinearColor kInfoAccent(0.42f, 0.66f, 0.88f);
const FLinearColor kWarnAccent(0.95f, 0.75f, 0.15f);
const FLinearColor kCriticalAccent(0.94f, 0.22f, 0.16f);

const FLinearColor& SeverityAccent(const ERA4AlertSeverity Severity)
{
    switch (Severity)
    {
    case ERA4AlertSeverity::Critical:
        return kCriticalAccent;
    case ERA4AlertSeverity::Warning:
        return kWarnAccent;
    case ERA4AlertSeverity::Info:
        return kInfoAccent;
    default:
        return kInfoAccent;
    }
}

UTextBlock* MakeFeedLabel(UWidgetTree* Tree, const FName Name, const FLinearColor& Colour,
                          const int32 Size, const bool bBold)
{
    UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    FSlateFontInfo Font = Label->GetFont();
    Font.Size = Size;
    Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
    Label->SetFont(Font);
    Label->SetColorAndOpacity(FSlateColor(Colour));
    return Label;
}
} // namespace

TSharedRef<SWidget> URA4NotificationFeedWidget::RebuildWidget()
{
    if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
    {
        FeedBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FeedBox"));
        WidgetTree->RootWidget = FeedBox;
    }
    return Super::RebuildWidget();
}

void URA4NotificationFeedWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (URA4UIDataProviderSubsystem* Provider = GetProvider())
    {
        AlertsChangeHandle = Provider->OnAlertsChanged.AddUObject(this, &URA4NotificationFeedWidget::Refresh);
        Refresh();
    }
}

void URA4NotificationFeedWidget::NativeDestruct()
{
    if (URA4UIDataProviderSubsystem* Provider = GetProvider())
    {
        Provider->OnAlertsChanged.Remove(AlertsChangeHandle);
    }
    AlertsChangeHandle.Reset();
    Super::NativeDestruct();
}

void URA4NotificationFeedWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bFlashActive || FeedBox == nullptr || FeedBox->GetChildrenCount() == 0)
    {
        return;
    }

    NewestAlertAge += InDeltaTime;
    UWidget* NewestRow = FeedBox->GetChildAt(0);
    if (NewestRow == nullptr)
    {
        bFlashActive = false;
        return;
    }

    if (NewestAlertAge >= kFlashDuration)
    {
        NewestRow->SetRenderOpacity(1.0f);
        bFlashActive = false;
        return;
    }

    // Two soft pulses over the flash window; settles at fully opaque.
    const float Phase = NewestAlertAge / kFlashDuration;
    const float Pulse = 0.72f + 0.28f * FMath::Cos(Phase * 2.0f * UE_PI * 2.0f);
    NewestRow->SetRenderOpacity(FMath::Lerp(Pulse, 1.0f, Phase));
}

URA4UIDataProviderSubsystem* URA4NotificationFeedWidget::GetProvider() const
{
    UWorld* World = GetWorld();
    return World != nullptr ? World->GetSubsystem<URA4UIDataProviderSubsystem>() : nullptr;
}

void URA4NotificationFeedWidget::TriggerAlertClick(int32 Index)
{
    if (AlertLocations.IsValidIndex(Index))
    {
        if (URA4UIDataProviderSubsystem* Provider = GetProvider())
        {
            Provider->RequestJumpToAlert(AlertLocations[Index]);
        }
    }
}

void URA4NotificationFeedWidget::OnAlert0Clicked() { TriggerAlertClick(0); }
void URA4NotificationFeedWidget::OnAlert1Clicked() { TriggerAlertClick(1); }
void URA4NotificationFeedWidget::OnAlert2Clicked() { TriggerAlertClick(2); }
void URA4NotificationFeedWidget::OnAlert3Clicked() { TriggerAlertClick(3); }
void URA4NotificationFeedWidget::OnAlert4Clicked() { TriggerAlertClick(4); }

void URA4NotificationFeedWidget::Refresh()
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    if (Provider == nullptr || FeedBox == nullptr || WidgetTree == nullptr)
    {
        return;
    }

    FeedBox->ClearChildren();
    AlertLocations.Reset();

    const TArray<FRA4Alert>& Alerts = Provider->GetAlerts();
    const int32 Count = FMath::Min(Alerts.Num(), kMaxVisibleAlerts);
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FRA4Alert& Alert = Alerts[Index];
        const FLinearColor& Accent = SeverityAccent(Alert.Severity);
        const FString Base = FString::Printf(TEXT("Alert%d"), Index);

        // Severity is shown as a coloured edge strip on the left of the row
        UBorder* EdgeStrip = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), *(Base + TEXT("Edge")));
        EdgeStrip->SetBrushColor(Accent);
        USizeBox* EdgeWidth = WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(), *(Base + TEXT("EdgeWidth")));
        EdgeWidth->SetWidthOverride(4.0f);
        EdgeWidth->AddChild(EdgeStrip);

        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(), *(Base + TEXT("Row")));
        Row->AddChildToHorizontalBox(EdgeWidth);

        UTextBlock* Message = MakeFeedLabel(
            WidgetTree, *(Base + TEXT("Message")), kTextNormal, 12,
            Alert.Severity == ERA4AlertSeverity::Critical);
        Message->SetText(Alert.Message);
        if (UHorizontalBoxSlot* MessageSlot = Row->AddChildToHorizontalBox(Message))
        {
            MessageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            MessageSlot->SetPadding(FMargin(8.0f, 4.0f));
            MessageSlot->SetVerticalAlignment(VAlign_Center);
        }

        if (Alert.RepeatCount > 1)
        {
            UTextBlock* Repeat = MakeFeedLabel(
                WidgetTree, *(Base + TEXT("Repeat")), Accent, 12, true);
            Repeat->SetText(FText::Format(
                NSLOCTEXT("RA4", "AlertRepeatFormat", "x{0}"), FText::AsNumber(Alert.RepeatCount)));
            if (UHorizontalBoxSlot* RepeatSlot = Row->AddChildToHorizontalBox(Repeat))
            {
                RepeatSlot->SetPadding(FMargin(0.0f, 4.0f, 8.0f, 4.0f));
                RepeatSlot->SetVerticalAlignment(VAlign_Center);
            }
        }

        UBorder* RowPanel = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), *(Base + TEXT("Panel")));
        RowPanel->SetBrushColor(kRowPanel);
        RowPanel->SetPadding(FMargin(0.0f));
        RowPanel->SetContent(Row);

        UButton* RowButton = WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(), *(Base + TEXT("Button")));
        FButtonStyle Style = RowButton->GetStyle();
        Style.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
        Style.Hovered.DrawAs = ESlateBrushDrawType::Box;
        Style.Hovered.TintColor = FSlateColor(FLinearColor(0.2f, 0.4f, 0.6f, 0.35f));
        Style.Pressed.DrawAs = ESlateBrushDrawType::Box;
        Style.Pressed.TintColor = FSlateColor(FLinearColor(0.3f, 0.6f, 0.9f, 0.6f));
        RowButton->SetStyle(Style);
        RowButton->SetContent(RowPanel);

        if (Alert.bHasLocation)
        {
            AlertLocations.Add(Alert.WorldLocation);
            if (Index == 0) RowButton->OnClicked.AddDynamic(this, &URA4NotificationFeedWidget::OnAlert0Clicked);
            else if (Index == 1) RowButton->OnClicked.AddDynamic(this, &URA4NotificationFeedWidget::OnAlert1Clicked);
            else if (Index == 2) RowButton->OnClicked.AddDynamic(this, &URA4NotificationFeedWidget::OnAlert2Clicked);
            else if (Index == 3) RowButton->OnClicked.AddDynamic(this, &URA4NotificationFeedWidget::OnAlert3Clicked);
            else if (Index == 4) RowButton->OnClicked.AddDynamic(this, &URA4NotificationFeedWidget::OnAlert4Clicked);
        }
        else
        {
            AlertLocations.Add(FVector2D::ZeroVector);
        }

        if (UVerticalBoxSlot* RowSlot = FeedBox->AddChildToVerticalBox(RowButton))
        {
            RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
            RowSlot->SetHorizontalAlignment(HAlign_Fill);
        }
    }

    // Flash only when something new arrived, not when an old alert merely aged out.
    if (Count > 0)
    {
        NewestAlertAge = 0.0f;
        bFlashActive = true;
    }
}
