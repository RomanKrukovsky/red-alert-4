// Copyright (c) Red Alert 4 project. Imports the project's WAV files as SoundWave
// assets.
//
// The repository ships ~1090 audio files (unit voice lines, EVA barks, music) but not
// a single .uasset: raw WAVs on disk are invisible to the engine until imported, so
// nothing could ever have played. This commandlet does that import headlessly and
// idempotently, which keeps the (large) source audio out of the asset tree while
// still producing the assets the runtime needs.
//
// Run with: UnrealEditor RedAlert4.uproject -run=RA4AudioImport
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RA4AudioImportCommandlet.generated.h"

UCLASS()
class RA4EDITOR_API URA4AudioImportCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    URA4AudioImportCommandlet();

    virtual int32 Main(const FString& Params) override;
};
