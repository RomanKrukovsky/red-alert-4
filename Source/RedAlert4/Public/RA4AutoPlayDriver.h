// Copyright (c) Red Alert 4 project. Scripted full-match auto-play driver.
//
// A console-driven state machine that plays a complete skirmish through the
// same command bus a human player uses: deploy the MCV, build the economy,
// train an army, attack the enemy base, and read the result. It exists so
// "the game is playable end to end" is a claim a log can prove, not a feeling
// a playtest has to remember. Enable with `ra4.AutoPlay 1` (or -ExecCmds).
#pragma once

#include "CoreMinimal.h"

class URA4SimWorldSubsystem;

// One transition per poll tick; every step logs "RA4AutoPlay:" so a capture
// can grep the whole chain and fail loudly on the first missing link.
namespace RA4AutoPlay
{
	// Call every presentation tick from the sim subsystem. Safe to call when
	// no match is running; it idles.
	void Tick(URA4SimWorldSubsystem& Subsystem, float DeltaTime);

	// Reset to idle (match torn down / new match).
	void Reset();
}
