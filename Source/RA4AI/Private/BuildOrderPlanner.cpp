// Copyright (c) Red Alert 4 project. Dynamic Build Order Planner.
#include "RA4AI/BuildOrderPlanner.h"

namespace RA4
{
namespace AI
{

std::vector<BuildStep> BuildOrderPlanner::ResolvePrerequisites(const ContentDatabase* Content, ContentId TargetItem)
{
    (void)Content;
    std::vector<BuildStep> Steps;
    BuildStep TargetStep;
    TargetStep.ItemId = TargetItem;
    TargetStep.Kind = BuildStepKind::Structure;
    TargetStep.EstimatedCost = 1000;
    TargetStep.Priority = 10;
    Steps.push_back(TargetStep);
    return Steps;
}

bool BuildOrderPlanner::CanAfford(const SimWorld& World, PlayerId Player, int32_t Cost)
{
    return (World.GetPlayer(Player).Credits >= Cost);
}

} // namespace AI
} // namespace RA4
