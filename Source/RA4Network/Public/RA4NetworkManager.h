// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RA4Core/Command.h"
#include "RA4NetworkManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCommandRejected, int32, ReasonCode);

UCLASS()
class RA4NETWORK_API URA4NetworkManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Queue a local command to be sent to the server.
    // In a lockstep / authoritative model, the client does not execute the command locally
    // until it receives the authoritative CommandFrame back from the server.
    void SendCommandToServer(const RA4::Command& Cmd);

    // Called by the server to broadcast a compiled CommandFrame to all clients.
    void BroadcastCommandFrame(const RA4::CommandFrame& Frame);

    // Called by the client when an authoritative CommandFrame is received.
    void OnCommandFrameReceived(const RA4::CommandFrame& Frame);

    // Submit checksum of local game state for verification against server
    void SubmitStateChecksum(uint32 TickIndex, uint64 Checksum);

    // Called by server to verify a client's checksum
    void VerifyClientChecksum(int32 ClientId, uint32 TickIndex, uint64 Checksum);

    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnCommandRejected OnCommandRejected;

private:
    // Temporary queue for locally issued commands before server packages them
    std::vector<RA4::Command> PendingLocalCommands;

    // A mapping of tick indices to authoritative checksums on the server
    TMap<uint32, uint64> AuthoritativeChecksums;
};
