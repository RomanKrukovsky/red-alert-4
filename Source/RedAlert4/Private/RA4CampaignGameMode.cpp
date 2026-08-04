// Copyright (c) Red Alert 4 project.
#include "RA4CampaignGameMode.h"

#include "RA4SimWorldSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
// The first Soviet mission. A default is needed because the level can be opened from
// the editor's play button with no URL at all, and a campaign map that comes up with
// no mission running would look like the mission system is broken rather than like an
// option was omitted.
const TCHAR* kDefaultMissionId = TEXT("sov_mission_1");
} // namespace

ARA4CampaignGameMode::ARA4CampaignGameMode()
{
}

void ARA4CampaignGameMode::StartSimulationMatch()
{
    FString Options = OptionsString;

    FString MissionId = UGameplayStatics::ParseOption(Options, TEXT("Mission"));
    if (MissionId.IsEmpty())
    {
        MissionId = kDefaultMissionId;
        UE_LOG(LogTemp, Warning, TEXT("RA4 campaign: no ?Mission option; defaulting to '%s'."), *MissionId);
    }

    const int32 Difficulty = UGameplayStatics::GetIntOption(Options, TEXT("Difficulty"), 1);

    // The campaign is single player. ExpectedPlayers is left at 1 so PostLogin takes
    // no lockstep path: a mission has one human in it by definition, and the objective
    // runtime is evaluated locally rather than agreed between peers.
    ExpectedPlayers = 1;

    UWorld* World = GetWorld();
    URA4SimWorldSubsystem* SimSub = World != nullptr ? World->GetSubsystem<URA4SimWorldSubsystem>() : nullptr;
    if (SimSub == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RA4 campaign: no simulation subsystem in this world."));
        return;
    }

    if (!SimSub->StartCampaignMission(MissionId, Difficulty))
    {
        // StartCampaignMission has already logged which id it could not find. There is
        // deliberately no fallback to another mission -- see the comment there.
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("RA4 campaign game mode started mission '%s' at difficulty %d."),
           *MissionId, Difficulty);
}
