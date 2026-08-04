// Copyright (c) Red Alert 4 project.
//
// The wire. One component per player, living on that player's PlayerController, so
// every RPC has an owning connection: the server can address a specific client and a
// client's submission is attributable to the player who sent it.
//
// Frames cross the wire as bytes produced by RA4::CommandFrame::Serialize rather than
// as replicated USTRUCTs. That is deliberate -- the byte layout in ByteStream.h is the
// same one the replay and the checksum use, so a frame recorded from a live match and
// a frame replayed from disk are the same bytes. A UStruct mirror would be a second
// definition of the command format that could drift from the first.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RA4NetworkChannel.generated.h"

namespace RA4 { struct CommandFrame; }

class URA4NetworkManager;

UCLASS(ClassGroup = (RA4), meta = (BlueprintSpawnableComponent))
class RA4NETWORK_API URA4NetworkChannel : public UActorComponent
{
    GENERATED_BODY()

public:
    URA4NetworkChannel();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Which player slot this channel speaks for. Assigned by the server when the
        player joins and replicated so the client knows its own slot. */
    UPROPERTY(ReplicatedUsing = OnRep_MatchSetup, BlueprintReadOnly, Category = "Network")
    int32 PlayerIndex = INDEX_NONE;

    /** Replicated alongside the slot because a client cannot start its session without
        them, and a client that guessed a different player count or input delay would
        desync on the command stream before the simulation ran a single step. */
    UPROPERTY(ReplicatedUsing = OnRep_MatchSetup, BlueprintReadOnly, Category = "Network")
    int32 MatchNumPlayers = 0;

    UPROPERTY(ReplicatedUsing = OnRep_MatchSetup, BlueprintReadOnly, Category = "Network")
    int32 MatchInputDelay = 0;

    /** Server: assigns this channel's slot and the match parameters, and starts the
        server's own session on the channel that belongs to the host. */
    void ConfigureAsServer(int32 InPlayerIndex, int32 InNumPlayers, int32 InInputDelay);

    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetPlayerIndex(int32 InPlayerIndex);

    /** Client -> server: this player's command slice for one tick. */
    void SendFrameToServer(const RA4::CommandFrame& Frame);

    /** Server -> this client: the assembled authoritative frame for one tick. */
    void SendAuthoritativeFrame(const RA4::CommandFrame& Frame);

    /** Client -> server: post-tick state checksum, for desync adjudication. */
    void SendChecksumToServer(uint32 TickIndex, uint64 Checksum);

private:
    /** A client learns its slot and the match parameters in one replication, then
        starts its session. Guarded so a later property update cannot restart a match
        that is already running. */
    UFUNCTION()
    void OnRep_MatchSetup();

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSubmitFrame(const TArray<uint8>& Payload);

    UFUNCTION(Client, Reliable)
    void ClientReceiveFrame(const TArray<uint8>& Payload);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSubmitChecksum(uint32 TickIndex, uint64 Checksum);

    URA4NetworkManager* GetManager() const;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Reused across sends so a 20 Hz command stream does not allocate a fresh array
    // every tick for the whole match.
    TArray<uint8> ScratchPayload;
};
