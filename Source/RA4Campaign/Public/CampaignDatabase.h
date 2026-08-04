// Copyright (c) Red Alert 4 project. Authoritative campaign mission database.
#pragma once

#include <vector>

#include "CampaignTypes.h"

namespace RA4
{

// Empty outside an Unreal build, where UnrealBuildTool defines it as the module's
// import/export attribute. Without it the campaign's symbols are local to
// libRA4Campaign and RedAlert4 fails to link against them.
#ifndef RA4CAMPAIGN_API
#define RA4CAMPAIGN_API
#endif

class RA4CAMPAIGN_API CampaignDatabase
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
