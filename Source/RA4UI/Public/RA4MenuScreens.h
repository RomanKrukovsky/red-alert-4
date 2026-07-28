// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4ActivatableWidget.h"
#include "RA4MenuScreens.generated.h"

// ---------------------------------------------------------
// Meta-game Screens (C++ logic bindings for Widget Blueprints)
// ---------------------------------------------------------

UCLASS(Abstract)
class RA4UI_API URA4SplashWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4MainMenuWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4CampaignWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4FactionSelectWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4MissionMapWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4BriefingWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4VideoCommsWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4LoadingWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4LobbyWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4EncyclopediaWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4TechTreeWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4SettingsWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4ModsWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};
