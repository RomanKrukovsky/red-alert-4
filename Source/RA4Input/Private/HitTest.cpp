// Copyright (c) Red Alert 4 project.
#include "RA4Input/HitTest.h"

#include <algorithm>

namespace RA4
{
namespace Input
{

namespace
{
// Sign of the 2D cross product of (B - A) and (P - A). Positive means P is to the
// left of the directed edge A->B.
int32_t EdgeSide(const Vec2& A, const Vec2& B, const Vec2& P)
{
    const Vec2 Edge = B - A;
    const Vec2 ToPoint = P - A;
    const Fixed Cross = Edge.X * ToPoint.Y - Edge.Y * ToPoint.X;
    if (Cross.Raw > 0) { return 1; }
    if (Cross.Raw < 0) { return -1; }
    return 0;
}
} // namespace

bool IsPointInConvexQuad(const Vec2 Quad[4], const Vec2& Point)
{
    // The point is inside when it is on the same side of all four edges. Collinear
    // results (side 0) are ignored rather than rejected, which keeps a point sitting
    // exactly on an edge -- or a fully degenerate zero-area quad -- from behaving
    // erratically.
    int32_t Sign = 0;
    for (int32_t I = 0; I < 4; ++I)
    {
        const int32_t Side = EdgeSide(Quad[I], Quad[(I + 1) % 4], Point);
        if (Side == 0)
        {
            continue;
        }
        if (Sign == 0)
        {
            Sign = Side;
        }
        else if (Side != Sign)
        {
            return false;
        }
    }
    return true;
}

std::vector<EntityId> PickAtPoint(const std::vector<PickCandidate>& Candidates, const Vec2& Point,
                                  Fixed ExtraTolerance)
{
    struct Hit
    {
        EntityId Id;
        Fixed DistanceSquared;
    };

    std::vector<Hit> Hits;
    Hits.reserve(Candidates.size());

    for (const PickCandidate& C : Candidates)
    {
        const Fixed Reach = C.Radius + ExtraTolerance;
        if (Reach <= Fixed::Zero())
        {
            continue;
        }
        const Fixed DistSq = DistanceSquared(C.Position, Point);
        if (DistSq <= Reach * Reach)
        {
            Hits.push_back(Hit{C.Id, DistSq});
        }
    }

    // Nearest first; ties broken by slot so overlapping entities always resolve the
    // same way for the same click.
    std::stable_sort(Hits.begin(), Hits.end(),
                     [](const Hit& A, const Hit& B)
                     {
                         if (A.DistanceSquared != B.DistanceSquared)
                         {
                             return A.DistanceSquared < B.DistanceSquared;
                         }
                         return A.Id.Index < B.Id.Index;
                     });

    std::vector<EntityId> Result;
    Result.reserve(Hits.size());
    for (const Hit& H : Hits)
    {
        Result.push_back(H.Id);
    }
    return Result;
}

std::vector<EntityId> PickInQuad(const std::vector<PickCandidate>& Candidates, const Vec2 Quad[4])
{
    std::vector<EntityId> Result;
    for (const PickCandidate& C : Candidates)
    {
        if (IsPointInConvexQuad(Quad, C.Position))
        {
            Result.push_back(C.Id);
        }
    }
    std::sort(Result.begin(), Result.end(),
              [](const EntityId& A, const EntityId& B) { return A.Index < B.Index; });
    return Result;
}

bool IsDragSignificant(const Vec2& Start, const Vec2& End, Fixed MinimumExtent)
{
    const Vec2 Delta = End - Start;
    // Extent rather than length: a long thin drag along one axis is still a marquee,
    // and comparing against the larger axis avoids a square root.
    return FxMax(FxAbs(Delta.X), FxAbs(Delta.Y)) >= MinimumExtent;
}

} // namespace Input
} // namespace RA4
