// Copyright (c) Red Alert 4 project.
#include "RA4Core/Command.h"

namespace RA4
{

const char* ToString(CommandType Type)
{
    switch (Type)
    {
        case CommandType::None: return "None";
        case CommandType::Move: return "Move";
        case CommandType::AttackMove: return "AttackMove";
        case CommandType::Attack: return "Attack";
        case CommandType::Stop: return "Stop";
        case CommandType::SetRallyPoint: return "SetRallyPoint";
        case CommandType::Harvest: return "Harvest";
        case CommandType::Guard: return "Guard";
        case CommandType::PlaceBuilding: return "PlaceBuilding";
        case CommandType::StartProduction: return "StartProduction";
        case CommandType::CancelProduction: return "CancelProduction";
        case CommandType::PauseProduction: return "PauseProduction";
        case CommandType::SellBuilding: return "SellBuilding";
        case CommandType::RepairBuilding: return "RepairBuilding";
        case CommandType::SetPowerPriority: return "SetPowerPriority";
        case CommandType::Surrender: return "Surrender";
        case CommandType::DirectControlEnter: return "DirectControlEnter";
        case CommandType::DirectControlExit: return "DirectControlExit";
        case CommandType::DirectControlDrive: return "DirectControlDrive";
        case CommandType::DirectControlFire: return "DirectControlFire";
        default: return "Unknown";
    }
}

const char* ToString(CommandReject Reason)
{
    switch (Reason)
    {
        case CommandReject::Accepted: return "Accepted";
        case CommandReject::UnknownType: return "UnknownType";
        case CommandReject::NoSuchEntity: return "NoSuchEntity";
        case CommandReject::NotOwner: return "NotOwner";
        case CommandReject::EntityDead: return "EntityDead";
        case CommandReject::UnknownContent: return "UnknownContent";
        case CommandReject::InsufficientCredits: return "InsufficientCredits";
        case CommandReject::InsufficientPower: return "InsufficientPower";
        case CommandReject::TechRequirementsUnmet: return "TechRequirementsUnmet";
        case CommandReject::InvalidPlacement: return "InvalidPlacement";
        case CommandReject::QueueFull: return "QueueFull";
        case CommandReject::NoProducer: return "NoProducer";
        case CommandReject::TargetInvalid: return "TargetInvalid";
        case CommandReject::RateLimited: return "RateLimited";
        case CommandReject::CommandCapExceeded: return "CommandCapExceeded";
        case CommandReject::MatchOver: return "MatchOver";
        case CommandReject::DirectIneligibleUnit: return "DirectIneligibleUnit";
        case CommandReject::DirectAlreadyControlled: return "DirectAlreadyControlled";
        case CommandReject::DirectNotControlling: return "DirectNotControlling";
        case CommandReject::DirectWeaponCooldown: return "DirectWeaponCooldown";
        case CommandReject::DirectWeaponEmpty: return "DirectWeaponEmpty";
        default: return "Unknown";
    }
}

} // namespace RA4
