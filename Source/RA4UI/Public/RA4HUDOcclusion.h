// Copyright (c) Scarlet Horizon project.
// Shared occlusion contract for every battle HUD. See ADR-0013.

#pragma once

#include "CoreMinimal.h"
#include "Math/Box2D.h"

/**
 * The rectangles a HUD draws over the battlefield, and what they cost.
 *
 * Both HUDs answer the same two questions: does a screen point land on a panel,
 * and how much of the fight is still visible. Before this existed the playable
 * HUD reserved one full-height strip while the reference viewer tracked
 * rectangles, so the two could not be measured against the same budget.
 */
class RA4UI_API FRA4HUDOcclusion
{
public:
    /** Canvas the HUD panels are authored against. */
    static constexpr float ReferenceCanvasWidth = 1920.0f;
    static constexpr float ReferenceCanvasHeight = 1080.0f;

    /** The design budget: below this the HUD swallows the fight, above it the panels are too thin to read. */
    static constexpr float MinBattlefieldShare = 0.65f;
    static constexpr float MaxBattlefieldShare = 0.72f;

    void Reset() { Regions.Reset(); }

    void Add(const FVector2D Position, const FVector2D Size)
    {
        if (Size.X > 0.0f && Size.Y > 0.0f)
        {
            Regions.Emplace(Position, Position + Size);
        }
    }

    int32 Num() const { return Regions.Num(); }

    bool IsBlocked(const FVector2D Point) const
    {
        for (const FBox2D& Region : Regions)
        {
            if (Region.IsInside(Point))
            {
                return true;
            }
        }
        return false;
    }

    /**
     * Share of the canvas the player can still see the battle through, with
     * overlapping panels counted once. Panels overlap, so summing their areas
     * would over-count; sampling a coarse grid gives the union directly and stays
     * exact enough for a layout budget.
     */
    float BattlefieldShare() const
    {
        if (Regions.Num() == 0)
        {
            return 1.0f;
        }

        constexpr int32 SampleColumns = 192;
        constexpr int32 SampleRows = 108;
        const FVector2D CellSize(
            ReferenceCanvasWidth / float(SampleColumns),
            ReferenceCanvasHeight / float(SampleRows));

        int32 FreeCells = 0;
        for (int32 Row = 0; Row < SampleRows; ++Row)
        {
            for (int32 Column = 0; Column < SampleColumns; ++Column)
            {
                const FVector2D Centre(
                    (float(Column) + 0.5f) * CellSize.X,
                    (float(Row) + 0.5f) * CellSize.Y);
                if (!IsBlocked(Centre))
                {
                    ++FreeCells;
                }
            }
        }
        return float(FreeCells) / float(SampleColumns * SampleRows);
    }

    bool IsWithinBudget() const
    {
        const float Share = BattlefieldShare();
        return Share >= MinBattlefieldShare && Share <= MaxBattlefieldShare;
    }

private:
    TArray<FBox2D> Regions;
};
