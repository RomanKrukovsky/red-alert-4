// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4ViewModelBase.h"
#include "RA4HUDViewModel.generated.h"

UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4HUDViewModel : public URA4ViewModelBase
{
    GENERATED_BODY()

public:
    URA4HUDViewModel();

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Economy")
    void SetCredits(int32 InCredits);
    
    UFUNCTION(BlueprintPure, Category = "ViewModel|Economy")
    int32 GetCredits() const;

    UFUNCTION(BlueprintCallable, Category = "ViewModel|Economy")
    void SetPower(int32 InPowerProvided, int32 InPowerDrained);

    UFUNCTION(BlueprintPure, Category = "ViewModel|Economy")
    float GetPowerRatio() const;

private:
    UPROPERTY(FieldNotify, Setter, Getter)
    int32 Credits;

    UPROPERTY(FieldNotify, Setter, Getter)
    float PowerRatio;
};
