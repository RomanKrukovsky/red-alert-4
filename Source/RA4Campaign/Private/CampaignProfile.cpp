// Copyright (c) Red Alert 4 project.
#include "CampaignProfile.h"

#include <algorithm>
#include <sstream>

namespace RA4
{

void CampaignProfile::RecordMissionVictory(const std::string& MissionId,
                                          uint32_t CompletionTicks,
                                          bool bSecondaryObjectivesDone,
                                          uint32_t ParTimeTicks,
                                          uint32_t TimestampSeconds)
{
    MissionRecord& Rec = Records[MissionId];
    Rec.MissionId = MissionId;
    Rec.bCompleted = true;
    Rec.CompletedTimestamp = TimestampSeconds;

    if (Rec.BestCompletionTick == 0 || CompletionTicks < Rec.BestCompletionTick)
    {
        Rec.BestCompletionTick = CompletionTicks;
    }

    Rec.bSecondaryObjectivesDone = Rec.bSecondaryObjectivesDone || bSecondaryObjectivesDone;

    // Calculate stars (1: Victory, 2: Secondary Objectives, 3: Under Par Time)
    uint8_t Stars = 1;
    if (Rec.bSecondaryObjectivesDone)
    {
        Stars += 1;
    }
    if (ParTimeTicks > 0 && Rec.BestCompletionTick <= ParTimeTicks)
    {
        Stars += 1;
    }

    if (Stars > Rec.StarsEarned)
    {
        Rec.StarsEarned = Stars;
    }
}

bool CampaignProfile::IsMissionCompleted(const std::string& MissionId) const
{
    const auto It = Records.find(MissionId);
    return It != Records.end() && It->second.bCompleted;
}

const MissionRecord* CampaignProfile::GetMissionRecord(const std::string& MissionId) const
{
    const auto It = Records.find(MissionId);
    if (It != Records.end())
    {
        return &It->second;
    }
    return nullptr;
}

int32_t CampaignProfile::GetTotalStars() const
{
    int32_t Total = 0;
    for (const auto& Pair : Records)
    {
        Total += Pair.second.StarsEarned;
    }
    return Total;
}

int32_t CampaignProfile::GetCompletedMissionCount() const
{
    int32_t Count = 0;
    for (const auto& Pair : Records)
    {
        if (Pair.second.bCompleted)
        {
            Count += 1;
        }
    }
    return Count;
}

void CampaignProfile::Reset()
{
    Records.clear();
}

bool CampaignProfile::SerializeJson(std::string& OutJson) const
{
    std::ostringstream Ss;
    Ss << "{\n  \"records\": [\n";

    bool bFirst = true;
    for (const auto& Pair : Records)
    {
        if (!bFirst)
        {
            Ss << ",\n";
        }
        bFirst = false;

        const MissionRecord& R = Pair.second;
        Ss << "    {\n";
        Ss << "      \"missionId\": \"" << R.MissionId << "\",\n";
        Ss << "      \"completed\": " << (R.bCompleted ? "true" : "false") << ",\n";
        Ss << "      \"bestCompletionTick\": " << R.BestCompletionTick << ",\n";
        Ss << "      \"starsEarned\": " << static_cast<int32_t>(R.StarsEarned) << ",\n";
        Ss << "      \"secondaryObjectivesDone\": " << (R.bSecondaryObjectivesDone ? "true" : "false") << ",\n";
        Ss << "      \"completedTimestamp\": " << R.CompletedTimestamp << "\n";
        Ss << "    }";
    }

    Ss << "\n  ]\n}";
    OutJson = Ss.str();
    return true;
}

bool CampaignProfile::DeserializeJson(const std::string& JsonStr)
{
    // Minimal, dependency-free key-value parser for simple JSON payload
    Reset();
    size_t Pos = 0;

    while ((Pos = JsonStr.find("\"missionId\":", Pos)) != std::string::npos)
    {
        Pos += 12;
        // Find opening quote of missionId
        const size_t QuoteStart = JsonStr.find('"', Pos);
        if (QuoteStart == std::string::npos) break;
        const size_t QuoteEnd = JsonStr.find('"', QuoteStart + 1);
        if (QuoteEnd == std::string::npos) break;

        const std::string MissionId = JsonStr.substr(QuoteStart + 1, QuoteEnd - QuoteStart - 1);
        MissionRecord Rec;
        Rec.MissionId = MissionId;

        // completed
        const size_t CompPos = JsonStr.find("\"completed\":", QuoteEnd);
        if (CompPos != std::string::npos && CompPos < JsonStr.find('}', QuoteEnd))
        {
            Rec.bCompleted = (JsonStr.find("true", CompPos) < JsonStr.find(',', CompPos));
        }

        // bestCompletionTick
        const size_t TickPos = JsonStr.find("\"bestCompletionTick\":", QuoteEnd);
        if (TickPos != std::string::npos && TickPos < JsonStr.find('}', QuoteEnd))
        {
            const size_t NumStart = JsonStr.find_first_of("0123456789", TickPos + 21);
            if (NumStart != std::string::npos)
            {
                Rec.BestCompletionTick = static_cast<uint32_t>(std::stoul(JsonStr.substr(NumStart)));
            }
        }

        // starsEarned
        const size_t StarPos = JsonStr.find("\"starsEarned\":", QuoteEnd);
        if (StarPos != std::string::npos && StarPos < JsonStr.find('}', QuoteEnd))
        {
            const size_t NumStart = JsonStr.find_first_of("0123456789", StarPos + 14);
            if (NumStart != std::string::npos)
            {
                Rec.StarsEarned = static_cast<uint8_t>(std::stoul(JsonStr.substr(NumStart)));
            }
        }

        // secondaryObjectivesDone
        const size_t SecPos = JsonStr.find("\"secondaryObjectivesDone\":", QuoteEnd);
        if (SecPos != std::string::npos && SecPos < JsonStr.find('}', QuoteEnd))
        {
            Rec.bSecondaryObjectivesDone = (JsonStr.find("true", SecPos) < JsonStr.find(',', SecPos));
        }

        // completedTimestamp
        const size_t TimePos = JsonStr.find("\"completedTimestamp\":", QuoteEnd);
        if (TimePos != std::string::npos && TimePos < JsonStr.find('}', QuoteEnd))
        {
            const size_t NumStart = JsonStr.find_first_of("0123456789", TimePos + 21);
            if (NumStart != std::string::npos)
            {
                Rec.CompletedTimestamp = static_cast<uint32_t>(std::stoul(JsonStr.substr(NumStart)));
            }
        }

        Records[MissionId] = Rec;
        Pos = QuoteEnd;
    }

    return true;
}

} // namespace RA4
