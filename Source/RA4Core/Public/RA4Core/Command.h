// Copyright (c) Red Alert 4 project. The only channel through which game state changes.
//
// Nothing outside the simulation ever mutates simulation state directly. Input,
// AI, mission scripts and the network layer all produce Commands; the simulation
// validates and applies them at a deterministic point in the tick. This is what
// makes replays, lockstep verification and server authority the same mechanism
// rather than three parallel systems.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/ByteStream.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"

namespace RA4
{

enum class CommandType : uint8_t
{
    None = 0,

    // Unit orders
    Move = 1,
    AttackMove = 2,
    Attack = 3,
    Stop = 4,
    SetRallyPoint = 5,
    Harvest = 6,
    Guard = 7,
    Deploy = 8,

    // Base management
    PlaceBuilding = 20,
    StartProduction = 21,
    CancelProduction = 22,
    PauseProduction = 23,
    SellBuilding = 24,
    RepairBuilding = 25,
    // ADR-0013. Param carries the PowerPriority level. A player override, so it goes
    // through the command bus like any other decision and lands in the replay.
    SetPowerPriority = 26,

    // Match flow
    Surrender = 40,

    // Direct vehicle control (authoritative, serialized, quantized). The
    // simulation -- never the client -- owns movement, turret aiming, firing
    // and exit. See Docs/Architecture/ADR/ADR-0029-direct-control.md.
    DirectControlEnter = 50,
    DirectControlExit  = 51,
    DirectControlDrive = 52,   // hull throttle/steering + turret yaw/pitch + flags
    DirectControlFire  = 53,   // primary/secondary/ability/optics toggle

    // --- Superweapons / abilities ---------------------------------------
    // Primary = the charged support structure, Tile = impact centre. Deliberately
    // one command rather than one per weapon: the structure's content definition
    // carries damage, radius and recharge, so a new superweapon is content, not
    // protocol.
    FireSuperweapon = 54,

    Max = 55,
};

// Quantized input axes for DirectControlDrive. Stored as int8 (-127..127) so a
// full drive command is fixed-size and deterministic across float ABIs. The
// simulation re-scales these to Fixed using the profile's sensitivity and
// limits, never trusting raw floats from a client.
struct DirectControlAxes
{
    int8_t Throttle = 0;     // -127 full reverse .. +127 full forward
    int8_t Steering = 0;     // -127 full left .. +127 full right
    int8_t TurretYaw = 0;    // requested yaw rate, quantized
    int8_t TurretPitch = 0;  // requested pitch rate, quantized
    uint8_t Flags = 0;       // bit0: primary fire, bit1: secondary fire,
                             // bit2: ability, bit3: optics toggle,
                             // bit4: manual reload request
};

// Order queueing mode carried by movement-class commands (Shift-click chains).
enum class OrderMode : uint8_t
{
    Replace = 0,
    Queue = 1,
};

// A single validated intent. Kept to a fixed, small footprint (~40 bytes) because at
// 8 players issuing group orders the command stream is the one thing that must stay
// cheap to broadcast, log and hash.
struct Command
{
    CommandType Type = CommandType::None;
    PlayerId Issuer = kInvalidPlayer;
    OrderMode Mode = OrderMode::Replace;
    uint8_t Slot = 0;             // production queue index / building sub-slot

    EntityId Primary;             // acting entity, or the queue-owning factory
    EntityId Target;              // order target entity, if any
    ContentId Content;            // unit or building type for production/placement
    Vec2 Location;                // world target
    TileCoord Tile;               // grid target for placement
    int32_t Param = 0;            // rotation for placement, count for production
    DirectControlAxes DirectAxes; // only used by DirectControlDrive

    void Serialize(ByteWriter& W) const
    {
        W.WriteUInt8(uint8_t(Type));
        W.WriteUInt8(Issuer);
        W.WriteUInt8(uint8_t(Mode));
        W.WriteUInt8(Slot);
        W.WriteUInt32(Primary.Index);
        W.WriteUInt32(Primary.Generation);
        W.WriteUInt32(Target.Index);
        W.WriteUInt32(Target.Generation);
        W.WriteUInt32(Content.Value);
        W.WriteInt64(Location.X.Raw);
        W.WriteInt64(Location.Y.Raw);
        W.WriteInt32(Tile.X);
        W.WriteInt32(Tile.Y);
        W.WriteInt32(Param);
        W.WriteInt8(DirectAxes.Throttle);
        W.WriteInt8(DirectAxes.Steering);
        W.WriteInt8(DirectAxes.TurretYaw);
        W.WriteInt8(DirectAxes.TurretPitch);
        W.WriteUInt8(DirectAxes.Flags);
    }

    static Command Deserialize(ByteReader& R)
    {
        Command C;
        C.Type = CommandType(R.ReadUInt8());
        C.Issuer = R.ReadUInt8();
        C.Mode = OrderMode(R.ReadUInt8());
        C.Slot = R.ReadUInt8();
        C.Primary.Index = R.ReadUInt32();
        C.Primary.Generation = R.ReadUInt32();
        C.Target.Index = R.ReadUInt32();
        C.Target.Generation = R.ReadUInt32();
        C.Content.Value = R.ReadUInt32();
        C.Location.X.Raw = R.ReadInt64();
        C.Location.Y.Raw = R.ReadInt64();
        C.Tile.X = R.ReadInt32();
        C.Tile.Y = R.ReadInt32();
        C.Param = R.ReadInt32();
        C.DirectAxes.Throttle = R.ReadInt8();
        C.DirectAxes.Steering = R.ReadInt8();
        C.DirectAxes.TurretYaw = R.ReadInt8();
        C.DirectAxes.TurretPitch = R.ReadInt8();
        C.DirectAxes.Flags = R.ReadUInt8();
        return C;
    }
};

// Commands for one simulation tick, from one or many players. The server assembles
// this, stamps the tick it will execute on, and every peer executes the identical
// list in the identical order.
struct CommandFrame
{
    TickIndex Tick = 0;
    std::vector<Command> Commands;

    void Serialize(ByteWriter& W) const
    {
        W.WriteUInt32(Tick);
        W.WriteUInt16(uint16_t(Commands.size()));
        for (const Command& C : Commands)
        {
            C.Serialize(W);
        }
    }

    static CommandFrame Deserialize(ByteReader& R)
    {
        CommandFrame F;
        F.Tick = R.ReadUInt32();
        const uint16_t Count = R.ReadUInt16();
        F.Commands.reserve(Count);
        for (uint16_t I = 0; I < Count && !R.HasError(); ++I)
        {
            F.Commands.push_back(Command::Deserialize(R));
        }
        return F;
    }
};

// Reasons a command can be refused. Returned to the issuing client for UI feedback
// and written to the server log; never silently dropped, because "my order did
// nothing" with no explanation is the hardest class of RTS bug to diagnose.
enum class CommandReject : uint8_t
{
    Accepted = 0,
    UnknownType,
    NoSuchEntity,
    NotOwner,
    EntityDead,
    UnknownContent,
    InsufficientCredits,
    InsufficientPower,
    TechRequirementsUnmet,
    InvalidPlacement,
    QueueFull,
    NoProducer,
    TargetInvalid,
    RateLimited,
    CommandCapExceeded,
    MatchOver,
    // Direct-control specific
    DirectIneligibleUnit,     // unit has no DirectControlProfile / not vehicle
    DirectAlreadyControlled,  // another player owns the slot
    DirectNotControlling,      // exit/fire without active possession
    DirectWeaponCooldown,      // fire rejected on cooldown
    DirectWeaponEmpty,         // no ammo / no weapon
    // Superweapon specific
    SuperweaponNotReady,       // still recharging
    SuperweaponUnpowered,      // base is in a power deficit
};

const char* ToString(CommandType Type);
const char* ToString(CommandReject Reason);

} // namespace RA4
