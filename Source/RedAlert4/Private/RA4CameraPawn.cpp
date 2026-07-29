// Copyright (c) Red Alert 4 project.
#include "RA4CameraPawn.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "RA4SimCoords.h"

ARA4CameraPawn::ARA4CameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    // The camera must keep gliding while the simulation is paused or the match has
    // ended, so it ticks independently of game pause.
    PrimaryActorTick.bTickEvenWhenPaused = true;

    RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    SetRootComponent(RootScene);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootScene);
    // Every kind of smoothing lives in CameraController, which is deterministic and
    // tested. Letting the spring arm add its own lag on top would make the camera
    // feel different from what the tests assert.
    SpringArm->bDoCollisionTest = false;
    SpringArm->bEnableCameraLag = false;
    SpringArm->bEnableCameraRotationLag = false;
    SpringArm->bUsePawnControlRotation = false;
    SpringArm->bInheritPitch = false;
    SpringArm->bInheritYaw = false;
    SpringArm->bInheritRoll = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;
}

void ARA4CameraPawn::BeginPlay()
{
    Super::BeginPlay();

    if (Camera != nullptr)
    {
        Camera->SetFieldOfView(FieldOfView);
    }
    // Place the camera correctly on the first frame instead of letting it slide in
    // from wherever the pawn was spawned.
    Tick(0.0f);
}

void ARA4CameraPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    CameraController.Update(DeltaSeconds);

    const RA4::Input::Vec2f Focus = CameraController.GetFocus();
    SetActorLocation(FVector(Focus.X, Focus.Y, RA4Coords::GroundZ));

    if (SpringArm != nullptr)
    {
        SpringArm->TargetArmLength = CameraController.GetHeight();
        SpringArm->SetWorldRotation(FRotator(PitchDegrees, CameraController.GetYawDegrees(), 0.0f));
    }

    if (!bReportedInitialCameraState && GetWorld() != nullptr &&
        GetWorld()->GetTimeSeconds() >= 0.5f && Camera != nullptr)
    {
        bReportedInitialCameraState = true;
        const RA4::Input::Vec2f Target = CameraController.GetTargetFocus();
        UE_LOG(LogTemp, Display,
               TEXT("RA4 camera state focus=(%.1f, %.1f) target=(%.1f, %.1f) pawn=%s camera=%s rotation=%s"),
               Focus.X, Focus.Y, Target.X, Target.Y,
               *GetActorLocation().ToCompactString(),
               *Camera->GetComponentLocation().ToCompactString(),
               *Camera->GetComponentRotation().ToCompactString());
    }
}
