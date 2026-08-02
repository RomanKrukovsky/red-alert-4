// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RA4UIInputRouter.generated.h"

class UInputMappingContext;
class APlayerController;

UENUM(BlueprintType)
enum class ERA4UIInputMode : uint8
{
    GameOnly,
    UIOnly,
    GameAndUI
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRA4OnInputModeChanged, ERA4UIInputMode, NewMode, ERA4UIInputMode, OldMode);

/**
 * Handles GameOnly, UIOnly, and GameAndUI input modes with Enhanced Input context binding.
 * Controls viewport mouse locking, cursor visibility, and input mapping context stack.
 */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4UIInputRouter : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Switches active input mode (GameOnly, UIOnly, GameAndUI) for the target player controller. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Input")
    void SetInputMode(ERA4UIInputMode NewMode, APlayerController* TargetPC = nullptr);

    /** Returns current input mode. */
    UFUNCTION(BlueprintPure, Category = "RA4|UI|Input")
    ERA4UIInputMode GetCurrentInputMode() const { return CurrentInputMode; }

    /** Pushes an Enhanced Input Mapping Context with specified priority for UI actions. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Input")
    void PushInputContext(UInputMappingContext* Context, int32 Priority = 0, APlayerController* TargetPC = nullptr);

    /** Pops an Enhanced Input Mapping Context. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Input")
    void PopInputContext(UInputMappingContext* Context, APlayerController* TargetPC = nullptr);

    /** Clears all UI Enhanced Input Mapping Contexts managed by this subsystem. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Input")
    void ClearInputContexts(APlayerController* TargetPC = nullptr);

    /** Explicitly controls mouse cursor visibility. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Input")
    void SetMouseCursorVisible(bool bVisible, APlayerController* TargetPC = nullptr);

    UPROPERTY(BlueprintAssignable, Category = "RA4|UI|Input")
    FRA4OnInputModeChanged OnInputModeChanged;

private:
    APlayerController* ResolvePlayerController(APlayerController* InPC) const;

    UPROPERTY(Transient)
    ERA4UIInputMode CurrentInputMode = ERA4UIInputMode::GameOnly;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UInputMappingContext>> ActiveUIContexts;
};
