// Copyright (c) Red Alert 4 project. Scripted campaign triggers, actions, and dialogue types.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "CampaignTypes.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"


namespace RA4
{

enum class TriggerConditionType : uint8_t
{
    None = 0,
    TickReached,
    AreaEntered,
    EntityDestroyed,
    EntityHealthBelowPercent,
    ObjectiveCompleted,
    CreditsThreshold,
};

enum class TriggerActionType : uint8_t
{
    None = 0,
    SpawnReinforcements,
    RevealObjective,
    PlayCinematicTransmission,
    ShiftMapBounds,
    ChangePlayerDiplomacy,
    SetAIProfile,
    RevealFogArea,
};

struct ReinforcementUnitSpawn
{
    ContentId Def;
    Vec2 Location;
    PlayerId Owner = 0;
};

struct ScriptTriggerAction
{
    TriggerActionType Type = TriggerActionType::None;

    // Parameters
    std::string TargetObjectiveId;
    std::string SpeakerName;
    std::string DialogueTextKey;
    std::string PortraitVideoId;
    uint32_t DurationTicks = 100;

    TileCoord MapNewOrigin;
    int32_t MapNewWidth = 0;
    int32_t MapNewHeight = 0;

    TileCoord RevealCenter;
    int32_t RevealRadiusTiles = 0;

    std::vector<ReinforcementUnitSpawn> Reinforcements;
};

struct MissionTrigger
{
    std::string Id;
    TriggerConditionType Condition = TriggerConditionType::None;
    bool bOneShot = true;
    bool bFired = false;

    // Condition parameters
    TickIndex TriggerTick = 0;
    TileCoord AreaCenter;
    int32_t AreaRadiusTiles = 0;
    PlayerId ConditionPlayer = 0;
    EntityId TargetEntity = EntityId::Invalid();
    int32_t HealthPercent = 50;
    std::string RequiredObjectiveId;

    std::vector<ScriptTriggerAction> Actions;
};

struct CinematicTransmission
{
    std::string Id;
    std::string Speaker;
    std::string Text;
    std::string PortraitVideo;
    TickIndex StartTick = 0;
    TickIndex EndTick = 0;
    bool bActive = false;
};

} // namespace RA4
