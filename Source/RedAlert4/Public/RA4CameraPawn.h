// Copyright (c) Red Alert 4 project. RTS camera pawn.
//
// A thin shell around RA4::Input::CameraController: the controller owns all the
// behaviour and is tested headlessly, while this class only turns its output into a
// spring arm length and a transform.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "RA4Input/CameraController.h"

#include "RA4CameraPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS()
class REDALERT4_API ARA4CameraPawn : public APawn
{
    GENERATED_BODY()

public:
    ARA4CameraPawn();

    virtual void Tick(float DeltaSeconds) override;

    // The controller is the source of truth; this class never stores camera state
    // of its own, so there is nothing to keep in sync.
    RA4::Input::CameraController& GetCameraController() { return CameraController; }
    const RA4::Input::CameraController& GetCameraController() const { return CameraController; }

    // Fixed downward pitch. A free-pitch RTS camera makes ground picking ambiguous
    // and gives no gameplay benefit, so pitch is configuration rather than input.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RA4|Camera",
              meta = (ClampMin = "-89.0", ClampMax = "-15.0"))
    float PitchDegrees = -55.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RA4|Camera")
    float FieldOfView = 60.0f;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "RA4|Camera")
    TObjectPtr<USceneComponent> RootScene;

    UPROPERTY(VisibleAnywhere, Category = "RA4|Camera")
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, Category = "RA4|Camera")
    TObjectPtr<UCameraComponent> Camera;

private:
    RA4::Input::CameraController CameraController;
    bool bReportedInitialCameraState = false;
};
