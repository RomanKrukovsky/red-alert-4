// Copyright (c) Red Alert 4 project. Direct-control presentation subsystem.
//
// This subsystem is the *only* UE-side code that translates keyboard/mouse
// input into DirectControl commands. It owns no simulation state. It reads
// the authoritative DirectControlComp from the SimWorld subsystem to decide
// what camera to use, what HUD to show, and whether to send Drive/Fire
// commands this frame.
//
// See ADR-0029 for the simulation/presentation split and the command schema.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "RA4Core/Command.h"
#include "RA4Core/Ids.h"

#include "RA4DirectControlProfile.h"

#include "RA4DirectControlSubsystem.generated.h"

class URA4SimWorldSubsystem;
class ARA4EntityActor;
class APlayerController;
class UCameraComponent;
class URA4DirectControlProfile;

UENUM(BlueprintType)
enum class ERA4DirectControlClientPhase : uint8
{
    RTS = 0,
    Entering,
    DirectControl,
    Exiting,
    VehicleDestroyed,
};

UCLASS()
class REDALERT4_API URA4DirectControlSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    URA4DirectControlSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Called by the player controller when F is pressed. Validates the
    // selected entity against the simulation, looks up the profile, submits
    // a DirectControlEnter command, and arms the input router. Returns true
    // if a command was submitted.
    bool RequestEnter(APlayerController* PC, RA4::EntityId Vehicle);

    // Called by the player controller when F is pressed while in direct
    // control. Submits DirectControlExit.
    bool RequestExit(APlayerController* PC);

    // Per-frame input sampling. Reads WASD/mouse from the player controller
    // and submits a single DirectControlDrive command per tick with the
    // quantized axes from the active profile.
    void TickInput(APlayerController* PC, float DeltaSeconds);

    // Reads authoritative state from the SimWorld subsystem and updates the
    // local phase + camera. Called from the player controller's PlayerTick.
    void TickPresentation(APlayerController* PC, float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category = "RA4|DirectControl")
    ERA4DirectControlClientPhase GetClientPhase() const { return ClientPhase; }

    UFUNCTION(BlueprintPure, Category = "RA4|DirectControl")
    bool IsInDirectControl() const { return ClientPhase == ERA4DirectControlClientPhase::DirectControl; }

    // Vehicle index the local player is controlling, or -1 if none. UHT
    // cannot reflect the simulation's EntityId, so we surface the slot index
    // and rebuild the EntityId internally.
    UFUNCTION(BlueprintPure, Category = "RA4|DirectControl")
    int32 GetControlledVehicleIndex() const { return ControlledVehicle.IsValid() ? int32(ControlledVehicle.Index) : -1; }

    UPROPERTY(Transient)
    TObjectPtr<URA4DirectControlProfile> ActiveProfile;

    // Build a DirectControlAxes from raw float input, applying the profile's
    // sensitivity, dead-zone, inversion and limits. Pure helper, no state.
    static RA4::DirectControlAxes QuantizeAxes(const FRA4DirectControlCameraSettings& Settings,
                                               float ThrottleRaw, float SteeringRaw,
                                               float TurretYawRaw, float TurretPitchRaw,
                                               uint8_t Flags);

private:
    // Resolves the DirectControlProfile for a given vehicle content id. The
    // lookup is content-driven: profiles live in /Game/RA4/Data/DirectControl
    // and are registered in the AssetRegistry under the BoundContentId tag.
    URA4DirectControlProfile* ResolveProfile(RA4::ContentId VehicleContentId) const;

    void SetClientPhase(ERA4DirectControlClientPhase NewPhase);
    void SubmitCommand(const RA4::Command& Cmd);

    UPROPERTY(Transient)
    TObjectPtr<URA4SimWorldSubsystem> SimSubsystem;

    ERA4DirectControlClientPhase ClientPhase = ERA4DirectControlClientPhase::RTS;
    RA4::EntityId ControlledVehicle{};
    RA4::PlayerId LocalPlayerId = RA4::kInvalidPlayer;

    // Accumulators for aim smoothing. The simulation integrates the
    // quantized deltas; the client only smooths the *display* of the turret.
    float SmoothedTurretYaw = 0.0f;
    float SmoothedTurretPitch = 0.0f;

    // Throttle the client command stream. The simulation enforces its own
    // kMaxCommandsPerPlayerPerTick; this is a client-side soft cap so the
    // input router does not spam Drive commands faster than the sim tick.
    float DriveAccumulatorSeconds = 0.0f;
    static constexpr float kDriveCommandIntervalSeconds = 0.05f; // 20 Hz
};