// Copyright (c) Red Alert 4 project.
#include "RA4HUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "RA4UIDataProviderSubsystem.h"

namespace
{
// Reference layout is 1672x941; the values below are that layout scaled to the
// project's 1920x1080 base. See docs/ui-reconstruction/SCREENSHOT_INVENTORY.md for
// why the reference is not already at base resolution.
constexpr float kBarHeight = 46.0f;
constexpr float kFieldSpacing = 22.0f;

const FLinearColor kTextNormal(0.85f, 0.88f, 0.92f);
const FLinearColor kTextDim(0.52f, 0.56f, 0.62f);
const FLinearColor kCredits(0.94f, 0.80f, 0.32f);
const FLinearColor kPowerOk(0.42f, 0.82f, 0.48f);
const FLinearColor kPowerLow(0.94f, 0.36f, 0.28f);

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
} // namespace

TSharedRef<SWidget> URA4ResourceBarWidget::RebuildWidget()
{
    if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),
                                                                          TEXT("ResourceRow"));
        WidgetTree->RootWidget = Row;

        // Widget names stay ASCII so they are readable in the widget reflector; the
        // caption itself is localizable text.
        auto AddField = [&](const TCHAR* BaseName, const FText& Caption,
                            TObjectPtr<UTextBlock>& OutValue, const FLinearColor& ValueColour)
        {
            const FString Base(BaseName);
            UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),
                                                                             *(Base + TEXT("Col")));
            UTextBlock* Title = MakeLabel(WidgetTree, *(Base + TEXT("Caption")), kTextDim, 9, false);
            Title->SetText(Caption);
            Column->AddChildToVerticalBox(Title);

            OutValue = MakeLabel(WidgetTree, *(Base + TEXT("Value")), ValueColour, 16, true);
            Column->AddChildToVerticalBox(OutValue);

            if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Column))
            {
                Slot->SetPadding(FMargin(0.0f, 0.0f, kFieldSpacing, 0.0f));
            }
        };

        AddField(TEXT("Credits"), NSLOCTEXT("RA4", "Res_Credits", "КРЕДИТЫ"), CreditsValue, kCredits);
        AddField(TEXT("Power"), NSLOCTEXT("RA4", "Res_Power", "ЭНЕРГИЯ"), PowerValue, kPowerOk);
        AddField(TEXT("Supply"), NSLOCTEXT("RA4", "Res_Supply", "ВОЙСКА"), SupplyValue, kTextNormal);
        AddField(TEXT("Timer"), NSLOCTEXT("RA4", "Res_Timer", "ВРЕМЯ"), TimerValue, kTextNormal);

        if (USizeBox* Sizer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BarSizer")))
        {
            Sizer->SetHeightOverride(kBarHeight);
            Sizer->AddChild(Row);
            WidgetTree->RootWidget = Sizer;
        }
    }
    return Super::RebuildWidget();
}

void URA4ResourceBarWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (URA4UIDataProviderSubsystem* Provider = GetProvider())
    {
        // Event driven: an idle base produces no interface work at all. A per-frame
        // poll here would invalidate Slate twenty times a second for nothing.
        ResourceChangeHandle = Provider->OnResourcesChanged.AddUObject(this, &URA4ResourceBarWidget::Refresh);
        Refresh();
    }
}

void URA4ResourceBarWidget::NativeDestruct()
{
    if (URA4UIDataProviderSubsystem* Provider = GetProvider())
    {
        Provider->OnResourcesChanged.Remove(ResourceChangeHandle);
    }
    ResourceChangeHandle.Reset();
    Super::NativeDestruct();
}

URA4UIDataProviderSubsystem* URA4ResourceBarWidget::GetProvider() const
{
    UWorld* World = GetWorld();
    return World != nullptr ? World->GetSubsystem<URA4UIDataProviderSubsystem>() : nullptr;
}

void URA4ResourceBarWidget::Refresh()
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    if (Provider == nullptr)
    {
        return;
    }

    if (CreditsValue != nullptr)
    {
        CreditsValue->SetText(FText::AsNumber(Provider->GetCredits()));
    }

    if (PowerValue != nullptr)
    {
        // Shown as produced/consumed rather than a single number: a player needs to
        // see how much headroom is left before the next building stalls the base.
        PowerValue->SetText(FText::Format(NSLOCTEXT("RA4", "PowerFormat", "{0} / {1}"),
                                          FText::AsNumber(Provider->GetPowerProduced()),
                                          FText::AsNumber(Provider->GetPowerConsumed())));
        PowerValue->SetColorAndOpacity(
            FSlateColor(Provider->IsPowerShortage() ? kPowerLow : kPowerOk));
    }

    if (SupplyValue != nullptr)
    {
        // The reference HUD shows "88 / 200", but the simulation has no population
        // cap yet. Rather than invent a limit, the field shows the fielded count and
        // is greyed until a cap actually exists.
        if (Provider->IsSupplyModelled())
        {
            SupplyValue->SetText(FText::Format(NSLOCTEXT("RA4", "SupplyFormat", "{0} / {1}"),
                                               FText::AsNumber(Provider->GetSupplyUsed()),
                                               FText::AsNumber(Provider->GetSupplyCap())));
            SupplyValue->SetColorAndOpacity(FSlateColor(kTextNormal));
        }
        else
        {
            SupplyValue->SetText(FText::AsNumber(Provider->GetSupplyUsed()));
            SupplyValue->SetColorAndOpacity(FSlateColor(kTextDim));
        }
    }

    if (TimerValue != nullptr)
    {
        const int32 Seconds = Provider->GetMatchElapsedSeconds();
        TimerValue->SetText(FText::FromString(
            FString::Printf(TEXT("%d:%02d"), Seconds / 60, Seconds % 60)));
    }
}
