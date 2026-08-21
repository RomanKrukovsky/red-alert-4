// Copyright (c) Red Alert 4 project. Direct-control presentation subsystem.
#include "RA4DirectControlSubsystem.h"

#include "RA4SimWorldSubsystem.h"
#include "RA4PlayerController.h"
#include "RA4EntityActor.h"

#include "Engine/AssetManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

URA4DirectControlSubsystem::URA4DirectControlSubsystem()
    : UWorldSubsystem()
{
}

void URA4DirectControlSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    SimSubsystem = GetWorld()->GetSubsystem<URA4SimWorldSubsystem>();
}

void URA4DirectControlSubsystem::Deinitialize()
{
    Super::Deinitialize();
    SimSubsystem = nullptr;
    ActiveProfile = nullptr;
}

void URA4DirectControlSubsystem::SetClientPhase(ERA4DirectControlClientPhase NewPhase)
{
    if (ClientPhase == NewPhase)
    {
        return;
    }
    ClientPhase = NewPhase;
}

void URA4DirectControlSubsystem::SubmitCommand(const RA4::Command& Cmd)
{
    if (SimSubsystem != nullptr)
    {
        SimSubsystem->EnqueueCommand(Cmd);
    }
}

RA4::DirectControlAxes URA4DirectControlSubsystem::QuantizeAxes(
    const FRA4DirectControlCameraSettings& Settings,
    float ThrottleRaw, float SteeringRaw,
    float TurretYawRaw, float TurretPitchRaw,
    uint8_t Flags)
{
    auto ApplyDeadZone = [](float V, float DeadZone) -> float
    {
        if (FMath::Abs(V) <= DeadZone) { return 0.0f; }
        const float Sign = V < 0.0f ? -1.0f : 1.0f;
        return Sign * FMath::Clamp((FMath::Abs(V) - DeadZone) / (1.0f - DeadZone), 0.0f, 1.0f);
    };

    RA4::DirectControlAxes Out;
    Out.Throttle = int8_t(FMath::Clamp(ApplyDeadZone(ThrottleRaw, Settings.InputDeadZone) * 127.0f, -127.0f, 127.0f));
    Out.Steering = int8_t(FMath::Clamp(ApplyDeadZone(SteeringRaw, Settings.InputDeadZone) * 127.0f, -127.0f, 127.0f));
    const float YawSign = Settings.bInvertPitch ? -1.0f : 1.0f; // pitch invert only
    Out.TurretYaw = int8_t(FMath::Clamp(TurretYawRaw * Settings.MouseSensitivityYaw * 32.0f, -127.0f, 127.0f));
    Out.TurretPitch = int8_t(FMath::Clamp(TurretPitchRaw * Settings.MouseSensitivityPitch * 32.0f * YawSign, -127.0f, 127.0f));
    Out.Flags = Flags;
    return Out;
}

URA4DirectControlProfile* URA4DirectControlSubsystem::ResolveProfile(RA4::ContentId VehicleContentId) const
{
    if (UAssetManager* Am = UAssetManager::GetIfInitialized())
    {
        TArray<FPrimaryAssetId> Found;
        Am->GetPrimaryAssetIdList(URA4DirectControlProfile::StaticClass()->GetFName(), Found);
        for (const FPrimaryAssetId& Id : Found)
        {
            if (UObject* Obj = Am->GetPrimaryAssetObject(Id))
            {
                if (URA4DirectControlProfile* Prof = Cast<URA4DirectControlProfile>(Obj))
                {
                    return Prof;
                }
            }
        }
    }
    // Dynamic fallback profile with responsive settings
    URA4DirectControlProfile* Fallback = NewObject<URA4DirectControlProfile>(const_cast<URA4DirectControlSubsystem*>(this));
    Fallback->EnterBlendTime = 0.25f;
    Fallback->ExitBlendTime = 0.25f;
    Fallback->Camera.WideFOV = 90.0f;
    Fallback->Camera.ZoomedFOV = 45.0f;
    Fallback->Camera.MouseSensitivityYaw = 1.0f;
    Fallback->Camera.MouseSensitivityPitch = 1.0f;
    Fallback->Camera.TurretPitchMin = -35.0f;
    Fallback->Camera.TurretPitchMax = 55.0f;
    Fallback->Camera.InputDeadZone = 0.05f;
    return Fallback;
}

bool URA4DirectControlSubsystem::RequestEnter(APlayerController* PC, RA4::EntityId Vehicle)
{
    if (SimSubsystem == nullptr || PC == nullptr || !Vehicle.IsValid())
    {
        return false;
    }
    if (ClientPhase == ERA4DirectControlClientPhase::DirectControl ||
        ClientPhase == ERA4DirectControlClientPhase::Entering)
    {
        return false;
    }
    const RA4::SimWorld* World = SimSubsystem->GetSimWorld();
    if (World == nullptr)
    {
        return false;
    }
    if (!World->IsAlive(Vehicle))
    {
        return false;
    }
    const RA4::EntityCore* Core = World->GetCore(Vehicle);
    if (Core == nullptr)
    {
        return false;
    }
    LocalPlayerId = RA4::PlayerId(PC->PlayerState != nullptr ? PC->PlayerState->GetPlayerId() : 0);
    if (Core->Owner != LocalPlayerId)
    {
        return false;
    }
    ActiveProfile = ResolveProfile(Core->Def);

    RA4::Command Cmd;
    Cmd.Type = RA4::CommandType::DirectControlEnter;
    Cmd.Issuer = LocalPlayerId;
    Cmd.Primary = Vehicle;
    SubmitCommand(Cmd);

    ControlledVehicle = Vehicle;
    SetClientPhase(ERA4DirectControlClientPhase::Entering);

    // Presentation-only view target switch. The simulation does not care
    // which Actor the camera is attached to; this is purely for the local
    // player's view. We blend over the profile's enter time so the camera
    // moves inside the vehicle smoothly.
    ARA4EntityActor* EntityActor = SimSubsystem->GetEntityActor(Vehicle);
    if (EntityActor != nullptr)
    {
        const float Fov = (ActiveProfile != nullptr && ActiveProfile->Camera.WideFOV > 0.0f) ? ActiveProfile->Camera.WideFOV : 90.0f;
        EntityActor->SetupDirectControlView(true, Fov);
        PC->SetViewTargetWithBlend(EntityActor, ActiveProfile != nullptr ? ActiveProfile->EnterBlendTime : 0.35f);
        PC->bShowMouseCursor = true;
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
    }
    return true;
}

bool URA4DirectControlSubsystem::RequestExit(APlayerController* PC)
{
    if (SimSubsystem == nullptr || PC == nullptr)
    {
        return false;
    }
    if (ClientPhase != ERA4DirectControlClientPhase::DirectControl &&
        ClientPhase != ERA4DirectControlClientPhase::Entering)
    {
        return false;
    }
    RA4::Command Cmd;
    Cmd.Type = RA4::CommandType::DirectControlExit;
    Cmd.Issuer = LocalPlayerId;
    Cmd.Primary = ControlledVehicle;
    SubmitCommand(Cmd);
    SetClientPhase(ERA4DirectControlClientPhase::Exiting);

    ARA4EntityActor* EntityActor = SimSubsystem->GetEntityActor(ControlledVehicle);
    if (EntityActor != nullptr)
    {
        EntityActor->SetupDirectControlView(false);
    }

    // Restore the RTS camera pawn as the view target. The blend time matches
    // the profile's exit transition so the camera glides back out.
    if (APawn* CamPawn = PC->GetPawn())
    {
        PC->SetViewTargetWithBlend(CamPawn, ActiveProfile != nullptr ? ActiveProfile->ExitBlendTime : 0.35f);
    }
    PC->bShowMouseCursor = true;
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(InputMode);

    ControlledVehicle = RA4::EntityId{};
    ActiveProfile = nullptr;
    return true;
}

void URA4DirectControlSubsystem::TickInput(APlayerController* PC, float DeltaSeconds)
{
    if (ClientPhase != ERA4DirectControlClientPhase::DirectControl &&
        ClientPhase != ERA4DirectControlClientPhase::Entering)
    {
        return;
    }
    if (PC == nullptr || SimSubsystem == nullptr)
    {
        return;
    }
    // Throttle: submit Drive at most every kDriveCommandIntervalSeconds so
    // the command stream matches the simulation's per-tick budget.
    DriveAccumulatorSeconds += DeltaSeconds;
    if (DriveAccumulatorSeconds < kDriveCommandIntervalSeconds)
    {
        return;
    }
    DriveAccumulatorSeconds = 0.0f;

    float Throttle = 0.0f;
    float Steering = 0.0f;
    if (PC->IsInputKeyDown(EKeys::W)) { Throttle += 1.0f; }
    if (PC->IsInputKeyDown(EKeys::S)) { Throttle -= 1.0f; }
    if (PC->IsInputKeyDown(EKeys::D)) { Steering += 1.0f; }
    if (PC->IsInputKeyDown(EKeys::A)) { Steering -= 1.0f; }

    float MouseDx = 0.0f;
    float MouseDy = 0.0f;
    PC->GetInputMouseDelta(MouseDx, MouseDy);

    uint8_t Flags = 0;
    if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)) { Flags |= 0x01; } // Primary fire on LMB
    if (PC->WasInputKeyJustPressed(EKeys::RightMouseButton)) { Flags |= 0x02; } // Special Ability / Secondary fire on RMB
    if (PC->WasInputKeyJustPressed(EKeys::Z) || PC->WasInputKeyJustPressed(EKeys::MiddleMouseButton)) { Flags |= 0x08; } // optics toggle on Z or MMB
    if (PC->WasInputKeyJustPressed(EKeys::R)) { Flags |= 0x10; } // manual reload request

    RA4::DirectControlAxes Axes = QuantizeAxes(
        ActiveProfile != nullptr ? ActiveProfile->Camera : FRA4DirectControlCameraSettings(),
        Throttle, Steering, MouseDx, MouseDy, Flags);

    RA4::Command Cmd;
    Cmd.Type = RA4::CommandType::DirectControlDrive;
    Cmd.Issuer = LocalPlayerId;
    Cmd.Primary = ControlledVehicle;
    Cmd.DirectAxes = Axes;
    SubmitCommand(Cmd);

    // Fire command for primary (0x01) or secondary / ability (0x02)
    if (Flags & 0x03)
    {
        RA4::Command Fire;
        Fire.Type = RA4::CommandType::DirectControlFire;
        Fire.Issuer = LocalPlayerId;
        Fire.Primary = ControlledVehicle;
        Fire.DirectAxes.Flags = Flags;
        SubmitCommand(Fire);
    }
}

void URA4DirectControlSubsystem::TickPresentation(APlayerController* PC, float DeltaSeconds)
{
    if (SimSubsystem == nullptr)
    {
        return;
    }
    const RA4::SimWorld* World = SimSubsystem->GetSimWorld();
    if (World == nullptr)
    {
        return;
    }

    // Mirror the authoritative phase. The server is the source of truth; if
    // the command was rejected or the vehicle died, we follow the sim.
    const RA4::DirectControlComp* Dc = World->GetDirectControl(ControlledVehicle);
    if (Dc == nullptr)
    {
        if (ClientPhase != ERA4DirectControlClientPhase::RTS)
        {
            SetClientPhase(ERA4DirectControlClientPhase::RTS);
            ControlledVehicle = RA4::EntityId{};
            ActiveProfile = nullptr;
        }
        return;
    }
    using SP = RA4::DirectControlPhase;
    using CP = ERA4DirectControlClientPhase;
    switch (Dc->Phase)
    {
        case SP::Inactive:
            SetClientPhase(CP::RTS);
            ControlledVehicle = RA4::EntityId{};
            ActiveProfile = nullptr;
            break;
        case SP::Entering:
            SetClientPhase(CP::Entering);
            break;
        case SP::Active:
            SetClientPhase(CP::DirectControl);
            break;
        case SP::Exiting:
            SetClientPhase(CP::Exiting);
            break;
        case SP::VehicleDestroyed:
            SetClientPhase(CP::VehicleDestroyed);
            break;
    }
}