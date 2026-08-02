// Copyright (c) Red Alert 4 project.

#include "RA4UIInputRouter.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void URA4UIInputRouter::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CurrentInputMode = ERA4UIInputMode::GameOnly;
    ActiveUIContexts.Reset();
}

void URA4UIInputRouter::Deinitialize()
{
    ActiveUIContexts.Reset();
    Super::Deinitialize();
}

APlayerController* URA4UIInputRouter::ResolvePlayerController(APlayerController* InPC) const
{
    if (InPC != nullptr)
    {
        return InPC;
    }

    if (UGameInstance* GI = GetGameInstance())
    {
        return GI->GetFirstLocalPlayerController();
    }

    return nullptr;
}

void URA4UIInputRouter::SetInputMode(ERA4UIInputMode NewMode, APlayerController* TargetPC)
{
    APlayerController* PC = ResolvePlayerController(TargetPC);
    if (!PC)
    {
        return;
    }

    ERA4UIInputMode OldMode = CurrentInputMode;
    CurrentInputMode = NewMode;

    switch (NewMode)
    {
    case ERA4UIInputMode::GameOnly:
    {
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = false;
        break;
    }
    case ERA4UIInputMode::UIOnly:
    {
        FInputModeUIOnly InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
        break;
    }
    case ERA4UIInputMode::GameAndUI:
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
        break;
    }
    }

    if (OldMode != NewMode)
    {
        OnInputModeChanged.Broadcast(NewMode, OldMode);
    }
}

void URA4UIInputRouter::PushInputContext(UInputMappingContext* Context, int32 Priority, APlayerController* TargetPC)
{
    if (!Context)
    {
        return;
    }

    APlayerController* PC = ResolvePlayerController(TargetPC);
    if (!PC)
    {
        return;
    }

    if (ULocalPlayer* LP = PC->GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* EISubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
        {
            EISubsystem->AddMappingContext(Context, Priority);
            ActiveUIContexts.AddUnique(Context);
        }
    }
}

void URA4UIInputRouter::PopInputContext(UInputMappingContext* Context, APlayerController* TargetPC)
{
    if (!Context)
    {
        return;
    }

    APlayerController* PC = ResolvePlayerController(TargetPC);
    if (!PC)
    {
        return;
    }

    if (ULocalPlayer* LP = PC->GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* EISubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
        {
            EISubsystem->RemoveMappingContext(Context);
            ActiveUIContexts.Remove(Context);
        }
    }
}

void URA4UIInputRouter::ClearInputContexts(APlayerController* TargetPC)
{
    APlayerController* PC = ResolvePlayerController(TargetPC);
    if (!PC)
    {
        return;
    }

    if (ULocalPlayer* LP = PC->GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* EISubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
        {
            for (UInputMappingContext* Context : ActiveUIContexts)
            {
                if (Context)
                {
                    EISubsystem->RemoveMappingContext(Context);
                }
            }
        }
    }

    ActiveUIContexts.Reset();
}

void URA4UIInputRouter::SetMouseCursorVisible(bool bVisible, APlayerController* TargetPC)
{
    if (APlayerController* PC = ResolvePlayerController(TargetPC))
    {
        PC->bShowMouseCursor = bVisible;
    }
}
