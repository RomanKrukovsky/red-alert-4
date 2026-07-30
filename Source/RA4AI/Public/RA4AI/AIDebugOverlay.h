// Copyright (c) Red Alert 4 project. Debug overlay data snapshot structures.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4AI/AIStrategy.h"
#include "RA4AI/ArmyGroup.h"
#include "RA4Core/Ids.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

struct ArmyGroupSnapshot
{
    uint32_t GroupId = 0;
    std::string Name;
    GroupRole Role = GroupRole::MainAssault;
    GroupStance Stance = GroupStance::Balanced;
    GroupTaskType Task = GroupTaskType::Idle;
    int32_t MemberCount = 0;
    int32_t MoralePercent = 100;
    Vec2 TargetLocation = Vec2::Zero();
};

struct AIDebugOverlaySnapshot
{
    PlayerId Player = 0;
    std::string CommanderName;
    std::string DoctrineName;
    AIStrategy ActiveStrategy = AIStrategy::ExpandEconomy;
    int32_t StrategyScore = 0;
    std::string CurrentGoal;

    int32_t Credits = 0;
    int32_t PowerProduced = 0;
    int32_t PowerConsumed = 0;

    int32_t KnownEnemiesCount = 0;
    int32_t AverageConfidencePercent = 100;

    std::vector<ArmyGroupSnapshot> ActiveGroups;
    std::vector<std::string> RecentDecisions;
};

class RA4AI_API AIDebugLogger
{
public:
    static AIDebugOverlaySnapshot CreateSnapshot(PlayerId Player,
                                                  const std::string& CommanderName,
                                                  const std::string& DoctrineName,
                                                  AIStrategy Strategy,
                                                  int32_t Score,
                                                  const std::string& Goal,
                                                  int32_t Credits,
                                                  int32_t PowerProduced,
                                                  int32_t PowerConsumed,
                                                  int32_t KnownEnemies,
                                                  int32_t Confidence,
                                                  const std::vector<ArmyGroup>& Groups,
                                                  const std::vector<std::string>& RecentLogs);
};

} // namespace AI
} // namespace RA4
