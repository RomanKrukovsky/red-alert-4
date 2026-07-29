// Copyright (c) Red Alert 4 project. Campaign structure and mission graph data types.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4Content/ContentTypes.h"
#include "RA4Core/Ids.h"

namespace RA4
{

enum class MissionType : uint8_t
{
    BaseBuilding = 0,
    LimitedForce,
    StealthInfiltration,
    DefenseHold,
    EscortConvoy,
    ObjectCapture,
    MapDynamicShift,
    JointOperation,
    FactionCivilWar,
    FinalClimax,
};

enum class ObjectiveState : uint8_t
{
    Hidden = 0,
    Active,
    Completed,
    Failed,
};

struct MissionObjective
{
    std::string Id;                  // e.g. "obj_protect_hq"
    std::string TextKey;             // localization key
    bool bIsPrimary = true;
    ObjectiveState State = ObjectiveState::Hidden;
};

struct CutsceneDialogueLine
{
    std::string SpeakerNameKey;      // e.g. "speaker.sokolov"
    std::string TextKey;             // e.g. "dialogue.sokolov_speech_01"
    std::string AudioEventId;        // e.g. "vo.sokolov.madam_president"
    float DurationSeconds = 4.0f;
};

struct CutsceneSequenceDef
{
    std::string CutsceneId;          // e.g. "cutscene.sov.m01_intro"
    std::string MapName;             // e.g. "RA4_Cutscene_SovietHQ"
    std::vector<CutsceneDialogueLine> DialogueLines;
    bool bCanBeSkipped = true;
};

struct CampaignMissionDef
{
    std::string MissionId;           // e.g. "sov_01_chronoscar"
    std::string TitleKey;            // e.g. "mission.sov_01.title"
    std::string BriefingTextKey;     // e.g. "mission.sov_01.briefing"
    std::string MapAssetPath;        // e.g. "/Game/Maps/Campaign/Sov_01"
    FactionId PlayerFaction = FactionId::Soviet;
    MissionType Type = MissionType::BaseBuilding;
    int32_t TargetDifficulty = 1;
    
    std::vector<MissionObjective> Objectives;
    CutsceneSequenceDef IntroCutscene;
    CutsceneSequenceDef OutroCutscene;
};

struct CampaignChapterDef
{
    FactionId Faction = FactionId::Soviet;
    std::string CampaignTitleKey;
    std::string CommanderNameKey;
    std::string DescriptionKey;
    bool bIsSecretCampaign = false;
    bool bUnlockedByDefault = true;
    std::vector<CampaignMissionDef> Missions;
};

} // namespace RA4
