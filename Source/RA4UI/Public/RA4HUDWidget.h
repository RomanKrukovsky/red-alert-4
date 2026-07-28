// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4ActivatableWidget.h"
#include "RA4HUDWidget.generated.h"

// ---------------------------------------------------------
// In-Game HUD Screens (C++ logic bindings for Widget Blueprints)
// ---------------------------------------------------------

/** Base class for the main in-game HUD covering the whole screen. */
UCLASS(Abstract)
class RA4UI_API URA4HUDWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** Container for the minimap rendering. */
UCLASS(Abstract)
class RA4UI_API URA4MinimapWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** Production queue container. */
UCLASS(Abstract)
class RA4UI_API URA4ProductionQueueWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** In-Game Pause menu. */
UCLASS(Abstract)
class RA4UI_API URA4PauseWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** In-Game Victory / Defeat screen. */
UCLASS(Abstract)
class RA4UI_API URA4MatchResultWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};
