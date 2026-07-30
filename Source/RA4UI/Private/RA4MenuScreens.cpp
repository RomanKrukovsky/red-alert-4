// Copyright (c) Red Alert 4 project.

#include "RA4MenuScreens.h"

void URA4RoutedMenuScreenWidget::NativeOnActivated()
{
    Super::NativeOnActivated();
    NavigateToScreen(GetScreenId());
}

// Empty implementations as these are currently just routing bases for UMG widgets.
