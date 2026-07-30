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
bool IsPowerPlant(const EntityDef& D) { return D.Kind == EntityKind::Building && D.Building.bIsPowerPlant; }
bool IsRefinery(const EntityDef& D) { return D.Kind == EntityKind::Building && D.Building.bIsRefinery; }

// A barracks and a war factory are identified by what they can produce rather than
// by name, so the commander needs no per-faction table.
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
    bAttacking = false;
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
    DecisionLog.clear();
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

    // Prefer the most expensive armed unit the tech tree and the treasury currently
    // allow: a commander that always buys the cheapest thing available never fields
    // anything that can break a base.
    ContentId Best;
    int32_t BestCost = -1;
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
        if (Def.Production.Cost > BestCost)
        {
            BestCost = Def.Production.Cost;
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
    }

    if (++TicksSinceMemoryUpdate < Config.MemoryUpdateIntervalTicks)
    {
        return;
    }
    TicksSinceMemoryUpdate = 0;
    Knowledge->UpdateMemory(TickIndex(std::max(0, Config.MemoryRetentionTicks)));
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
        // Cheapest armed unit available, so scouting does not consume the assault
        // force. Chosen in entity-index order for determinism.
        const std::vector<EntityCore>& Cores = World.GetAllCores();
        for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
        {
            if (!Cores[I].bAlive || Cores[I].Owner != Player || Cores[I].Kind != EntityKind::Unit)
            {
                continue;
            }
            const EntityDef* Def = Content->FindEntity(Cores[I].Def);
            if (Def == nullptr || Def->Unit.bIsHarvester || Def->Unit.bIsBuilder ||
                Def->Unit.MaxSpeed <= Fixed::Zero())
            {
                continue;
            }
            ScoutUnit = World.MakeId(I);
            break;
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
    // Reads memory only. There is deliberately no path here into SimWorld: an enemy
    // the commander has never observed does not exist as far as this function is
    // concerned, which is what stops the AI attacking through fog.
    if (Knowledge == nullptr)
    {
        return false;
    }

    // Production buildings first, then anything else: razing a war factory ends the
    // match faster than chasing the scout that happened to be nearest. Iterated in
    // memory order, which is insertion order, so the choice is deterministic.
    const EnemyMemory* BestBuilding = nullptr;
    const EnemyMemory* BestOther = nullptr;
    for (const EnemyMemory& Mem : Knowledge->GetKnownEnemies())
    {
        if (Mem.Kind == EntityKind::Building)
        {
            if (BestBuilding == nullptr)
            {
                BestBuilding = &Mem;
            }
        }
        else if (Mem.Kind == EntityKind::Unit && BestOther == nullptr)
        {
            BestOther = &Mem;
        }
    }

    const EnemyMemory* Chosen = BestBuilding != nullptr ? BestBuilding : BestOther;
    if (Chosen == nullptr)
    {
        return false;
    }

    Out.Entity = Chosen->Entity;
    Out.Tile = Chosen->Position;
    Out.Kind = Chosen->Kind;
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
    Assessment.bAssaultActive = bAttacking;

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

    // Power first: a shortage throttles everything else the commander is about to do.
    if (State.PowerConsumed >= State.PowerProduced)
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
        if (Have < Config.TargetHarvesters)
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

void AICommander::CommandArmy(const SimWorld& World, std::vector<Command>& Out)
{
    const ContentDatabase* Content = World.GetContent();
    if (Content == nullptr)
    {
        return;
    }

    const auto IsWounded = [&World](EntityId Id)
    {
        const HealthComp* Health = World.GetHealth(Id);
        return Health != nullptr && Health->Max > 0 &&
               int64_t(Health->Current) * 4 < int64_t(Health->Max);
    };

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
            if (Def == nullptr || !Def->Weapon.IsValid() ||
                Def->Unit.bIsHarvester || Def->Unit.bIsBuilder)
            {
                continue;
            }

            const EntityId Id = World.MakeId(I);
            const OrderQueue* Orders = World.GetOrders(Id);
            if (!IsWounded(Id) || Orders == nullptr || Orders->Count > 0)
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

    const int32_t ArmySize = CountOwnedUnits(World, /*bCombatOnly*/ true);
    if (ArmySize == 0)
    {
        // Wiped out: stop attacking and rebuild before committing again.
        bAttacking = false;
        if (ActiveOperation.State != OperationState::Aborted && ActiveOperation.State != OperationState::Completed)
        {
            ActiveOperation.TransitionTo(OperationState::Aborted, World.GetTick());
        }
        return;
    }
    if (!bAttacking && ArmySize < Config.AttackArmySize)
    {
        return;
    }
    if (bAttacking && ArmySize < Config.MinimumAttackSize)
    {
        bAttacking = false;
        ActiveOperation.TransitionTo(OperationState::Retreating, World.GetTick());
        return;
    }

    KnownTarget Target;
    if (!FindKnownEnemyTarget(Target))
    {
        if (bAttacking && ActiveOperation.State == OperationState::Engaging)
        {
            ActiveOperation.TransitionTo(OperationState::Completed, World.GetTick());
        }
        return;
    }

    // The army is sent to where the enemy was last SEEN, not to where it actually is.
    // Tracking a live transform through fog would be the same cheat in a subtler form.
    const Vec2 TargetPosition = World.GetMap().TileCenterToWorld(Target.Tile);
    ActiveOperation.TargetLocation = Target.Tile;

    // Only idle units are ordered, so a unit already advancing is not reset every
    // decision tick -- that would keep clearing its order queue and stall the push.
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    int32_t Ordered = 0;
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Owner != Player || Cores[I].Kind != EntityKind::Unit)
        {
            continue;
        }
        const EntityDef* Def = Content->FindEntity(Cores[I].Def);
        if (Def == nullptr || !Def->Weapon.IsValid() || Def->Unit.bIsHarvester || Def->Unit.bIsBuilder)
        {
            continue;
        }
        const EntityId Id = World.MakeId(I);

        // A retreat command may already have been emitted above. Do not append a
        // contradictory attack order in the same command frame.
        if (IsWounded(Id))
        {
            continue;
        }

        const OrderQueue* Orders = World.GetOrders(Id);
        if (Orders == nullptr || Orders->Count > 0)
        {
            continue;
        }

        Command C;
        C.Type = CommandType::AttackMove;
        C.Issuer = Player;
        C.Primary = Id;
        C.Location = TargetPosition;
        Out.push_back(C);
        ++Ordered;
    }

    if (Ordered > 0)
    {
        if (!bAttacking)
        {
            Log(World.GetTick(), CommandType::AttackMove, ContentId(), "army at strength, attacking");
            ActiveOperation.OperationId++;
            ActiveOperation.StartTick = World.GetTick();
            ActiveOperation.TransitionTo(OperationState::Advancing, World.GetTick());
        }
        else if (ActiveOperation.State == OperationState::Advancing)
        {
            ActiveOperation.TransitionTo(OperationState::Engaging, World.GetTick());
        }
        bAttacking = true;
    }
}

bool AICommander::ExecuteStrategy(AIStrategy Strategy,
                                  const SimWorld& World,
                                  std::vector<Command>& Out)
{
    switch (Strategy)
    {
        case AIStrategy::ExpandEconomy:
        case AIStrategy::Recover:
            return TryBuildEconomy(World, Out);
        case AIStrategy::TechUp:
            return TryBuildTech(World, Out);
        case AIStrategy::Fortify:
            if (TryBuildDefence(World, Out))
            {
                return true;
            }
            return TryTrainArmy(World, Out);
        case AIStrategy::AssembleArmy:
            return TryTrainArmy(World, Out);
        case AIStrategy::Assault:
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

    if (IsUnderAttack(World))
    {
        LastUnderAttackTick = World.GetTick();
        bHasSeenAttack = true;
    }

    // Observation runs on its own cadence, before and independently of the decision
    // gate below: a unit that crosses a vision cone between two decisions must still
    // be remembered.
    UpdateKnowledge(World);

    ++TicksSinceDecision;
    if (TicksSinceDecision < Config.DecisionIntervalTicks)
    {
        return;
    }
    TicksSinceDecision = 0;

    AIWorldAssessment Assessment = BuildAssessment(World);
    if (Assessment.ArmedUnits == 0)
    {
        bAttacking = false;
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
