// Copyright (c) Red Alert 4 project.

#include "RA4NetworkManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void URA4NetworkManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void URA4NetworkManager::Deinitialize()
{
    Super::Deinitialize();
}

void URA4NetworkManager::SendCommandToServer(const RA4::Command& Cmd)
{
    // In a real UE implementation, this would invoke a Server RPC on the player controller.
    // For now, we queue it locally.
    PendingLocalCommands.push_back(Cmd);
}

void URA4NetworkManager::BroadcastCommandFrame(const RA4::CommandFrame& Frame)
{
    // The server gathers all commands from all players into a CommandFrame
    // and sends it to all clients via a Multicast RPC or a reliable UDP channel.
    
    // For local testing without real network, just immediately pass to self
    OnCommandFrameReceived(Frame);
}

void URA4NetworkManager::OnCommandFrameReceived(const RA4::CommandFrame& Frame)
{
    // When the client receives the authoritative frame, it queues it for execution
    // on the Simulation World at the corresponding TickIndex.
    // This ensures deterministic execution across all clients.
}

void URA4NetworkManager::SubmitStateChecksum(uint32 TickIndex, uint64 Checksum)
{
    // Client computes its checksum for TickIndex and sends it to Server via RPC
}

void URA4NetworkManager::VerifyClientChecksum(int32 ClientId, uint32 TickIndex, uint64 Checksum)
{
    if (uint64* AuthChecksum = AuthoritativeChecksums.Find(TickIndex))
    {
        if (*AuthChecksum != Checksum)
        {
            // Desync detected!
            UE_LOG(LogTemp, Error, TEXT("DESYNC DETECTED! Client %d desynced at tick %d. Auth: %llu, Client: %llu"), ClientId, TickIndex, *AuthChecksum, Checksum);
            // Handle desync (e.g. request full snapshot, kick, etc.)
        }
    }
}
