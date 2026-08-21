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
    Defeat,
    BuildingLost,
    UnitLost
};

USTRUCT(BlueprintType)
struct REDALERT4_API FRA4MusicTrackInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RA4|Audio")
    FString Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RA4|Audio")
    FString PrimaryAssetPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RA4|Audio")
    FString FallbackAssetPath;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRA4MusicTrackChanged, int32, TrackIndex, const FString&, TrackTitle, bool, bIsPlaying);

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

    /** Switch to a specific music track by index. */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    void PlayTrackByIndex(int32 TrackIndex);

    /** Advance to the next music track in playlist. */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    void NextTrack();

    /** Step back to the previous music track in playlist. */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    void PreviousTrack();

    /** Toggle play / pause of current music. */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    void ToggleMusicPause();

    /** Set master volume (0.0 to 1.0). */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    void SetMasterVolume(float Volume);

    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    float GetMasterVolume() const { return CurrentMasterVolume; }

    /** Set SFX volume (0.0 to 1.0). */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    void SetSfxVolume(float Volume);

    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    float GetSfxVolume() const { return CurrentSfxVolume; }

    /** Set EVA volume (0.0 to 1.0). */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    void SetEvaVolume(float Volume);

    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    float GetEvaVolume() const { return CurrentEvaVolume; }

    /** Toggle unit voice chatter. */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    void SetUnitVoicesEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    bool IsUnitVoicesEnabled() const { return bUnitVoicesEnabled; }

    /** Set the music volume (0.0 to 1.0). */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    void SetMusicVolume(float Volume);

    /** Get current music volume. */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    float GetMusicVolume() const { return CurrentMusicVolume; }

    /** Returns true if music is currently actively playing (not stopped or paused). */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    bool IsMusicPlaying() const;

    /** Get current track title. */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    FString GetCurrentTrackTitle() const;

    /** Get current track index. */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    int32 GetCurrentTrackIndex() const { return CurrentTrackIndex; }

    /** Get all available track titles. */
    UFUNCTION(BlueprintCallable, Category = "RA4|Audio")
    TArray<FString> GetTrackTitles() const;

    UPROPERTY(BlueprintAssignable, Category = "RA4|Audio")
    FOnRA4MusicTrackChanged OnMusicTrackChanged;

private:
    USoundBase* FindVoiceClip(const FString& VoiceId, ERA4VoiceEvent Event);
    USoundBase* FindEVAClip(uint8 Faction, ERA4EVAEvent Event);
    void InitPlaylist();

    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<USoundBase>> ClipCache;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> MusicComponent;

    UPROPERTY(Transient)
    TArray<FRA4MusicTrackInfo> Playlist;

    int32 CurrentTrackIndex = 0;
    float CurrentMasterVolume = 0.85f;
    float CurrentMusicVolume = 0.35f;
    float CurrentSfxVolume = 1.0f;
    float CurrentEvaVolume = 1.0f;
    bool bUnitVoicesEnabled = true;
    bool bMusicPaused = false;

    // Rapid re-selection of the same unit would otherwise retrigger its line every
    // click and turn the mix into a stutter.
    double LastVoiceTimeSeconds = -1000.0;
    double LastEvaTimeSeconds = -1000.0;
    FString LastVoiceId;
};
