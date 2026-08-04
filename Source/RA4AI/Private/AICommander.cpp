// Copyright (c) Red Alert 4 project.
#include "RA4AI/AICommander.h"

#include "RA4Core/SimConfig.h"

#include <algorithm>

namespace RA4
{
namespace AI
{

namespace
{
bool IsPowerPlant(const EntityDef& D) { return D.Kind == EntityKind::Building && (D.Building.bIsPowerPlant || HasRole(D.Roles, EntityRole::Power)); }
bool IsRefinery(const EntityDef& D) { return D.Kind == EntityKind::Building && (D.Building.bIsRefinery || HasRole(D.Roles, EntityRole::Refinery)); }

    // Deterministic integer-only staging offset.  A tile step toward the target,
    // clamped so the staging point stays on the map and never collapses into the yard.
    TileCoord StagingOffsetTowardTarget(const MapDescription& Map,
                                        TileCoord YardTile,
                                        TileCoord TargetTile,
                                        int32_t MaxOffset)
    {
        int32_t DX = TargetTile.X - YardTile.X;
        int32_t DY = TargetTile.Y - YardTile.Y;
        if (DX == 0 && DY == 0)
        {
            return YardTile;
        }

        // Scale each axis independently so the staging point is roughly MaxOffset
        // tiles away along the dominant direction.  Pure integer math, no sqrt,
        // no trig, no platform-dependent floating point.
        const int32_t AbsDX = DX >= 0 ? DX : -DX;
        const int32_t AbsDY = DY >= 0 ? DY : -DY;
        const int32_t Dominant = AbsDX > AbsDY ? AbsDX : AbsDY;
        const int32_t Scale = Dominant > MaxOffset ? MaxOffset : Dominant;

        const int32_t StepX = Dominant == 0 ? 0 : (DX * Scale) / Dominant;
        const int32_t StepY = Dominant == 0 ? 0 : (DY * Scale) / Dominant;

        TileCoord Result(YardTile.X + StepX, YardTile.Y + StepY);
        Result.X = Result.X < 0 ? 0 : (Result.X >= Map.Width ? Map.Width - 1 : Result.X);
        Result.Y = Result.Y < 0 ? 0 : (Result.Y >= Map.Height ? Map.Height - 1 : Result.Y);
        return Result;
    }

    // A barracks and a war factory are identified by what they can produce rather than
    // by name, so the commander needs no per-faction table.

bool IsCombatUnitDef(const EntityDef* Def)
{
    if (Def == nullptr || Def->Kind != EntityKind::Unit)
    {
        return false;
    }
    if (HasRole(Def->Roles, EntityRole::Combat))
    {
        return true;
    }
    return Def->Weapon.IsValid() && !Def->Unit.bIsHarvester && !Def->Unit.bIsBuilder;
}

bool IsWounded(const SimWorld& World, EntityId Id)
{
    const HealthComp* Health = World.GetHealth(Id);
    return Health != nullptr && Health->Max > 0 &&
           int64_t(Health->Current) * 4 < int64_t(Health->Max);
}

bool ProducesCategory(const ContentDatabase& Content, ContentId Building, ProductionCategory Category)
{
    for (const EntityDef& Def : Content.GetEntities())
    {
        if (Def.Production.Category != Category)
        {
            continue;
        }
        if (std::find(Def.Production.ProducedBy.begin(), Def.Production.ProducedBy.end(), Building) !=
            Def.Production.ProducedBy.end())
        {
            return true;
        }
    }
    return false;
}
} // namespace

void AICommander::Initialize(PlayerId InPlayer, AIProfile InProfile, uint64_t Seed)
{
    Player = InPlayer;
    Profile = InProfile;
    Config = MakeProfileConfig(InProfile);
    // Seeded from the match seed and the player slot, so two AIs in one match do not
    // draw an identical sequence while a replay still reproduces both exactly.
    Rng.Reset(Seed ^ (uint64_t(InPlayer) * 0x9E3779B97F4A7C15ull));
    Reset();
}

void AICommander::Reset()
{
    TicksSinceDecision = 0;
    ActiveStrategy = AIStrategy::ExpandEconomy;
    PreviousStrategyForDecision = AIStrategy::ExpandEconomy;
    ActiveStrategyScore = 0;
    bHasActiveStrategy = false;
    LastUnderAttackTick = 0;
    bHasSeenAttack = false;
    // Everything the commander thought it knew about the enemy is forgotten with it.
    Knowledge.reset();
    KnowledgeWorld = nullptr;
    TicksSinceMemoryUpdate = 0;
    ScoutUnit = EntityId::Invalid();
    ScoutWaypointIndex = 0;
    LastScoutOrderTick = 0;
    bHasScoutOrder = false;
    ActiveOperation = TacticalOperation();
    DecisionLog.clear();
    // The doctrine is the only Reset exception: it depends on the commander's
    // faction, which is world state, so it is re-resolved on the next first Tick.
    bDoctrineLoaded = false;
    LastHarassRaidTick = 0;
    bHasHarassRaid = false;
}

void AICommander::Log(TickIndex Tick, CommandType Type, ContentId Content, const char* Reason)
{
    AIDecision D;
    D.Tick = Tick;
    D.Command = Type;
    D.Content = Content;
    D.Strategy = ActiveStrategy;
    D.StrategyScore = ActiveStrategyScore;
    D.PreviousStrategy = PreviousStrategyForDecision;
    D.Reason = std::string(ToString(ActiveStrategy)) + ": " + Reason;
    DecisionLog.push_back(D);
    if (DecisionLog.size() > DecisionLogLimit)
    {
        DecisionLog.erase(DecisionLog.begin());
    }
}

void AICommander::LogIdleStrategyDecision(TickIndex Tick)
{
    Log(Tick, CommandType::None, ContentId(),
        "strategy selected; no valid action");
}

void AICommander::LoadDoctrine(const SimWorld& World)
{
    const PlayerState& State = World.GetPlayer(Player);
    Doctrine = AIDoctrineRegistry::GetDoctrineForFaction(State.Faction, Profile);
    Personality = Doctrine.Personality;
    bDoctrineLoaded = true;
}

int32_t AICommander::EffectiveTargetHarvesters() const
{
    // Doctrine beats config only when loaded and non-default: a doctrine field of
    // zero must behave exactly like the old config-only path.
    if (bDoctrineLoaded && Doctrine.TargetHarvesterCount > 0)
    {
        return Doctrine.TargetHarvesterCount;
    }
    return Config.TargetHarvesters;
}

int32_t AICommander::EffectiveAssaultArmySize() const
{
    if (bDoctrineLoaded && Doctrine.MinimumAssaultArmySize > 0)
    {
        return Doctrine.MinimumAssaultArmySize;
    }
    return Config.AttackArmySize;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

ContentId AICommander::FindStructure(const SimWorld& World, bool (*Predicate)(const EntityDef&)) const
{
    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return ContentId();
    }
    const FactionId Faction = World.GetPlayer(Player).Faction;
    for (const EntityDef& Def : Content->GetEntities())
    {
        if (Def.Faction != Faction || Def.Production.ProducedBy.empty())
        {
            continue;
        }
        if (Predicate(Def))
        {
            return Def.Id;
        }
    }
    return ContentId();
}

ContentId AICommander::FindDefenceStructure(const SimWorld& World) const
{
    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return ContentId();
    }
    const FactionId Faction = World.GetPlayer(Player).Faction;
    for (const EntityDef& Def : Content->GetEntities())
    {
        if (Def.Faction != Faction || Def.Kind != EntityKind::Building)
        {
            continue;
        }
        if (Def.Production.Category == ProductionCategory::Defense && Def.Weapon.IsValid())
        {
            return Def.Id;
        }
    }
    return ContentId();
}

ContentId AICommander::FindHarvesterUnit(const SimWorld& World) const
{
    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return ContentId();
    }
    const FactionId Faction = World.GetPlayer(Player).Faction;
    for (const EntityDef& Def : Content->GetEntities())
    {
        if (Def.Faction == Faction && Def.Kind == EntityKind::Unit && Def.Unit.bIsHarvester)
        {
            return Def.Id;
        }
    }
    return ContentId();
}

ContentId AICommander::FindCombatUnit(const SimWorld& World) const
{
    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return ContentId();
    }
    const FactionId Faction = World.GetPlayer(Player).Faction;
    const PlayerState& State = World.GetPlayer(Player);

    // Determine observed enemy composition from Knowledge to select counter-units
    bool bEnemyHasArmored = false;
    bool bEnemyHasAir = false;
    if (Knowledge != nullptr)
    {
        for (const EnemyMemory& Mem : Knowledge->GetKnownEnemies())
        {
            if (Mem.Kind == EntityKind::Unit && Content != nullptr)
            {
                const EntityDef* EnemyDef = Content->FindEntity(Mem.DefId);
                if (EnemyDef != nullptr)
                {
                    if (HasRole(EnemyDef->Roles, EntityRole::Combat) || HasRole(EnemyDef->Roles, EntityRole::AntiArmor))
                    {
                        bEnemyHasArmored = true;
                    }
                    if (HasRole(EnemyDef->Roles, EntityRole::AntiAir))
                    {
                        bEnemyHasAir = true;
                    }
                }
            }
        }
    }

    ContentId Best;
    int32_t BestScore = -1;
    for (const EntityDef& Def : Content->GetEntities())
    {
        if (Def.Faction != Faction || Def.Kind != EntityKind::Unit || !Def.Weapon.IsValid())
        {
            continue;
        }
        if (Def.Unit.bIsHarvester || Def.Unit.bIsBuilder || Def.Production.ProducedBy.empty())
        {
            continue;
        }
        if (!World.HasPrerequisites(Player, Def))
        {
            continue;
        }
        if (State.Credits <
            Def.Production.Cost +
                RequiredCreditReserve(ActiveStrategy, Config))
        {
            continue;
        }

        int32_t Score = Def.Production.Cost;
        if (bEnemyHasArmored && HasRole(Def.Roles, EntityRole::AntiArmor))
        {
            Score += 150;
        }
        if (bEnemyHasAir && HasRole(Def.Roles, EntityRole::AntiAir))
        {
            Score += 150;
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            Best = Def.Id;
        }
    }
    return Best;
}

int32_t AICommander::CountOwned(const SimWorld& World, bool (*Predicate)(const EntityDef&)) const
{
    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return 0;
    }
    int32_t Count = 0;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Owner != Player)
        {
            continue;
        }
        const EntityDef* Def = Content->FindEntity(Cores[I].Def);
        if (Def != nullptr && Predicate(*Def))
        {
            ++Count;
        }
    }
    return Count;
}

int32_t AICommander::CountOwnedUnits(const SimWorld& World, bool bCombatOnly) const
{
    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return 0;
    }
    int32_t Count = 0;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Owner != Player || Cores[I].Kind != EntityKind::Unit)
        {
            continue;
        }
        const EntityDef* Def = Content->FindEntity(Cores[I].Def);
        if (Def == nullptr)
        {
            continue;
        }
        if (bCombatOnly && (!Def->Weapon.IsValid() || Def->Unit.bIsHarvester || Def->Unit.bIsBuilder))
        {
            continue;
        }
        ++Count;
    }
    return Count;
}

int32_t AICommander::CountIdleCombatUnits(const SimWorld& World) const
{
    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return 0;
    }

    int32_t Count = 0;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Owner != Player || Cores[I].Kind != EntityKind::Unit)
        {
            continue;
        }
        const EntityDef* Def = Content->FindEntity(Cores[I].Def);
        if (!IsCombatUnitDef(Def) || IsWounded(World, World.MakeId(I)))
        {
            continue;
        }
        const OrderQueue* Orders = World.GetOrders(World.MakeId(I));
        if (Orders != nullptr && Orders->Count > 0)
        {
            continue;
        }
        ++Count;
    }
    return Count;
}

int32_t AICommander::CountQueued(const SimWorld& World, ContentId Content) const
{
    int32_t Count = 0;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Owner != Player || Cores[I].Kind != EntityKind::Building)
        {
            continue;
        }
        const BuildingComp* Building = World.GetBuilding(World.MakeId(I));
        if (Building == nullptr)
        {
            continue;
        }
        for (const ProductionItem& Item : Building->Queue)
        {
            if (Item.Content == Content)
            {
                ++Count;
            }
        }
    }
    return Count;
}

EntityId AICommander::FindOwnConstructionYard(const SimWorld& World) const
{
    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return EntityId::Invalid();
    }
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Owner != Player || Cores[I].Kind != EntityKind::Building)
        {
            continue;
        }
        const EntityDef* Def = Content->FindEntity(Cores[I].Def);
        if (Def != nullptr && Def->Building.bIsConstructionYard)
        {
            return World.MakeId(I);
        }
    }
    return EntityId::Invalid();
}

void AICommander::UpdateKnowledge(const SimWorld& World)
{
    // A commander reused for a second match must not carry the first one's sightings.
    if (Knowledge == nullptr || KnowledgeWorld != &World)
    {
        Knowledge.reset(new SimWorldView(World, Player));
        KnowledgeWorld = &World;
        TicksSinceMemoryUpdate = Config.MemoryUpdateIntervalTicks;   // observe at once

        // Initialize spatial maps to match dimensions.
        const MapDescription& Map = World.GetMap();
        Threats.Init(Map.Width, Map.Height);
        Values.Init(Map.Width, Map.Height);
    }

    if (++TicksSinceMemoryUpdate < Config.MemoryUpdateIntervalTicks)
    {
        return;
    }
    TicksSinceMemoryUpdate = 0;
    Knowledge->UpdateMemory(TickIndex(std::max(0, Config.MemoryRetentionTicks)));

    // Recompute spatial awareness maps from fog-limited memory.
    const ContentDatabase* Content = World.GetContent();
    Threats.UpdateFromMemory(Knowledge->GetKnownEnemies(), Content,
                             World.GetMap(), World.GetTick());
    Values.UpdateFromWorld(World, Player, Knowledge->GetKnownEnemies(),
                           Content, World.GetTick());
}

TileCoord AICommander::NextScoutWaypoint(const SimWorld& World) const
{
    // A fixed ring of candidate locations derived from the map size: the far corner
    // first, because in a symmetric skirmish that is where the opponent starts, then
    // the remaining quadrants. Deterministic by construction -- index driven, no RNG,
    // so two replays of the same seed scout in the same order.
    const MapDescription& Map = World.GetMap();
    const int32_t W = Map.Width;
    const int32_t H = Map.Height;
    const TileCoord Candidates[6] = {
        TileCoord(W - W / 8, H - H / 8),     // opposite corner: the likely enemy base
        TileCoord(W / 2, H / 2),             // centre
        TileCoord(W - W / 8, H / 8),
        TileCoord(W / 8, H - H / 8),
        TileCoord(W / 2, H - H / 8),
        TileCoord(W - W / 8, H / 2),
    };
    const int32_t Count = int32_t(sizeof(Candidates) / sizeof(Candidates[0]));
    return Candidates[((ScoutWaypointIndex % Count) + Count) % Count];
}

TileCoord AICommander::GetAndAdvanceScoutWaypoint(const SimWorld& World)
{
    const TileCoord Waypoint = NextScoutWaypoint(World);
    ++ScoutWaypointIndex;
    return Waypoint;
}

bool AICommander::TryScout(const SimWorld& World, std::vector<Command>& Out)
{
    // Nothing to look for once something is known.
    KnownTarget Known;
    if (FindKnownEnemyTarget(Known))
    {
        return false;
    }

    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return false;
    }

    // Personalities with a low scout priority deliberately starve scouting of fresh
    // orders: the gap between scout orders stretches from 20 ticks (priority 100)
    // to 600 (priority 0). Default 70 keeps the old, un-paced behaviour visible.
    if (bDoctrineLoaded && bHasScoutOrder && LastScoutOrderTick != 0)
    {
        const int32_t P = Personality.ScoutPriority < 0 ? 0 :
                          (Personality.ScoutPriority > 100 ? 100 : Personality.ScoutPriority);
        const int32_t ScoutGapTicks = 20 + (100 - P) * 580 / 100;
        const TickIndex Now = World.GetTick();
        if (Now >= LastScoutOrderTick && Now - LastScoutOrderTick < TickIndex(ScoutGapTicks))
        {
            return false;
        }
    }

    // Is the current scout still alive and still busy walking to its waypoint?
    if (ScoutUnit.IsValid())
    {
        const EntityCore* Core = World.GetCore(ScoutUnit);
        const bool bAlive = Core != nullptr && Core->bAlive && Core->Owner == Player;
        if (bAlive)
        {
            const OrderQueue* Orders = World.GetOrders(ScoutUnit);
            if (Orders != nullptr && Orders->Count > 0)
            {
                return false;   // still en route; do not re-issue and reset its path
            }
            // Arrived (or was interrupted) and still found nothing: try the next spot.
            if (bHasScoutOrder)
            {
                ++ScoutWaypointIndex;
            }
        }
        else
        {
            // The scout died. That is information in itself, but a replacement is
            // still needed, so fall through and pick one.
            ScoutUnit = EntityId::Invalid();
            bHasScoutOrder = false;
        }
    }

    if (!ScoutUnit.IsValid())
    {
        // First pass: prefer units with EntityRole::Scout
        const std::vector<EntityCore>& Cores = World.GetAllCores();
        for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
        {
            if (!Cores[I].bAlive || Cores[I].Owner != Player || Cores[I].Kind != EntityKind::Unit)
            {
                continue;
            }
            const EntityDef* Def = Content->FindEntity(Cores[I].Def);
            if (Def == nullptr || Def->Unit.bIsHarvester || Def->Unit.bIsBuilder || Def->Unit.MaxSpeed <= Fixed::Zero())
            {
                continue;
            }
            if (HasRole(Def->Roles, EntityRole::Scout))
            {
                ScoutUnit = World.MakeId(I);
                break;
            }
        }

        // Fallback pass: cheapest available mobile unit
        if (!ScoutUnit.IsValid())
        {
            for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
            {
                if (!Cores[I].bAlive || Cores[I].Owner != Player || Cores[I].Kind != EntityKind::Unit)
                {
                    continue;
                }
                const EntityDef* Def = Content->FindEntity(Cores[I].Def);
                if (Def == nullptr || Def->Unit.bIsHarvester || Def->Unit.bIsBuilder || Def->Unit.MaxSpeed <= Fixed::Zero())
                {
                    continue;
                }
                ScoutUnit = World.MakeId(I);
                break;
            }
        }
    }

    if (!ScoutUnit.IsValid())
    {
        return false;
    }

    const TileCoord Waypoint = NextScoutWaypoint(World);
    Command C;
    C.Type = CommandType::Move;
    C.Issuer = Player;
    C.Primary = ScoutUnit;
    C.Location = World.GetMap().TileCenterToWorld(Waypoint);
    Out.push_back(C);
    bHasScoutOrder = true;
    LastScoutOrderTick = World.GetTick();
    Log(World.GetTick(), CommandType::Move, ContentId(), "scouting for the enemy");
    return true;
}

bool AICommander::FindKnownEnemyTarget(KnownTarget& Out) const
{
    // Reads memory only from Knowledge (SimWorldView). No path to SimWorld.
    if (Knowledge == nullptr)
    {
        return false;
    }

    const ContentDatabase* Content = Knowledge->GetContent();
    const EnemyMemory* BestTarget = nullptr;
    int32_t BestScore = -1;

    for (const EnemyMemory& Mem : Knowledge->GetKnownEnemies())
    {
        int32_t Score = 0;
        if (Mem.Kind == EntityKind::Building)
        {
            Score += 1000;
            if (Content != nullptr)
            {
                const EntityDef* Def = Content->FindEntity(Mem.DefId);
                if (Def != nullptr)
                {
                    if (HasRole(Def->Roles, EntityRole::BaseBuilding) || Def->Building.bIsConstructionYard) Score += 500;
                    if (HasRole(Def->Roles, EntityRole::Production)) Score += 400;
                    if (HasRole(Def->Roles, EntityRole::Refinery)) Score += 300;
                    if (HasRole(Def->Roles, EntityRole::Power)) Score += 200;
                    if (HasRole(Def->Roles, EntityRole::Defense)) Score += 100;
                }
            }
        }
        else if (Mem.Kind == EntityKind::Unit)
        {
            Score += 100;
            if (Content != nullptr)
            {
                const EntityDef* Def = Content->FindEntity(Mem.DefId);
                if (Def != nullptr)
                {
                    if (HasRole(Def->Roles, EntityRole::Combat)) Score += 50;
                    if (HasRole(Def->Roles, EntityRole::Harvester)) Score += 40;
                }
            }
        }

        if (BestTarget == nullptr || Score > BestScore ||
            (Score == BestScore && Mem.LastSeenTick > BestTarget->LastSeenTick))
        {
            BestScore = Score;
            BestTarget = &Mem;
        }
    }

    if (BestTarget == nullptr)
    {
        return false;
    }

    Out.Entity = BestTarget->Entity;
    Out.Tile = BestTarget->Position;
    Out.Kind = BestTarget->Kind;
    return true;
}

bool AICommander::IsUnderAttack(const SimWorld& World) const
{
    for (const SimEvent& Event : World.GetEvents())
    {
        if (Event.Type != SimEventType::DamageApplied)
        {
            continue;
        }
        const EntityCore* Core = World.GetCore(Event.Entity);
        if (Core != nullptr && Core->Owner == Player)
        {
            return true;
        }
    }
    return false;
}

AIWorldAssessment AICommander::BuildAssessment(const SimWorld& World) const
{
    AIWorldAssessment Assessment;
    const PlayerState& State = World.GetPlayer(Player);
    Assessment.Credits = State.Credits;
    Assessment.PowerProduced = State.PowerProduced;
    Assessment.PowerConsumed = State.PowerConsumed;
    Assessment.TotalHarvested = State.TotalHarvested;
    Assessment.bAssaultActive =
        (ActiveOperation.State == OperationState::Advancing ||
         ActiveOperation.State == OperationState::Engaging ||
         ActiveOperation.State == OperationState::Staging);

    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return Assessment;
    }

    const ContentId HarvesterContent = FindHarvesterUnit(World);
    const EntityDef* HarvesterDef = Content->FindEntity(HarvesterContent);
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        const EntityCore& Core = Cores[I];
        if (!Core.bAlive)
        {
            continue;
        }

        if (Core.Owner != Player)
        {
            // Deliberately nothing here. Whether an enemy target exists is answered
            // from fog-limited memory below, not by scanning the live world.
            continue;
        }

        const EntityDef* Def = Content->FindEntity(Core.Def);
        if (Def == nullptr)
        {
            continue;
        }

        if (Core.Kind == EntityKind::Building)
        {
            if (Def->Building.bIsConstructionYard)
            {
                Assessment.bHasConstructionYard = true;
            }
            if (Def->Building.bIsPowerPlant)
            {
                ++Assessment.PowerPlants;
            }
            if (Def->Building.bIsRefinery)
            {
                ++Assessment.Refineries;
            }
            if (Def->Production.Category == ProductionCategory::Defense &&
                Def->Weapon.IsValid())
            {
                ++Assessment.Defences;
            }
            if (ProducesCategory(*Content, Def->Id,
                                 ProductionCategory::Infantry) ||
                ProducesCategory(*Content, Def->Id,
                                 ProductionCategory::Vehicle))
            {
                ++Assessment.ProductionBuildings;
            }
            if (HarvesterDef != nullptr &&
                std::find(HarvesterDef->Production.ProducedBy.begin(),
                          HarvesterDef->Production.ProducedBy.end(),
                          Def->Id) !=
                    HarvesterDef->Production.ProducedBy.end())
            {
                Assessment.bCanProduceHarvester = true;
            }
            continue;
        }

        if (Core.Kind != EntityKind::Unit)
        {
            continue;
        }
        if (Def->Unit.bIsHarvester)
        {
            ++Assessment.Harvesters;
        }
        else if (!Def->Unit.bIsBuilder && Def->Weapon.IsValid())
        {
            ++Assessment.ArmedUnits;
        }
    }

    Assessment.bAssaultActive =
        Assessment.bAssaultActive && Assessment.ArmedUnits > 0;
    Assessment.bUnderAttack =
        bHasSeenAttack && World.GetTick() >= LastUnderAttackTick &&
        World.GetTick() - LastUnderAttackTick <=
            TickIndex(Config.UnderAttackMemoryTicks);

    // Only what has actually been observed counts as a target worth planning against.
    KnownTarget Known;
    Assessment.bHasEnemyTarget = FindKnownEnemyTarget(Known);
    return Assessment;
}

bool AICommander::FindPlacementTile(const SimWorld& World, ContentId Structure, TileCoord& OutTile) const
{
    const EntityId Yard = FindOwnConstructionYard(World);
    if (!Yard.IsValid())
    {
        return false;
    }
    const BuildingComp* YardBuilding = World.GetBuilding(Yard);
    if (YardBuilding == nullptr)
    {
        return false;
    }
    const TileCoord Origin = YardBuilding->OriginTile;

    // Expanding rings around the yard, scanned in a fixed order so placement is
    // reproducible. The server still validates the result; this only avoids
    // spamming rejected commands.
    for (int32_t Radius = 2; Radius <= 10; ++Radius)
    {
        for (int32_t DY = -Radius; DY <= Radius; ++DY)
        {
            for (int32_t DX = -Radius; DX <= Radius; ++DX)
            {
                if (std::max(std::abs(DX), std::abs(DY)) != Radius)
                {
                    continue;   // ring only
                }
                const TileCoord Candidate(Origin.X + DX, Origin.Y + DY);
                if (World.IsPlacementValid(Structure, Player, Candidate))
                {
                    OutTile = Candidate;
                    return true;
                }
            }
        }
    }
    return false;
}

bool AICommander::QueueProduction(const SimWorld& World, ContentId Content, const char* Reason,
                                  std::vector<Command>& Out)
{
    if (!Content.IsValid())
    {
        return false;
    }
    const EntityDef* Def = World.GetContent() != nullptr ? World.GetContent()->FindEntity(Content) : nullptr;
    if (Def == nullptr || !World.HasPrerequisites(Player, *Def))
    {
        return false;
    }
    bool bHasCompletedProducer = false;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Owner != Player || Cores[I].Kind != EntityKind::Building)
        {
            continue;
        }
        if (std::find(Def->Production.ProducedBy.begin(), Def->Production.ProducedBy.end(), Cores[I].Def) ==
            Def->Production.ProducedBy.end())
        {
            continue;
        }
        const BuildingComp* Producer = World.GetBuilding(World.MakeId(I));
        if (Producer != nullptr && Producer->State == ConstructionState::Complete)
        {
            bHasCompletedProducer = true;
            break;
        }
    }
    if (!bHasCompletedProducer)
    {
        return false;
    }
    const int32_t RequiredReserve =
        RequiredCreditReserve(ActiveStrategy, Config);
    if (World.GetPlayer(Player).Credits <
        Def->Production.Cost + RequiredReserve)
    {
        return false;
    }
    // One of a kind in flight at a time; without this the AI queues six refineries
    // the moment it can afford one.
    if (CountQueued(World, Content) > 0)
    {
        return false;
    }

    Command C;
    C.Type = CommandType::StartProduction;
    C.Issuer = Player;
    C.Content = Content;
    Out.push_back(C);
    Log(World.GetTick(), CommandType::StartProduction, Content, Reason);
    return true;
}

// ---------------------------------------------------------------------------
// Decision steps
// ---------------------------------------------------------------------------

bool AICommander::TryPlaceFinishedStructure(const SimWorld& World, std::vector<Command>& Out)
{
    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return false;
    }
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Owner != Player || Cores[I].Kind != EntityKind::Building)
        {
            continue;
        }
        const BuildingComp* Building = World.GetBuilding(World.MakeId(I));
        if (Building == nullptr)
        {
            continue;
        }
        for (const ProductionItem& Item : Building->Queue)
        {
            const EntityDef* Def = Content->FindEntity(Item.Content);
            if (Def == nullptr || Def->Kind != EntityKind::Building)
            {
                continue;
            }
            // Structures sit in the queue once complete, waiting for a location.
            if (Item.ProgressTicks < Item.TotalTicks * 100)
            {
                continue;
            }
            TileCoord Tile;
            if (!FindPlacementTile(World, Item.Content, Tile))
            {
                continue;   // no legal spot yet; try again next decision
            }
            Command C;
            C.Type = CommandType::PlaceBuilding;
            C.Issuer = Player;
            C.Content = Item.Content;
            C.Tile = Tile;
            Out.push_back(C);
            Log(World.GetTick(), CommandType::PlaceBuilding, Item.Content, "structure finished, placing");
            return true;
        }
    }
    return false;
}

bool AICommander::TryBuildEconomy(const SimWorld& World, std::vector<Command>& Out)
{
    const PlayerState& State = World.GetPlayer(Player);

    // Power first: a shortage throttles everything else the commander is about to
    // do. The doctrine's buffer tightens the trigger (surplus counted in integer
    // percent of produced power) without changing behaviour when it is zero.
    const int32_t BufferPercent = bDoctrineLoaded ? Doctrine.PowerPlantBuffer : 0;
    const bool bPowerShort = State.PowerConsumed >= State.PowerProduced ||
        (BufferPercent > 0 &&
         int64_t(State.PowerConsumed) * 100 >=
             int64_t(State.PowerProduced) * (100 - BufferPercent));
    if (bPowerShort)
    {
        if (QueueProduction(World, FindStructure(World, &IsPowerPlant), "power at or over capacity", Out))
        {
            return true;
        }
    }

    if (CountOwned(World, &IsRefinery) == 0)
    {
        if (QueueProduction(World, FindStructure(World, &IsRefinery), "no refinery: no income", Out))
        {
            return true;
        }
    }

    const ContentId Harvester = FindHarvesterUnit(World);
    if (Harvester.IsValid())
    {
        const int32_t Have = CountOwnedUnits(World, /*bCombatOnly*/ false) - CountOwnedUnits(World, true);
        if (Have < EffectiveTargetHarvesters())
        {
            if (QueueProduction(World, Harvester, "below harvester target", Out))
            {
                return true;
            }
        }
    }
    return false;
}

bool AICommander::TryBuildTech(const SimWorld& World, std::vector<Command>& Out)
{
    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return false;
    }
    const FactionId Faction = World.GetPlayer(Player).Faction;

    // Walk production categories in tech order and build the first producer the
    // commander is missing.
    const ProductionCategory Wanted[2] = {ProductionCategory::Infantry, ProductionCategory::Vehicle};
    for (const ProductionCategory Category : Wanted)
    {
        ContentId Producer;
        for (const EntityDef& Def : Content->GetEntities())
        {
            if (Def.Faction != Faction || Def.Kind != EntityKind::Building ||
                Def.Production.ProducedBy.empty())
            {
                continue;
            }
            if (ProducesCategory(*Content, Def.Id, Category))
            {
                Producer = Def.Id;
                break;
            }
        }
        if (!Producer.IsValid())
        {
            continue;
        }

        int32_t Owned = 0;
        const std::vector<EntityCore>& Cores = World.GetAllCores();
        for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
        {
            if (Cores[I].bAlive && Cores[I].Owner == Player && Cores[I].Def == Producer)
            {
                ++Owned;
            }
        }
        if (Owned == 0 && QueueProduction(World, Producer, "missing production building", Out))
        {
            return true;
        }
    }
    return false;
}

bool AICommander::TryBuildDefence(const SimWorld& World, std::vector<Command>& Out)
{
    if (Config.TargetDefences <= 0)
    {
        return false;
    }
    const ContentId Defence = FindDefenceStructure(World);
    if (!Defence.IsValid())
    {
        return false;
    }

    const ContentDatabase* Content = World.GetContent();
    int32_t Owned = 0;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (Cores[I].bAlive && Cores[I].Owner == Player && Cores[I].Def == Defence)
        {
            ++Owned;
        }
    }
    (void)Content;
    if (Owned >= Config.TargetDefences)
    {
        return false;
    }
    return QueueProduction(World, Defence, "below defence target", Out);
}

bool AICommander::TryTrainArmy(const SimWorld& World, std::vector<Command>& Out)
{
    const ContentId Unit = FindCombatUnit(World);
    if (!Unit.IsValid())
    {
        return false;
    }
    return QueueProduction(World, Unit, "growing the army", Out);
}

void AICommander::ReconcileSquad(const SimWorld& World)
{
    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return;
    }

    // A personality that hates regrouping drops fewer casualties per decision: the
    // prune runs only every RegroupFrequencyTicks decision cycles (or every cycle
    // when the value is unset). Recruiting idle units is cheap and always allowed.
    const int32_t RegroupGap = bDoctrineLoaded && Personality.RegroupFrequencyTicks > 0
        ? Personality.RegroupFrequencyTicks : 1;

    // Drop destroyed, non-combat or wounded units from the assigned squad.  The
    // order of the remaining units is preserved so that assignment is stable.
    if (World.GetTick() % TickIndex(RegroupGap) == 0)
    {
    std::vector<EntityId> Kept;
    Kept.reserve(ActiveOperation.AssignedUnits.size());
    for (EntityId Id : ActiveOperation.AssignedUnits)
    {
        const EntityCore* Core = World.GetCore(Id);
        if (Core == nullptr || !Core->bAlive || Core->Owner != Player)
        {
            continue;
        }
        const EntityDef* Def = Content->FindEntity(Core->Def);
        if (!IsCombatUnitDef(Def) || IsWounded(World, Id))
        {
            continue;
        }
        Kept.push_back(Id);
    }
    ActiveOperation.AssignedUnits.swap(Kept);
    }

    // Recruit every idle, non-wounded combat unit into an active operation.  Once a
    // push has started, leaving units at base while a small squad attacks alone is the
    // main cause of slow, inconclusive AI matches; the whole idle army should move
    // together.  RequiredCombatUnits only governs the minimum size to *start* an
    // operation, not the cap on how many units may join it.
    if (ActiveOperation.State != OperationState::Completed &&
        ActiveOperation.State != OperationState::Aborted)
    {
        const std::vector<EntityCore>& Cores = World.GetAllCores();
        for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
        {
            const EntityCore& Core = Cores[I];
            if (!Core.bAlive || Core.Owner != Player || Core.Kind != EntityKind::Unit)
            {
                continue;
            }
            const EntityDef* Def = Content->FindEntity(Core.Def);
            if (!IsCombatUnitDef(Def))
            {
                continue;
            }
            const EntityId Id = World.MakeId(I);
            if (Id == ScoutUnit)
            {
                continue;
            }
            if (IsWounded(World, Id))
            {
                continue;
            }
            if (std::find(ActiveOperation.AssignedUnits.begin(),
                          ActiveOperation.AssignedUnits.end(), Id) !=
                ActiveOperation.AssignedUnits.end())
            {
                continue;
            }
            const OrderQueue* Orders = World.GetOrders(Id);
            if (Orders == nullptr || Orders->Count > 0)
            {
                continue;
            }
            ActiveOperation.AssignedUnits.push_back(Id);
        }
    }
}

void AICommander::ComputeStagingPoint(const SimWorld& World)
{
    const EntityId Yard = FindOwnConstructionYard(World);
    const TransformComp* YardTransform =
        Yard.IsValid() ? World.GetTransform(Yard) : nullptr;
    if (YardTransform == nullptr)
    {
        ActiveOperation.StagingPoint = ActiveOperation.TargetLocation;
        return;
    }

    const TileCoord YardTile = World.GetMap().WorldToTile(YardTransform->Position);
    const TileCoord TargetTile = ActiveOperation.TargetLocation;

    constexpr int32_t kStagingOffset = 4;
    ActiveOperation.StagingPoint =
        StagingOffsetTowardTarget(World.GetMap(), YardTile, TargetTile, kStagingOffset);
}

bool AICommander::AllSquadAtStaging(const SimWorld& World) const
{
    if (ActiveOperation.AssignedUnits.empty())
    {
        return false;
    }

    const TickIndex ElapsedStaging = World.GetTick() >= ActiveOperation.LastStateChangeTick ?
        World.GetTick() - ActiveOperation.LastStateChangeTick : 0;
    if (ElapsedStaging >= 100)
    {
        return true;
    }

    const Vec2 StagingWorld = World.GetMap().TileCenterToWorld(ActiveOperation.StagingPoint);
    constexpr int64_t kStagingArriveUnits = 400; // 4 metres
    const Fixed ArriveSq = Fixed::FromInt(kStagingArriveUnits * kStagingArriveUnits);

    size_t ArrivedCount = 0;
    for (EntityId Id : ActiveOperation.AssignedUnits)
    {
        const TransformComp* Transform = World.GetTransform(Id);
        if (Transform != nullptr && DistanceSquared(Transform->Position, StagingWorld) <= ArriveSq)
        {
            ++ArrivedCount;
        }
    }

    return ArrivedCount == ActiveOperation.AssignedUnits.size() ||
           (ArrivedCount * 4 >= ActiveOperation.AssignedUnits.size() * 3);
}

bool AICommander::AnySquadNearTarget(const SimWorld& World, const TileCoord& TargetTile) const
{
    const Vec2 TargetWorld = World.GetMap().TileCenterToWorld(TargetTile);
    constexpr int64_t kEngageArriveUnits = 600; // 6 metres
    const Fixed ArriveSq = Fixed::FromInt(kEngageArriveUnits * kEngageArriveUnits);

    for (EntityId Id : ActiveOperation.AssignedUnits)
    {
        const TransformComp* Transform = World.GetTransform(Id);
        if (Transform != nullptr &&
            DistanceSquared(Transform->Position, TargetWorld) <= ArriveSq)
        {
            return true;
        }
    }
    return false;
}

void AICommander::IssueSquadAttackMove(const SimWorld& World, const Vec2& Destination,
                                       std::vector<Command>& Out)
{
    for (EntityId Id : ActiveOperation.AssignedUnits)
    {
        const OrderQueue* Orders = World.GetOrders(Id);
        if (Orders != nullptr && Orders->Count > 0)
        {
            continue;
        }

        Command C;
        C.Type = CommandType::AttackMove;
        C.Issuer = Player;
        C.Primary = Id;
        C.Location = Destination;
        Out.push_back(C);
    }
}

void AICommander::IssueSquadRetreat(const SimWorld& World, std::vector<Command>& Out)
{
    const EntityId Yard = FindOwnConstructionYard(World);
    const TransformComp* YardTransform =
        Yard.IsValid() ? World.GetTransform(Yard) : nullptr;
    if (YardTransform == nullptr)
    {
        return;
    }

    const Vec2 Destination = YardTransform->Position;
    for (EntityId Id : ActiveOperation.AssignedUnits)
    {
        const OrderQueue* Orders = World.GetOrders(Id);
        if (Orders != nullptr && Orders->Count > 0)
        {
            continue;
        }

        Command C;
        C.Type = CommandType::Move;
        C.Issuer = Player;
        C.Primary = Id;
        C.Location = Destination;
        Out.push_back(C);
    }
}

void AICommander::PruneRetreatedUnits(const SimWorld& World)
{
    const EntityId Yard = FindOwnConstructionYard(World);
    const TransformComp* YardTransform =
        Yard.IsValid() ? World.GetTransform(Yard) : nullptr;
    if (YardTransform == nullptr)
    {
        ActiveOperation.AssignedUnits.clear();
        return;
    }

    constexpr int64_t kRetreatArriveUnits = 500; // 5 metres
    const Fixed ArriveSq = Fixed::FromInt(kRetreatArriveUnits * kRetreatArriveUnits);

    std::vector<EntityId> StillRetreating;
    for (EntityId Id : ActiveOperation.AssignedUnits)
    {
        const TransformComp* Transform = World.GetTransform(Id);
        if (Transform != nullptr &&
            DistanceSquared(Transform->Position, YardTransform->Position) > ArriveSq)
        {
            StillRetreating.push_back(Id);
        }
    }
    ActiveOperation.AssignedUnits.swap(StillRetreating);
}

bool AICommander::TryHarassRaid(const SimWorld& World, int32_t ArmySize,
                                std::vector<Command>& Out)
{
    if (!bDoctrineLoaded)
    {
        return false;
    }
    // Personalities that neither commit nor flank never raid; a zero/default
    // personality keeps the old behaviour of no raids at all.
    if (Personality.Aggressiveness < 60 || Personality.FlankingTendency < 30)
    {
        return false;
    }

    const bool bOperationIdle =
        ActiveOperation.State == OperationState::Proposed ||
        ActiveOperation.State == OperationState::Completed ||
        ActiveOperation.State == OperationState::Aborted;
    if (!bOperationIdle)
    {
        return false;
    }

    // Enough force for a probe, but not the full push -- that is the push's job.
    const int32_t RaidThreshold = std::max(Config.MinimumAttackSize,
                                           EffectiveAssaultArmySize() / 2);
    if (ArmySize < RaidThreshold || ArmySize >= EffectiveAssaultArmySize())
    {
        return false;
    }

    // Raids are gated so one fires at most every RegroupFrequencyTicks ticks.
    const int32_t Gap = Personality.RegroupFrequencyTicks > 0
        ? Personality.RegroupFrequencyTicks : 1;
    const TickIndex Now = World.GetTick();
    if (bHasHarassRaid && Now >= LastHarassRaidTick &&
        Now - LastHarassRaidTick < TickIndex(Gap))
    {
        return false;
    }

    // Raids are an alternative to the big push only while the operation is idle;
    // once the squad is gathering/staging/advancing, harass must not steal ticks.
    if (ActiveOperation.State == OperationState::Gathering ||
        ActiveOperation.State == OperationState::Staging ||
        ActiveOperation.State == OperationState::Advancing)
    {
        return false;
    }

    if (Knowledge == nullptr)
    {
        return false;
    }

    // Raid the most recent harvester/expansion sighting; fall back to any memory
    // that is not the enemy main base (a construction yard is the main push's job).
    const ContentDatabase* Content = Knowledge->GetContent();
    const EnemyMemory* Best = nullptr;
    const EnemyMemory* Fallback = nullptr;
    for (const EnemyMemory& Mem : Knowledge->GetKnownEnemies())
    {
        const EntityDef* Def = Content != nullptr ? Content->FindEntity(Mem.DefId) : nullptr;
        const bool bIsMainBase = Def != nullptr && Def->Building.bIsConstructionYard;
        if (bIsMainBase)
        {
            continue;
        }
        if (Fallback == nullptr || Mem.LastSeenTick > Fallback->LastSeenTick)
        {
            Fallback = &Mem;
        }
        const bool bIsEconomy =
            (Def != nullptr && (HasRole(Def->Roles, EntityRole::Harvester) ||
                                HasRole(Def->Roles, EntityRole::Refinery)));
        if (bIsEconomy && (Best == nullptr || Mem.LastSeenTick > Best->LastSeenTick))
        {
            Best = &Mem;
        }
    }
    if (Best == nullptr)
    {
        Best = Fallback;
    }
    if (Best == nullptr)
    {
        return false;
    }

    // Send half the idle squad through the standard AttackMove path. Which half is
    // chosen from the seeded stream so a replay picks the same raiders.
    ReconcileSquad(World);
    const size_t RaidCount = std::max<size_t>(1, ActiveOperation.AssignedUnits.size() / 2);
    const Vec2 Destination = World.GetMap().TileCenterToWorld(Best->Position);
    size_t Issued = 0;
    for (size_t I = 0; I < ActiveOperation.AssignedUnits.size() && Issued < RaidCount; ++I)
    {
        const size_t Index =
            (I + Rng.NextBelow(uint32_t(ActiveOperation.AssignedUnits.size()))) %
            ActiveOperation.AssignedUnits.size();
        const EntityId Id = ActiveOperation.AssignedUnits[Index];
        const OrderQueue* Orders = World.GetOrders(Id);
        if (Orders != nullptr && Orders->Count > 0)
        {
            continue;
        }
        Command C;
        C.Type = CommandType::AttackMove;
        C.Issuer = Player;
        C.Primary = Id;
        C.Location = Destination;
        Out.push_back(C);
        ++Issued;
    }
    if (Issued == 0)
    {
        return false;
    }

    bHasHarassRaid = true;
    LastHarassRaidTick = Now;
    Log(Now, CommandType::AttackMove, ContentId(), "harass raid");
    return true;
}

void AICommander::CommandArmy(const SimWorld& World, std::vector<Command>& Out)
{
    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return;
    }

    // Retreat is an emergency unit action, not an army-level strategy. It must run
    // even while the force is still smaller than the profile's attack threshold.
    const EntityId Yard = FindOwnConstructionYard(World);
    const TransformComp* YardTransform =
        Yard.IsValid() ? World.GetTransform(Yard) : nullptr;
    if (YardTransform != nullptr)
    {
        const std::vector<EntityCore>& Cores = World.GetAllCores();
        for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
        {
            if (!Cores[I].bAlive || Cores[I].Owner != Player ||
                Cores[I].Kind != EntityKind::Unit)
            {
                continue;
            }
            const EntityDef* Def = Content->FindEntity(Cores[I].Def);
            if (!IsCombatUnitDef(Def))
            {
                continue;
            }

            const EntityId Id = World.MakeId(I);
            const OrderQueue* Orders = World.GetOrders(Id);
            if (!IsWounded(World, Id) || Orders == nullptr || Orders->Count > 0)
            {
                continue;
            }

            Command C;
            C.Type = CommandType::Move;
            C.Issuer = Player;
            C.Primary = Id;
            C.Location = YardTransform->Position;
            Out.push_back(C);
            Log(World.GetTick(), CommandType::Move, Cores[I].Def,
                "wounded unit retreating to base");
        }
    }

    ActiveOperation.RequiredCombatUnits = std::max(EffectiveAssaultArmySize(), Config.AttackArmySize);
    ActiveOperation.MinRetreatUnits = Config.MinimumAttackSize;

    const int32_t ArmySize = CountOwnedUnits(World, /*bCombatOnly*/ true);
    KnownTarget Target;
    const bool bHasTarget = FindKnownEnemyTarget(Target);

    // If no enemy has been spotted yet and we already have idle combat units, start
    // gathering toward the most likely enemy base corner.  This prevents economic or
    // defensive profiles from turtling forever; once they arrive they will sight the
    // real enemy and the target updates to the observed position.
    if (!bHasTarget &&
        (ActiveOperation.State == OperationState::Proposed ||
         ActiveOperation.State == OperationState::Completed ||
         ActiveOperation.State == OperationState::Aborted))
    {
        const int32_t IdleCombat = CountIdleCombatUnits(World);
        if (IdleCombat >= ActiveOperation.MinRetreatUnits)
        {
            ActiveOperation.TargetLocation = GetAndAdvanceScoutWaypoint(World);
            ActiveOperation.StartTick = World.GetTick();
            ActiveOperation.TransitionTo(OperationState::Gathering, World.GetTick());
        }
        else
        {
            // Not enough force to venture out yet; keep building.
            return;
        }
    }

    // Small raids keep pressure on between big operations, but only when a target
    // is actually known: with nothing in memory the scouting/gathering fallback
    // above must run, otherwise one turtle steals every decision tick forever.
    if (bHasTarget && TryHarassRaid(World, ArmySize, Out))
    {
        return;
    }

    if (ArmySize == 0)
    {
        // Wiped out: abort the operation and rebuild before committing again.
        if (ActiveOperation.State != OperationState::Aborted &&
            ActiveOperation.State != OperationState::Completed)
        {
            ActiveOperation.TransitionTo(OperationState::Aborted, World.GetTick());
        }
        ActiveOperation.AssignedUnits.clear();
        return;
    }

    // If the operation is retreating, keep moving surviving units home until they
    // arrive; then reset to Proposed so a new operation can form.
    if (ActiveOperation.State == OperationState::Retreating)
    {
        PruneRetreatedUnits(World);
        if (ActiveOperation.AssignedUnits.empty())
        {
            ActiveOperation.TransitionTo(OperationState::Completed, World.GetTick());
            ActiveOperation.State = OperationState::Proposed;
        }
        else
        {
            IssueSquadRetreat(World, Out);
        }
        return;
    }

    // If we have a fresh target, update the objective.  Otherwise, keep driving
    // toward the last known location so a squad that has committed does not stop
    // dead the moment fog rolls in.
    if (bHasTarget)
    {
        ActiveOperation.TargetLocation = Target.Tile;
    }
    else if (ActiveOperation.State == OperationState::Proposed ||
             ActiveOperation.State == OperationState::Completed ||
             ActiveOperation.State == OperationState::Aborted)
    {
        // Nothing to attack and no active operation to finish.
        return;
    }
    else if (ActiveOperation.State == OperationState::Advancing ||
             ActiveOperation.State == OperationState::Engaging)
    {
        // The squad arrived at a stale target and saw nothing: there may be a real
        // base just over the fog edge. Pivot to the next scout waypoint instead of
        // idling at an empty tile until MemoryRetentionTicks expire.
        if (AnySquadNearTarget(World, ActiveOperation.TargetLocation))
        {
            ActiveOperation.TargetLocation = GetAndAdvanceScoutWaypoint(World);
        }
    }

    ReconcileSquad(World);
    ComputeStagingPoint(World);

    const size_t CommandsBefore = Out.size();

    switch (ActiveOperation.State)
    {
        case OperationState::Proposed:
        case OperationState::Completed:
        case OperationState::Aborted:
        {
            // Start gathering whenever we know where the enemy is.
            ActiveOperation.StartTick = World.GetTick();
            ActiveOperation.TransitionTo(OperationState::Gathering, World.GetTick());
            // Fall through to gather/stage logic on the same decision tick.
        }
        [[fallthrough]];
        case OperationState::Gathering:
        {
            // Commit once the squad reaches the minimum attack size.  Waiting for the
            // full attack size made the AI too passive in skirmishes where vision was
            // limited or the opponent turtled.
            if (ActiveOperation.AssignedUnits.size() >=
                size_t(ActiveOperation.MinRetreatUnits))
            {
                ActiveOperation.TransitionTo(OperationState::Staging, World.GetTick());
            }
            else
            {
                // Units are still being recruited into the squad; issue no orders so
                // they wait near the base and do not wander toward the target alone.
                break;
            }
        }
        [[fallthrough]];
        case OperationState::Staging:
        {
            const Vec2 StagingWorld =
                World.GetMap().TileCenterToWorld(ActiveOperation.StagingPoint);
            IssueSquadAttackMove(World, StagingWorld, Out);
            if (AllSquadAtStaging(World))
            {
                ActiveOperation.TransitionTo(OperationState::Advancing, World.GetTick());
            }
            break;
        }
        case OperationState::Advancing:
        {
            const Vec2 TargetWorld =
                World.GetMap().TileCenterToWorld(ActiveOperation.TargetLocation);
            IssueSquadAttackMove(World, TargetWorld, Out);
            if (AnySquadNearTarget(World, ActiveOperation.TargetLocation))
            {
                ActiveOperation.TransitionTo(OperationState::Engaging, World.GetTick());
            }
            break;
        }
        case OperationState::Engaging:
        {
            // Re-issue AttackMove to keep idle stragglers moving toward the target.
            const Vec2 TargetWorld =
                World.GetMap().TileCenterToWorld(ActiveOperation.TargetLocation);
            IssueSquadAttackMove(World, TargetWorld, Out);

            if (ActiveOperation.AssignedUnits.size() < size_t(ActiveOperation.MinRetreatUnits))
            {
                ActiveOperation.TransitionTo(OperationState::Retreating, World.GetTick());
                IssueSquadRetreat(World, Out);
            }
            else if (bDoctrineLoaded && int64_t(ActiveOperation.AssignedUnits.size()) * 100 <
                     int64_t(ActiveOperation.RequiredCombatUnits > 0 ? ActiveOperation.RequiredCombatUnits : 1) *
                         int64_t(Personality.AcceptableLossesPercent < 100 ? Personality.AcceptableLossesPercent : 99) &&
                     int64_t(Personality.AcceptableLossesPercent) * 2 < 100)
            {
                // A personality that hates accepting losses breaks off once roughly
                // AcceptableLossesPercent of the required force is gone; the default
                // (no doctrine) keeps the original single MinRetreatUnits rule.
                ActiveOperation.TransitionTo(OperationState::Retreating, World.GetTick());
                IssueSquadRetreat(World, Out);
            }
            else
            {
                // The push has committed and still has combat strength.  Keep refreshing
                // the target from memory so the army pivots to newly sighted enemy
                // buildings or units rather than attacking a ghost position forever.
                if (bHasTarget)
                {
                    ActiveOperation.TargetLocation = Target.Tile;
                }
                else if (AnySquadNearTarget(World, ActiveOperation.TargetLocation))
                {
                    ActiveOperation.TransitionTo(OperationState::Completed, World.GetTick());
                    ActiveOperation.State = OperationState::Proposed;
                }
            }
            break;
        }
        case OperationState::Retreating:
        {
            // Handled above; kept here to silence exhaustiveness warnings.
            IssueSquadRetreat(World, Out);
            break;
        }
    }

    if (Out.size() > CommandsBefore)
    {
        const char* Reason = nullptr;
        switch (ActiveOperation.State)
        {
            case OperationState::Staging:
                Reason = "squad staging before assault";
                break;
            case OperationState::Advancing:
                Reason = "squad advancing on last known enemy position";
                break;
            case OperationState::Engaging:
                Reason = "squad engaging target area";
                break;
            case OperationState::Retreating:
                Reason = "squad retreating after losses";
                break;
            default:
                Reason = "squad regrouping";
                break;
        }
        Log(World.GetTick(), CommandType::AttackMove, ContentId(), Reason);
        // The first time the squad actually moves, allocate a real operation id.
        if (ActiveOperation.OperationId == 0)
        {
            ActiveOperation.OperationId = 1;
        }
    }
}

bool AICommander::ExecuteStrategy(AIStrategy Strategy,
                                  const SimWorld& World,
                                  std::vector<Command>& Out)
{
    switch (Strategy)
    {
        case AIStrategy::Opening:
        case AIStrategy::ExpandEconomy:
        case AIStrategy::Expansion:
        case AIStrategy::Recover:
        {
            // An economy-focused strategy that has nothing left to build (harvester
            // target met, power fine, treasury full) must not starve the army: the
            // Economic profile otherwise accumulates credits forever and the match
            // never produces a winner. If the economy step issued nothing, fall
            // through to army training whenever a producer exists.
            if (TryBuildEconomy(World, Out))
            {
                return true;
            }
            if (CountOwnedUnits(World, /*bCombatOnly*/ true) < Config.AttackArmySize &&
                FindCombatUnit(World).IsValid())
            {
                return TryTrainArmy(World, Out);
            }
            return false;
        }
        case AIStrategy::TechUp:
            return TryBuildTech(World, Out);
        case AIStrategy::Fortify:
        {
            // A Fortify commander with no production and no army can never satisfy
            // either half of the branch (no defence prerequisites, no unit
            // producers), so it turtles at full credits forever. Build the missing
            // producers first; this is the only way Fortify can act at all.
            if (CountOwnedUnits(World, /*bCombatOnly*/ true) == 0 &&
                TryBuildTech(World, Out))
            {
                return true;
            }
            if (TryBuildDefence(World, Out))
            {
                return true;
            }
            return TryTrainArmy(World, Out);
        }
        case AIStrategy::AssembleArmy:
            return TryTrainArmy(World, Out);
        case AIStrategy::Assault:
        case AIStrategy::FinalAssault:
        {
            const size_t CommandCountBefore = Out.size();
            CommandArmy(World, Out);
            const bool bArmyCommandIssued =
                Out.size() > CommandCountBefore;
            return TryTrainArmy(World, Out) || bArmyCommandIssued;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void AICommander::Tick(const SimWorld& World, std::vector<Command>& OutCommands)
{
    if (World.GetPhase() != MatchPhase::Running)
    {
        return;
    }
    const PlayerState& State = World.GetPlayer(Player);
    if (!State.bActive || State.bDefeated)
    {
        return;
    }

    // A commander whose production chain is broken (no training → stuck in Fortify
    // with no action possible) must not score Fortify as a live option or it
    // turtles at full credits forever, as the Defensive profile did in headless
    // skirmishes.
    if (ActiveStrategy == AIStrategy::Fortify && CountOwnedUnits(World, true) == 0 &&
        ActiveOperation.AssignedUnits.empty())
    {
        ActiveStrategy = AIStrategy::AssembleArmy;
    }

    // The faction is only a snapshot once the world is set up, so resolve the
    // doctrine here rather than in Initialize, where the SimWorld may not hold a
    // real faction for this player yet.
    if (!bDoctrineLoaded)
    {
        LoadDoctrine(World);
    }

    if (IsUnderAttack(World))
    {
        LastUnderAttackTick = World.GetTick();
        bHasSeenAttack = true;
    }

    // Observation runs on its own cadence, before and independently of the decision
    // gate below: a unit that crosses a vision cone between two decisions must still
    // be remembered.
    UpdateKnowledge(World);

    if (++TicksSinceDecision < Config.DecisionIntervalTicks)
    {
        return;
    }
    TicksSinceDecision = 0;

    AIWorldAssessment Assessment = BuildAssessment(World);
    if (Assessment.ArmedUnits == 0)
    {
        Assessment.bAssaultActive = false;
    }

    const std::vector<AIStrategyScore> Scores =
        ScoreStrategies(Assessment, Config);
    const AIStrategy Previous = ActiveStrategy;
    ActiveStrategy =
        SelectStrategy(Scores, ActiveStrategy, bHasActiveStrategy, Config);
    bHasActiveStrategy = true;
    PreviousStrategyForDecision = Previous;
    ActiveStrategyScore = FindStrategyScore(Scores, ActiveStrategy);

    const size_t CommandCountBefore = OutCommands.size();

    // Scouting runs before army command: with nothing known there is nothing to
    // attack, and this is what turns an unknown map into a known one.
    TryScout(World, OutCommands);

    CommandArmy(World, OutCommands);

    if (!TryPlaceFinishedStructure(World, OutCommands))
    {
        const bool bCanActWithoutYard =
            ActiveStrategy == AIStrategy::Assault ||
            ActiveStrategy == AIStrategy::FinalAssault ||
            ActiveStrategy == AIStrategy::AssembleArmy;
        if (FindOwnConstructionYard(World).IsValid() ||
            bCanActWithoutYard)
        {
            ExecuteStrategy(ActiveStrategy, World, OutCommands);
        }
    }

    if (OutCommands.size() == CommandCountBefore)
    {
        LogIdleStrategyDecision(World.GetTick());
    }
}

} // namespace AI
} // namespace RA4
