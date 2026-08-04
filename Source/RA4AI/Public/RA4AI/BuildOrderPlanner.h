// Copyright (c) Red Alert 4 project. Dynamic Build Order Planner.
#pragma once

#include <cstdint>
#include <vector>
#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Ids.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

enum class BuildStepKind : uint8_t
{
    Structure = 0,
    Unit = 1,
    TechUpgrade = 2
};

struct BuildStep
{
    ContentId ItemId;
    BuildStepKind Kind = BuildStepKind::Structure;
    int32_t EstimatedCost = 0;
    int32_t Priority = 0;
};

class RA4AI_API BuildOrderPlanner
{
public:
    static std::vector<BuildStep> ResolvePrerequisites(const ContentDatabase* Content, ContentId TargetItem);
    static bool CanAfford(const SimWorld& World, PlayerId Player, int32_t Cost);
};

} // namespace AI
} // namespace RA4
