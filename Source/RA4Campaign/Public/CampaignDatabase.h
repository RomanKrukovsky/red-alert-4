// Copyright (c) Red Alert 4 project. Authoritative campaign mission database.
#pragma once

#include <vector>

#include "CampaignTypes.h"

namespace RA4
{

class CampaignDatabase
{
public:
    CampaignDatabase();

    void InitializeDefaultCampaigns();

    const std::vector<CampaignChapterDef>& GetChapters() const { return Chapters; }
    const CampaignChapterDef* FindChapter(FactionId Faction) const;
    const CampaignMissionDef* FindMission(const std::string& MissionId) const;

    // Cutscene demonstration helper for Sokolov's speech to US President Eleonora Ward
    CutsceneSequenceDef GetSokolovDemonstrationCutscene() const;

private:
    std::vector<CampaignChapterDef> Chapters;
};

} // namespace RA4
