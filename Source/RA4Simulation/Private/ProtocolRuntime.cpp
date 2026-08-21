// Copyright (c) Red Alert 4 project. Top-Secret Protocols and Global Commander Powers runtime implementation.
#include "RA4Simulation/ProtocolRuntime.h"

namespace RA4
{

ProtocolRuntime::ProtocolRuntime()
{
    RegisterDefaultProtocols();
    Reset();
}

void ProtocolRuntime::RegisterProtocol(const ProtocolPowerDef& Def)
{
    ProtocolCatalog[Def.Id] = Def;
}

const ProtocolPowerDef* ProtocolRuntime::FindProtocol(const std::string& ProtocolId) const
{
    auto It = ProtocolCatalog.find(ProtocolId);
    if (It != ProtocolCatalog.end())
    {
        return &It->second;
    }
    return nullptr;
}

void ProtocolRuntime::RegisterDefaultProtocols()
{
    // Soviet Tree
    {
        ProtocolPowerDef P;
        P.Id = "sov_protocol_production";
        P.NameKey = "protocol.sov.production.name";
        P.DescriptionKey = "protocol.sov.production.desc";
        P.Faction = FactionId::Soviet;
        P.Branch = 0;
        P.Tier = 1;
        P.Kind = ProtocolPowerKind::Passive;
        P.ProductionCostDiscountPercent = 10;
        RegisterProtocol(P);
    }
    {
        ProtocolPowerDef P;
        P.Id = "sov_protocol_orbital_strike";
        P.NameKey = "protocol.sov.orbital_strike.name";
        P.DescriptionKey = "protocol.sov.orbital_strike.desc";
        P.Faction = FactionId::Soviet;
        P.Branch = 0;
        P.Tier = 2;
        P.PrerequisiteId = "sov_protocol_production";
        P.Kind = ProtocolPowerKind::OrbitalStrike;
        P.CooldownTicks = 1200; // 60s @ 20Hz
        P.Damage = 1000;
        P.Radius = Fixed::FromInt(500);
        P.Warhead = WarheadClass::Fragmentation;
        RegisterProtocol(P);
    }
    {
        ProtocolPowerDef P;
        P.Id = "sov_protocol_magnetic_satellite";
        P.NameKey = "protocol.sov.magnetic_satellite.name";
        P.DescriptionKey = "protocol.sov.magnetic_satellite.desc";
        P.Faction = FactionId::Soviet;
        P.Branch = 0;
        P.Tier = 3;
        P.PrerequisiteId = "sov_protocol_orbital_strike";
        P.Kind = ProtocolPowerKind::MagneticDraw;
        P.CooldownTicks = 1800; // 90s @ 20Hz
        P.Damage = 1500;
        P.Radius = Fixed::FromInt(600);
        RegisterProtocol(P);
    }

    // Allied Tree
    {
        ProtocolPowerDef P;
        P.Id = "all_protocol_recon_surge";
        P.NameKey = "protocol.all.recon_surge.name";
        P.DescriptionKey = "protocol.all.recon_surge.desc";
        P.Faction = FactionId::Alliance;
        P.Branch = 1;
        P.Tier = 1;
        P.Kind = ProtocolPowerKind::ReconSurge;
        P.CooldownTicks = 600; // 30s @ 20Hz
        P.Radius = Fixed::FromInt(1200);
        RegisterProtocol(P);
    }
    {
        ProtocolPowerDef P;
        P.Id = "all_protocol_cryo_freeze";
        P.NameKey = "protocol.all.cryo_freeze.name";
        P.DescriptionKey = "protocol.all.cryo_freeze.desc";
        P.Faction = FactionId::Alliance;
        P.Branch = 1;
        P.Tier = 2;
        P.PrerequisiteId = "all_protocol_recon_surge";
        P.Kind = ProtocolPowerKind::CryoFreeze;
        P.CooldownTicks = 1200;
        P.Damage = 200;
        P.Radius = Fixed::FromInt(600);
        RegisterProtocol(P);
    }

    // Coalition Tree
    {
        ProtocolPowerDef P;
        P.Id = "ec_protocol_speed";
        P.NameKey = "protocol.ec.speed.name";
        P.DescriptionKey = "protocol.ec.speed.desc";
        P.Faction = FactionId::EasternCoalition;
        P.Branch = 2;
        P.Tier = 1;
        P.Kind = ProtocolPowerKind::Passive;
        P.SpeedMultiplier = Fixed::FromRatio(115, 100);
        RegisterProtocol(P);
    }
    {
        ProtocolPowerDef P;
        P.Id = "ec_protocol_repair_drone";
        P.NameKey = "protocol.ec.repair_drone.name";
        P.DescriptionKey = "protocol.ec.repair_drone.desc";
        P.Faction = FactionId::EasternCoalition;
        P.Branch = 2;
        P.Tier = 2;
        P.PrerequisiteId = "ec_protocol_speed";
        P.Kind = ProtocolPowerKind::EmergencyRepairDrone;
        P.CooldownTicks = 1200;
        P.Damage = -500; // Healing
        P.Radius = Fixed::FromInt(500);
        RegisterProtocol(P);
    }

}

void ProtocolRuntime::Reset()
{
    for (uint32_t I = 0; I < kMaxPlayers; ++I)
    {
        PlayerStates[I] = PlayerProtocolState{};
    }
}

void ProtocolRuntime::AwardExperience(PlayerId Player, uint32_t Exp)
{
    if (Player >= kMaxPlayers)
    {
        return;
    }

    auto& State = PlayerStates[Player];
    State.TotalExperience += Exp;

    // Determine total points earned from thresholds
    static const uint32_t Thresholds[] = {1000, 3000, 7000, 15000, 30000};
    uint32_t TotalEarned = 0;
    for (uint32_t T : Thresholds)
    {
        if (State.TotalExperience >= T)
        {
            TotalEarned++;
        }
    }

    if (TotalEarned > State.SpentPoints)
    {
        State.AvailablePoints = TotalEarned - State.SpentPoints;
    }
    else
    {
        State.AvailablePoints = 0;
    }
}

void ProtocolRuntime::ProcessSimEvents(const std::vector<SimEvent>& Events)
{
    for (const auto& Ev : Events)
    {
        switch (Ev.Type)
        {
        case SimEventType::DamageApplied:
            if (Ev.Player < kMaxPlayers)
            {
                AwardExperience(Ev.Player, 50);
            }
            break;

        case SimEventType::EntityDestroyed:
            if (Ev.Player < kMaxPlayers)
            {
                AwardExperience(Ev.Player, 200);
            }
            break;
        default:
            break;
        }
    }
}

bool ProtocolRuntime::CanUnlockProtocol(PlayerId Player, const std::string& ProtocolId) const
{
    if (Player >= kMaxPlayers)
    {
        return false;
    }

    const auto& State = PlayerStates[Player];
    if (State.AvailablePoints == 0)
    {
        return false;
    }

    if (State.HasProtocol(ProtocolId))
    {
        return false;
    }

    const auto* Def = FindProtocol(ProtocolId);
    if (Def == nullptr)
    {
        return false;
    }

    if (!Def->PrerequisiteId.empty() && !State.HasProtocol(Def->PrerequisiteId))
    {
        return false;
    }

    return true;
}

bool ProtocolRuntime::UnlockProtocol(PlayerId Player, const std::string& ProtocolId)
{
    if (!CanUnlockProtocol(Player, ProtocolId))
    {
        return false;
    }

    auto& State = PlayerStates[Player];
    State.SpentPoints++;
    if (State.AvailablePoints > 0)
    {
        State.AvailablePoints--;
    }
    State.UnlockedProtocols.push_back(ProtocolId);
    return true;
}

bool ProtocolRuntime::CanCastPower(PlayerId Player, const std::string& ProtocolId, const Vec2& Location, const SimWorld& World) const
{
    (void)Location;
    if (Player >= kMaxPlayers)
    {
        return false;
    }

    const auto& State = PlayerStates[Player];
    if (!State.HasProtocol(ProtocolId))
    {
        return false;
    }

    if (State.IsOnCooldown(ProtocolId, World.GetTick()))
    {
        return false;
    }

    const auto* Def = FindProtocol(ProtocolId);
    if (Def == nullptr || Def->Kind == ProtocolPowerKind::Passive)
    {
        return false;
    }

    return true;
}

bool ProtocolRuntime::CastPower(PlayerId Player, const std::string& ProtocolId, const Vec2& Location, SimWorld& World)
{
    if (!CanCastPower(Player, ProtocolId, Location, World))
    {
        return false;
    }

    const auto* Def = FindProtocol(ProtocolId);
    if (Def == nullptr)
    {
        return false;
    }

    auto& State = PlayerStates[Player];
    const TickIndex ReadyTick = World.GetTick() + Def->CooldownTicks;

    bool bFound = false;
    for (auto& C : State.Cooldowns)
    {
        if (C.first == ProtocolId)
        {
            C.second = ReadyTick;
            bFound = true;
            break;
        }
    }
    if (!bFound)
    {
        State.Cooldowns.push_back({ProtocolId, ReadyTick});
    }

    ExecutePowerEffect(*Def, Player, Location, World);
    return true;
}

void ProtocolRuntime::ExecutePowerEffect(const ProtocolPowerDef& Def, PlayerId Player, const Vec2& Location, SimWorld& World)
{
    const Fixed Radius = Def.Radius.Raw > 0 ? Def.Radius : Fixed::FromInt(500);
    const Fixed RadiusSq = Radius * Radius;

    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive)
        {
            continue;
        }

        const TransformComp* T = World.GetTransform(World.MakeId(I));
        if (T == nullptr)
        {
            continue;
        }

        if ((T->Position - Location).LengthSquared() <= RadiusSq)
        {
            const bool bIsAllied = (Cores[I].Owner == Player);

            if (Def.Kind == ProtocolPowerKind::EmergencyRepairDrone && bIsAllied)
            {
                // Heal friendly unit
                const HealthComp* HRead = World.GetHealth(World.MakeId(I));
                if (HRead != nullptr)
                {
                    HealthComp* H = const_cast<HealthComp*>(HRead);
                    H->Current = std::min(H->Current + 500, H->Max);
                }
            }
            else if (!bIsAllied && (Def.Kind == ProtocolPowerKind::OrbitalStrike ||
                                    Def.Kind == ProtocolPowerKind::MagneticDraw ||
                                    Def.Kind == ProtocolPowerKind::CryoFreeze ||
                                    Def.Kind == ProtocolPowerKind::TimeBomb))
            {
                // Damage enemy unit
                const HealthComp* HRead = World.GetHealth(World.MakeId(I));
                if (HRead != nullptr)
                {
                    HealthComp* H = const_cast<HealthComp*>(HRead);
                    const int32_t Dmg = Def.Damage > 0 ? Def.Damage : 500;
                    H->Current = std::max(0, H->Current - Dmg);
                    if (H->Current <= 0)
                    {
                        const_cast<EntityCore&>(Cores[I]).bAlive = false;
                    }
                }
            }
        }
    }
}

std::vector<SuperweaponStatus> ProtocolRuntime::GetSuperweaponStatuses(const SimWorld& World) const
{
    std::vector<SuperweaponStatus> Result;

    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Kind != EntityKind::Building)
        {
            continue;
        }

        const EntityId Id = World.MakeId(I);
        const BuildingComp* B = World.GetBuilding(Id);
        if (B == nullptr)
        {
            continue;
        }

        const auto* Content = World.GetContent();
        if (Content == nullptr)
        {
            continue;
        }

        const auto* Def = Content->FindEntity(Cores[I].Def);
        if (Def == nullptr || Def->Building.SuperweaponRechargeTicks <= 0)
        {
            continue;
        }

        SuperweaponStatus Status;
        Status.BuildingEntity = Id;
        Status.Owner = Cores[I].Owner;
        Status.DefId = Cores[I].Def;
        Status.Name = Def->Name;
        Status.ChargeTicks = B->SuperweaponChargeTicks;
        Status.TotalRechargeTicks = Def->Building.SuperweaponRechargeTicks;
        Status.ChargePercent = (B->SuperweaponChargeTicks * 100) / Def->Building.SuperweaponRechargeTicks;
        Status.bReady = (B->SuperweaponChargeTicks >= Def->Building.SuperweaponRechargeTicks);

        const PlayerState& P = World.GetPlayer(Cores[I].Owner);
        Status.bPowered = (P.PowerProduced >= P.PowerConsumed);

        Result.push_back(Status);
    }

    return Result;
}

const PlayerProtocolState& ProtocolRuntime::GetPlayerState(PlayerId Player) const
{
    static const PlayerProtocolState EmptyState{};
    if (Player < kMaxPlayers)
    {
        return PlayerStates[Player];
    }
    return EmptyState;
}

PlayerProtocolState& ProtocolRuntime::GetPlayerStateMutable(PlayerId Player)
{
    static PlayerProtocolState FallbackState{};
    if (Player < kMaxPlayers)
    {
        return PlayerStates[Player];
    }
    return FallbackState;
}

void ProtocolRuntime::Tick(const SimWorld& World)
{
    (void)World;
}

} // namespace RA4
