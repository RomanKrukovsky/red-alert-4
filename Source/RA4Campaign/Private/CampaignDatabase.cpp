// Copyright (c) Red Alert 4 project. Campaign database implementation.
#include "CampaignDatabase.h"

#include <algorithm>

namespace RA4
{

CampaignDatabase::CampaignDatabase()
{
    InitializeDefaultCampaigns();
}

void CampaignDatabase::InitializeDefaultCampaigns()
{
    Chapters.clear();

    // ---------------------------------------------------------------------------
    // 1. Soviet Union Campaign (10 Missions)
    // Marshal Viktor Sokolov
    // ---------------------------------------------------------------------------
    {
        CampaignChapterDef Sov;
        Sov.Faction = FactionId::Soviet;
        Sov.CampaignTitleKey = "campaign.sov.title";
        Sov.CommanderNameKey = "character.sokolov.name";
        Sov.DescriptionKey = "campaign.sov.description";
        Sov.bUnlockedByDefault = true;

        for (int32_t M = 1; M <= 10; ++M)
        {
            CampaignMissionDef Def;
            Def.MissionId = "sov_mission_" + std::to_string(M);
            Def.TitleKey = "mission.sov_" + std::to_string(M) + ".title";
            Def.BriefingTextKey = "mission.sov_" + std::to_string(M) + ".briefing";
            Def.MapAssetPath = "/Game/Maps/Campaign/Sov_" + std::to_string(M);
            Def.PlayerFaction = FactionId::Soviet;
            Def.TargetDifficulty = std::min(M, 5);

            if (M == 1)
            {
                Def.Type = MissionType::LimitedForce;
                Def.Objectives.push_back({"obj_secure_bunker", "mission.sov_1.obj_secure_bunker", true, ObjectiveState::Active});
                Def.Objectives.push_back({"obj_contact_sokolov", "mission.sov_1.obj_contact_sokolov", true, ObjectiveState::Active});
                Def.IntroCutscene = GetSokolovDemonstrationCutscene();
            }
            else if (M == 2)
            {
                Def.Type = MissionType::BaseBuilding;
                Def.Objectives.push_back({"obj_build_refinery", "mission.sov_2.obj_build_refinery", true, ObjectiveState::Active});
                Def.Objectives.push_back({"obj_destroy_allied_outpost", "mission.sov_2.obj_destroy_outpost", true, ObjectiveState::Active});
            }
            else if (M == 3)
            {
                Def.Type = MissionType::StealthInfiltration;
                Def.Objectives.push_back({"obj_infiltrate_lab", "mission.sov_3.obj_infiltrate_lab", true, ObjectiveState::Active});
            }
            else if (M == 4)
            {
                Def.Type = MissionType::DefenseHold;
                Def.Objectives.push_back({"obj_hold_chronosphere", "mission.sov_4.obj_hold_chronosphere", true, ObjectiveState::Active});
            }
            else if (M == 5)
            {
                Def.Type = MissionType::EscortConvoy;
                Def.Objectives.push_back({"obj_escort_mcv", "mission.sov_5.obj_escort_mcv", true, ObjectiveState::Active});
            }
            else if (M == 6)
            {
                Def.Type = MissionType::ObjectCapture;
                Def.Objectives.push_back({"obj_capture_radar", "mission.sov_6.obj_capture_radar", true, ObjectiveState::Active});
            }
            else if (M == 7)
            {
                Def.Type = MissionType::MapDynamicShift;
                Def.Objectives.push_back({"obj_survive_timeline_collapse", "mission.sov_7.obj_survive_collapse", true, ObjectiveState::Active});
            }
            else if (M == 8)
            {
                Def.Type = MissionType::JointOperation;
                Def.Objectives.push_back({"obj_coop_allied_defectors", "mission.sov_8.obj_coop", true, ObjectiveState::Active});
            }
            else if (M == 9)
            {
                Def.Type = MissionType::FactionCivilWar;
                Def.Objectives.push_back({"obj_defeat_alternate_ussr", "mission.sov_9.obj_defeat_alt_ussr", true, ObjectiveState::Active});
            }
            else
            {
                Def.Type = MissionType::FinalClimax;
                Def.Objectives.push_back({"obj_destroy_liberty_core", "mission.sov_10.obj_destroy_liberty", true, ObjectiveState::Active});
            }

            Sov.Missions.push_back(Def);
        }
        Chapters.push_back(Sov);
    }

    // ---------------------------------------------------------------------------
    // 2. Alliance Campaign (10 Missions)
    // Strategic AI LIBERTY & Global Security
    // ---------------------------------------------------------------------------
    {
        CampaignChapterDef All;
        All.Faction = FactionId::Alliance;
        All.CampaignTitleKey = "campaign.all.title";
        All.CommanderNameKey = "character.ward.name";
        All.DescriptionKey = "campaign.all.description";
        All.bUnlockedByDefault = true;

        for (int32_t M = 1; M <= 10; ++M)
        {
            CampaignMissionDef Def;
            Def.MissionId = "all_mission_" + std::to_string(M);
            Def.TitleKey = "mission.all_" + std::to_string(M) + ".title";
            Def.BriefingTextKey = "mission.all_" + std::to_string(M) + ".briefing";
            Def.MapAssetPath = "/Game/Maps/Campaign/All_" + std::to_string(M);
            Def.PlayerFaction = FactionId::Alliance;
            Def.TargetDifficulty = std::min(M, 5);

            Def.Objectives.push_back({"obj_primary", "mission.all_" + std::to_string(M) + ".obj_primary", true, ObjectiveState::Active});
            All.Missions.push_back(Def);
        }
        Chapters.push_back(All);
    }

    // ---------------------------------------------------------------------------
    // 3. Eastern Coalition Campaign (10 Missions)
    // Digital Emperor & Underwater Complex
    // ---------------------------------------------------------------------------
    {
        CampaignChapterDef Eac;
        Eac.Faction = FactionId::EasternCoalition;
        Eac.CampaignTitleKey = "campaign.eac.title";
        Eac.CommanderNameKey = "character.emperor.name";
        Eac.DescriptionKey = "campaign.eac.description";
        Eac.bUnlockedByDefault = true;

        for (int32_t M = 1; M <= 10; ++M)
        {
            CampaignMissionDef Def;
            Def.MissionId = "eac_mission_" + std::to_string(M);
            Def.TitleKey = "mission.eac_" + std::to_string(M) + ".title";
            Def.BriefingTextKey = "mission.eac_" + std::to_string(M) + ".briefing";
            Def.MapAssetPath = "/Game/Maps/Campaign/Eac_" + std::to_string(M);
            Def.PlayerFaction = FactionId::EasternCoalition;
            Def.TargetDifficulty = std::min(M, 5);

            Def.Objectives.push_back({"obj_primary", "mission.eac_" + std::to_string(M) + ".obj_primary", true, ObjectiveState::Active});
            Eac.Missions.push_back(Def);
        }
        Chapters.push_back(Eac);
    }

    // ---------------------------------------------------------------------------
    // 4. Chronolegion Secret Campaign (8 Missions)
    // Erased Timelines & The Archivist
    // ---------------------------------------------------------------------------
    {
        CampaignChapterDef Chro;
        Chro.Faction = FactionId::ChronoLegion;
        Chro.CampaignTitleKey = "campaign.chro.title";
        Chro.CommanderNameKey = "character.archivist.name";
        Chro.DescriptionKey = "campaign.chro.description";
        Chro.bIsSecretCampaign = true;
        Chro.bUnlockedByDefault = false;

        for (int32_t M = 1; M <= 8; ++M)
        {
            CampaignMissionDef Def;
            Def.MissionId = "chro_mission_" + std::to_string(M);
            Def.TitleKey = "mission.chro_" + std::to_string(M) + ".title";
            Def.BriefingTextKey = "mission.chro_" + std::to_string(M) + ".briefing";
            Def.MapAssetPath = "/Game/Maps/Campaign/Chro_" + std::to_string(M);
            Def.PlayerFaction = FactionId::ChronoLegion;
            Def.TargetDifficulty = std::min(M + 2, 5);

            Def.Objectives.push_back({"obj_primary", "mission.chro_" + std::to_string(M) + ".obj_primary", true, ObjectiveState::Active});
            Chro.Missions.push_back(Def);
        }
        Chapters.push_back(Chro);
    }
}

const CampaignChapterDef* CampaignDatabase::FindChapter(FactionId Faction) const
{
    for (const CampaignChapterDef& Chapter : Chapters)
    {
        if (Chapter.Faction == Faction)
        {
            return &Chapter;
        }
    }
    return nullptr;
}

const CampaignMissionDef* CampaignDatabase::FindMission(const std::string& MissionId) const
{
    for (const CampaignChapterDef& Chapter : Chapters)
    {
        for (const CampaignMissionDef& Mission : Chapter.Missions)
        {
            if (Mission.MissionId == MissionId)
            {
                return &Mission;
            }
        }
    }
    return nullptr;
}

CutsceneSequenceDef CampaignDatabase::GetSokolovDemonstrationCutscene() const
{
    CutsceneSequenceDef Cutscene;
    Cutscene.CutsceneId = "cutscene.sov.briefing_sokolov_ward";
    Cutscene.MapName = "/Game/Maps/Cutscenes/SovietCommandCentre";
    Cutscene.bCanBeSkipped = true;

    CutsceneDialogueLine Line1;
    Line1.SpeakerNameKey = "character.sokolov.title";
    Line1.TextKey = "dialogue.sokolov.madam_president_speech";
    Line1.AudioEventId = "vo.sokolov.madam_president";
    Line1.DurationSeconds = 5.0f;
    Cutscene.DialogueLines.push_back(Line1);

    CutsceneDialogueLine Line2;
    Line2.SpeakerNameKey = "character.ward.title";
    Line2.TextKey = "dialogue.ward.response";
    Line2.AudioEventId = "vo.ward.no_compromise";
    Line2.DurationSeconds = 4.5f;
    Cutscene.DialogueLines.push_back(Line2);

    return Cutscene;
}

} // namespace RA4
