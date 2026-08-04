// Copyright (c) Red Alert 4 project. Mission objective evaluation.
#include "MissionRuntime.h"

#include "RA4Content/ContentDatabase.h"

namespace RA4
{

namespace
{

/** Counts live entities owned by Subject. An invalid Def matches every entity, which
    is what makes an at-most-zero condition on an invalid Def mean "eliminated". */
int32_t CountOwned(const SimWorld& World, PlayerId Subject, ContentId Def)
{
    int32_t Count = 0;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (size_t I = 0; I < Cores.size(); ++I)
    {
        const EntityCore& C = Cores[I];
        if (!C.bAlive || C.Owner != Subject)
        {
            continue;
        }
        // Resource nodes are terrain that happens to be an entity. Counting them
        // would make "destroy everything this player owns" impossible on any map
        // with ore on it, and would make "own at least one thing" trivially true.
        if (C.Kind == EntityKind::ResourceNode || C.Kind == EntityKind::Projectile)
        {
            continue;
        }
        if (Def.IsValid() && C.Def != Def)
        {
            continue;
        }
        ++Count;
    }
    return Count;
}

/** True if any live entity of Subject is inside RadiusTiles of TargetTile. Distance
    is compared squared and in tile units, so the whole test is integer arithmetic and
    cannot drift between two peers running the same mission. */
bool AnyOwnedNearTile(const SimWorld& World, PlayerId Subject, const TileCoord& Target,
                      int32_t RadiusTiles)
{
    if (RadiusTiles < 0)
    {
        return false;
    }

    const MapDescription& Map = World.GetMap();
    const int64_t RadiusSq = int64_t(RadiusTiles) * int64_t(RadiusTiles);

    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (size_t I = 0; I < Cores.size(); ++I)
    {
        const EntityCore& C = Cores[I];
        if (!C.bAlive || C.Owner != Subject || C.Kind == EntityKind::ResourceNode ||
            C.Kind == EntityKind::Projectile)
        {
            continue;
        }

        const TransformComp* Transform = World.GetTransform(World.MakeId(uint32_t(I)));
        if (Transform == nullptr)
        {
            continue;
        }

        const TileCoord Tile = Map.WorldToTile(Transform->Position);
        const int64_t DX = int64_t(Tile.X) - int64_t(Target.X);
        const int64_t DY = int64_t(Tile.Y) - int64_t(Target.Y);
        if (DX * DX + DY * DY <= RadiusSq)
        {
            return true;
        }
    }
    return false;
}

} // namespace

// --- MissionRuntime ---------------------------------------------------------

void MissionRuntime::Begin(const CampaignMissionDef& Mission, PlayerId LocalPlayer,
                           TickIndex InStartTick)
{
    Objectives = Mission.Objectives;
    FailureConditions = Mission.FailureConditions;
    Transitions.clear();
    Player = LocalPlayer;
    StartTick = InStartTick;
    TriggeredFailure = -1;
    Status = MissionStatus::InProgress;
}

const MissionObjective* MissionRuntime::FindObjective(const std::string& Id) const
{
    for (const MissionObjective& Objective : Objectives)
    {
        if (Objective.Id == Id)
        {
            return &Objective;
        }
    }
    return nullptr;
}

bool MissionRuntime::RevealObjective(const std::string& Id)
{
    for (MissionObjective& Objective : Objectives)
    {
        if (Objective.Id == Id && Objective.State == ObjectiveState::Hidden)
        {
            Objective.State = ObjectiveState::Active;
            Transitions.push_back({Objective.Id, ObjectiveState::Active, StartTick});
            return true;
        }
    }
    return false;
}

void MissionRuntime::SetObjectiveState(MissionObjective& Objective, ObjectiveState NewState,
                                       TickIndex Tick)
{
    if (Objective.State == NewState)
    {
        return;
    }
    Objective.State = NewState;
    Transitions.push_back({Objective.Id, NewState, Tick});
}

bool MissionRuntime::EvaluateCondition(const SimWorld& World, const ObjectiveCondition& C) const
{
    switch (C.Type)
    {
    case ObjectiveConditionType::EntityCountAtMost:
        return CountOwned(World, C.Subject, C.Def) <= C.Amount;

    case ObjectiveConditionType::EntityCountAtLeast:
        return CountOwned(World, C.Subject, C.Def) >= C.Amount;

    case ObjectiveConditionType::CreditsAtLeast:
        if (C.Subject >= kMaxPlayers)
        {
            return false;
        }
        return World.GetPlayer(C.Subject).Credits >= C.Amount;

    case ObjectiveConditionType::SurviveTicks:
        // Unsigned tick arithmetic: comparing the difference rather than
        // StartTick + Amount keeps this correct if the tick counter ever wraps.
        return World.GetTick() >= StartTick &&
               (World.GetTick() - StartTick) >= TickIndex(C.Amount);

    case ObjectiveConditionType::ReachLocation:
        return AnyOwnedNearTile(World, C.Subject, C.TargetTile, C.RadiusTiles);

    case ObjectiveConditionType::None:
    default:
        // An unauthored condition is never met. This is the whole reason the
        // placeholder objectives the campaign used to ship with are now detectable:
        // a mission built out of them cannot be won, and a test says so.
        return false;
    }
}

bool MissionRuntime::AllPrimariesComplete() const
{
    bool bFoundPrimary = false;
    for (const MissionObjective& Objective : Objectives)
    {
        if (!Objective.bIsPrimary || Objective.State == ObjectiveState::Hidden)
        {
            continue;
        }
        bFoundPrimary = true;
        if (Objective.State != ObjectiveState::Completed)
        {
            return false;
        }
    }
    // A mission with no visible primary objective is not a mission the player has
    // won; it is a mission nobody finished writing.
    return bFoundPrimary;
}

MissionStatus MissionRuntime::Evaluate(const SimWorld& World)
{
    if (Status != MissionStatus::InProgress)
    {
        return Status;
    }

    const TickIndex Tick = World.GetTick();

    for (size_t I = 0; I < FailureConditions.size(); ++I)
    {
        if (EvaluateCondition(World, FailureConditions[I]))
        {
            TriggeredFailure = int32_t(I);
            Status = MissionStatus::Lost;
            // Every objective still open is now unreachable. Marking them failed is
            // what lets the debrief screen show what was left undone rather than a
            // list frozen mid-progress.
            for (MissionObjective& Objective : Objectives)
            {
                if (Objective.State == ObjectiveState::Active)
                {
                    SetObjectiveState(Objective, ObjectiveState::Failed, Tick);
                }
            }
            return Status;
        }
    }

    // The simulation's own victory system outranks the objective list. If the match
    // has ended, the mission has ended with it, whatever the objectives say -- a
    // player who lost every structure has not won just because a survive-timer
    // happened to elapse on the same tick.
    if (World.GetPhase() == MatchPhase::Finished)
    {
        const PlayerId Winner = World.GetWinner();
        Status = (Winner == Player) ? MissionStatus::Won : MissionStatus::Lost;
        if (Status == MissionStatus::Lost)
        {
            for (MissionObjective& Objective : Objectives)
            {
                if (Objective.State == ObjectiveState::Active)
                {
                    SetObjectiveState(Objective, ObjectiveState::Failed, Tick);
                }
            }
        }
        return Status;
    }

    for (MissionObjective& Objective : Objectives)
    {
        // Completion latches. "Build a refinery" stays done after the refinery is
        // shelled: an objective the player achieved is not un-achieved by later
        // losses, and a mission that oscillated between won and unwon would be
        // decided by whichever tick the last objective happened to be checked on.
        if (Objective.State != ObjectiveState::Active)
        {
            continue;
        }
        if (EvaluateCondition(World, Objective.Condition))
        {
            SetObjectiveState(Objective, ObjectiveState::Completed, Tick);
        }
    }

    if (AllPrimariesComplete())
    {
        Status = MissionStatus::Won;
    }

    return Status;
}

// --- Bringing a mission up ---------------------------------------------------

MatchSetup BuildMissionMatchSetup(const CampaignMissionDef& Mission)
{
    const MissionSetupDef& Setup = Mission.Setup;

    MatchSetup Out;
    Out.Seed = Setup.Seed;
    Out.Map.Name = Setup.MapName.empty() ? Mission.MissionId : Setup.MapName;
    Out.Map.Resize(Setup.MapWidth, Setup.MapHeight, Tile_GroundPassable);

    for (int32_t I = 0; I < int32_t(kMaxPlayers); ++I)
    {
        Out.Players[I].bActive = Setup.Players[I].bActive;
        Out.Players[I].Faction = Setup.Players[I].Faction;
        Out.Players[I].StartingCredits = Setup.Players[I].StartingCredits;
        Out.Players[I].StartPositionIndex = I;
    }

    return Out;
}

int32_t SpawnMissionEntities(SimWorld& World, const CampaignMissionDef& Mission)
{
    const ContentDatabase* Content = World.GetContent();
    int32_t Placed = 0;

    for (const MissionSpawn& Spawn : Mission.Setup.Spawns)
    {
        if (Content == nullptr)
        {
            break;
        }

        // A mission referring to content this build does not have is a data bug, not
        // a crash. Skipping keeps the rest of the mission placeable so the shortfall
        // is visible as a count instead of as an empty map.
        EntityId Id;
        switch (Spawn.Kind)
        {
        case MissionSpawnKind::Building:
            if (Content->FindEntity(Spawn.Def) != nullptr)
            {
                Id = World.SpawnBuilding(Spawn.Def, Spawn.Owner, Spawn.Tile, /*bInstantComplete*/ true);
            }
            break;

        case MissionSpawnKind::ResourceNode:
            if (Content->FindResourceNode(Spawn.Def) != nullptr)
            {
                Id = World.SpawnResourceNode(Spawn.Def, Spawn.Tile, Spawn.Amount);
            }
            break;

        case MissionSpawnKind::Unit:
        default:
            if (Content->FindEntity(Spawn.Def) != nullptr)
            {
                Id = World.SpawnUnit(Spawn.Def, Spawn.Owner, World.GetMap().TileCenterToWorld(Spawn.Tile));
            }
            break;
        }

        if (Id.IsValid())
        {
            ++Placed;
        }
    }

    return Placed;
}

} // namespace RA4
