// Copyright (c) Red Alert 4 project. One-shot commandlet that repairs asset
// references broken by a Content Browser folder move that skipped "Fixup
// Redirectors" -- the reference stays a plain string inside the serialized asset,
// so moving the file on disk does not update it; only re-pointing and resaving does.
//
// Run with: UnrealEditor RedAlert4.uproject -run=RA4FixAssetReferences
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RA4FixAssetReferencesCommandlet.generated.h"

UCLASS()
class RA4EDITOR_API URA4FixAssetReferencesCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    URA4FixAssetReferencesCommandlet();

    virtual int32 Main(const FString& Params) override;
};
