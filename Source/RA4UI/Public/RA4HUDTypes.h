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

    /**
     * ADR-0012: production is paid a slice per tick, so an item can stall because the
     * treasury is empty rather than because the player paused it. Without this the
     * two are indistinguishable on screen -- a frozen progress bar either way.
     */
    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    bool bStarvedForCredits = false;

    /** Credits paid into this item so far, for the funding bar. */
    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    int32 PaidCredits = 0;

    /** Full price of this item, fixed when it was queued. */
    UPROPERTY(BlueprintReadOnly, Category = "RA4|Production")
    int32 TotalCost = 0;

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
