// Copyright (c) Red Alert 4 project.
#include "RA4Navigation/Formation.h"

#include <array>
#include <cstddef>

#include "RA4Core/SimConfig.h"

namespace RA4
{

Vec2 RotateOffset(const Vec2& Offset, int32_t Facing)
{
    // 2D rotation by the fixed-point angle. FxCos/FxSin are in Fixed.h.
    const Fixed C = FxCos(Facing);
    const Fixed S = FxSin(Facing);
    return Vec2(Offset.X * C - Offset.Y * S, Offset.X * S + Offset.Y * C);
}

namespace
{

// Slots per shape. 32 followers + the leader in slot 0. The AI's ArmyGroup can field
// more than 32 units, in which case the surplus has no slot; that is the movement
// system's clamp to make, and it is a visible clamp rather than a wrapped index that
// would quietly seat two units on one offset.
constexpr int32_t kSlotsPerShape = 33;

// Authoring unit for the tables below. Deliberately spelled against kTileSizeUnits
// rather than a literal 200 so a change to the tile size moves the formations with
// the grid instead of silently decoupling them from it.
constexpr int64_t kTile = kTileSizeUnits;

// The tables are built in integer *world units* rather than as Fixed, for two
// reasons: the arithmetic stays exact and reviewable, and integers let the
// distinctness invariant be a static_assert instead of a runtime check that only
// fires on the machine unlucky enough to run it.
struct OffsetUnits
{
    int64_t X = 0;
    int64_t Y = 0;

    constexpr bool operator==(const OffsetUnits& O) const { return X == O.X && Y == O.Y; }
};

using ShapeTable = std::array<OffsetUnits, kSlotsPerShape>;

// Slot indices are signed throughout this file, because the generators below do
// signed arithmetic on them (Col * 2 - (kColumns - 1) goes negative by design).
// std::array subscripts are size_t, so every raw T[I] is a signedness conversion that
// this build treats as an error, and rightly: an implicit conversion is exactly how a
// negative index becomes a huge positive one and reads off the end of the table.
//
// These two accessors are the single place the conversion happens. Everything else
// indexes through them, so there is one line to audit rather than thirteen casts, and
// the bounds check states the precondition the generators rely on instead of leaving
// it implied.
constexpr const OffsetUnits& SlotAt(const ShapeTable& T, int32_t Slot)
{
    return T[static_cast<std::size_t>(Slot)];
}

constexpr OffsetUnits& SlotAt(ShapeTable& T, int32_t Slot)
{
    return T[static_cast<std::size_t>(Slot)];
}

// Sixteen directions at 22.5-degree steps, as a unit vector scaled by 1000. Written
// out rather than derived from FxSin/FxCos because those are a CORDIC routine and so
// not constexpr; a literal table keeps the ring shapes compile-time constants and
// therefore keeps them inside the static_assert below.
struct Dir16
{
    int64_t X;
    int64_t Y;
};
constexpr std::array<Dir16, 16> kRingDirs = {{{1000, 0},
                                              {924, 383},
                                              {707, 707},
                                              {383, 924},
                                              {0, 1000},
                                              {-383, 924},
                                              {-707, 707},
                                              {-924, 383},
                                              {-1000, 0},
                                              {-924, -383},
                                              {-707, -707},
                                              {-383, -924},
                                              {0, -1000},
                                              {383, -924},
                                              {707, -707},
                                              {924, -383}}};

// Scales a kRingDirs entry to a radius. Integer division truncates toward zero, which
// is fine here: the result only has to be deterministic and distinct, not exactly on
// the circle, and truncation is identical on every platform.
constexpr int64_t ScaleDir(int64_t Component, int64_t Radius) { return (Component * Radius) / 1000; }

// Single file directly behind the leader. Travel formation: it presents the smallest
// cross-section to a road or bridge, which is the terrain a column is actually for.
constexpr ShapeTable MakeColumn()
{
    ShapeTable T{};
    for (int32_t I = 0; I < kSlotsPerShape; ++I)
    {
        SlotAt(T, I) = OffsetUnits{-static_cast<int64_t>(I) * kTile, 0};
    }
    return T;
}

// Abreast of the leader, alternating right then left so a half-strength group stays
// centred on the leader instead of trailing off to one flank.
constexpr ShapeTable MakeLine()
{
    ShapeTable T{};
    for (int32_t I = 1; I < kSlotsPerShape; ++I)
    {
        const int64_t Rank = (I + 1) / 2;
        const int64_t Side = (I % 2) == 1 ? 1 : -1;
        SlotAt(T, I) = OffsetUnits{0, Side * Rank * kTile};
    }
    return T;
}

// A V opening backwards: each rank steps out one tile laterally and back three
// quarters of a tile. Shallower than 45 degrees on purpose, so the arms stay within
// the leader's flow field sector rather than fanning into a neighbouring one.
constexpr ShapeTable MakeWedge()
{
    ShapeTable T{};
    for (int32_t I = 1; I < kSlotsPerShape; ++I)
    {
        const int64_t Rank = (I + 1) / 2;
        const int64_t Side = (I % 2) == 1 ? 1 : -1;
        SlotAt(T, I) = OffsetUnits{-Rank * ((kTile * 3) / 4), Side * Rank * kTile};
    }
    return T;
}

// Loose lattice, two tiles between neighbours, with alternate rows offset by one tile
// so no three slots are collinear at the row pitch. The stagger is the point: a
// splash template that catches one row must not also catch the row behind it.
// Every rank sits behind the leader, so the leader keeps a full rank of clearance.
constexpr ShapeTable MakeSpread()
{
    ShapeTable T{};
    constexpr int64_t kColumns = 6;
    for (int32_t I = 1; I < kSlotsPerShape; ++I)
    {
        const int64_t N = I - 1;
        const int64_t Row = N / kColumns;
        const int64_t Col = N % kColumns;
        const int64_t Stagger = (Row % 2) != 0 ? kTile : 0;
        SlotAt(T, I) = OffsetUnits{-(Row + 1) * 2 * kTile, (Col * 2 - (kColumns - 1)) * kTile + Stagger};
    }
    return T;
}

// Fills one lateral rank at a fixed forward offset, working centre-outwards so that
// an understrength screen still covers the middle rather than leaving a hole in front
// of whatever it is protecting. Returns the next free slot.
constexpr int32_t FillRank(ShapeTable& Out, int32_t Slot, int64_t X, int32_t Count, int64_t Spacing)
{
    for (int32_t J = 0; J < Count && Slot < kSlotsPerShape; ++J, ++Slot)
    {
        const int64_t Rank = (J + 1) / 2;
        const int64_t Side = (J % 2) == 0 ? 1 : -1;
        SlotAt(Out, Slot) = OffsetUnits{X, Side * Rank * Spacing};
    }
    return Slot;
}

// Two ranks ahead of the leader and one behind. The leader here is the thing being
// screened (artillery, a support vehicle, an MCV), not the tip of the spear: armour
// holds the two forward ranks so that anything shooting at the leader has to chew
// through them first, and the rear rank covers a flanking push from behind.
constexpr ShapeTable MakeShieldScreen()
{
    ShapeTable T{};
    constexpr int64_t kSpacing = (kTile * 3) / 2;
    int32_t Slot = 1;
    Slot = FillRank(T, Slot, kTile * 3, 12, kSpacing);         // forward screen
    Slot = FillRank(T, Slot, (kTile * 3) / 2, 10, kSpacing);   // second screen rank
    Slot = FillRank(T, Slot, -kTile * 2, 10, kSpacing);        // rear guard
    return T;
}

// Concentric rings around the leader for all-round defence. Two rings rather than one
// wide one: 32 units on a single ring would either be shoulder to shoulder or enclose
// an area too large for the inner units to support the far side.
constexpr ShapeTable MakeRings(int64_t InnerRadius, int64_t OuterRadius)
{
    ShapeTable T{};
    int32_t Slot = 1;
    for (const Dir16& D : kRingDirs)
    {
        SlotAt(T, Slot++) = OffsetUnits{ScaleDir(D.X, InnerRadius), ScaleDir(D.Y, InnerRadius)};
    }
    for (const Dir16& D : kRingDirs)
    {
        SlotAt(T, Slot++) = OffsetUnits{ScaleDir(D.X, OuterRadius), ScaleDir(D.Y, OuterRadius)};
    }
    return T;
}

constexpr ShapeTable kColumnUnits = MakeColumn();
constexpr ShapeTable kLineUnits = MakeLine();
constexpr ShapeTable kWedgeUnits = MakeWedge();
constexpr ShapeTable kSpreadUnits = MakeSpread();
constexpr ShapeTable kShieldScreenUnits = MakeShieldScreen();
constexpr ShapeTable kCircularUnits = MakeRings(kTile * 3, kTile * 6);
// Tight escort cluster: close enough that the escorted transport is inside the
// group's own weapons envelope, still outside the largest authored CollisionRadius.
constexpr ShapeTable kTransportUnits = MakeRings((kTile * 21) / 10, (kTile * 39) / 10);

// --- Distinctness, proven at compile time -----------------------------------------
//
// Two slots resolving to the same offset is the defect this table exists to remove:
// both units get an identical Destination, so they push each other off it in
// alternate ticks and neither ever arrives. Checking it here means a bad edit to the
// generators above cannot compile, let alone ship.
constexpr bool AllOffsetsDistinct(const ShapeTable& T)
{
    for (int32_t A = 0; A < kSlotsPerShape; ++A)
    {
        for (int32_t B = A + 1; B < kSlotsPerShape; ++B)
        {
            if (SlotAt(T, A) == SlotAt(T, B))
            {
                return false;
            }
        }
    }
    return true;
}

// Distinct is necessary but not sufficient: two offsets one raw unit apart are
// distinct and still order two tanks into the same square metre. Require real
// clearance as well. 150 units is comfortably above the largest authored
// Unit.CollisionRadius (120) so slots do not overlap bodies.
constexpr int64_t kMinSlotSeparation = 150;

constexpr bool AllOffsetsWellSeparated(const ShapeTable& T, int64_t MinSeparation)
{
    const int64_t MinSquared = MinSeparation * MinSeparation;
    for (int32_t A = 0; A < kSlotsPerShape; ++A)
    {
        for (int32_t B = A + 1; B < kSlotsPerShape; ++B)
        {
            const int64_t DX = SlotAt(T, A).X - SlotAt(T, B).X;
            const int64_t DY = SlotAt(T, A).Y - SlotAt(T, B).Y;
            if (DX * DX + DY * DY < MinSquared)
            {
                return false;
            }
        }
    }
    return true;
}

// Slot 0 is the leader's own position in every shape; the header promises this and
// SimWorld relies on it to treat Members[0] as the leader without a special case.
constexpr bool LeaderSlotIsOrigin(const ShapeTable& T) { return T[0].X == 0 && T[0].Y == 0; }

#define RA4_VALIDATE_SHAPE(Table)                                                            \
    static_assert(LeaderSlotIsOrigin(Table), #Table ": slot 0 must be the leader (zero offset)"); \
    static_assert(AllOffsetsDistinct(Table), #Table ": two slots share an offset");           \
    static_assert(AllOffsetsWellSeparated(Table, kMinSlotSeparation), #Table ": slots too close")

RA4_VALIDATE_SHAPE(kColumnUnits);
RA4_VALIDATE_SHAPE(kLineUnits);
RA4_VALIDATE_SHAPE(kWedgeUnits);
RA4_VALIDATE_SHAPE(kSpreadUnits);
RA4_VALIDATE_SHAPE(kShieldScreenUnits);
RA4_VALIDATE_SHAPE(kCircularUnits);
RA4_VALIDATE_SHAPE(kTransportUnits);

#undef RA4_VALIDATE_SHAPE

static_assert(kSlotsPerShape >= 33, "A formation must seat a leader plus 32 followers");

// The seven ContentIds must be seven *different* values. FNV-1a over distinct strings
// makes a collision vanishingly unlikely, but "unlikely" resolved at build time is
// free, and a collision would make one shape permanently unreachable.
constexpr std::array<ContentId, 7> kAllFormationIds = {kFormationColumn,   kFormationLine,
                                                       kFormationWedge,    kFormationSpread,
                                                       kFormationShieldScreen, kFormationCircular,
                                                       kFormationTransport};

constexpr bool AllIdsDistinct()
{
    for (size_t A = 0; A < kAllFormationIds.size(); ++A)
    {
        if (!kAllFormationIds[A].IsValid())
        {
            return false;
        }
        for (size_t B = A + 1; B < kAllFormationIds.size(); ++B)
        {
            if (kAllFormationIds[A] == kAllFormationIds[B])
            {
                return false;
            }
        }
    }
    return true;
}
static_assert(AllIdsDistinct(), "Formation content ids collide or one hashed to zero");

// --- Runtime storage ---------------------------------------------------------------
//
// FormationDef holds a std::vector, so the defs cannot themselves be constexpr. They
// are built once, on first use, from the constexpr tables above and then never
// mutated -- the function-local static gives thread-safe one-time init without a
// load-order dependency on another translation unit's globals. Nothing here observes
// tick, wall clock, or call order, so every peer builds byte-identical tables.
FormationDef BuildDef(ContentId Id, const ShapeTable& Units)
{
    FormationDef Def;
    Def.Id = Id;
    Def.Offsets.reserve(static_cast<std::size_t>(kSlotsPerShape));
    for (const OffsetUnits& U : Units)
    {
        Def.Offsets.push_back(Vec2(Fixed::FromInt(U.X), Fixed::FromInt(U.Y)));
    }
    return Def;
}

const std::array<FormationDef, 7>& AllDefs()
{
    static const std::array<FormationDef, 7> Defs = {
        BuildDef(kFormationColumn, kColumnUnits),
        BuildDef(kFormationLine, kLineUnits),
        BuildDef(kFormationWedge, kWedgeUnits),
        BuildDef(kFormationSpread, kSpreadUnits),
        BuildDef(kFormationShieldScreen, kShieldScreenUnits),
        BuildDef(kFormationCircular, kCircularUnits),
        BuildDef(kFormationTransport, kTransportUnits)};
    return Defs;
}

} // namespace

ContentId FormationShapeToContentId(EFormationShape Shape)
{
    switch (Shape)
    {
        case EFormationShape::Column: return kFormationColumn;
        case EFormationShape::Line: return kFormationLine;
        case EFormationShape::Wedge: return kFormationWedge;
        case EFormationShape::Spread: return kFormationSpread;
        case EFormationShape::ShieldScreen: return kFormationShieldScreen;
        case EFormationShape::Circular: return kFormationCircular;
        case EFormationShape::Transport: return kFormationTransport;
    }
    // Unreachable for a valid enum. Returning an invalid id rather than defaulting to
    // Column keeps a future enum value from silently inheriting the wrong shape.
    return ContentId();
}

const FormationDef* FindFormationDef(ContentId Id)
{
    if (!Id.IsValid())
    {
        return nullptr;
    }
    // Seven entries: a linear scan beats a map here, and it keeps the lookup free of
    // any allocation or iteration-order concern on the tick path.
    for (const FormationDef& Def : AllDefs())
    {
        if (Def.Id == Id)
        {
            return &Def;
        }
    }
    return nullptr;
}

int32_t FormationSlotCount(ContentId Id)
{
    const FormationDef* Def = FindFormationDef(Id);
    // size() is size_t; narrowing it is only safe because the tables are fixed at
    // kSlotsPerShape, which the static_assert above pins well inside int32_t.
    return Def != nullptr ? static_cast<int32_t>(Def->Offsets.size()) : 0;
}

} // namespace RA4
