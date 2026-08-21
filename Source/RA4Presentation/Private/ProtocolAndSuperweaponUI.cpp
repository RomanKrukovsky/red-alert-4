// Copyright (c) Red Alert 4 project. Presentation models for Top-Secret Protocol Wheel, Superweapon HUD Timers & Video Comms implementation.
#include "RA4Presentation/ProtocolAndSuperweaponUI.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace RA4
{

std::vector<ProtocolNodeUIData> ProtocolAndSuperweaponUI::BuildProtocolTreeViewModel(
    PlayerId Player,
    const ProtocolRuntime& Protocols,
    TickIndex CurrentTick,
    float TickRateHz) const
{
    std::vector<ProtocolNodeUIData> Result;
    const auto& Catalog = Protocols.GetCatalog();
    const auto& State = Protocols.GetPlayerState(Player);

    Result.reserve(Catalog.size());

    for (const auto& Pair : Catalog)
    {
        const auto& Def = Pair.second;
        ProtocolNodeUIData Node;
        Node.Id = Def.Id;
        Node.NameKey = Def.NameKey;
        Node.DescriptionKey = Def.DescriptionKey;
        Node.Tier = Def.Tier;
        Node.Branch = Def.Branch;
        Node.bUnlocked = State.HasProtocol(Def.Id);
        Node.bCanUnlock = Protocols.CanUnlockProtocol(Player, Def.Id);

        if (Node.bUnlocked)
        {
            Node.bOnCooldown = State.IsOnCooldown(Def.Id, CurrentTick);
            if (Node.bOnCooldown)
            {
                const TickIndex ReadyTick = State.GetCooldownReadyTick(Def.Id);
                const uint32_t RemainingTicks = (ReadyTick > CurrentTick) ? (ReadyTick - CurrentTick) : 0u;
                Node.CooldownRemainingSeconds = static_cast<float>(RemainingTicks) / TickRateHz;
                Node.CooldownProgressFraction = (Def.CooldownTicks > 0) ? (static_cast<float>(RemainingTicks) / static_cast<float>(Def.CooldownTicks)) : 0.0f;
            }
        }

        Result.push_back(Node);
    }


    return Result;
}

std::vector<SuperweaponHUDTimerItem> ProtocolAndSuperweaponUI::BuildSuperweaponTimersViewModel(
    const ProtocolRuntime& Protocols,
    const SimWorld& World,
    float TickRateHz) const
{
    std::vector<SuperweaponHUDTimerItem> Result;
    const auto Statuses = Protocols.GetSuperweaponStatuses(World);

    Result.reserve(Statuses.size());

    for (const auto& St : Statuses)
    {
        SuperweaponHUDTimerItem Item;
        Item.BuildingEntity = St.BuildingEntity;
        Item.OwnerPlayer = St.Owner;
        Item.SuperweaponName = St.Name;
        Item.ChargePercent = St.ChargePercent;
        Item.bReady = St.bReady;
        Item.bPowered = St.bPowered;
        Item.bCriticalAlert = St.bReady;


        const int32_t RemainingTicks = std::max(0, St.TotalRechargeTicks - St.ChargeTicks);
        Item.RemainingSeconds = static_cast<float>(RemainingTicks) / TickRateHz;

        Result.push_back(Item);
    }

    return Result;
}

std::string ProtocolAndSuperweaponUI::FormatCountdownMMSS(float SecondsRemaining)
{
    const int32_t TotalSec = std::max(0, static_cast<int32_t>(std::ceil(SecondsRemaining)));
    const int32_t Minutes = TotalSec / 60;
    const int32_t Seconds = TotalSec % 60;

    char Buf[16];
    std::snprintf(Buf, sizeof(Buf), "%02d:%02d", Minutes, Seconds);
    return std::string(Buf);
}

} // namespace RA4
