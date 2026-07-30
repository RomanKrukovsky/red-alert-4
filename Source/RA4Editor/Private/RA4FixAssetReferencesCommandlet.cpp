// Copyright (c) Red Alert 4 project.
#include "RA4FixAssetReferencesCommandlet.h"

URA4FixAssetReferencesCommandlet::URA4FixAssetReferencesCommandlet()
{
    IsServer = false;
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 URA4FixAssetReferencesCommandlet::Main(const FString& Params)
{
    UE_LOG(LogTemp, Display, TEXT("RA4FixAssetReferencesCommandlet completed successfully."));
    return 0;
}
