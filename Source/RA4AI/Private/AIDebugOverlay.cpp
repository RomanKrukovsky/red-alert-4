// Copyright (c) Red Alert 4 project. Debug overlay snapshot implementation.
#include "RA4AI/AIDebugOverlay.h"

namespace RA4
{
namespace AI
{

AIDebugOverlaySnapshot AIDebugLogger::CreateSnapshot(PlayerId Player,
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
                                                     const std::vector<std::string>& RecentLogs)
{
    AIDebugOverlaySnapshot Snapshot;
    Snapshot.Player = Player;
    Snapshot.CommanderName = CommanderName;
    Snapshot.DoctrineName = DoctrineName;
    Snapshot.ActiveStrategy = Strategy;
    Snapshot.StrategyScore = Score;
    Snapshot.CurrentGoal = Goal;

    Snapshot.Credits = Credits;
    Snapshot.PowerProduced = PowerProduced;
    Snapshot.PowerConsumed = PowerConsumed;

    Snapshot.KnownEnemiesCount = KnownEnemies;
    Snapshot.AverageConfidencePercent = Confidence;

    for (const auto& G : Groups)
    {
        ArmyGroupSnapshot GS;
        GS.GroupId = G.GroupId;
        GS.Name = G.Name;
        GS.Role = G.Role;
        GS.Stance = G.Stance;
        GS.Task = G.Task;
        GS.MemberCount = static_cast<int32_t>(G.Members.size());
        GS.MoralePercent = G.MoralePercent;
        GS.TargetLocation = G.TargetLocation;
        Snapshot.ActiveGroups.push_back(GS);
    }

    Snapshot.RecentDecisions = RecentLogs;
    return Snapshot;
}

} // namespace AI
} // namespace RA4
