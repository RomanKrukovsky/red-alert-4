// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RA4UIShowcaseGameMode.generated.h"

class UUserWidget;

/** Shows the UI prototype on the Entry map while gameplay maps are still authored. */
UCLASS()
class REDALERT4_API ARA4UIShowcaseGameMode : public AGameModeBase
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;

private:
    void ShowInterface(APlayerController* PlayerController);
    void CaptureInterfaceForQA();

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> ActiveRootWidget;

    FTimerHandle CaptureTimer;
    bool bCaptureScheduled = false;
};
