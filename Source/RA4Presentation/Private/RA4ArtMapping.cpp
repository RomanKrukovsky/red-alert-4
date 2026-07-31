// Copyright (c) Red Alert 4 project. Data-driven mapping implementation.

#include "RA4Presentation/RA4ArtMapping.h"

bool URA4ArtMappingDataAsset::FindUnitArt(FName UnitId, FRA4UnitArtDefinition& OutDefinition) const
{
#ifdef HEADLESS_BUILD
    if (const FRA4UnitArtDefinition* Found = Units.Find(UnitId.Name))
    {
        OutDefinition = *Found;
        return true;
    }
#else
    if (const FRA4UnitArtDefinition* Found = Units.Find(UnitId.ToString()))
    {
        OutDefinition = *Found;
        return true;
    }
#endif
    return false;
}

bool URA4ArtMappingDataAsset::FindBuildingArt(FName BuildingId, FRA4BuildingArtDefinition& OutDefinition) const
{
#ifdef HEADLESS_BUILD
    if (const FRA4BuildingArtDefinition* Found = Buildings.Find(BuildingId.Name))
    {
        OutDefinition = *Found;
        return true;
    }
#else
    if (const FRA4BuildingArtDefinition* Found = Buildings.Find(BuildingId.ToString()))
    {
        OutDefinition = *Found;
        return true;
    }
#endif
    return false;
}
