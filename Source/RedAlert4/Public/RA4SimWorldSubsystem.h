// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RA4EntityActor.h"
#include "RA4Presentation/RA4ArtMapping.h"

// Forward declare the C++ simulation classes without including their headers here
// to minimize compile-time dependencies. Command.h is engine-free and cheap, and is
// included because the pending-command queue needs the complete type.
#include "RA4Core/Command.h"
#include "RA4Recon/ReconConfig.h"

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

    // The local player's fog-of-war visibility as a texture (ADR-0030). The
    // landscape material and the fog post-process sample this; nothing reads it
    // to make a gameplay decision -- those still ask SimWorld, because the
    // texture is a picture of the answer, never the answer.
    UFUNCTION(BlueprintCallable, Category = "RA4|Fog")
    UTexture2D* GetFogVisibilityTexture() const { return FogVisibilityTexture; }

    // Tiles across the fog texture, so a material can convert a world position
    // into a UV without hardcoding the map size.
    // Accessibility controls from ADR-0030 section 4. Fog is a gameplay signal,
    // so its strength has a hard floor: it may be softened for readability but
    // never to the point where unexplored and visible ground look the same --
    // that would hand the player information the rules deny them. The clamp is
    // here, in code, rather than trusted to a config file.
    UFUNCTION(BlueprintCallable, Category = "RA4|Fog")
    void SetFogStrength(float Strength);
    UFUNCTION(BlueprintCallable, Category = "RA4|Fog")
    float GetFogStrength() const { return FogStrength; }
    // High-contrast fog: a hard boundary and a wider value separation between the
    // four states, for players who cannot resolve the gentle ramp (RISK-19).
    UFUNCTION(BlueprintCallable, Category = "RA4|Fog")
    void SetHighContrastFog(bool bEnabled);
    // --- Dynamic Day / Night Cycle ------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "RA4|DayNight")
    void SetTimeOfDay(float NormalizedTime); // 0.0=Midnight, 0.25=Dawn, 0.5=Noon, 0.75=Dusk

    UFUNCTION(BlueprintCallable, Category = "RA4|DayNight")
    float GetTimeOfDay() const { return CurrentTimeOfDay; }

    UFUNCTION(BlueprintCallable, Category = "RA4|DayNight")
    void SetDayNightCycleSpeed(float SpeedMultiplier);

    UFUNCTION(BlueprintCallable, Category = "RA4|DayNight")
    void SetDayNightCycleEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "RA4|DayNight")
    bool IsDayNightCycleEnabled() const { return bDayNightCycleEnabled; }

    UFUNCTION(BlueprintCallable, Category = "RA4|DayNight")
    bool IsNight() const;

    void UpdateDayNightCycle(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "RA4|Fog")
    void GetFogTextureDimensions(int32& OutWidth, int32& OutHeight) const
    {
        OutWidth = FogTextureWidth;
        OutHeight = FogTextureHeight;
    }

    // Returns the latest computed snapshot of the match for UI presentation
    const RA4::Presentation::HudSnapshot* GetLatestHudSnapshot() const { return LatestSnapshot; }

    // Initializes the simulation state for a skirmish match. Should be called by the
    // GameMode. NumAI and AISpot default to the classic 1v1 layout.
    // Skirmish option (owner decision: an open option, not a cheat code). Call
    // BEFORE StartSkirmishMatch; true loads Content/RA4/Data/Recon/recon_settings.json
    // with enabled=true so belief drives the HUD. ShowTruth additionally raises
    // the two-maps overlay for the whole match.
    void ConfigureRecon(bool bEnabled, bool bShowTruth);

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

    void SpawnVehicleWreckage(UWorld* World, const FVector& Location, const std::string& EntityName);

    void UpdateMysteryCrates(float DeltaTime);

    // Height of the terrain surface at a world XY, or the flat sim ground level if
    // there is no landscape in this map. Visual-only: the simulation stays 2D.
    float SampleGroundHeight(double WorldX, double WorldY);

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

    // Data-driven art mapping asset that maps simulation IDs to presentation assets.
    // Loaded at Initialize(); production meshes in this asset override blockout fallbacks.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
    TSoftObjectPtr<URA4ArtMappingDataAsset> ArtMappingAsset;

private:
    void LoadBlockoutMesh(uint32 ContentIdValue, const TCHAR* AssetPath);
    void RegisterDefaultBlockoutMeshes();

    // Resolves production meshes from ArtMappingAsset and overrides blockout entries
    // in ContentMeshRegistry. Called once during Initialize after blockout registration.
    void ApplyArtMapping();
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
    void BakeTerrainPassabilityFromLandscape();

    // --- Fog of war rendering (ADR-0030) ------------------------------------
    // Uploads the local player's visibility grid into FogVisibilityTexture and
    // publishes it to the landscape material. One-way: the simulation never
    // learns this texture exists, so a dropped upload changes what is drawn and
    // never what is simulated.
    void UpdateFogVisibilityTexture();
    // Pushes the texture and its dimensions into the landscape's material
    // instance. Separate from the upload because the material only needs
    // rebinding when the landscape or the texture is (re)created, while the
    // pixels change every frame.
    void PublishFogParametersToTerrain();
    // Installs the fog post-process material on the local camera and keeps its
    // parameters in step with the terrain's (ADR-0030 / V-7). Without this the
    // landscape is fogged but props, water and buildings stay lit over
    // unexplored ground, which points at exactly what the player must not know
    // is there.
    void PublishFogParametersToCamera();

    // UPROPERTY so the texture is not garbage-collected out from under the
    // material while the match is running.
    UPROPERTY(Transient)
    UTexture2D* FogVisibilityTexture = nullptr;
    UPROPERTY(Transient)
    UMaterialInstanceDynamic* TerrainFogMaterial = nullptr;
    UPROPERTY(Transient)
    UMaterialInstanceDynamic* CameraFogMaterial = nullptr;
    // 1.0 is the ADR's intended look; kMinFogStrength is the floor below which
    // fog stops doing its job. Enforced in SetFogStrength, not documented and hoped for.
    float FogStrength = 1.0f;
    bool bHighContrastFog = false;
    int32 FogTextureWidth = 0;
    int32 FogTextureHeight = 0;
    // Reused between frames: a per-frame allocation of the whole grid would
    // churn the heap 20 times a second for no reason.
    std::vector<uint8_t> FogTexelScratch;
    // Which tick the scratch buffer currently describes. The dirty-region upload
    // is only valid when this frame follows the tick the last upload saw: the
    // simulation may advance several ticks between two presentation updates, and
    // a dirty list covering only the newest of them would leave the tiles that
    // changed in the skipped ticks showing stale fog. Guarding on the tick number
    // keeps the fast path for the common one-tick case and falls back to the full
    // rebuild whenever the gap is anything else.
    uint32_t LastFogUploadTick = 0;
    bool bFogScratchValid = false;
    bool bFogContentsLogged = false;
    int32 FogUploadsSeen = 0;
    bool bFogMaterialBound = false;
    bool bCameraFogBound = false;

    // Cached rather than re-found every actor every frame; a level either has one
    // landscape or none; there is no reason to search for it 20 times a second.
    TWeakObjectPtr<class ALandscapeProxy> CachedLandscape;
    // Frame of the last landscape lookup, so a failed search is retried next frame
    // instead of latching for the rest of the match.
    uint64 LastLandscapeSearchFrame = 0;

    // Automatic screenshot timer, driven by ra4.AutoShot. See the comment on the
    // console variable: a screenshot taken at engine-init time photographs the
    // loading screen, not the game.
    float AutoShotElapsed = 0.0f;
    bool bAutoShotTaken = false;
    // FitGroundPlaneToMap runs once, on the first Tick rather than in Initialize --
    // see the call site for why.
    bool bGroundPlaneFitted = false;

    // --- Dynamic Astronomical Day / Night Cycle State ---
    float CurrentTimeOfDay = 0.40f; // Mid-morning default (10:00)
    float DayNightDurationSeconds = 480.0f; // 8 minutes full cycle
    float DayNightSpeedMultiplier = 1.0f;
    bool bDayNightCycleEnabled = true;

    TWeakObjectPtr<class ADirectionalLight> CachedSunLight;
    TWeakObjectPtr<class ASkyLight> CachedSkyLight;
    TWeakObjectPtr<class APostProcessVolume> CachedPostProcess;
    TWeakObjectPtr<class AExponentialHeightFog> CachedHeightFog;

    // Pointer to the deterministic C++ simulation core
    RA4::SimWorld* SimWorld;

    // Owned by the subsystem and outlives SimWorld, which holds a raw pointer to it
    // for the whole match.
    RA4::ContentDatabase* Content;
    // Owned here because SimWorld only borrows the settings pointer (Restart()
    // re-reads it), so its lifetime must span the whole match.
    RA4::Recon::ReconSettings ReconSettings;
    bool bReconEnabled = false;
    bool bReconShowTruth = false;

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

    TArray<FVector> ActiveMysteryCratePositions;
    float TimeSinceLastCrateSpawn = 10.0f;

    struct FRA4ExplosiveBarrel
    {
        FVector Location;
        float Health = 100.0f;
        bool bAlive = true;
    };

    struct FRA4RadiationZone
    {
        FVector Location;
        float Radius = 350.0f;
        float RemainingSeconds = 25.0f;
    };

    struct FRA4FlyingTurret
    {
        FVector Location;
        FVector Velocity;
        float Yaw = 0.0f;
        float Pitch = 0.0f;
        float RemainingSeconds = 2.5f;
    };

    TArray<FRA4ExplosiveBarrel> ActiveExplosiveBarrels;
    TArray<FRA4RadiationZone> ActiveRadiationZones;
    TArray<FRA4FlyingTurret> ActiveFlyingTurrets;
    bool bBarrelsInitialized = false;

    void UpdateExplosiveBarrels(float DeltaTime);
    void UpdateRadiationZones(float DeltaTime);
    void UpdateFlyingTurrets(float DeltaTime);

public:
    void SpawnRadiationZone(const FVector& Location, float Radius = 350.0f, float Duration = 25.0f);
    void SpawnFlyingTurret(const FVector& Location);
};
