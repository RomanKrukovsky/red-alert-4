// Copyright (c) Red Alert 4 project. Runtime audio: unit voice lines and music.
//
// Sound assets are looked up by convention rather than through a hand-authored table:
// the voice pack is generated, so its file names already encode unit StableID and
// event, and a table would only be a second copy of that to keep in sync.
//
// Nothing here is allowed to affect the simulation. Audio reacts to events the
// simulation already emitted, and never feeds anything back, so a match with sound
// muted produces an identical replay and state checksum.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "RA4AudioSubsystem.generated.h"

class USoundBase;
class UAudioComponent;

/** The unit voice events the generated pack provides, one clip per unit per event. */
UENUM(BlueprintType)
enum class ERA4VoiceEvent : uint8
{
    Selected,
    Move,
    Attack,
    Ability,
    Damaged,
    Elite,
    Idle,
    Death
};

UENUM(BlueprintType)
enum class ERA4EVAEvent : uint8
{
    ConstructionComplete,
    UnitReady,
    BaseUnderAttack,
    InsufficientFunds,
    PowerLow,
    Victory,
    Defeat
};

UCLASS()
class REDALERT4_API URA4AudioSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Plays the clip for a unit responding to an order or state change. VoiceId is the
     * unit's Stable ID (e.g. "SU_RubezhRifleman"). Silently does nothing when the pack
     * has no clip for that unit, which is the normal case for factions that have not
     * been recorded yet.
     */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    void PlayUnitVoice(const FString& VoiceId, ERA4VoiceEvent Event, bool bBypassCooldown = false);

    /** Plays a faction announcer line imported by RA4AudioImport. */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    void PlayEVA(uint8 Faction, ERA4EVAEvent Event, bool bBypassCooldown = false);

    /** Starts the background music track, looping. Safe to call more than once. */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    void StartMusic();

    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    void StopMusic();

private:
    USoundBase* FindVoiceClip(const FString& VoiceId, ERA4VoiceEvent Event);
    USoundBase* FindEVAClip(uint8 Faction, ERA4EVAEvent Event);

    // Resolved clips are cached because a miss costs a synchronous package load, and
    // most lookups miss: only Soviet units are recorded so far.
    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<USoundBase>> ClipCache;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> MusicComponent;

    // Rapid re-selection of the same unit would otherwise retrigger its line every
    // click and turn the mix into a stutter.
    double LastVoiceTimeSeconds = -1000.0;
    double LastEvaTimeSeconds = -1000.0;
    FString LastVoiceId;
};
