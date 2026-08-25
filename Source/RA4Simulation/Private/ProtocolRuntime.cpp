// Copyright (c) Red Alert 4 project. Top-Secret Protocols and Global Commander Powers runtime implementation.
#include "RA4Simulation/ProtocolRuntime.h"

#include <algorithm>
#include <limits>

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
    {
        // Salvage economy: the Soviet doctrine pays for what it destroys, so the
        // passive lives in the Defense/Support branch where the other eco powers sit.
        ProtocolPowerDef P;
        P.Id = "sov_protocol_salvage_bounty";
        P.NameKey = "protocol.sov.salvage_bounty.name";
        P.DescriptionKey = "protocol.sov.salvage_bounty.desc";
        P.Faction = FactionId::Soviet;
        P.Branch = 2;
        P.Tier = 1;
        P.Kind = ProtocolPowerKind::Passive;
        P.CreditPercentPerKill = 25;
        RegisterProtocol(P);
    }
    {
        // Reinforcement drop reuses Damage as unit count by design (see
        // ProtocolPowerDef): the drop size is the power's headline magnitude.
        ProtocolPowerDef P;
        P.Id = "sov_protocol_troop_drop";
        P.NameKey = "protocol.sov.troop_drop.name";
        P.DescriptionKey = "protocol.sov.troop_drop.desc";
        P.Faction = FactionId::Soviet;
        P.Branch = 0;
        P.Tier = 4;
        P.PrerequisiteId = "sov_protocol_magnetic_satellite";
        P.Kind = ProtocolPowerKind::TroopDrop;
        P.CooldownTicks = 2400; // 120s @ 20Hz
        P.Damage = 4;           // 4 squadsmen per drop
        P.Radius = Fixed::FromInt(400);
        P.DeployUnitId = MakeContentId("unit.sov.conscript");
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
    {
        // Cheap early Assault option: a short salvo of expendable warheads.
        ProtocolPowerDef P;
        P.Id = "ec_protocol_kamikaze_raid";
        P.NameKey = "protocol.ec.kamikaze_raid.name";
        P.DescriptionKey = "protocol.ec.kamikaze_raid.desc";
        P.Faction = FactionId::EasternCoalition;
        P.Branch = 0;
        P.Tier = 1;
        P.Kind = ProtocolPowerKind::KamikazeSquadron;
        P.CooldownTicks = 1800; // 90s @ 20Hz
        P.Damage = 250;         // per warhead
        P.PayloadCount = 6;
        P.Radius = Fixed::FromInt(450);
        RegisterProtocol(P);
    }

    // ChronoLegion Tree -- the faction's powers bend space rather than matter,
    // so its kit is control (stun) and protection (phase), never raw damage.
    {
        ProtocolPowerDef P;
        P.Id = "cl_protocol_emp_pulse";
        P.NameKey = "protocol.cl.emp_pulse.name";
        P.DescriptionKey = "protocol.cl.emp_pulse.desc";
        P.Faction = FactionId::ChronoLegion;
        P.Branch = 1;
        P.Tier = 1;
        P.Kind = ProtocolPowerKind::EmpPulse;
        P.CooldownTicks = 1200; // 60s @ 20Hz
        P.Radius = Fixed::FromInt(500);
        P.StatusDurationTicks = 120; // 6s of paralysis @ 20Hz
        RegisterProtocol(P);
    }
    {
        ProtocolPowerDef P;
        P.Id = "cl_protocol_phase_field";
        P.NameKey = "protocol.cl.phase_field.name";
        P.DescriptionKey = "protocol.cl.phase_field.desc";
        P.Faction = FactionId::ChronoLegion;
        P.Branch = 2;
        P.Tier = 2;
        P.Kind = ProtocolPowerKind::PhaseField;
        P.CooldownTicks = 1800; // 90s @ 20Hz
        P.Radius = Fixed::FromInt(600);
        P.StatusDurationTicks = 150; // 7.5s untouchable @ 20Hz
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

void ProtocolRuntime::ProcessSimEvents(const std::vector<SimEvent>& Events, SimWorld& World)
{
    // Same XP feed as the world-less overload; salvage is layered on top so the
    // two entry points can never drift apart on what counts as a kill.
    ProcessSimEvents(Events);
    AwardSalvageBounties(Events, World);
}

void ProtocolRuntime::AwardSalvageBounties(const std::vector<SimEvent>& Events, SimWorld& World)
{
    for (const auto& Ev : Events)
    {
        if (Ev.Type != SimEventType::EntityDestroyed)
        {
            continue;
        }

        // Attribution rides on Ev.Other (the killer entity). A kill with no
        // attacker (world hazard, debug tool) pays nobody: guessing an owner
        // here would let passive protocols farm uncontested deaths.
        if (!Ev.Other.IsValid() || !Ev.Content.IsValid())
        {
            continue;
        }

        // The killer may itself have died in the same trade; dead slots read
        // bAlive=false but keep their Core data until reuse, and GetCore gates
        // on liveness -- a dead killer legitimately forfeits the bounty.
        const EntityCore* KillerCore = World.GetCore(Ev.Other);
        if (KillerCore == nullptr || KillerCore->Owner >= kMaxPlayers)
        {
            continue;
        }

        const PlayerId KillerOwner = KillerCore->Owner;
        if (!World.IsHostile(KillerOwner, Ev.Player))
        {
            continue;
        }

        const auto* Content = World.GetContent();
        const EntityDef* VictimDef = Content != nullptr ? Content->FindEntity(Ev.Content) : nullptr;
        if (VictimDef == nullptr)
        {
            continue;
        }

        auto& State = PlayerStates[KillerOwner];
        for (const std::string& UnlockedId : State.UnlockedProtocols)
        {
            const ProtocolPowerDef* Def = FindProtocol(UnlockedId);
            if (Def == nullptr || Def->Kind != ProtocolPowerKind::Passive ||
                Def->CreditPercentPerKill <= 0)
            {
                continue;
            }

            // int64 intermediate: cost * percent overflows int32 for expensive
            // epic units (cost > ~21M at 100%).
            const int64_t RawBounty = (int64_t(VictimDef->Production.Cost) * Def->CreditPercentPerKill) / 100;
            if (RawBounty > 0)
            {
                World.AddCredits(KillerOwner, static_cast<int32_t>(RawBounty));
            }
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
    // Powers that are not "scan entities and mutate them" run through their own
    // handlers; only the legacy scan-and-damage kinds fall through to the loop
    // below, so adding a kind can never silently change an older power.
    switch (Def.Kind)
    {
    case ProtocolPowerKind::TroopDrop:
        ExecuteTroopDrop(Def, Player, Location, World);
        return;

    case ProtocolPowerKind::PhaseField:
    case ProtocolPowerKind::EmpPulse:
    {
        StatusComp Template;
        if (Def.Kind == ProtocolPowerKind::PhaseField)
        {
            Template.InvulnerableTicks = Def.StatusDurationTicks;
        }
        else
        {
            Template.StunTicks = Def.StatusDurationTicks;
        }
        ApplyRadiusStatus(Player, Location,
                          Def.Radius.Raw > 0 ? Def.Radius : Fixed::FromInt(500),
                          Template, /*bEnemiesOnly=*/Def.Kind == ProtocolPowerKind::EmpPulse, World);
        return;
    }

    case ProtocolPowerKind::KamikazeSquadron:
        ExecuteKamikazeStrike(Def, Player, Location, World);
        return;

    default:
        break;
    }

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

void ProtocolRuntime::ExecuteTroopDrop(const ProtocolPowerDef& Def, PlayerId Player, const Vec2& Location, SimWorld& World)
{
    if (!Def.DeployUnitId.IsValid() || Def.Damage <= 0)
    {
        return;
    }

    // The draw sequence is fixed (per trooper: one angle jitter, one radius
    // fraction, ascending index), so every lockstep peer scatters the squad
    // identically from the same seeded world RNG.
    Random& Rng = World.GetRandom();
    const Fixed ScatterRadius = Def.Radius.Raw > 0 ? Def.Radius : Fixed::FromInt(300);
    // Cap guards against a corrupt/hostile content def allocating an army in a
    // single cast; legitimate drop sizes sit far below it.
    const int32_t Count = std::min(Def.Damage, 64);

    for (int32_t I = 0; I < Count; ++I)
    {
        const int32_t Angle = WrapAngle(((I * kAngleTurn) / Count) + Rng.NextRange(-128, 128));
        const Fixed Dist = ScatterRadius * Rng.NextUnitFixed();
        const Vec2 DropPos = Location + Vec2::FromAngle(Angle) * Dist;
        World.SpawnUnit(Def.DeployUnitId, Player, DropPos, Angle);
    }
}

void ProtocolRuntime::ExecuteKamikazeStrike(const ProtocolPowerDef& Def, PlayerId Player, const Vec2& Location, SimWorld& World)
{
    if (Def.Damage <= 0)
    {
        return;
    }

    const int32_t Warheads = std::max(1, std::min(Def.PayloadCount, 32));
    const Fixed BlastRadius = Def.Radius.Raw > 0 ? Def.Radius : Fixed::FromInt(500);

    // Impact points share TroopDrop's scatter discipline so the salvo pattern is
    // reproducible on every peer.
    Random& Rng = World.GetRandom();
    std::vector<Vec2> Impacts;
    Impacts.reserve(static_cast<uint32_t>(Warheads));
    for (int32_t W = 0; W < Warheads; ++W)
    {
        const int32_t Angle = WrapAngle(((W * kAngleTurn) / Warheads) + Rng.NextRange(-256, 256));
        const Fixed Dist = BlastRadius * Rng.NextUnitFixed();
        Impacts.push_back(Location + Vec2::FromAngle(Angle) * Dist);
    }

    const Fixed BlastRadiusSq = BlastRadius * BlastRadius;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Owner == Player)
        {
            continue;
        }

        const TransformComp* T = World.GetTransform(World.MakeId(I));
        if (T == nullptr || (T->Position - Location).LengthSquared() > BlastRadiusSq)
        {
            continue;
        }

        // Each warhead contributes its damage with linear falloff to zero at the
        // blast edge. Accumulating in Fixed and rounding once keeps the result a
        // pure integer function of position -- identical on every machine.
        Fixed TotalDamage = Fixed::Zero();
        for (const Vec2& Impact : Impacts)
        {
            const Fixed D = Distance(T->Position, Impact); // FxSqrt: deterministic
            if (D < BlastRadius)
            {
                TotalDamage += Fixed::FromInt(Def.Damage) * (Fixed::One() - (D / BlastRadius));
            }
        }

        const int64_t RoundedDamage = TotalDamage.ToIntRound();
        const int32_t DamageAmount = static_cast<int32_t>(std::clamp<int64_t>(
            RoundedDamage,
            static_cast<int64_t>(std::numeric_limits<int32_t>::min()),
            static_cast<int64_t>(std::numeric_limits<int32_t>::max())));
        if (DamageAmount <= 0)
        {
            continue;
        }

        const HealthComp* HRead = World.GetHealth(World.MakeId(I));
        if (HRead == nullptr)
        {
            continue;
        }

        HealthComp* H = const_cast<HealthComp*>(HRead);
        H->Current = std::max(0, H->Current - DamageAmount);
        if (H->Current == 0)
        {
            const_cast<EntityCore&>(Cores[I]).bAlive = false;
        }
    }
}

void ProtocolRuntime::ApplyRadiusStatus(PlayerId Caster, const Vec2& Center, Fixed Radius,
                                        const StatusComp& Template, bool bEnemiesOnly, SimWorld& World)
{
    const Fixed RadiusSq = Radius * Radius;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Kind != EntityKind::Unit)
        {
            continue;
        }

        const EntityId Id = World.MakeId(I);

        // Boarded units are dormant cargo; like SimWorld::ApplyStatusInRadius we
        // skip them rather than shield/stun someone hidden inside a transport.
        const PassengerComp* Ride = World.GetPassengerOf(Id);
        if (Ride != nullptr && Ride->Transport.IsValid())
        {
            continue;
        }

        if (bEnemiesOnly && !World.IsHostile(Caster, Cores[I].Owner))
        {
            continue;
        }

        const TransformComp* T = World.GetTransform(Id);
        if (T == nullptr || DistanceSquared(Center, T->Position) > RadiusSq)
        {
            continue;
        }

        const StatusComp* SRead = World.GetStatus(Id);
        if (SRead == nullptr)
        {
            continue;
        }

        // Keep the larger countdown per effect so overlapping casts can never
        // shorten an effect that is already ticking.
        StatusComp* S = const_cast<StatusComp*>(SRead);
        S->StunTicks = std::max(S->StunTicks, Template.StunTicks);
        S->FreezeTicks = std::max(S->FreezeTicks, Template.FreezeTicks);
        S->ShrinkTicks = std::max(S->ShrinkTicks, Template.ShrinkTicks);
        S->InfectionTicks = std::max(S->InfectionTicks, Template.InfectionTicks);
        S->InvulnerableTicks = std::max(S->InvulnerableTicks, Template.InvulnerableTicks);
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
