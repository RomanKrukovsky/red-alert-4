// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RA4UIScreenViewModel.h"
#include "RA4UIScreenHost.generated.h"

class UUserWidget;

/**
 * The missing link between the router and the viewport.
 *
 * The router (URA4UIRouterSubsystem) updates the screen view-model and fires
 * OnScreenChanged, but nothing listened — so Splash->MainMenu->Campaign
 * navigation updated a number without ever swapping the widget. This host
 * subscribes to OnScreenChanged and instantiates the matching C++ widget
 * for every menu screen, removing the previous one.
 *
 * Only menu-family screens are owned here. In-game HUDs (EurasianHud etc.)
 * are driven by the match controller, not menu navigation; calling NavigateTo
 * on them leaves the host alone (ResolveWidgetClassForScreen returns null).
 */
UCLASS()
class RA4UI_API URA4UIScreenHost : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Shows the initial menu screen (Splash) on the given controller. */
    void ShowInitialScreen(APlayerController* PlayerController);

    /** Swaps the current menu widget to the one matching ScreenId. */
    void SwapToScreen(APlayerController* PlayerController, ERA4UIScreenId ScreenId);

protected:
    /** Resolves the concrete widget class for a menu screen id, or null. */
    TSubclassOf<UUserWidget> ResolveWidgetClassForScreen(ERA4UIScreenId ScreenId) const;

private:
    UFUNCTION()
    void HandleScreenChanged(ERA4UIScreenId NewScreen);

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> ActiveMenuWidget;
};