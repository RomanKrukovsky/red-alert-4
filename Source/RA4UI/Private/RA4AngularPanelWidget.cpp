// Copyright (c) Red Alert 4 project.

#include "RA4AngularPanelWidget.h"

#include "Brushes/SlateColorBrush.h"
#include "RA4UITheme.h"

namespace
{
constexpr FLinearColor NeutralPanelColor(0.008f, 0.012f, 0.020f, 0.92f);
}

URA4AngularPanelWidget::URA4AngularPanelWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetPanelRole(PanelRole);
    SetBrush(FSlateColorBrush(NeutralPanelColor));
}

void URA4AngularPanelWidget::SetTheme(const URA4UITheme* Theme)
{
    if (Theme)
    {
        SetBrush(Theme->PanelBrush);
        return;
    }

    SetBrush(FSlateColorBrush(NeutralPanelColor));
}

void URA4AngularPanelWidget::SetPanelRole(const ERA4PanelRole Role)
{
    PanelRole = Role;

    switch (Role)
    {
    case ERA4PanelRole::Compact:
        SetPadding(FMargin(8.0f));
        break;
    case ERA4PanelRole::Standard:
        SetPadding(FMargin(16.0f));
        break;
    case ERA4PanelRole::DenseHUD:
        SetPadding(FMargin(10.0f));
        break;
    case ERA4PanelRole::Hero:
        SetPadding(FMargin(24.0f));
        break;
    default:
        checkNoEntry();
        SetPadding(FMargin(16.0f));
        break;
    }
}
