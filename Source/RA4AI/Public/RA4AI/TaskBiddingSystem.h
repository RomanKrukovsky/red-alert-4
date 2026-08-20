// Copyright (c) Red Alert 4 project. Task Bidding & Role Assignment System.
#pragma once

#include <cstdint>
#include <vector>
#include "RA4AI/ArmyGroup.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

struct TaskRequirement
{
    GroupRole TargetRole = GroupRole::MainAssault;
    Vec2 TargetLocation;
    int32_t DesiredAntiArmor = 0;
    int32_t DesiredAntiAir = 0;
    int32_t RequiredSpeed = 0;
    int32_t MaxArrivalSeconds = 60;
    int32_t PriorityScore = 50; // 0..100
};

struct TaskBid
{
    uint32_t GroupId = 0;
    int32_t SuitabilityScore = 0; // 0..100 percent
};

class RA4AI_API TaskBiddingSystem
{
public:
    static TaskBid EvaluateBestBid(const std::vector<ArmyGroup>& Groups, const TaskRequirement& Requirement);
};

} // namespace AI
} // namespace RA4
