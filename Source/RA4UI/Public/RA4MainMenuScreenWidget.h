// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4MainMenuViewModel.h"
#include "RA4ScreenRootWidget.h"
#include "RA4MainMenuScreenWidget.generated.h"

class UButton;
class UCanvasPanel;
class UImage;
class UVerticalBox;

/** Native command-centre main menu matching reference 2. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4MainMenuScreenWidget : public URA4ScreenRootWidget
{
    GENERATED_BODY()

public:
    URA4MainMenuScreenWidget(const FObjectInitializer& ObjectInitializer);

    const TArray<TObjectPtr<UButton>>& GetMenuButtons() const { return MenuButtons; }
    int32 GetSelectedMenuIndex() const;
    UImage* GetLogoImage() const { return LogoImage; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    UButton* CreateMenuButton(
        UVerticalBox* Menu,
        const FRA4MainMenuEntry& Entry,
        int32 Index);
    void BuildInformationCard(
        UCanvasPanel* Canvas,
        const FText& Heading,
        const FText& Body,
        const FVector2D& Position,
        const FVector2D& Size,
        FName Name);

    UFUNCTION()
    void OpenCampaign();

    UFUNCTION()
    void OpenMultiplayer();

    UFUNCTION()
    void OpenSkirmish();

    UFUNCTION()
    void OpenEditor();

    UFUNCTION()
    void OpenEncyclopedia();

    UFUNCTION()
    void OpenModifications();

    UFUNCTION()
    void OpenSettings();

    UFUNCTION()
    void ExitToSplash();

    UPROPERTY(Transient)
    TObjectPtr<URA4MainMenuViewModel> MainMenuViewModel;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UButton>> MenuButtons;

    UPROPERTY(Transient)
    TObjectPtr<UImage> LogoImage;
};
