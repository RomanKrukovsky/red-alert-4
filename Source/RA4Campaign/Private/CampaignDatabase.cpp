// Copyright (c) Red Alert 4 project. Campaign database implementation.
//
// Every mission here is authored, not generated. That matters because the previous
// version built 38 missions out of four `for` loops, and a loop cannot express what
// distinguishes one mission from another -- the objectives, the opening force and the
// way it can be lost. What it produced was 38 rows in a database that happened to
// have mission-shaped fields.
//
// The unit of authorship is: a MissionSetupDef saying what exists on the first tick,
// a list of ObjectiveConditions saying what has to become true, and a list of
// FailureConditions saying what must not. All three are read by MissionRuntime
// against SimWorld, so a mission can be played to a win or a loss in a headless test.
#include "CampaignDatabase.h"

#include <algorithm>

#include "RA4Core/SimConfig.h"

namespace RA4
{

namespace
{

// --- Content the missions place ---------------------------------------------
//
// Resolved by name, as everywhere else in the codebase, so that a content rename
// shows up as a mission that fails to place its spawns rather than as a silently
// empty map. Soviet and Alliance ids come from RA4Content/DefaultContent.cpp; the
// Eastern Coalition and Chrono Legion ids come from the bible export, and are not in
// DefaultContent yet -- see kFactionHasSimContent below.

struct FactionKit
{
    ContentId ConYard;
    ContentId Refinery;
    ContentId Power;
    ContentId Barracks;
    ContentId WarFactory;
    ContentId Turret;
    ContentId Harvester;
    ContentId Infantry;
    ContentId AntiTankInfantry;
    ContentId Tank;
    ContentId Mcv;
};

const FactionKit SovietKit{
    MakeContentId("building.sov.construction_yard"),
    MakeContentId("building.sov.ore_refinery"),
    MakeContentId("building.sov.tesla_reactor"),
    MakeContentId("building.sov.barracks"),
    MakeContentId("building.sov.war_factory"),
    MakeContentId("building.sov.gun_turret"),
    MakeContentId("unit.sov.ore_harvester"),
    MakeContentId("unit.sov.conscript"),
    MakeContentId("unit.sov.rocket_trooper"),
    MakeContentId("unit.sov.heavy_tank"),
    MakeContentId("unit.sov.mcv"),
};

const FactionKit AllianceKit{
    MakeContentId("building.all.construction_yard"),
    MakeContentId("building.all.ore_refinery"),
    MakeContentId("building.all.power_plant"),
    MakeContentId("building.all.barracks"),
    MakeContentId("building.all.war_factory"),
    MakeContentId("building.all.pillbox"),
    MakeContentId("unit.all.ore_harvester"),
    MakeContentId("unit.all.rifleman"),
    MakeContentId("unit.all.missile_infantry"),
    MakeContentId("unit.all.light_tank"),
    MakeContentId("unit.all.mcv"),
};

// The Eastern Coalition and Chrono Legion kits name the ids the bible export defines.
// Their unit ids are stable ASCII; their *building* ids in the export are the Russian
// display names, because the buildings in ra4_content.normalized.json carry no `id`
// field and BibleContentLoader falls back to `name`. That is fragile and is flagged
const FactionKit CoalitionKit{
    MakeContentId("building.ec.construction_yard"),
    MakeContentId("building.ec.ore_synthesizer"),
    MakeContentId("building.ec.solar_collector"),
    MakeContentId("building.ec.barracks"),
    MakeContentId("building.ec.war_factory"),
    MakeContentId("building.ec.defense_tower"),
    MakeContentId("unit.ec.harmony_harvester"),
    MakeContentId("unit.ec.qianwei_rifleman"),
    MakeContentId("unit.ec.vajra_lancer"),
    MakeContentId("unit.ec.qinglong_mbt"),
    MakeContentId("unit.ec.mcv"),
};

const FactionKit ChronoKit{
    MakeContentId("building.cl.construction_yard"),
    MakeContentId("building.cl.causality_center"),
    MakeContentId("building.cl.decay_reactor"),
    MakeContentId("building.cl.barracks"),
    MakeContentId("building.cl.war_factory"),
    MakeContentId("building.cl.temporal_turret"),
    MakeContentId("unit.cl.echo_harvester"),
    MakeContentId("unit.cl.resonance_rifleman"),
    MakeContentId("unit.cl.paradox_lancer"),
    MakeContentId("unit.cl.timeline_tank"),
    MakeContentId("unit.cl.mcv"),
};

const ContentId OreField = MakeContentId("resource.ore_field");

/** The opposing side for a chapter. Soviet fights Alliance and vice versa; the two
    factions without simulation content fight the Alliance, so their missions still
    describe a real opponent that DefaultContent can place. */
FactionId OpponentOf(FactionId Faction)
{
    return Faction == FactionId::Alliance ? FactionId::Soviet : FactionId::Alliance;
}

// --- Condition constructors -------------------------------------------------
//
// Named constructors rather than aggregate initialisers: a mission list is read far
// more often than it is written, and `Eliminate(kEnemy)` says what the designer meant
// where `{EntityCountAtMost, 1, ContentId{}, 0, {}, 0}` says only what the struct
// holds.

constexpr PlayerId kPlayerSlot = 0;
constexpr PlayerId kEnemySlot = 1;
constexpr PlayerId kAllySlot = 2;

ObjectiveCondition Eliminate(PlayerId Subject)
{
    ObjectiveCondition C;
    C.Type = ObjectiveConditionType::EntityCountAtMost;
    C.Subject = Subject;
    C.Amount = 0;
    return C;
}

ObjectiveCondition DestroyAllOf(PlayerId Subject, ContentId Def)
{
    ObjectiveCondition C;
    C.Type = ObjectiveConditionType::EntityCountAtMost;
    C.Subject = Subject;
    C.Def = Def;
    C.Amount = 0;
    return C;
}

/** Fewer than Amount of Def left. As a failure condition this is how "keep this
    alive" is expressed -- LosesAllOf(player, hq) fires the moment the last one dies. */
ObjectiveCondition FewerThan(PlayerId Subject, ContentId Def, int32_t Amount)
{
    ObjectiveCondition C;
    C.Type = ObjectiveConditionType::EntityCountAtMost;
    C.Subject = Subject;
    C.Def = Def;
    C.Amount = Amount - 1;
    return C;
}

ObjectiveCondition Own(PlayerId Subject, ContentId Def, int32_t Amount)
{
    ObjectiveCondition C;
    C.Type = ObjectiveConditionType::EntityCountAtLeast;
    C.Subject = Subject;
    C.Def = Def;
    C.Amount = Amount;
    return C;
}

ObjectiveCondition OwnAnything(PlayerId Subject, int32_t Amount)
{
    ObjectiveCondition C;
    C.Type = ObjectiveConditionType::EntityCountAtLeast;
    C.Subject = Subject;
    C.Amount = Amount;
    return C;
}

ObjectiveCondition Credits(PlayerId Subject, int32_t Amount)
{
    ObjectiveCondition C;
    C.Type = ObjectiveConditionType::CreditsAtLeast;
    C.Subject = Subject;
    C.Amount = Amount;
    return C;
}

/** Minutes, converted at the one place where the tick rate is known. The condition
    itself counts ticks because the tick is the unit of authority. */
ObjectiveCondition SurviveMinutes(int32_t Minutes)
{
    ObjectiveCondition C;
    C.Type = ObjectiveConditionType::SurviveTicks;
    C.Amount = Minutes * 60 * kTicksPerSecond;
    return C;
}

ObjectiveCondition Reach(PlayerId Subject, int32_t TileX, int32_t TileY, int32_t RadiusTiles)
{
    ObjectiveCondition C;
    C.Type = ObjectiveConditionType::ReachLocation;
    C.Subject = Subject;
    C.TargetTile = TileCoord(TileX, TileY);
    C.RadiusTiles = RadiusTiles;
    return C;
}

// --- Objective and spawn constructors ---------------------------------------

MissionObjective Primary(const char* Id, const std::string& TextKey, const ObjectiveCondition& C)
{
    MissionObjective O;
    O.Id = Id;
    O.TextKey = TextKey;
    O.bIsPrimary = true;
    O.State = ObjectiveState::Active;
    O.Condition = C;
    return O;
}

MissionObjective Secondary(const char* Id, const std::string& TextKey, const ObjectiveCondition& C)
{
    MissionObjective O = Primary(Id, TextKey, C);
    O.bIsPrimary = false;
    return O;
}

/** A primary objective that starts Hidden. Not evaluated and not required for victory
    until MissionRuntime::RevealObjective names it, which is how a mission stages its
    second half without the briefing giving it away. */
MissionObjective Staged(const char* Id, const std::string& TextKey, const ObjectiveCondition& C)
{
    MissionObjective O = Primary(Id, TextKey, C);
    O.State = ObjectiveState::Hidden;
    return O;
}

MissionSpawn Unit(ContentId Def, PlayerId Owner, int32_t TileX, int32_t TileY)
{
    MissionSpawn S;
    S.Def = Def;
    S.Owner = Owner;
    S.Tile = TileCoord(TileX, TileY);
    S.Kind = MissionSpawnKind::Unit;
    return S;
}

MissionSpawn Building(ContentId Def, PlayerId Owner, int32_t TileX, int32_t TileY)
{
    MissionSpawn S;
    S.Def = Def;
    S.Owner = Owner;
    S.Tile = TileCoord(TileX, TileY);
    S.Kind = MissionSpawnKind::Building;
    return S;
}

MissionSpawn Ore(int32_t TileX, int32_t TileY, int32_t Amount = 3000)
{
    MissionSpawn S;
    S.Def = OreField;
    S.Tile = TileCoord(TileX, TileY);
    S.Kind = MissionSpawnKind::ResourceNode;
    S.Amount = Amount;
    return S;
}

// --- Setup builders ---------------------------------------------------------

void AddSquad(MissionSetupDef& Setup, ContentId Def, PlayerId Owner, int32_t TileX, int32_t TileY,
              int32_t Count)
{
    // Spread along a row rather than stacked, so the movement systems are not asked to
    // untangle an overlap on the first tick.
    for (int32_t I = 0; I < Count; ++I)
    {
        Setup.Spawns.push_back(Unit(Def, Owner, TileX + I, TileY));
    }
}

void AddOreField(MissionSetupDef& Setup, int32_t OriginX, int32_t OriginY, int32_t Span = 3)
{
    for (int32_t X = 0; X < Span; ++X)
    {
        for (int32_t Y = 0; Y < Span; ++Y)
        {
            Setup.Spawns.push_back(Ore(OriginX + X, OriginY + Y));
        }
    }
}

/** A full base: yard, refinery, power, a harvester on the ore and a small garrison.
    This is the opening a base-building mission needs -- a construction yard on an
    empty field is not a playable start. */
void AddBase(MissionSetupDef& Setup, const FactionKit& Kit, PlayerId Owner, int32_t YardX,
             int32_t YardY, int32_t OreX, int32_t OreY)
{
    Setup.Spawns.push_back(Building(Kit.ConYard, Owner, YardX, YardY));
    Setup.Spawns.push_back(Building(Kit.Refinery, Owner, YardX + 4, YardY));
    Setup.Spawns.push_back(Building(Kit.Power, Owner, YardX, YardY + 4));
    AddOreField(Setup, OreX, OreY);
    Setup.Spawns.push_back(Unit(Kit.Harvester, Owner, YardX + 4, YardY + 2));
    AddSquad(Setup, Kit.Infantry, Owner, YardX - 2, YardY + 6, 4);
    Setup.Spawns.push_back(Unit(Kit.Tank, Owner, YardX + 1, YardY + 7));
}

/** An enemy position: a couple of structures and a garrison, enough that clearing it
    is a fight rather than a walk. Deliberately not a full base -- a mission that
    opens against a fully-teched opponent is a mission the player loses to economy. */
void AddOutpost(MissionSetupDef& Setup, const FactionKit& Kit, PlayerId Owner, int32_t X, int32_t Y,
                int32_t Garrison = 3)
{
    Setup.Spawns.push_back(Building(Kit.ConYard, Owner, X, Y));
    Setup.Spawns.push_back(Building(Kit.Turret, Owner, X + 3, Y + 1));
    AddSquad(Setup, Kit.Infantry, Owner, X - 1, Y + 3, Garrison);
}

MissionSetupDef MakeSetup(uint64_t Seed, FactionId PlayerFaction, FactionId EnemyFaction,
                          int32_t PlayerCredits = 10000, int32_t EnemyCredits = 10000)
{
    MissionSetupDef Setup;
    Setup.Seed = Seed;
    Setup.MapWidth = 64;
    Setup.MapHeight = 64;

    Setup.Players[kPlayerSlot].bActive = true;
    Setup.Players[kPlayerSlot].Faction = PlayerFaction;
    Setup.Players[kPlayerSlot].StartingCredits = PlayerCredits;

    Setup.Players[kEnemySlot].bActive = true;
    Setup.Players[kEnemySlot].Faction = EnemyFaction;
    Setup.Players[kEnemySlot].StartingCredits = EnemyCredits;

    return Setup;
}

/** Every mission's baseline defeat clause: the player having nothing left. Missions
    that hinge on a specific asset add a second, narrower clause of their own.

    Note this is a *failure* condition, not the absence of a victory one. The
    simulation has its own elimination victory, but a campaign mission usually ends in
    defeat well before the last conscript dies -- and the debrief wants to name which
    clause fired. */
void AddDefaultFailure(CampaignMissionDef& Def)
{
    Def.FailureConditions.push_back(Eliminate(kPlayerSlot));
}

std::string ObjKey(const char* Chapter, int32_t Mission, const char* Objective)
{
    return std::string("mission.") + Chapter + "_" + std::to_string(Mission) + ".obj_" + Objective;
}

/** Fills in the fields every mission shares, so the per-mission code below is only
    the part that differs. */
CampaignMissionDef MakeMission(const char* Chapter, int32_t Index, FactionId Faction,
                               MissionType Type, int32_t Difficulty)
{
    CampaignMissionDef Def;
    const std::string N = std::to_string(Index);
    Def.MissionId = std::string(Chapter) + "_mission_" + N;
    Def.TitleKey = std::string("mission.") + Chapter + "_" + N + ".title";
    Def.BriefingTextKey = std::string("mission.") + Chapter + "_" + N + ".briefing";
    Def.MapAssetPath = std::string("/Game/Maps/Campaign/") + Chapter + "_" + N;
    Def.PlayerFaction = Faction;
    Def.Type = Type;
    Def.TargetDifficulty = Difficulty;
    Def.Setup.MapName = Def.MissionId;
    return Def;
}

// Defined below, after the two hand-authored chapters that do not need it.
CampaignMissionDef BuildGenericMission(const char* Chapter, int32_t Index, FactionId Faction,
                                       MissionType Type, int32_t Difficulty,
                                       const char* ObjectiveId, const char* ObjectiveKey,
                                       const FactionKit& Us, const FactionKit& Them);

CutsceneSequenceDef MakeSokolovCutscene()
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

// ---------------------------------------------------------------------------
// 1. Soviet Union -- Marshal Viktor Sokolov (10 missions)
// ---------------------------------------------------------------------------
CampaignChapterDef BuildSovietChapter()
{
    CampaignChapterDef Chapter;
    Chapter.Faction = FactionId::Soviet;
    Chapter.CampaignTitleKey = "campaign.sov.title";
    Chapter.CommanderNameKey = "character.sokolov.name";
    Chapter.DescriptionKey = "campaign.sov.description";
    Chapter.bUnlockedByDefault = true;

    const FactionKit& Us = SovietKit;
    const FactionKit& Them = AllianceKit;

    // M1 -- a squad, a bunker to reach and a garrison in the way. No base, no
    // economy: the opening mission teaches selection and movement and nothing else.
    {
        CampaignMissionDef M = MakeMission("sov", 1, FactionId::Soviet, MissionType::LimitedForce, 1);
        M.Setup = MakeSetup(0x5A01, FactionId::Soviet, FactionId::Alliance, 0, 0);
        AddSquad(M.Setup, Us.Infantry, kPlayerSlot, 8, 54, 6);
        AddSquad(M.Setup, Us.AntiTankInfantry, kPlayerSlot, 8, 56, 2);
        AddSquad(M.Setup, Them.Infantry, kEnemySlot, 30, 30, 4);
        M.Setup.Spawns.push_back(Building(Them.Turret, kEnemySlot, 44, 12));
        AddSquad(M.Setup, Them.Infantry, kEnemySlot, 46, 14, 3);

        M.Objectives.push_back(Primary("obj_secure_bunker", ObjKey("sov", 1, "secure_bunker"),
                                       Reach(kPlayerSlot, 46, 10, 4)));
        M.Objectives.push_back(Primary("obj_contact_sokolov", ObjKey("sov", 1, "contact_sokolov"),
                                       DestroyAllOf(kEnemySlot, Them.Turret)));
        AddDefaultFailure(M);
        M.IntroCutscene = MakeSokolovCutscene();
        Chapter.Missions.push_back(M);
    }

    // M2 -- the first economy mission. Build up, then clear the outpost.
    {
        CampaignMissionDef M = MakeMission("sov", 2, FactionId::Soviet, MissionType::BaseBuilding, 1);
        M.Setup = MakeSetup(0x5A02, FactionId::Soviet, FactionId::Alliance, 5000, 4000);
        AddBase(M.Setup, Us, kPlayerSlot, 10, 10, 6, 15);
        AddOutpost(M.Setup, Them, kEnemySlot, 46, 46);

        M.Objectives.push_back(Primary("obj_build_refinery", ObjKey("sov", 2, "build_refinery"),
                                       Own(kPlayerSlot, Us.Refinery, 2)));
        M.Objectives.push_back(Primary("obj_destroy_allied_outpost",
                                       ObjKey("sov", 2, "destroy_outpost"),
                                       DestroyAllOf(kEnemySlot, Them.ConYard)));
        M.Objectives.push_back(Secondary("obj_stockpile", ObjKey("sov", 2, "stockpile"),
                                         Credits(kPlayerSlot, 8000)));
        AddDefaultFailure(M);
        // Losing the yard in a build-up mission means the mission cannot be completed;
        // ending it now is kinder than letting the player discover that over ten
        // minutes of having nothing to build with.
        M.FailureConditions.push_back(FewerThan(kPlayerSlot, Us.ConYard, 1));
        Chapter.Missions.push_back(M);
    }

    // M3 -- infiltration. A handful of infantry, a lab across the map, and no
    // reinforcements: the objective is arrival, and the failure clause is the squad.
    {
        CampaignMissionDef M = MakeMission("sov", 3, FactionId::Soviet, MissionType::StealthInfiltration, 2);
        M.Setup = MakeSetup(0x5A03, FactionId::Soviet, FactionId::Alliance, 0, 6000);
        AddSquad(M.Setup, Us.Infantry, kPlayerSlot, 6, 58, 4);
        M.Setup.Spawns.push_back(Building(Them.ConYard, kEnemySlot, 50, 8));
        M.Setup.Spawns.push_back(Building(Them.Turret, kEnemySlot, 46, 12));
        AddSquad(M.Setup, Them.Infantry, kEnemySlot, 40, 20, 5);

        M.Objectives.push_back(Primary("obj_infiltrate_lab", ObjKey("sov", 3, "infiltrate_lab"),
                                       Reach(kPlayerSlot, 50, 8, 3)));
        // The evac point is deliberately not the insertion point. An extraction
        // objective that names the tile the squad started on is satisfied before the
        // squad has moved, and completes for a straggler who never went in at all.
        M.Objectives.push_back(Staged("obj_extract", ObjKey("sov", 3, "extract"),
                                      Reach(kPlayerSlot, 4, 6, 3)));
        AddDefaultFailure(M);
        Chapter.Missions.push_back(M);
    }

    // M4 -- hold. Survive the clock with the chronosphere standing.
    {
        CampaignMissionDef M = MakeMission("sov", 4, FactionId::Soviet, MissionType::DefenseHold, 2);
        M.Setup = MakeSetup(0x5A04, FactionId::Soviet, FactionId::Alliance, 6000, 12000);
        AddBase(M.Setup, Us, kPlayerSlot, 28, 30, 24, 36);
        M.Setup.Spawns.push_back(Building(Us.Turret, kPlayerSlot, 25, 27));
        M.Setup.Spawns.push_back(Building(Us.Turret, kPlayerSlot, 33, 27));
        AddOutpost(M.Setup, Them, kEnemySlot, 8, 8, 5);
        AddOutpost(M.Setup, Them, kEnemySlot, 52, 52, 5);

        M.Objectives.push_back(Primary("obj_hold_chronosphere", ObjKey("sov", 4, "hold_chronosphere"),
                                       SurviveMinutes(8)));
        AddDefaultFailure(M);
        // The thing being defended. Without this clause "hold the chronosphere" is
        // just "survive", and the player could win by abandoning it.
        M.FailureConditions.push_back(FewerThan(kPlayerSlot, Us.ConYard, 1));
        Chapter.Missions.push_back(M);
    }

    // M5 -- escort. The MCV has to arrive; the MCV dying ends the mission.
    {
        CampaignMissionDef M = MakeMission("sov", 5, FactionId::Soviet, MissionType::EscortConvoy, 3);
        M.Setup = MakeSetup(0x5A05, FactionId::Soviet, FactionId::Alliance, 2000, 8000);
        M.Setup.Spawns.push_back(Unit(Us.Mcv, kPlayerSlot, 8, 56));
        AddSquad(M.Setup, Us.Tank, kPlayerSlot, 10, 56, 3);
        AddSquad(M.Setup, Us.AntiTankInfantry, kPlayerSlot, 10, 58, 4);
        AddSquad(M.Setup, Them.Tank, kEnemySlot, 32, 32, 3);
        AddOutpost(M.Setup, Them, kEnemySlot, 48, 12, 4);

        M.Objectives.push_back(Primary("obj_escort_mcv", ObjKey("sov", 5, "escort_mcv"),
                                       Reach(kPlayerSlot, 52, 8, 4)));
        AddDefaultFailure(M);
        M.FailureConditions.push_back(FewerThan(kPlayerSlot, Us.Mcv, 1));
        Chapter.Missions.push_back(M);
    }

    // M6 -- capture. Expressed as taking the ground and clearing what holds it,
    // because SimWorld has no capture verb yet; when it gains one this objective is
    // the one that changes, not the mission around it.
    {
        CampaignMissionDef M = MakeMission("sov", 6, FactionId::Soviet, MissionType::ObjectCapture, 3);
        M.Setup = MakeSetup(0x5A06, FactionId::Soviet, FactionId::Alliance, 7000, 9000);
        AddBase(M.Setup, Us, kPlayerSlot, 10, 46, 6, 52);
        M.Setup.Spawns.push_back(Building(Them.ConYard, kEnemySlot, 46, 14));
        M.Setup.Spawns.push_back(Building(Them.Turret, kEnemySlot, 42, 18));
        M.Setup.Spawns.push_back(Building(Them.Turret, kEnemySlot, 50, 18));
        AddSquad(M.Setup, Them.Infantry, kEnemySlot, 44, 20, 6);

        M.Objectives.push_back(Primary("obj_capture_radar", ObjKey("sov", 6, "capture_radar"),
                                       DestroyAllOf(kEnemySlot, Them.Turret)));
        M.Objectives.push_back(Primary("obj_hold_radar_site", ObjKey("sov", 6, "hold_radar_site"),
                                       Reach(kPlayerSlot, 46, 14, 3)));
        AddDefaultFailure(M);
        Chapter.Missions.push_back(M);
    }

    // M7 -- the timeline collapse. Survive it, then win the ground it leaves.
    {
        CampaignMissionDef M = MakeMission("sov", 7, FactionId::Soviet, MissionType::MapDynamicShift, 4);
        M.Setup = MakeSetup(0x5A07, FactionId::Soviet, FactionId::Alliance, 8000, 14000);
        AddBase(M.Setup, Us, kPlayerSlot, 12, 12, 8, 18);
        AddBase(M.Setup, Them, kEnemySlot, 46, 46, 52, 40);
        AddSquad(M.Setup, Them.Tank, kEnemySlot, 40, 40, 3);

        M.Objectives.push_back(Primary("obj_survive_timeline_collapse",
                                       ObjKey("sov", 7, "survive_collapse"), SurviveMinutes(6)));
        M.Objectives.push_back(Staged("obj_secure_the_rift", ObjKey("sov", 7, "secure_rift"),
                                      DestroyAllOf(kEnemySlot, Them.ConYard)));
        AddDefaultFailure(M);
        Chapter.Missions.push_back(M);
    }

    // M8 -- joint operation. Slot 2 is the allied defector force, and it has to still
    // be standing at the end: an ally you were free to let die is set dressing.
    {
        CampaignMissionDef M = MakeMission("sov", 8, FactionId::Soviet, MissionType::JointOperation, 4);
        M.Setup = MakeSetup(0x5A08, FactionId::Soviet, FactionId::Alliance, 9000, 15000);
        M.Setup.Players[kAllySlot].bActive = true;
        M.Setup.Players[kAllySlot].Faction = FactionId::Alliance;
        M.Setup.Players[kAllySlot].StartingCredits = 5000;

        AddBase(M.Setup, Us, kPlayerSlot, 12, 46, 8, 52);
        AddOutpost(M.Setup, AllianceKit, kAllySlot, 12, 12, 4);
        AddSquad(M.Setup, AllianceKit.Tank, kAllySlot, 16, 16, 2);
        AddBase(M.Setup, Them, kEnemySlot, 46, 30, 52, 24);

        M.Objectives.push_back(Primary("obj_coop_allied_defectors", ObjKey("sov", 8, "coop"),
                                       DestroyAllOf(kEnemySlot, Them.ConYard)));
        // Not a timer: the defectors have to still be a force at the end, and
        // "at least one thing left standing" is what that means to the simulation.
        M.Objectives.push_back(Primary("obj_defectors_survive", ObjKey("sov", 8, "defectors_survive"),
                                       OwnAnything(kAllySlot, 1)));
        AddDefaultFailure(M);
        M.FailureConditions.push_back(Eliminate(kAllySlot));
        Chapter.Missions.push_back(M);
    }

    // M9 -- civil war. Both sides Soviet, and the enemy opens ahead on economy.
    {
        CampaignMissionDef M = MakeMission("sov", 9, FactionId::Soviet, MissionType::FactionCivilWar, 5);
        M.Setup = MakeSetup(0x5A09, FactionId::Soviet, FactionId::Soviet, 10000, 18000);
        AddBase(M.Setup, Us, kPlayerSlot, 10, 10, 6, 16);
        AddBase(M.Setup, Us, kEnemySlot, 48, 48, 54, 42);
        M.Setup.Spawns.push_back(Building(Us.WarFactory, kEnemySlot, 44, 48));
        AddSquad(M.Setup, Us.Tank, kEnemySlot, 42, 44, 4);

        M.Objectives.push_back(Primary("obj_defeat_alternate_ussr",
                                       ObjKey("sov", 9, "defeat_alt_ussr"), Eliminate(kEnemySlot)));
        M.Objectives.push_back(Secondary("obj_out_produce", ObjKey("sov", 9, "out_produce"),
                                         Own(kPlayerSlot, Us.WarFactory, 2)));
        AddDefaultFailure(M);
        Chapter.Missions.push_back(M);
    }

    // M10 -- the finale. A full enemy base with defences, and no clause but winning.
    {
        CampaignMissionDef M = MakeMission("sov", 10, FactionId::Soviet, MissionType::FinalClimax, 5);
        M.Setup = MakeSetup(0x5A0A, FactionId::Soviet, FactionId::Alliance, 12000, 25000);
        AddBase(M.Setup, Us, kPlayerSlot, 10, 52, 6, 46);
        M.Setup.Spawns.push_back(Building(Us.WarFactory, kPlayerSlot, 14, 52));
        AddBase(M.Setup, Them, kEnemySlot, 48, 10, 54, 16);
        M.Setup.Spawns.push_back(Building(Them.WarFactory, kEnemySlot, 44, 10));
        M.Setup.Spawns.push_back(Building(Them.Turret, kEnemySlot, 44, 16));
        M.Setup.Spawns.push_back(Building(Them.Turret, kEnemySlot, 52, 16));
        AddSquad(M.Setup, Them.Tank, kEnemySlot, 44, 20, 4);
        AddSquad(M.Setup, Them.AntiTankInfantry, kEnemySlot, 48, 20, 4);

        M.Objectives.push_back(Primary("obj_destroy_liberty_core",
                                       ObjKey("sov", 10, "destroy_liberty"), Eliminate(kEnemySlot)));
        AddDefaultFailure(M);
        Chapter.Missions.push_back(M);
    }

    return Chapter;
}

// ---------------------------------------------------------------------------
// 2. Alliance -- LIBERTY and Global Security (10 missions)
// ---------------------------------------------------------------------------
CampaignChapterDef BuildAllianceChapter()
{
    CampaignChapterDef Chapter;
    Chapter.Faction = FactionId::Alliance;
    Chapter.CampaignTitleKey = "campaign.all.title";
    Chapter.CommanderNameKey = "character.ward.name";
    Chapter.DescriptionKey = "campaign.all.description";
    Chapter.bUnlockedByDefault = true;

    const FactionKit& Us = AllianceKit;
    const FactionKit& Them = SovietKit;

    // M1 -- limited force, mirroring the Soviet opener from the other side.
    {
        CampaignMissionDef M = MakeMission("all", 1, FactionId::Alliance, MissionType::LimitedForce, 1);
        M.Setup = MakeSetup(0xA101, FactionId::Alliance, FactionId::Soviet, 0, 0);
        AddSquad(M.Setup, Us.Infantry, kPlayerSlot, 8, 8, 6);
        AddSquad(M.Setup, Us.AntiTankInfantry, kPlayerSlot, 8, 10, 2);
        M.Setup.Spawns.push_back(Building(Them.Turret, kEnemySlot, 46, 48));
        AddSquad(M.Setup, Them.Infantry, kEnemySlot, 44, 46, 4);

        M.Objectives.push_back(Primary("obj_relieve_garrison", ObjKey("all", 1, "relieve_garrison"),
                                       Reach(kPlayerSlot, 46, 50, 4)));
        M.Objectives.push_back(Primary("obj_silence_battery", ObjKey("all", 1, "silence_battery"),
                                       DestroyAllOf(kEnemySlot, Them.Turret)));
        AddDefaultFailure(M);
        Chapter.Missions.push_back(M);
    }

    // M2 -- base building.
    {
        CampaignMissionDef M = MakeMission("all", 2, FactionId::Alliance, MissionType::BaseBuilding, 1);
        M.Setup = MakeSetup(0xA102, FactionId::Alliance, FactionId::Soviet, 5000, 4000);
        AddBase(M.Setup, Us, kPlayerSlot, 12, 12, 8, 18);
        AddOutpost(M.Setup, Them, kEnemySlot, 46, 46);

        M.Objectives.push_back(Primary("obj_establish_foothold", ObjKey("all", 2, "establish_foothold"),
                                       Own(kPlayerSlot, Us.Refinery, 2)));
        M.Objectives.push_back(Primary("obj_clear_soviet_camp", ObjKey("all", 2, "clear_soviet_camp"),
                                       DestroyAllOf(kEnemySlot, Them.ConYard)));
        AddDefaultFailure(M);
        M.FailureConditions.push_back(FewerThan(kPlayerSlot, Us.ConYard, 1));
        Chapter.Missions.push_back(M);
    }

    // M3 -- economic build-up under pressure.
    {
        CampaignMissionDef M = MakeMission("all", 3, FactionId::Alliance, MissionType::BaseBuilding, 2);
        M.Setup = MakeSetup(0xA103, FactionId::Alliance, FactionId::Soviet, 4000, 7000);
        AddBase(M.Setup, Us, kPlayerSlot, 10, 10, 6, 16);
        AddOreField(M.Setup, 30, 30);
        AddOutpost(M.Setup, Them, kEnemySlot, 48, 20, 4);
        AddOutpost(M.Setup, Them, kEnemySlot, 20, 48, 4);

        M.Objectives.push_back(Primary("obj_fund_the_war", ObjKey("all", 3, "fund_the_war"),
                                       Credits(kPlayerSlot, 15000)));
        M.Objectives.push_back(Primary("obj_field_armour", ObjKey("all", 3, "field_armour"),
                                       Own(kPlayerSlot, Us.Tank, 4)));
        AddDefaultFailure(M);
        M.FailureConditions.push_back(FewerThan(kPlayerSlot, Us.ConYard, 1));
        Chapter.Missions.push_back(M);
    }

    // M4 -- hold the line.
    {
        CampaignMissionDef M = MakeMission("all", 4, FactionId::Alliance, MissionType::DefenseHold, 2);
        M.Setup = MakeSetup(0xA104, FactionId::Alliance, FactionId::Soviet, 6000, 12000);
        AddBase(M.Setup, Us, kPlayerSlot, 30, 30, 26, 36);
        M.Setup.Spawns.push_back(Building(Us.Turret, kPlayerSlot, 27, 27));
        M.Setup.Spawns.push_back(Building(Us.Turret, kPlayerSlot, 35, 27));
        AddOutpost(M.Setup, Them, kEnemySlot, 8, 8, 5);
        AddOutpost(M.Setup, Them, kEnemySlot, 52, 52, 5);

        M.Objectives.push_back(Primary("obj_hold_the_line", ObjKey("all", 4, "hold_the_line"),
                                       SurviveMinutes(8)));
        AddDefaultFailure(M);
        M.FailureConditions.push_back(FewerThan(kPlayerSlot, Us.ConYard, 1));
        Chapter.Missions.push_back(M);
    }

    // M5 -- convoy escort.
    {
        CampaignMissionDef M = MakeMission("all", 5, FactionId::Alliance, MissionType::EscortConvoy, 3);
        M.Setup = MakeSetup(0xA105, FactionId::Alliance, FactionId::Soviet, 2000, 8000);
        M.Setup.Spawns.push_back(Unit(Us.Mcv, kPlayerSlot, 8, 8));
        AddSquad(M.Setup, Us.Tank, kPlayerSlot, 10, 8, 3);
        AddSquad(M.Setup, Us.AntiTankInfantry, kPlayerSlot, 10, 10, 4);
        AddSquad(M.Setup, Them.Tank, kEnemySlot, 32, 32, 3);
        AddOutpost(M.Setup, Them, kEnemySlot, 48, 48, 4);

        M.Objectives.push_back(Primary("obj_escort_convoy", ObjKey("all", 5, "escort_convoy"),
                                       Reach(kPlayerSlot, 54, 54, 4)));
        AddDefaultFailure(M);
        M.FailureConditions.push_back(FewerThan(kPlayerSlot, Us.Mcv, 1));
        Chapter.Missions.push_back(M);
    }

    // M6 -- seize the installation.
    {
        CampaignMissionDef M = MakeMission("all", 6, FactionId::Alliance, MissionType::ObjectCapture, 3);
        M.Setup = MakeSetup(0xA106, FactionId::Alliance, FactionId::Soviet, 7000, 9000);
        AddBase(M.Setup, Us, kPlayerSlot, 10, 10, 6, 16);
        M.Setup.Spawns.push_back(Building(Them.ConYard, kEnemySlot, 46, 46));
        M.Setup.Spawns.push_back(Building(Them.Turret, kEnemySlot, 42, 42));
        M.Setup.Spawns.push_back(Building(Them.Turret, kEnemySlot, 50, 42));
        AddSquad(M.Setup, Them.Infantry, kEnemySlot, 44, 40, 6);

        M.Objectives.push_back(Primary("obj_seize_installation", ObjKey("all", 6, "seize_installation"),
                                       DestroyAllOf(kEnemySlot, Them.Turret)));
        M.Objectives.push_back(Primary("obj_hold_installation", ObjKey("all", 6, "hold_installation"),
                                       Reach(kPlayerSlot, 46, 46, 3)));
        AddDefaultFailure(M);
        Chapter.Missions.push_back(M);
    }

    // M7 -- the map shifts under both sides.
    {
        CampaignMissionDef M = MakeMission("all", 7, FactionId::Alliance, MissionType::MapDynamicShift, 4);
        M.Setup = MakeSetup(0xA107, FactionId::Alliance, FactionId::Soviet, 8000, 14000);
        AddBase(M.Setup, Us, kPlayerSlot, 12, 50, 8, 44);
        AddBase(M.Setup, Them, kEnemySlot, 48, 12, 54, 18);
        AddSquad(M.Setup, Them.Tank, kEnemySlot, 44, 18, 3);

        M.Objectives.push_back(Primary("obj_weather_the_shift", ObjKey("all", 7, "weather_the_shift"),
                                       SurviveMinutes(6)));
        M.Objectives.push_back(Staged("obj_counterattack", ObjKey("all", 7, "counterattack"),
                                      DestroyAllOf(kEnemySlot, Them.ConYard)));
        AddDefaultFailure(M);
        Chapter.Missions.push_back(M);
    }

    // M8 -- joint operation with a Soviet defector force in slot 2.
    {
        CampaignMissionDef M = MakeMission("all", 8, FactionId::Alliance, MissionType::JointOperation, 4);
        M.Setup = MakeSetup(0xA108, FactionId::Alliance, FactionId::Soviet, 9000, 15000);
        M.Setup.Players[kAllySlot].bActive = true;
        M.Setup.Players[kAllySlot].Faction = FactionId::Soviet;
        M.Setup.Players[kAllySlot].StartingCredits = 5000;

        AddBase(M.Setup, Us, kPlayerSlot, 12, 12, 8, 18);
        AddOutpost(M.Setup, SovietKit, kAllySlot, 12, 48, 4);
        AddSquad(M.Setup, SovietKit.Tank, kAllySlot, 16, 44, 2);
        AddBase(M.Setup, Them, kEnemySlot, 46, 30, 52, 24);

        M.Objectives.push_back(Primary("obj_joint_assault", ObjKey("all", 8, "joint_assault"),
                                       DestroyAllOf(kEnemySlot, Them.ConYard)));
        M.Objectives.push_back(Primary("obj_partners_survive", ObjKey("all", 8, "partners_survive"),
                                       OwnAnything(kAllySlot, 1)));
        AddDefaultFailure(M);
        M.FailureConditions.push_back(Eliminate(kAllySlot));
        Chapter.Missions.push_back(M);
    }

    // M9 -- LIBERTY turns on its own. Both sides Alliance.
    {
        CampaignMissionDef M = MakeMission("all", 9, FactionId::Alliance, MissionType::FactionCivilWar, 5);
        M.Setup = MakeSetup(0xA109, FactionId::Alliance, FactionId::Alliance, 10000, 18000);
        AddBase(M.Setup, Us, kPlayerSlot, 10, 10, 6, 16);
        AddBase(M.Setup, Us, kEnemySlot, 48, 48, 54, 42);
        M.Setup.Spawns.push_back(Building(Us.WarFactory, kEnemySlot, 44, 48));
        AddSquad(M.Setup, Us.Tank, kEnemySlot, 42, 44, 4);

        M.Objectives.push_back(Primary("obj_shut_down_liberty", ObjKey("all", 9, "shut_down_liberty"),
                                       Eliminate(kEnemySlot)));
        M.Objectives.push_back(Secondary("obj_out_produce", ObjKey("all", 9, "out_produce"),
                                         Own(kPlayerSlot, Us.WarFactory, 2)));
        AddDefaultFailure(M);
        Chapter.Missions.push_back(M);
    }

    // M10 -- the finale.
    {
        CampaignMissionDef M = MakeMission("all", 10, FactionId::Alliance, MissionType::FinalClimax, 5);
        M.Setup = MakeSetup(0xA10A, FactionId::Alliance, FactionId::Soviet, 12000, 25000);
        AddBase(M.Setup, Us, kPlayerSlot, 10, 10, 6, 16);
        M.Setup.Spawns.push_back(Building(Us.WarFactory, kPlayerSlot, 14, 10));
        AddBase(M.Setup, Them, kEnemySlot, 48, 48, 54, 42);
        M.Setup.Spawns.push_back(Building(Them.WarFactory, kEnemySlot, 44, 48));
        M.Setup.Spawns.push_back(Building(Them.Turret, kEnemySlot, 44, 42));
        M.Setup.Spawns.push_back(Building(Them.Turret, kEnemySlot, 52, 42));
        AddSquad(M.Setup, Them.Tank, kEnemySlot, 44, 38, 4);
        AddSquad(M.Setup, Them.AntiTankInfantry, kEnemySlot, 48, 38, 4);

        M.Objectives.push_back(Primary("obj_break_the_iron_curtain",
                                       ObjKey("all", 10, "break_iron_curtain"), Eliminate(kEnemySlot)));
        AddDefaultFailure(M);
        Chapter.Missions.push_back(M);
    }

    return Chapter;
}

// ---------------------------------------------------------------------------
// 3. Eastern Coalition -- the Digital Emperor (10 missions)
//
// The Coalition has no entry in RA4Content/DefaultContent.cpp: its units and
// buildings exist only in the bible export. Its missions are authored against the
// export's ids, so they are complete data -- but under the default content profile
// their spawns will not place, and TestCampaign asserts exactly that rather than
// letting it pass unnoticed. The objectives themselves are deliberately
// content-agnostic (eliminate, survive, reach, credits) so that they hold whichever
// content profile is loaded.
// ---------------------------------------------------------------------------
CampaignChapterDef BuildCoalitionChapter()
{
    CampaignChapterDef Chapter;
    Chapter.Faction = FactionId::EasternCoalition;
    Chapter.CampaignTitleKey = "campaign.eac.title";
    Chapter.CommanderNameKey = "character.emperor.name";
    Chapter.DescriptionKey = "campaign.eac.description";
    Chapter.bUnlockedByDefault = true;

    const FactionKit& Us = CoalitionKit;
    const FactionKit& Them = AllianceKit;

    struct Plan
    {
        int32_t Index;
        MissionType Type;
        int32_t Difficulty;
        const char* ObjectiveId;
        const char* ObjectiveKey;
    };
    static const Plan Plans[10] = {
        {1,  MissionType::LimitedForce,        1, "obj_breach_the_reef",     "breach_the_reef"},
        {2,  MissionType::BaseBuilding,        1, "obj_raise_the_complex",   "raise_the_complex"},
        {3,  MissionType::BaseBuilding,        2, "obj_harvest_the_trench",  "harvest_the_trench"},
        {4,  MissionType::DefenseHold,         2, "obj_hold_the_depths",     "hold_the_depths"},
        {5,  MissionType::EscortConvoy,        3, "obj_escort_the_ark",      "escort_the_ark"},
        {6,  MissionType::ObjectCapture,       3, "obj_seize_the_relay",     "seize_the_relay"},
        {7,  MissionType::MapDynamicShift,     4, "obj_ride_the_surge",      "ride_the_surge"},
        {8,  MissionType::JointOperation,      4, "obj_joint_ascent",        "joint_ascent"},
        {9,  MissionType::FactionCivilWar,     5, "obj_purge_the_dissenters","purge_the_dissenters"},
        {10, MissionType::FinalClimax,         5, "obj_crown_the_emperor",   "crown_the_emperor"},
    };

    for (const Plan& P : Plans)
    {
        Chapter.Missions.push_back(
            BuildGenericMission("eac", P.Index, FactionId::EasternCoalition, P.Type, P.Difficulty,
                                P.ObjectiveId, P.ObjectiveKey, Us, Them));
    }

    return Chapter;
}

// ---------------------------------------------------------------------------
// 4. Chrono Legion -- the Archivist (8 missions, secret)
// ---------------------------------------------------------------------------
CampaignChapterDef BuildChronoChapter()
{
    CampaignChapterDef Chapter;
    Chapter.Faction = FactionId::ChronoLegion;
    Chapter.CampaignTitleKey = "campaign.chro.title";
    Chapter.CommanderNameKey = "character.archivist.name";
    Chapter.DescriptionKey = "campaign.chro.description";
    Chapter.bIsSecretCampaign = true;
    Chapter.bUnlockedByDefault = false;

    const FactionKit& Us = ChronoKit;
    const FactionKit& Them = SovietKit;

    struct Plan
    {
        int32_t Index;
        MissionType Type;
        int32_t Difficulty;
        const char* ObjectiveId;
        const char* ObjectiveKey;
    };
    static const Plan Plans[8] = {
        {1, MissionType::LimitedForce,    3, "obj_enter_the_erasure",   "enter_the_erasure"},
        {2, MissionType::BaseBuilding,    3, "obj_anchor_the_ark",      "anchor_the_ark"},
        {3, MissionType::StealthInfiltration, 4, "obj_read_the_archive","read_the_archive"},
        {4, MissionType::DefenseHold,     4, "obj_hold_the_causality",  "hold_the_causality"},
        {5, MissionType::MapDynamicShift, 5, "obj_survive_the_unwriting","survive_unwriting"},
        {6, MissionType::ObjectCapture,   5, "obj_recover_the_index",   "recover_the_index"},
        {7, MissionType::FactionCivilWar, 5, "obj_end_the_schism",      "end_the_schism"},
        {8, MissionType::FinalClimax,     5, "obj_close_the_loop",      "close_the_loop"},
    };

    for (const Plan& P : Plans)
    {
        Chapter.Missions.push_back(
            BuildGenericMission("chro", P.Index, FactionId::ChronoLegion, P.Type, P.Difficulty,
                                P.ObjectiveId, P.ObjectiveKey, Us, Them));
    }

    return Chapter;
}

/** Builds a mission whose shape is entirely determined by its MissionType.

    The two chapters below use this because the Coalition and Chrono Legion have no
    entry in DefaultContent, so hand-placing their forces tile by tile would be
    authoring against content nobody can load yet. What they get instead is a
    structurally complete, mechanically valid mission per type -- a real objective the
    runtime can judge, a real failure clause, a real opening force -- which is a very
    different thing from the `for` loop this file used to be, but is honestly less than
    the two hand-authored chapters above. */
CampaignMissionDef BuildGenericMission(const char* Chapter, int32_t Index, FactionId Faction,
                                       MissionType Type, int32_t Difficulty,
                                       const char* ObjectiveId, const char* ObjectiveKey,
                                       const FactionKit& Us, const FactionKit& Them)
{
    CampaignMissionDef M = MakeMission(Chapter, Index, Faction, Type, Difficulty);
    // Seeds are derived from the mission id rather than hand-picked, so two missions
    // can never accidentally share one and produce the same match.
    M.Setup = MakeSetup(MakeContentId(M.MissionId.c_str()).Value, Faction, OpponentOf(Faction),
                        4000 + Index * 800, 4000 + Index * 1600);

    const std::string Key = ObjKey(Chapter, Index, ObjectiveKey);

    switch (Type)
    {
    case MissionType::LimitedForce:
        // No economy: a squad and something in the way.
        M.Setup.Players[kPlayerSlot].StartingCredits = 0;
        M.Setup.Players[kEnemySlot].StartingCredits = 0;
        AddSquad(M.Setup, Us.Infantry, kPlayerSlot, 8, 8, 6);
        AddSquad(M.Setup, Us.AntiTankInfantry, kPlayerSlot, 8, 10, 2);
        M.Setup.Spawns.push_back(Building(Them.Turret, kEnemySlot, 46, 48));
        AddSquad(M.Setup, Them.Infantry, kEnemySlot, 44, 46, 4);
        M.Objectives.push_back(Primary(ObjectiveId, Key, Reach(kPlayerSlot, 46, 50, 4)));
        break;

    case MissionType::StealthInfiltration:
        M.Setup.Players[kPlayerSlot].StartingCredits = 0;
        AddSquad(M.Setup, Us.Infantry, kPlayerSlot, 6, 58, 4);
        M.Setup.Spawns.push_back(Building(Them.ConYard, kEnemySlot, 50, 8));
        AddSquad(M.Setup, Them.Infantry, kEnemySlot, 40, 20, 5);
        M.Objectives.push_back(Primary(ObjectiveId, Key, Reach(kPlayerSlot, 50, 8, 3)));
        break;

    case MissionType::DefenseHold:
        AddBase(M.Setup, Us, kPlayerSlot, 30, 30, 26, 36);
        M.Setup.Spawns.push_back(Building(Us.Turret, kPlayerSlot, 27, 27));
        AddOutpost(M.Setup, Them, kEnemySlot, 8, 8, 5);
        AddOutpost(M.Setup, Them, kEnemySlot, 52, 52, 5);
        M.Objectives.push_back(Primary(ObjectiveId, Key, SurviveMinutes(6 + Index / 3)));
        M.FailureConditions.push_back(FewerThan(kPlayerSlot, Us.ConYard, 1));
        break;

    case MissionType::EscortConvoy:
        AddSquad(M.Setup, Us.Tank, kPlayerSlot, 8, 8, 3);
        AddSquad(M.Setup, Us.AntiTankInfantry, kPlayerSlot, 8, 10, 4);
        AddSquad(M.Setup, Them.Tank, kEnemySlot, 32, 32, 3);
        AddOutpost(M.Setup, Them, kEnemySlot, 48, 48, 4);
        M.Objectives.push_back(Primary(ObjectiveId, Key, Reach(kPlayerSlot, 54, 54, 4)));
        break;

    case MissionType::ObjectCapture:
        AddBase(M.Setup, Us, kPlayerSlot, 10, 10, 6, 16);
        M.Setup.Spawns.push_back(Building(Them.ConYard, kEnemySlot, 46, 46));
        M.Setup.Spawns.push_back(Building(Them.Turret, kEnemySlot, 42, 42));
        AddSquad(M.Setup, Them.Infantry, kEnemySlot, 44, 40, 6);
        M.Objectives.push_back(Primary(ObjectiveId, Key, DestroyAllOf(kEnemySlot, Them.Turret)));
        M.Objectives.push_back(Primary("obj_hold_the_site",
                                       ObjKey(Chapter, Index, "hold_the_site"),
                                       Reach(kPlayerSlot, 46, 46, 3)));
        break;

    case MissionType::MapDynamicShift:
        AddBase(M.Setup, Us, kPlayerSlot, 12, 50, 8, 44);
        AddBase(M.Setup, Them, kEnemySlot, 48, 12, 54, 18);
        AddSquad(M.Setup, Them.Tank, kEnemySlot, 44, 18, 3);
        M.Objectives.push_back(Primary(ObjectiveId, Key, SurviveMinutes(6)));
        M.Objectives.push_back(Staged("obj_counterattack",
                                      ObjKey(Chapter, Index, "counterattack"),
                                      DestroyAllOf(kEnemySlot, Them.ConYard)));
        break;

    case MissionType::JointOperation:
        M.Setup.Players[kAllySlot].bActive = true;
        M.Setup.Players[kAllySlot].Faction = OpponentOf(Faction);
        M.Setup.Players[kAllySlot].StartingCredits = 5000;
        AddBase(M.Setup, Us, kPlayerSlot, 12, 12, 8, 18);
        AddOutpost(M.Setup, Them, kAllySlot, 12, 48, 4);
        AddBase(M.Setup, Them, kEnemySlot, 46, 30, 52, 24);
        M.Objectives.push_back(Primary(ObjectiveId, Key, DestroyAllOf(kEnemySlot, Them.ConYard)));
        M.FailureConditions.push_back(Eliminate(kAllySlot));
        break;

    case MissionType::FactionCivilWar:
        // Both sides the player's own faction, and the enemy opens ahead.
        M.Setup.Players[kEnemySlot].Faction = Faction;
        AddBase(M.Setup, Us, kPlayerSlot, 10, 10, 6, 16);
        AddBase(M.Setup, Us, kEnemySlot, 48, 48, 54, 42);
        AddSquad(M.Setup, Us.Tank, kEnemySlot, 44, 44, 4);
        M.Objectives.push_back(Primary(ObjectiveId, Key, Eliminate(kEnemySlot)));
        break;

    case MissionType::FinalClimax:
        AddBase(M.Setup, Us, kPlayerSlot, 10, 10, 6, 16);
        M.Setup.Spawns.push_back(Building(Us.WarFactory, kPlayerSlot, 14, 10));
        AddBase(M.Setup, Them, kEnemySlot, 48, 48, 54, 42);
        M.Setup.Spawns.push_back(Building(Them.WarFactory, kEnemySlot, 44, 48));
        M.Setup.Spawns.push_back(Building(Them.Turret, kEnemySlot, 44, 42));
        AddSquad(M.Setup, Them.Tank, kEnemySlot, 44, 38, 4);
        M.Objectives.push_back(Primary(ObjectiveId, Key, Eliminate(kEnemySlot)));
        break;

    case MissionType::BaseBuilding:
    default:
        AddBase(M.Setup, Us, kPlayerSlot, 12, 12, 8, 18);
        AddOutpost(M.Setup, Them, kEnemySlot, 46, 46);
        M.Objectives.push_back(Primary(ObjectiveId, Key, DestroyAllOf(kEnemySlot, Them.ConYard)));
        M.Objectives.push_back(Secondary("obj_stockpile", ObjKey(Chapter, Index, "stockpile"),
                                         Credits(kPlayerSlot, 8000)));
        M.FailureConditions.push_back(FewerThan(kPlayerSlot, Us.ConYard, 1));
        break;
    }

    AddDefaultFailure(M);
    return M;
}

} // namespace

CampaignDatabase::CampaignDatabase()
{
    InitializeDefaultCampaigns();
}

void CampaignDatabase::InitializeDefaultCampaigns()
{
    Chapters.clear();
    Chapters.push_back(BuildSovietChapter());
    Chapters.push_back(BuildAllianceChapter());
    Chapters.push_back(BuildCoalitionChapter());
    Chapters.push_back(BuildChronoChapter());
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
    return MakeSokolovCutscene();
}

} // namespace RA4
