// Copyright (c) Red Alert 4 project. Task Bidding & Role Assignment System.
#include "RA4AI/TaskBiddingSystem.h"
#include <algorithm>

namespace RA4
{
namespace AI
{

TaskBid TaskBiddingSystem::EvaluateBestBid(const std::vector<ArmyGroup>& Groups, const TaskRequirement& Requirement)
{
    TaskBid BestBid;
    BestBid.GroupId = 0;
    BestBid.SuitabilityScore = 0;

    for (const auto& Group : Groups)
    {
        int32_t Score = 50;
        if (Group.Role == Requirement.TargetRole)
            Score += 30;
        if (Group.CombatReadiness >= 80)
            Score += 20;

        if (Score > BestBid.SuitabilityScore)
        {
            BestBid.SuitabilityScore = Score;
            BestBid.GroupId = Group.GroupId;
        }
    }

    return BestBid;
}

} // namespace AI
} // namespace RA4
