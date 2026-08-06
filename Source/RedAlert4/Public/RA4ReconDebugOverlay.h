// Copyright (c) Red Alert 4 project. The two-maps overlay: what is vs what you believe.
//
// The debugging tool ADR-0026 §7 calls mandatory: without seeing ground truth and
// the perceived state side by side, distortion cannot be tuned and cannot be
// trusted. Strictly read-only over the simulation (INVARIANT 3): it draws, it
// never writes, and it consumes belief exclusively through the PerceivedTrack
// read surface -- with one deliberate exception: ground truth markers, which are
// the left half of the comparison and exist only in this debug view.
//
// Console:
//   recon.Overlay 0|1|2   -- 0 off, 1 belief only, 2 belief + ground truth
//
// The "show truth" skirmish option (owner decision: open option, not a cheat
// code) drives the same drawing through its own flag; recon.Overlay is the
// developer path.
#pragma once

#include "CoreMinimal.h"

#include "RA4ReconDebugOverlay.generated.h"

class APlayerController;
class UCanvas;
class URA4SimWorldSubsystem;

UCLASS()
class REDALERT4_API URA4ReconDebugOverlay : public UObject
{
    GENERATED_BODY()

public:
    // Draws the overlay onto the given canvas. Called from ARA4RtsHud::DrawHUD;
    // Projector supplies world->screen projection, ViewerPlayer selects whose
    // belief is shown.
    static void Draw(UCanvas* Canvas, const APlayerController* Projector,
                     const URA4SimWorldSubsystem* Sim, uint8 ViewerPlayer);

    // Current overlay mode from the console variable (0 = off).
    static int32 GetOverlayMode();
};
