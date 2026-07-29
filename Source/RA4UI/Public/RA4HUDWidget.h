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

UCLASS(Abstract)
class RA4UI_API URA4SovietHUDWidget : public URA4HUDWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4AlliesHUDWidget : public URA4HUDWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4EasternHUDWidget : public URA4HUDWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4ChronoHUDWidget : public URA4HUDWidget
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

/** Resource counters and power state. Replaceable by an imported RTS kit widget. */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4ResourceBarWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** Category tabs above the unit and structure production grid. */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4ProductionTabsWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** One production item, including cost, prerequisites, progress, and disabled state. */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4ProductionCardWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** Selected unit portrait, health, veterancy, and contextual information. */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4SelectionPanelWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** Contextual unit orders and stance controls. */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4CommandGridWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** Timed tactical messages and economy warnings. */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4NotificationFeedWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** Current primary, secondary, and optional mission objectives. */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4ObjectivesWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** Full-width animated EVA alert overlay. */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4EVAAlertWidget : public URA4ActivatableWidget
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
