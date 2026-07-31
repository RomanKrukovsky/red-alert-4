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
    // e.g. .../Voice/Soviet/SU_RubezhRifleman/VO_RU_SU_RubezhRifleman_Selected_01
    const FString AssetName =
        FString::Printf(TEXT("VO_RU_%s_%s_01"), *VoiceId, ToEventName(Event));
    const FString ObjectPath =
        FString::Printf(TEXT("%s/%s/%s.%s"), kVoiceRoot, *VoiceId, *AssetName, *AssetName);

    if (TObjectPtr<USoundBase>* Cached = ClipCache.Find(ObjectPath))
    {
        return Cached->Get();
    }

    // A null result is cached too: most units have no recorded pack yet, and retrying
    // a failing synchronous load on every order would be the expensive path.
    USoundBase* Clip = LoadObject<USoundBase>(nullptr, *ObjectPath);
    ClipCache.Add(ObjectPath, Clip);
    return Clip;
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

void URA4AudioSubsystem::StartMusic()
{
    if (MusicComponent != nullptr && MusicComponent->IsPlaying())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    USoundBase* Track = LoadObject<USoundBase>(nullptr, kMusicAsset);
    if (Track == nullptr)
    {
        UE_LOG(LogTemp, Warning,
               TEXT("RA4 audio: music track not found at %s -- run the RA4AudioImport commandlet"),
               kMusicAsset);
        return;
    }

    MusicComponent = UGameplayStatics::SpawnSound2D(World, Track, /*VolumeMultiplier*/ 0.35f,
                                                    /*PitchMultiplier*/ 1.0f, /*StartTime*/ 0.0f,
                                                    /*ConcurrencySettings*/ nullptr,
                                                    /*bPersistAcrossLevelTransition*/ false,
                                                    /*bAutoDestroy*/ false);
    if (MusicComponent != nullptr)
    {
        MusicComponent->bIsUISound = true;
        UE_LOG(LogTemp, Display, TEXT("RA4 audio: music started"));
    }
}

void URA4AudioSubsystem::StopMusic()
{
    if (MusicComponent != nullptr)
    {
        MusicComponent->Stop();
        MusicComponent = nullptr;
    }
}
