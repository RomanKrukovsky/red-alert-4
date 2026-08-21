// Copyright (c) Red Alert 4 project.
#include "RA4AudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
// Matches the layout produced by RA4AudioImportCommandlet, which mirrors the
// generated voice pack's own directory and file naming.
const TCHAR* kVoiceRoot = TEXT("/Game/RA4/Audio/Generated/Voice/Soviet");
const TCHAR* kMusicAsset =
    TEXT("/Game/RA4/Audio/Generated/Music/Iron_Parade.Iron_Parade");

// A selected unit answering every single click is noise rather than feedback.
constexpr double kVoiceCooldownSeconds = 0.6;
constexpr double kEvaCooldownSeconds = 2.5;

const TCHAR* ToEventName(ERA4VoiceEvent Event)
{
    switch (Event)
    {
    case ERA4VoiceEvent::Selected: return TEXT("Selected");
    case ERA4VoiceEvent::Move:     return TEXT("Move");
    case ERA4VoiceEvent::Attack:   return TEXT("Attack");
    case ERA4VoiceEvent::Ability:  return TEXT("Ability");
    case ERA4VoiceEvent::Damaged:  return TEXT("Damaged");
    case ERA4VoiceEvent::Elite:    return TEXT("Elite");
    case ERA4VoiceEvent::Idle:     return TEXT("Idle");
    case ERA4VoiceEvent::Death:    return TEXT("Death");
    default:                       return TEXT("Selected");
    }
}

const TCHAR* ToEvaEventName(ERA4EVAEvent Event)
{
    switch (Event)
    {
    case ERA4EVAEvent::ConstructionComplete: return TEXT("CONSTRUCTION_COMPLETE");
    case ERA4EVAEvent::UnitReady:             return TEXT("UNIT_READY_GENERIC");
    case ERA4EVAEvent::BaseUnderAttack:       return TEXT("BASE_UNDER_ATTACK");
    case ERA4EVAEvent::InsufficientFunds:     return TEXT("INSUFFICIENT_FUNDS");
    case ERA4EVAEvent::PowerLow:              return TEXT("POWER_LOW");
    case ERA4EVAEvent::Victory:               return TEXT("MATCH_VICTORY");
    case ERA4EVAEvent::Defeat:                return TEXT("MATCH_DEFEAT");
    case ERA4EVAEvent::BuildingLost:          return TEXT("BUILDING_LOST");
    case ERA4EVAEvent::UnitLost:              return TEXT("UNIT_LOST");
    default:                                  return TEXT("UNIT_READY_GENERIC");
    }
}
} // namespace

void URA4AudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void URA4AudioSubsystem::Deinitialize()
{
    StopMusic();
    ClipCache.Empty();
    Super::Deinitialize();
}

USoundBase* URA4AudioSubsystem::FindVoiceClip(const FString& VoiceId, ERA4VoiceEvent Event)
{
    const TCHAR* FactionFolder = TEXT("Soviet");
    if (VoiceId.StartsWith(TEXT("AL_")))
    {
        FactionFolder = TEXT("Alliance");
    }
    else if (VoiceId.StartsWith(TEXT("CO_")))
    {
        FactionFolder = TEXT("Coalition");
    }
    else if (VoiceId.StartsWith(TEXT("CH_")))
    {
        FactionFolder = TEXT("Chrono");
    }

    const int32 VariationIndex = FMath::RandRange(1, 4);
    const TCHAR* EventName = ToEventName(Event);

    // List candidate asset paths to try in priority order
    TArray<FString, TInlineAllocator<8>> CandidatePaths;
    
    // 1. Faction folder with randomized variation
    CandidatePaths.Add(FString::Printf(TEXT("/Game/RA4/Audio/Generated/Voice/%s/%s/VO_RU_%s_%s_%02d.VO_RU_%s_%s_%02d"),
        FactionFolder, *VoiceId, *VoiceId, EventName, VariationIndex, *VoiceId, EventName, VariationIndex));
    // 2. Faction folder with base variation _01
    CandidatePaths.Add(FString::Printf(TEXT("/Game/RA4/Audio/Generated/Voice/%s/%s/VO_RU_%s_%s_01.VO_RU_%s_%s_01"),
        FactionFolder, *VoiceId, *VoiceId, EventName, *VoiceId, EventName));
    // 3. Mastered folder with variation
    CandidatePaths.Add(FString::Printf(TEXT("/Game/RA4/Audio/Generated/Voice/Mastered/%s/VO_RU_%s_%s_%02d.VO_RU_%s_%s_%02d"),
        *VoiceId, *VoiceId, EventName, VariationIndex, *VoiceId, EventName, VariationIndex));
    // 4. Mastered folder with base variation _01
    CandidatePaths.Add(FString::Printf(TEXT("/Game/RA4/Audio/Generated/Voice/Mastered/%s/VO_RU_%s_%s_01.VO_RU_%s_%s_01"),
        *VoiceId, *VoiceId, EventName, *VoiceId, EventName));
    // 5. English / non-RU fallback
    CandidatePaths.Add(FString::Printf(TEXT("/Game/RA4/Audio/Generated/Voice/%s/%s/VO_EN_%s_%s_01.VO_EN_%s_%s_01"),
        FactionFolder, *VoiceId, *VoiceId, EventName, *VoiceId, EventName));
    // 6. Generic variation
    CandidatePaths.Add(FString::Printf(TEXT("/Game/RA4/Audio/Generated/Voice/%s/%s/VO_%s_%s_01.VO_%s_%s_01"),
        FactionFolder, *VoiceId, *VoiceId, EventName, *VoiceId, EventName));

    for (const FString& ObjectPath : CandidatePaths)
    {
        if (TObjectPtr<USoundBase>* Cached = ClipCache.Find(ObjectPath))
        {
            if (Cached->Get() != nullptr)
            {
                return Cached->Get();
            }
            continue;
        }

        USoundBase* Clip = LoadObject<USoundBase>(nullptr, *ObjectPath);
        if (Clip != nullptr)
        {
            ClipCache.Add(ObjectPath, Clip);
            return Clip;
        }
        ClipCache.Add(ObjectPath, nullptr);
    }

    return nullptr;
}

void URA4AudioSubsystem::PlayUnitVoice(const FString& VoiceId, ERA4VoiceEvent Event, bool bBypassCooldown)
{
    if (VoiceId.IsEmpty())
    {
        return;
    }

    const UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    const double Now = World->GetTimeSeconds();
    if (!bBypassCooldown && Now - LastVoiceTimeSeconds < kVoiceCooldownSeconds)
    {
        return;
    }

    USoundBase* Clip = FindVoiceClip(VoiceId, Event);
    if (Clip == nullptr)
    {
        return;
    }

    // 2D: these are the commander's radio, not a sound emitted at a world position.
    UGameplayStatics::PlaySound2D(World, Clip);
    LastVoiceTimeSeconds = Now;
    LastVoiceId = VoiceId;
}

USoundBase* URA4AudioSubsystem::FindEVAClip(uint8 Faction, ERA4EVAEvent Event)
{
    const TCHAR* Folder = TEXT("Soviet");
    const TCHAR* Code = TEXT("SU");
    const TCHAR* AltPrefix = TEXT("Soviet");

    if (Faction == 2)
    {
        Folder = TEXT("Alliance");
        Code = TEXT("AL");
        AltPrefix = TEXT("Alliance");
    }
    else if (Faction == 3)
    {
        Folder = TEXT("Coalition");
        Code = TEXT("CO");
        AltPrefix = TEXT("Coalition");
    }
    else if (Faction == 4)
    {
        Folder = TEXT("Chrono");
        Code = TEXT("CH");
        AltPrefix = TEXT("Chrono");
    }

    const FString AssetName = FString::Printf(
        TEXT("VO_RU_%s_EVA_%s_01"), Code, ToEvaEventName(Event));
    const FString ObjectPath = FString::Printf(
        TEXT("/Game/RA4/Audio/Generated/EVA/%s/%s.%s"),
        Folder, *AssetName, *AssetName);

    if (TObjectPtr<USoundBase>* Cached = ClipCache.Find(ObjectPath))
    {
        return Cached->Get();
    }

    USoundBase* Clip = LoadObject<USoundBase>(nullptr, *ObjectPath);
    if (Clip == nullptr)
    {
        FString AltEventName;
        switch (Event)
        {
        case ERA4EVAEvent::ConstructionComplete: AltEventName = TEXT("BuildingConstructionComplete"); break;
        case ERA4EVAEvent::UnitReady: AltEventName = TEXT("UnitReady"); break;
        case ERA4EVAEvent::BaseUnderAttack: AltEventName = TEXT("BaseUnderAttack"); break;
        case ERA4EVAEvent::InsufficientFunds: AltEventName = TEXT("ResourcesLow"); break;
        case ERA4EVAEvent::PowerLow: AltEventName = TEXT("PowerLow"); break;
        case ERA4EVAEvent::Victory: AltEventName = TEXT("PlayerVictory"); break;
        case ERA4EVAEvent::Defeat: AltEventName = TEXT("PlayerDefeat"); break;
        case ERA4EVAEvent::BuildingLost: AltEventName = TEXT("BuildingLost"); break;
        case ERA4EVAEvent::UnitLost: AltEventName = TEXT("UnitLost"); break;
        }
        if (!AltEventName.IsEmpty())
        {
            const FString AltAssetName = FString::Printf(TEXT("VO_%s_EVA_%s"), AltPrefix, *AltEventName);
            const FString AltObjectPath = FString::Printf(TEXT("/Game/RA4/Audio/Generated/EVA/%s/%s.%s"), Folder, *AltAssetName, *AltAssetName);
            Clip = LoadObject<USoundBase>(nullptr, *AltObjectPath);
        }
    }

    ClipCache.Add(ObjectPath, Clip);
    return Clip;
}

void URA4AudioSubsystem::PlayEVA(uint8 Faction, ERA4EVAEvent Event, bool bBypassCooldown)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    const double Now = World->GetTimeSeconds();
    if (!bBypassCooldown && Now - LastEvaTimeSeconds < kEvaCooldownSeconds)
    {
        return;
    }

    if (USoundBase* Clip = FindEVAClip(Faction, Event))
    {
        UGameplayStatics::PlaySound2D(World, Clip);
        LastEvaTimeSeconds = Now;
    }
}

void URA4AudioSubsystem::InitPlaylist()
{
    if (Playlist.Num() > 0)
    {
        return;
    }

    Playlist.Add({
        TEXT("Steel Horizon Pact"),
        TEXT("/Game/RA4/Audio/Generated/Music/Steel_Horizon_Pact.Steel_Horizon_Pact"),
        TEXT("/Game/RA4/Audio/Music/Steel_Horizon_Pact.Steel_Horizon_Pact")
    });
    Playlist.Add({
        TEXT("Iron Parade"),
        TEXT("/Game/RA4/Audio/Generated/Music/Iron_Parade.Iron_Parade"),
        TEXT("/Game/RA4/Audio/Music/Iron_Parade.Iron_Parade")
    });
    Playlist.Add({
        TEXT("Command Overdrive"),
        TEXT("/Game/RA4/Audio/Music/Command_Overdrive.Command_Overdrive"),
        TEXT("/Game/RA4/Audio/Music/Command_Overdrive.Command_Overdrive")
    });
    Playlist.Add({
        TEXT("March of Steel"),
        TEXT("/Game/RA4/Audio/Music/March_of_Steel.March_of_Steel"),
        TEXT("/Game/RA4/Audio/Music/March_of_Steel.March_of_Steel")
    });
    Playlist.Add({
        TEXT("Red Iron March"),
        TEXT("/Game/RA4/Audio/Music/Red_Iron_March.Red_Iron_March"),
        TEXT("/Game/RA4/Audio/Music/Red_Iron_March.Red_Iron_March")
    });
    Playlist.Add({
        TEXT("Red Banner Forge"),
        TEXT("/Game/RA4/Audio/Music/Red_Banner_Forge.Red_Banner_Forge"),
        TEXT("/Game/RA4/Audio/Music/Red_Banner_Forge.Red_Banner_Forge")
    });
    Playlist.Add({
        TEXT("Tesla Overdrive"),
        TEXT("/Game/RA4/Audio/Music/Tesla_Overdrive.Tesla_Overdrive"),
        TEXT("/Game/RA4/Audio/Music/Tesla_Overdrive.Tesla_Overdrive")
    });
    Playlist.Add({
        TEXT("Red Alert 4 Main Theme"),
        TEXT("/Game/RA4/Audio/Generated/Music/RA4_MainMenu_Theme.RA4_MainMenu_Theme"),
        TEXT("/Game/RA4/Audio/Generated/Music/RA4_MainMenu_Theme.RA4_MainMenu_Theme")
    });
}

void URA4AudioSubsystem::StartMusic()
{
    InitPlaylist();
    if (MusicComponent != nullptr && MusicComponent->IsPlaying())
    {
        return;
    }

    PlayTrackByIndex(CurrentTrackIndex);
}

void URA4AudioSubsystem::StopMusic()
{
    if (MusicComponent != nullptr)
    {
        MusicComponent->Stop();
        MusicComponent = nullptr;
        bMusicPaused = false;
    }
}

void URA4AudioSubsystem::PlayTrackByIndex(int32 TrackIndex)
{
    InitPlaylist();
    if (Playlist.Num() == 0)
    {
        return;
    }

    CurrentTrackIndex = FMath::Clamp(TrackIndex, 0, Playlist.Num() - 1);
    const FRA4MusicTrackInfo& TrackInfo = Playlist[CurrentTrackIndex];

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    USoundBase* Track = LoadObject<USoundBase>(nullptr, *TrackInfo.PrimaryAssetPath);
    if (Track == nullptr && !TrackInfo.FallbackAssetPath.IsEmpty())
    {
        Track = LoadObject<USoundBase>(nullptr, *TrackInfo.FallbackAssetPath);
    }

    if (Track == nullptr)
    {
        UE_LOG(LogTemp, Warning,
               TEXT("RA4 audio: music track '%s' not found at %s"),
               *TrackInfo.Title, *TrackInfo.PrimaryAssetPath);
        return;
    }

    if (MusicComponent != nullptr)
    {
        MusicComponent->Stop();
        MusicComponent = nullptr;
    }

    MusicComponent = UGameplayStatics::SpawnSound2D(World, Track, CurrentMusicVolume,
                                                    /*PitchMultiplier*/ 1.0f, /*StartTime*/ 0.0f,
                                                    /*ConcurrencySettings*/ nullptr,
                                                    /*bPersistAcrossLevelTransition*/ false,
                                                    /*bAutoDestroy*/ false);
    if (MusicComponent != nullptr)
    {
        MusicComponent->bIsUISound = true;
        bMusicPaused = false;
        UE_LOG(LogTemp, Display, TEXT("RA4 audio: music started [%d/%d]: %s"),
               CurrentTrackIndex + 1, Playlist.Num(), *TrackInfo.Title);
    }

    OnMusicTrackChanged.Broadcast(CurrentTrackIndex, TrackInfo.Title, IsMusicPlaying());
}

void URA4AudioSubsystem::NextTrack()
{
    InitPlaylist();
    if (Playlist.Num() == 0)
    {
        return;
    }

    const int32 NextIdx = (CurrentTrackIndex + 1) % Playlist.Num();
    PlayTrackByIndex(NextIdx);
}

void URA4AudioSubsystem::PreviousTrack()
{
    InitPlaylist();
    if (Playlist.Num() == 0)
    {
        return;
    }

    const int32 PrevIdx = (CurrentTrackIndex - 1 + Playlist.Num()) % Playlist.Num();
    PlayTrackByIndex(PrevIdx);
}

void URA4AudioSubsystem::ToggleMusicPause()
{
    InitPlaylist();
    if (MusicComponent == nullptr)
    {
        StartMusic();
        return;
    }

    if (bMusicPaused)
    {
        MusicComponent->SetPaused(false);
        bMusicPaused = false;
    }
    else if (MusicComponent->IsPlaying())
    {
        MusicComponent->SetPaused(true);
        bMusicPaused = true;
    }
    else
    {
        PlayTrackByIndex(CurrentTrackIndex);
        return;
    }

    OnMusicTrackChanged.Broadcast(CurrentTrackIndex, GetCurrentTrackTitle(), IsMusicPlaying());
}

void URA4AudioSubsystem::SetMusicVolume(float Volume)
{
    CurrentMusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
    if (MusicComponent != nullptr)
    {
        MusicComponent->SetVolumeMultiplier(CurrentMusicVolume);
    }
}

bool URA4AudioSubsystem::IsMusicPlaying() const
{
    return MusicComponent != nullptr && MusicComponent->IsPlaying() && !bMusicPaused;
}

FString URA4AudioSubsystem::GetCurrentTrackTitle() const
{
    if (Playlist.IsValidIndex(CurrentTrackIndex))
    {
        return Playlist[CurrentTrackIndex].Title;
    }
    return TEXT("Steel Horizon Pact");
}

TArray<FString> URA4AudioSubsystem::GetTrackTitles() const
{
    const_cast<URA4AudioSubsystem*>(this)->InitPlaylist();
    TArray<FString> Titles;
    for (const FRA4MusicTrackInfo& Track : Playlist)
    {
        Titles.Add(Track.Title);
    }
    return Titles;
}
