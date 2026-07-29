// Copyright (c) Red Alert 4 project. Temporary skirmish seeding.
//
// Until the lobby exists, something has to decide the map, the factions and the
// starting bases. Keeping that here rather than inside the subsystem means the
// lobby can replace one function call later instead of unpicking initialisation.
#pragma once

#include "CoreMinimal.h"

namespace RA4
{
class ContentDatabase;
class SimWorld;
}

struct REDALERT4_API FRA4MatchBootstrap
{
    // Fills Content, initialises World, and seeds two opposing bases plus an ore
    // field for each. Content must outlive World: the simulation holds a raw
    // pointer to it for the whole match.
    static void BuildSkirmish(RA4::ContentDatabase& Content, RA4::SimWorld& World, uint64 Seed);
};
