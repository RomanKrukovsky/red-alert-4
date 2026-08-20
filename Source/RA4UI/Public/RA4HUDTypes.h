// Copyright (c) Red Alert 4 project. Blueprint-facing mirrors of the HUD snapshot.
//
// RA4Presentation produces plain C++ structs once per simulation tick. UMG cannot
// bind to those, so this header restates the parts the HUD needs as USTRUCTs. The
// duplication is deliberate and one-directional: the simulation side stays free of
// UObject, and nothing here ever travels back into the simulation.
#pragma once

#include "CoreMinimal.h"

#include "RA4HUDTypes.generated.h"

UENUM(BlueprintType)
enum class ERA4SelectionKind : uint8
{
    Empty,
    SingleUnit,
    SingleBuilding,
    MultipleUnits,
    Mixed,
};

UENUM(BlueprintType)
enum class ERA4BuildBlockReason : uint8
{
    None,
    MissingPrerequisite,
    InsufficientCredits,
    NoProducer,
    QueueFull,
    MatchOver,
};

UENUM(BlueprintType)
enum class ERA4AlertSeverity : uint8
{
    Info,
    Warning,
    Critical,
};

UENUM(BlueprintType)
enum class ERA4MatchPhase : uint8
{
    NotStarted,
    Running,
    Finished,
};

UENUM(BlueprintType)
enum class ERA4RadarMarkerKind : uint8
{
    Unit,
    Building,
    Resource,
};

USTRUCT(BlueprintType)
struct FRA4RadarMarker
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Radar")
    FVector2D WorldPosition = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Radar")
    int32 Owner = -1;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Radar")
    ERA4RadarMarkerKind Kind = ERA4RadarMarkerKind::Unit;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Radar")
    bool bSelected = false;
};

/** One row of the multi-selection grid: same-type units collapsed into a count. */
USTRUCT(BlueprintType)
struct FRA4SelectionGroup
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Selection")
    int64 ContentId = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Selection")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Selection")
    int32 Count = 0;

    /** 0..1 over the whole group, so a damaged squad reads as damaged. */
    UPROPERTY(BlueprintReadOnly, Category = "RA4|Selection")
    float HealthRatio = 1.0f;
};

/** An item being produced, for the queue strip. */
USTRUCT(BlueprintType)
struct FRA4ProductionEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    int64 ContentId = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    int32 ProgressPercent = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    float RemainingSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    bool bPaused = false;

    /** Finished structure waiting for the player to choose a spot. */
    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    bool bAwaitingPlacement = false;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    int32 SlotIndex = 0;
};

/** A card on the build sidebar, including why it is greyed out. */
USTRUCT(BlueprintType)
struct FRA4BuildOption
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    int64 ContentId = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    int32 Cost = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    float BuildSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    int32 PowerDelta = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    FText PrerequisiteText;

    /** Matches ProductionCategory: 0 structure, 1 defence, 2 infantry, 3 vehicle... */
    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    int32 Category = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    bool bAvailable = false;

    /** The card shows this instead of guessing why the player cannot click it. */
    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    ERA4BuildBlockReason BlockReason = ERA4BuildBlockReason::NoProducer;
};

/** One line of the notification feed. Repeats are merged, not stacked. */
USTRUCT(BlueprintType)
struct FRA4Alert
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Alerts")
    FText Message;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Alerts")
    ERA4AlertSeverity Severity = ERA4AlertSeverity::Info;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Alerts")
    int32 RepeatCount = 1;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Alerts")
    FVector2D WorldLocation = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Alerts")
    bool bHasLocation = false;
};

USTRUCT(BlueprintType)
struct FRA4HUDObjective
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Objectives")
    FText Label;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Objectives")
    bool bCompleted = false;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Objectives")
    bool bOptional = false;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Objectives")
    int32 Current = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|Objectives")
    int32 Target = 0;
};

UENUM(BlueprintType, meta = (Bitflags))
enum class ERA4HUDChangeFlags : uint8
{
    None = 0,
    Resources = 1 << 0,
    Selection = 1 << 1,
    Production = 1 << 2,
    Objectives = 1 << 3,
    Alerts = 1 << 4,
};
ENUM_CLASS_FLAGS(ERA4HUDChangeFlags)

/** Immutable, Blueprint-safe HUD projection consumed by the view model. */
USTRUCT(BlueprintType)
struct FRA4HUDSnapshotView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    int32 Credits = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    int32 CreditsDelta = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    int32 PowerProduced = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    int32 PowerConsumed = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    bool bPowerShortage = false;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    int32 SupplyUsed = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    int32 SupplyCap = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    int32 MatchElapsedSeconds = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    ERA4SelectionKind SelectionKind = ERA4SelectionKind::Empty;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    int32 SelectionCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    float SelectionHealthRatio = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    FString PrimaryEntityName;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    bool bPrimaryOwned = false;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    int32 HarvesterCargo = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    int32 HarvesterCapacity = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    TArray<FRA4SelectionGroup> SelectionGroups;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    TArray<FRA4ProductionEntry> ProductionQueue;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    TArray<FRA4BuildOption> BuildOptions;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    TArray<FRA4HUDObjective> Objectives;

    UPROPERTY(BlueprintReadOnly, Category = "RA4|HUD")
    TArray<FRA4Alert> Alerts;
};
