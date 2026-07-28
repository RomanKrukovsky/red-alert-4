// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4ViewModelBase.h"
#include "RA4MainMenuViewModel.generated.h"

UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4MainMenuViewModel : public URA4ViewModelBase
{
    GENERATED_BODY()

public:
    URA4MainMenuViewModel();

    // Field-notifying properties for MVVM data binding
    
    UFUNCTION(BlueprintCallable, Category = "ViewModel")
    void SetPlayerProfileName(const FString& InName);
    
    UFUNCTION(BlueprintPure, Category = "ViewModel")
    FString GetPlayerProfileName() const;

private:
    UPROPERTY(FieldNotify, Setter, Getter)
    FString PlayerProfileName;
};
