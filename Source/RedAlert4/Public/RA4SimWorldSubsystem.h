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

    // Read-only view of the authoritative state, for input picking and the HUD.
    // Nothing outside the simulation may mutate it directly.
    const RA4::SimWorld* GetSimWorld() const { return SimWorld; }

    // Returns the latest computed snapshot of the match for UI presentation
    const RA4::Presentation::HudSnapshot* GetLatestHudSnapshot() const { return LatestSnapshot; }

    // Initializes the simulation state for a skirmish match. Should be called by the GameMode.
    void StartSkirmishMatch(uint8 PlayerFaction, uint8 EnemyFaction, int32 Difficulty);

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

    // Presentation HUD Snapshot builder & latest snapshot
    RA4::Presentation::HudSnapshotBuilder* SnapshotBuilder = nullptr;
    RA4::Presentation::HudSnapshot* LatestSnapshot = nullptr;
    std::vector<RA4::EntityId> LocalSelection;

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

    bool bReportedPresentationState = false;
};
