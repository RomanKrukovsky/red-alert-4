// Copyright (c) Red Alert 4 project. Campaign structure and mission graph data types.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4Content/ContentTypes.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"

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

// --- Objective conditions ---------------------------------------------------
//
// An objective is only an objective if something can decide whether it is met. The
// five conditions below are deliberately the smallest set that covers every mission
// type in the campaign: everything is a count, a resource total, a deadline or a
// position, all of which SimWorld already answers exactly and deterministically.
//
// The conditions are pure predicates over simulation state. They read the world and
// never write to it, so evaluating them cannot itself change the outcome of a match
// and a mission stays replay-safe.

enum class ObjectiveConditionType : uint8_t
{
    // Unauthored. Never satisfiable; see MissionObjective::Condition.
    None = 0,

    // count(entities owned by Subject matching Def) <= Amount.
    // Amount 0 with an invalid Def is "wipe this player out"; with a Def it is
    // "destroy every one of these". Also how protect-objectives are expressed as
    // failure conditions.
    EntityCountAtMost,

    // count(entities owned by Subject matching Def) >= Amount.
    // Build orders ("field three war factories") and hold orders alike.
    EntityCountAtLeast,

    // Subject's credit balance >= Amount.
    CreditsAtLeast,

    // Amount ticks have elapsed since the mission began. Ticks rather than seconds
    // because the tick is the unit of authority; seconds only exist for display.
    SurviveTicks,

    // Any live entity owned by Subject is within RadiusTiles of TargetTile.
    ReachLocation,
};

struct ObjectiveCondition
{
    ObjectiveConditionType Type = ObjectiveConditionType::None;

    // Whose entities or credits the condition is about. This is a player slot, not
    // "the player" -- a Soviet mission can require that an allied AI's convoy
    // survives just as easily as that the enemy's base does not.
    PlayerId Subject = kInvalidPlayer;

    // Which entity type the count is over. An invalid ContentId means "any entity",
    // which is what makes EntityCountAtMost(Subject, invalid, 0) mean elimination.
    ContentId Def;

    int32_t Amount = 0;

    TileCoord TargetTile;
    int32_t RadiusTiles = 0;
};

struct MissionObjective
{
    std::string Id;                  // e.g. "obj_protect_hq"
    std::string TextKey;             // localization key
    bool bIsPrimary = true;
    ObjectiveState State = ObjectiveState::Hidden;

    // What the simulation must look like for this objective to be met. An objective
    // with ConditionType::None can never complete on its own, which is what a
    // placeholder looks like from the runtime's point of view -- MissionRuntime
    // treats such an objective as unsatisfiable rather than as trivially satisfied,
    // so an unauthored mission fails a test instead of silently auto-winning.
    ObjectiveCondition Condition;
};

// --- Mission starting conditions -------------------------------------------
//
// A mission has to describe the match it starts, not just the .umap it opens.
// Everything here feeds SimWorld::Initialize and the opening spawns, so a mission
// is runnable headlessly in a unit test as well as behind a UWorld. The map asset
// is presentation on top of this, not the source of truth for it.

enum class MissionSpawnKind : uint8_t
{
    Unit = 0,
    Building,
    ResourceNode,
};

struct MissionSpawn
{
    ContentId Def;
    PlayerId Owner = 0;
    TileCoord Tile;
    MissionSpawnKind Kind = MissionSpawnKind::Unit;
    // Ore in the field, for ResourceNode. Ignored otherwise.
    int32_t Amount = 0;
};

struct MissionPlayerSlot
{
    bool bActive = false;
    FactionId Faction = FactionId::None;
    int32_t StartingCredits = 10000;
};

struct MissionSetupDef
{
    uint64_t Seed = 0;
    int32_t MapWidth = 64;
    int32_t MapHeight = 64;
    std::string MapName;
    MissionPlayerSlot Players[8];
    std::vector<MissionSpawn> Spawns;
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

    // Any one of these being true ends the mission in defeat. They are separate from
    // the objective list because a loss condition is not a task the player is working
    // towards -- showing "your headquarters was destroyed" in the objectives panel
    // alongside "capture the radar" reads as an instruction rather than a warning.
    std::vector<ObjectiveCondition> FailureConditions;

    MissionSetupDef Setup;
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
