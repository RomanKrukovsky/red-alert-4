// Copyright (c) Red Alert 4 project.
#include "RA4NetworkChannel.h"
#include "RA4NetworkManager.h"

#include "RA4Core/ByteStream.h"
#include "RA4Core/Command.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogRA4NetChannel, Log, All);

namespace
{

void FrameToPayload(const RA4::CommandFrame& Frame, TArray<uint8>& OutPayload)
{
    RA4::ByteWriter Writer;
    Frame.Serialize(Writer);

    const std::vector<uint8_t>& Bytes = Writer.GetBuffer();
    OutPayload.SetNumUninitialized(int32(Bytes.size()), EAllowShrinking::No);
    if (!Bytes.empty())
    {
        FMemory::Memcpy(OutPayload.GetData(), Bytes.data(), Bytes.size());
    }
}

/** False if the payload was truncated or malformed. A caller must not act on the
    frame in that case: a half-decoded command list is a desync with extra steps. */
bool PayloadToFrame(const TArray<uint8>& Payload, RA4::CommandFrame& OutFrame)
{
    if (Payload.Num() <= 0)
    {
        return false;
    }

    RA4::ByteReader Reader(Payload.GetData(), size_t(Payload.Num()));
    OutFrame = RA4::CommandFrame::Deserialize(Reader);
    return !Reader.HasError();
}

} // namespace

URA4NetworkChannel::URA4NetworkChannel()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void URA4NetworkChannel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(URA4NetworkChannel, PlayerIndex);
    DOREPLIFETIME(URA4NetworkChannel, MatchNumPlayers);
    DOREPLIFETIME(URA4NetworkChannel, MatchInputDelay);
}

void URA4NetworkChannel::ConfigureAsServer(int32 InPlayerIndex, int32 InNumPlayers, int32 InInputDelay)
{
    MatchNumPlayers = InNumPlayers;
    MatchInputDelay = InInputDelay;
    SetPlayerIndex(InPlayerIndex);
}

void URA4NetworkChannel::OnRep_MatchSetup()
{
    if (PlayerIndex == INDEX_NONE || MatchNumPlayers <= 0)
    {
        // The three properties can arrive in separate replications; wait until the
        // set is coherent rather than starting a session with a placeholder slot.
        return;
    }

    URA4NetworkManager* Manager = GetManager();
    if (Manager == nullptr || Manager->IsMatchActive())
    {
        return;
    }

    Manager->RegisterChannel(uint8(PlayerIndex), this);
    Manager->BeginMatch(uint8(PlayerIndex), uint8(MatchNumPlayers), /*bIsServer*/ false,
                        uint32(MatchInputDelay));
}

void URA4NetworkChannel::BeginPlay()
{
    Super::BeginPlay();

    if (PlayerIndex != INDEX_NONE)
    {
        if (URA4NetworkManager* Manager = GetManager())
        {
            Manager->RegisterChannel(uint8(PlayerIndex), this);
        }
    }
}

void URA4NetworkChannel::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (PlayerIndex != INDEX_NONE)
    {
        if (URA4NetworkManager* Manager = GetManager())
        {
            Manager->UnregisterChannel(uint8(PlayerIndex));
        }
    }
    Super::EndPlay(EndPlayReason);
}

void URA4NetworkChannel::SetPlayerIndex(int32 InPlayerIndex)
{
    URA4NetworkManager* Manager = GetManager();

    if (Manager != nullptr && PlayerIndex != INDEX_NONE)
    {
        Manager->UnregisterChannel(uint8(PlayerIndex));
    }

    PlayerIndex = InPlayerIndex;

    if (Manager != nullptr && PlayerIndex != INDEX_NONE)
    {
        Manager->RegisterChannel(uint8(PlayerIndex), this);
    }
}

URA4NetworkManager* URA4NetworkChannel::GetManager() const
{
    if (const UWorld* World = GetWorld())
    {
        return World->GetSubsystem<URA4NetworkManager>();
    }
    return nullptr;
}

// --- Outbound --------------------------------------------------------------

void URA4NetworkChannel::SendFrameToServer(const RA4::CommandFrame& Frame)
{
    FrameToPayload(Frame, ScratchPayload);
    ServerSubmitFrame(ScratchPayload);
}

void URA4NetworkChannel::SendAuthoritativeFrame(const RA4::CommandFrame& Frame)
{
    FrameToPayload(Frame, ScratchPayload);
    ClientReceiveFrame(ScratchPayload);
}

void URA4NetworkChannel::SendChecksumToServer(uint32 TickIndex, uint64 Checksum)
{
    ServerSubmitChecksum(TickIndex, Checksum);
}

// --- Inbound ---------------------------------------------------------------

bool URA4NetworkChannel::ServerSubmitFrame_Validate(const TArray<uint8>& Payload)
{
    // A frame is a tick plus a bounded command list; anything larger than this is not
    // a frame this build produces, so it is refused before it is parsed.
    constexpr int32 MaxFramePayloadBytes = 64 * 1024;
    return Payload.Num() > 0 && Payload.Num() <= MaxFramePayloadBytes;
}

void URA4NetworkChannel::ServerSubmitFrame_Implementation(const TArray<uint8>& Payload)
{
    RA4::CommandFrame Frame;
    if (!PayloadToFrame(Payload, Frame))
    {
        UE_LOG(LogRA4NetChannel, Warning, TEXT("Discarded malformed command frame from player %d."),
               PlayerIndex);
        return;
    }

    if (PlayerIndex == INDEX_NONE)
    {
        return;
    }

    // The slot comes from this channel, never from the payload. A client that stamped
    // someone else's player id on its frame would otherwise be issuing their orders.
    if (URA4NetworkManager* Manager = GetManager())
    {
        Manager->HandlePlayerFrame(uint8(PlayerIndex), Frame);
    }
}

void URA4NetworkChannel::ClientReceiveFrame_Implementation(const TArray<uint8>& Payload)
{
    RA4::CommandFrame Frame;
    if (!PayloadToFrame(Payload, Frame))
    {
        UE_LOG(LogRA4NetChannel, Error, TEXT("Discarded malformed authoritative frame."));
        return;
    }

    if (URA4NetworkManager* Manager = GetManager())
    {
        Manager->HandleAuthoritativeFrame(Frame);
    }
}

bool URA4NetworkChannel::ServerSubmitChecksum_Validate(uint32 TickIndex, uint64 Checksum)
{
    return true;
}

void URA4NetworkChannel::ServerSubmitChecksum_Implementation(uint32 TickIndex, uint64 Checksum)
{
    if (PlayerIndex == INDEX_NONE)
    {
        return;
    }

    if (URA4NetworkManager* Manager = GetManager())
    {
        Manager->VerifyClientChecksum(PlayerIndex, TickIndex, Checksum);
    }
}
