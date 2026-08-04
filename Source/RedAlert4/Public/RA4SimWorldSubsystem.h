// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RA4EntityActor.h"

// Forward declare the C++ simulation classes without including their headers here
// to minimize compile-time dependencies. Command.h is engine-free and cheap, and is
// included because the pending-command queue needs the complete type.
#include "RA4Core/Command.h"

#include <vector>

namespace RA4
{
    class SimWorld;
    class ContentDatabase;
    class CampaignDatabase;
    class MissionRuntime;
    namespace Presentation
    {
        struct HudSnapshot;
        class HudSnapshotBuilder;
    }
    namespace AI
    {
        class AICommander;
    }
}

#include "RA4SimWorldSubsystem.generated.h"

class URA4NetworkManager;

UCLASS()
class REDALERT4_API URA4SimWorldSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    URA4SimWorldSubsystem();
    virtual ~URA4SimWorldSubsystem();

    // UWorldSubsystem interface
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime);
    virtual TStatId GetStatId() const override;

    // Registers a presentation actor to a simulation entity ID
    UFUNCTION(BlueprintCallable, Category = "Simulation")
    void RegisterEntityActor(int32 EntityIndex, ARA4EntityActor* Actor);

    // Unregisters a presentation actor
    UFUNCTION(BlueprintCallable, Category = "Simulation")
    void UnregisterEntityActor(int32 EntityIndex);

    // Returns the presentation actor for a given simulation entity ID
    UFUNCTION(BlueprintCallable, Category = "Simulation")
    ARA4EntityActor* GetEntityActor(int32 EntityIndex) const;
    ARA4EntityActor* GetEntityActor(RA4::EntityId Id) const;

    // Read-only view of the authoritative state, for input picking and the HUD.
    // Nothing outside the simulation may mutate it directly.
    const RA4::SimWorld* GetSimWorld() const { return SimWorld; }

    // Returns the latest computed snapshot of the match for UI presentation
    const RA4::Presentation::HudSnapshot* GetLatestHudSnapshot() const { return LatestSnapshot; }

    // Initializes the simulation state for a skirmish match. Should be called by the
    // GameMode. NumAI and AISpot default to the classic 1v1 layout.
    void StartSkirmishMatch(uint8 PlayerFaction, uint8 EnemyFaction, int32 Difficulty,
                            int32 NumAI = 1, int32 AISpot = -1);

    // Brings up a campaign mission by id: the mission's own MissionSetupDef decides
    // the map, the player slots and the opening forces, so a mission is a match in
    // its own right rather than a skirmish with a briefing on top. Returns false if
    // the id is not in the campaign database, in which case no match is started.
    bool StartCampaignMission(const FString& MissionId, int32 Difficulty);

    // The objective runtime for the mission in progress, or null in a skirmish. The
    // HUD reads objective state through this; nothing outside may advance it, because
    // objectives must be evaluated exactly once per simulation tick.
    const RA4::MissionRuntime* GetMissionRuntime() const { return Mission; }

    // Sets the local player's selection for HUD projection
    void SetSelectedEntitiesForHUD(const std::vector<RA4::EntityId>& Selection);

    // The single entry point for player and AI intent. Commands are queued here and
    // applied at the start of the next fixed tick, which is exactly where the
    // network layer will serialise and send them, so single player and multiplayer
    // follow the same path.
    void EnqueueCommand(const RA4::Command& Command);

    // Map of ContentId / UnitType ID to Unreal StaticMesh asset
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
    TMap<int32, UStaticMesh*> ContentMeshRegistry;

    // Base Actor class to spawn for visual representations
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
    TSubclassOf<ARA4EntityActor> EntityActorClass;

private:
    void LoadBlockoutMesh(uint32 ContentIdValue, const TCHAR* AssetPath);
    void RegisterDefaultBlockoutMeshes();
    void FitGroundPlaneToMap();
    void TickSimulation();

    // One commander per active player other than LocalPlayer. Shared by the skirmish
    // and campaign entry points so a mission's enemies are driven by the same AI a
    // skirmish uses -- an unattended enemy base is not an opponent.
    void AttachAICommanders(int32 Difficulty, uint64 Seed, RA4::PlayerId LocalPlayer);

    // Advances objective state by exactly one tick and reports what changed. Called
    // from TickSimulation immediately after the world tick, so objectives judge the
    // state the next frame will act on -- the same order the headless mission tests
    // use, which is what makes a mission play out identically in both.
    void EvaluateMission();

    // The network manager if a lockstep match is running, otherwise null. Null is the
    // single-player case and is not an error: every networked branch in this file is
    // written so that null means "run locally exactly as before".
    URA4NetworkManager* GetActiveNetwork() const;
    void ProcessPresentationEvents();
    void SyncPresentation();
    // Height of the terrain surface at a world XY, or the flat sim ground level if
    // there is no landscape in this map. Visual-only: the simulation stays 2D.
    float SampleGroundHeight(double WorldX, double WorldY);

    // Cached rather than re-found every actor every frame; a level either has one
    // landscape or none; there is no reason to search for it 20 times a second.
    TWeakObjectPtr<class ALandscapeProxy> CachedLandscape;
    bool bLandscapeSearched = false;
    // FitGroundPlaneToMap runs once, on the first Tick rather than in Initialize --
    // see the call site for why.
    bool bGroundPlaneFitted = false;

    // Pointer to the deterministic C++ simulation core
    RA4::SimWorld* SimWorld;

    // Owned by the subsystem and outlives SimWorld, which holds a raw pointer to it
    // for the whole match.
    RA4::ContentDatabase* Content;

    // Null in a skirmish. Both are owned here and freed in Deinitialize; the runtime
    // holds copies of the mission's objectives rather than a pointer into the
    // database, so their lifetimes are independent of each other.
    RA4::CampaignDatabase* Campaign = nullptr;
    RA4::MissionRuntime* Mission = nullptr;

    // The mission's own id, kept for logging and for the debrief screen to name what
    // was just played.
    FString ActiveMissionId;

    // A mission ends once. Without this the won/lost line would be logged twenty
    // times a second for as long as the level stayed open.
    bool bMissionResultReported = false;

    // Presentation HUD Snapshot builder & latest snapshot
    RA4::Presentation::HudSnapshotBuilder* SnapshotBuilder = nullptr;
    RA4::Presentation::HudSnapshot* LatestSnapshot = nullptr;
    std::vector<RA4::EntityId> LocalSelection;
    bool bWasLocalPowerShortage = false;

    // How many newly spawned actors still dump their render state to the log.
    int32 DiagnosticDumpsRemaining = 6;

    // Drained into a CommandFrame once per simulation tick.
    std::vector<RA4::Command> PendingCommands;

    // One commander per active player the local human is not driving. The AI is the
    // same code the headless match dump plays with, ticked on the same schedule, so
    // a skirmish in the editor behaves like the one the tests cover.
    std::vector<RA4::AI::AICommander*> AICommanders;

    // Accumulator for fixed-step simulation ticking
    float TimeSinceLastSimTick;
    static constexpr float SimTickRate = 20.0f;
    static constexpr float SimTickDelta = 1.0f / SimTickRate;

    // Mapping from simulation EntityIndex to presentation AActor
    TMap<uint32, ARA4EntityActor*> EntityActors;

    // One-shot diagnostics so a missing presentation asset is visible in the log
    // instead of silently leaving a cube on the battlefield.
    TSet<uint32> MissingMeshContentIds;

    bool bReportedPresentationState = false;
};
