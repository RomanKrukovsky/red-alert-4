// Copyright (c) Red Alert 4 project. Campaign meta-progression and profile persistence.
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "RA4Content/ContentTypes.h"

namespace RA4
{

struct MissionRecord
{
    std::string MissionId;
    bool bCompleted = false;
    uint32_t BestCompletionTick = 0;
    uint8_t StarsEarned = 0;              // 1: Primary, 2: Secondary, 3: Par Time
    bool bSecondaryObjectivesDone = false;
    uint32_t CompletedTimestamp = 0;
};

class CampaignProfile
{
public:
    CampaignProfile() = default;

    void RecordMissionVictory(const std::string& MissionId,
                              uint32_t CompletionTicks,
                              bool bSecondaryObjectivesDone,
                              uint32_t ParTimeTicks,
                              uint32_t TimestampSeconds = 0);

    bool IsMissionCompleted(const std::string& MissionId) const;
    const MissionRecord* GetMissionRecord(const std::string& MissionId) const;

    int32_t GetTotalStars() const;
    int32_t GetCompletedMissionCount() const;

    bool SerializeJson(std::string& OutJson) const;
    bool DeserializeJson(const std::string& JsonStr);

    void Reset();

private:
    std::unordered_map<std::string, MissionRecord> Records;
};

} // namespace RA4
