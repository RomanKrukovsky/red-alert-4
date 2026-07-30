// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4ActivatableWidget.h"
#include "RA4MenuScreens.generated.h"

// ---------------------------------------------------------
// Meta-game Screens (C++ logic bindings for Widget Blueprints)
// ---------------------------------------------------------

UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4RoutedMenuScreenWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const PURE_VIRTUAL(URA4RoutedMenuScreenWidget::GetScreenId, return ERA4UIScreenId::Splash;);
    virtual void NativeOnActivated() override;
};

UCLASS(Abstract)
class RA4UI_API URA4SplashWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::Splash; }
};

UCLASS(Abstract)
class RA4UI_API URA4MainMenuWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::MainMenu; }
};

UCLASS(Abstract)
class RA4UI_API URA4CampaignWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::CampaignSelect; }
};

UCLASS(Abstract)
class RA4UI_API URA4FactionSelectWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::CampaignSelect; }
};

UCLASS(Abstract)
class RA4UI_API URA4MissionMapWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::MissionMap; }
};

UCLASS(Abstract)
class RA4UI_API URA4BriefingWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::Briefing; }
};

UCLASS(Abstract)
class RA4UI_API URA4VideoCommsWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::VideoComms; }
};

UCLASS(Abstract)
class RA4UI_API URA4LoadingWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::Loading; }
};

UCLASS(Abstract)
class RA4UI_API URA4LobbyWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::MultiplayerLobby; }
};

UCLASS(Abstract)
class RA4UI_API URA4EncyclopediaWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::Encyclopedia; }
};

UCLASS(Abstract)
class RA4UI_API URA4TechTreeWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::TechTree; }
};

UCLASS(Abstract)
class RA4UI_API URA4SettingsWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::Settings; }
};

UCLASS(Abstract)
class RA4UI_API URA4ModsWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::Mods; }
};

UCLASS(Abstract)
class RA4UI_API URA4SovietCampaignWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::SovietCampaign; }
};

UCLASS(Abstract)
class RA4UI_API URA4AlliesCampaignWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::AlliesCampaign; }
};

UCLASS(Abstract)
class RA4UI_API URA4EasternCampaignWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::EasternCampaign; }
};

UCLASS(Abstract)
class RA4UI_API URA4ChronoCampaignWidget : public URA4RoutedMenuScreenWidget
{
    GENERATED_BODY()

protected:
    virtual ERA4UIScreenId GetScreenId() const override { return ERA4UIScreenId::ChronoCampaign; }
};


