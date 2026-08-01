// Copyright (c) Red Alert 4 project. Tests for the deterministic primitives.
#include "TestFramework.h"

#include "RA4Core/ByteStream.h"
#include "RA4Core/Checksum.h"
#include "RA4Core/Command.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Random.h"
#include "RA4Core/Vector.h"

#include <cmath>
#include <set>

using namespace RA4;

RA4_TEST(FixedMath, BasicArithmetic)
{
    const Fixed A = Fixed::FromInt(7);
    const Fixed B = Fixed::FromRatio(1, 2);

    RA4_EXPECT((A * B).Raw == Fixed::FromRatio(7, 2).Raw);
    RA4_EXPECT((A + B).ToIntFloor() == 7);
    RA4_EXPECT((A / Fixed::FromInt(2)).Raw == Fixed::FromRatio(7, 2).Raw);
    RA4_EXPECT((-A).Raw == -A.Raw);
    RA4_EXPECT(FxAbs(-A) == A);

    // Floor must truncate toward negative infinity so tile indexing is continuous
    // across the origin.
    RA4_EXPECT(Fixed::FromRatio(-3, 2).ToIntFloor() == -2);
    RA4_EXPECT(Fixed::FromRatio(3, 2).ToIntFloor() == 1);
}

RA4_TEST(FixedMath, SqrtAccuracy)
{
    // Exact squares must come back exactly; anything else compounds into position
    // drift over thousands of ticks.
    for (int64_t I = 0; I <= 200; ++I)
    {
        const Fixed Root = FxSqrt(Fixed::FromInt(I * I));
        RA4_EXPECT_EQ(Root.ToIntRound(), I);
    }

    RA4_EXPECT(FxSqrt(Fixed::Zero()).Raw == 0);
    RA4_EXPECT(FxSqrt(Fixed::FromInt(-5)).Raw == 0);

    // Large values (a full map diagonal squared) must not overflow.
    const Fixed Big = Fixed::FromInt(100000);
    const Fixed BigRoot = FxSqrt(Big * Big);
    RA4_EXPECT_NEAR(BigRoot.ToIntRound(), int64_t(100000), int64_t(2));
}

RA4_TEST(FixedMath, TrigonometryMatchesReference)
{
    // Compare against libm here only: this is the one place a float is allowed,
    // because the test is asserting that the integer implementation is correct.
    for (int32_t Angle = 0; Angle < kAngleTurn; Angle += 7)
    {
        const double Radians = (double(Angle) / double(kAngleTurn)) * 2.0 * 3.14159265358979323846;
        const double ExpectedSin = std::sin(Radians);
        const double ExpectedCos = std::cos(Radians);

        const double ActualSin = FxSin(Angle).ToDoubleUnsafe();
        const double ActualCos = FxCos(Angle).ToDoubleUnsafe();

        RA4_EXPECT(std::fabs(ActualSin - ExpectedSin) < 0.001);
        RA4_EXPECT(std::fabs(ActualCos - ExpectedCos) < 0.001);
    }

    RA4_EXPECT_EQ(FxSin(0).Raw, int64_t(0));
    RA4_EXPECT_NEAR(FxCos(0).Raw, kFixedOne, int64_t(40));
}

RA4_TEST(FixedMath, Atan2RoundTrip)
{
    for (int32_t Angle = 0; Angle < kAngleTurn; Angle += 13)
    {
        const Vec2 V = Vec2::FromAngle(Angle) * Fixed::FromInt(1000);
        const int32_t Recovered = V.ToAngle();
        const int32_t Diff = AngleDelta(Angle, Recovered);
        // Two units out of 4096 is 0.18 degrees; well inside the turret tolerance.
        RA4_EXPECT(Diff >= -2 && Diff <= 2);
    }

    RA4_EXPECT_EQ(FxAtan2(Fixed::Zero(), Fixed::Zero()), 0);
    RA4_EXPECT_EQ(FxAtan2(Fixed::Zero(), Fixed::FromInt(10)), 0);
    RA4_EXPECT_NEAR(FxAtan2(Fixed::FromInt(10), Fixed::Zero()), kAngleTurn / 4, 2);
}

RA4_TEST(FixedMath, AngleWrapping)
{
    RA4_EXPECT_EQ(WrapAngle(kAngleTurn), 0);
    RA4_EXPECT_EQ(WrapAngle(-1), kAngleTurn - 1);
    RA4_EXPECT_EQ(AngleDelta(0, kAngleTurn - 1), -1);
    RA4_EXPECT_EQ(AngleDelta(kAngleTurn - 1, 0), 1);
    RA4_EXPECT_EQ(AngleDelta(0, kAngleTurn / 2), kAngleTurn / 2);
}

RA4_TEST(Vector, LengthAndNormalize)
{
    const Vec2 V(Fixed::FromInt(3000), Fixed::FromInt(4000));
    RA4_EXPECT_NEAR(V.Length().ToIntRound(), int64_t(5000), int64_t(2));

    const Vec2 N = V.Normalized();
    RA4_EXPECT_NEAR(N.Length().Raw, kFixedOne, int64_t(200));

    RA4_EXPECT(Vec2::Zero().Normalized() == Vec2::Zero());
}

RA4_TEST(Random, ReproducibleFromSeed)
{
    Random A(999);
    Random B(999);
    for (int32_t I = 0; I < 1000; ++I)
    {
        RA4_EXPECT_EQ(A.NextUInt32(), B.NextUInt32());
    }

    Random C(1000);
    Random D(999);
    // Different seeds must actually diverge; a broken seeding step is invisible
    // until every match plays out identically.
    bool bDiffers = false;
    for (int32_t I = 0; I < 32 && !bDiffers; ++I)
    {
        bDiffers = C.NextUInt32() != D.NextUInt32();
    }
    RA4_EXPECT(bDiffers);
}

RA4_TEST(Random, BoundedDrawsStayInRange)
{
    Random R(42);
    std::set<uint32_t> Seen;
    for (int32_t I = 0; I < 10000; ++I)
    {
        const uint32_t V = R.NextBelow(6);
        RA4_EXPECT(V < 6);
        Seen.insert(V);
    }
    // With 10k draws every bucket must appear, or the rejection loop is broken.
    RA4_EXPECT_EQ(int32_t(Seen.size()), 6);

    RA4_EXPECT_EQ(R.NextBelow(0), 0u);
    RA4_EXPECT_EQ(R.NextRange(5, 5), 5);

    for (int32_t I = 0; I < 1000; ++I)
    {
        const int32_t V = R.NextRange(-3, 3);
        RA4_EXPECT(V >= -3 && V <= 3);
    }
}

RA4_TEST(ByteStream, RoundTripsEveryType)
{
    ByteWriter W;
    W.WriteUInt8(0xAB);
    W.WriteInt8(-42);
    W.WriteUInt16(0xBEEF);
    W.WriteInt32(-123456);
    W.WriteUInt64(0x0123456789ABCDEFull);
    W.WriteInt64(-9007199254740993ll);
    W.WriteBool(true);
    W.WriteString("Krasnaya trevoga");

    ByteReader R(W.GetBuffer());
    RA4_EXPECT_EQ(R.ReadUInt8(), uint8_t(0xAB));
    RA4_EXPECT_EQ(R.ReadInt8(), int8_t(-42));
    RA4_EXPECT_EQ(R.ReadUInt16(), uint16_t(0xBEEF));
    RA4_EXPECT_EQ(R.ReadInt32(), -123456);
    RA4_EXPECT(R.ReadUInt64() == 0x0123456789ABCDEFull);
    RA4_EXPECT(R.ReadInt64() == -9007199254740993ll);
    RA4_EXPECT(R.ReadBool());
    RA4_EXPECT(R.ReadString() == "Krasnaya trevoga");
    RA4_EXPECT(!R.HasError());
}

RA4_TEST(ByteStream, TruncatedInputSetsErrorInsteadOfReadingPastEnd)
{
    ByteWriter W;
    W.WriteUInt64(1234);
    std::vector<uint8_t> Truncated = W.GetBuffer();
    Truncated.resize(3);

    ByteReader R(Truncated);
    R.ReadUInt64();
    RA4_EXPECT(R.HasError());

    // A hostile packet claiming a huge string must not read out of bounds.
    ByteWriter W2;
    W2.WriteUInt16(60000);
    ByteReader R2(W2.GetBuffer());
    const std::string S = R2.ReadString();
    RA4_EXPECT(S.empty());
    RA4_EXPECT(R2.HasError());
}

RA4_TEST(Command, SerializationIsStable)
{
    Command C;
    C.Type = CommandType::Attack;
    C.Issuer = 3;
    C.Mode = OrderMode::Queue;
    C.Slot = 2;
    C.Primary = EntityId(17, 4);
    C.Target = EntityId(99, 1);
    C.Content = MakeContentId("unit.sov.heavy_tank");
    C.Location = Vec2(Fixed::FromInt(1234), Fixed::FromInt(-5678));
    C.Tile = TileCoord(12, 34);
    C.Param = -7;

    ByteWriter W;
    C.Serialize(W);
    ByteReader R(W.GetBuffer());
    const Command D = Command::Deserialize(R);

    RA4_EXPECT(D.Type == C.Type);
    RA4_EXPECT_EQ(D.Issuer, C.Issuer);
    RA4_EXPECT(D.Mode == C.Mode);
    RA4_EXPECT_EQ(D.Slot, C.Slot);
    RA4_EXPECT(D.Primary == C.Primary);
    RA4_EXPECT(D.Target == C.Target);
    RA4_EXPECT(D.Content == C.Content);
    RA4_EXPECT(D.Location == C.Location);
    RA4_EXPECT(D.Tile == C.Tile);
    RA4_EXPECT_EQ(D.Param, C.Param);
    RA4_EXPECT(!R.HasError());
}

RA4_TEST(Ids, GenerationDistinguishesRecycledSlots)
{
    const EntityId A(5, 0);
    const EntityId B(5, 1);
    RA4_EXPECT(A != B);
    RA4_EXPECT(A.IsValid());
    RA4_EXPECT(!EntityId::Invalid().IsValid());
    RA4_EXPECT(A.Packed() != B.Packed());
}

RA4_TEST(Ids, ContentHashesAreStableAndDistinct)
{
    // These literals are baked into replays and network handshakes; changing the
    // hash function silently invalidates every stored replay.
    RA4_EXPECT_EQ(HashName(""), 2166136261u);
    RA4_EXPECT(MakeContentId("unit.sov.conscript") != MakeContentId("unit.sov.conscript2"));
    RA4_EXPECT(MakeContentId("unit.sov.conscript") == MakeContentId("unit.sov.conscript"));
}

RA4_TEST(Checksum, DetectsSingleBitChanges)
{
    const uint8_t A[] = {1, 2, 3, 4, 5};
    const uint8_t B[] = {1, 2, 3, 4, 6};
    RA4_EXPECT(HashBytes(A, sizeof(A)) != HashBytes(B, sizeof(B)));
    RA4_EXPECT(HashBytes(A, sizeof(A)) == HashBytes(A, sizeof(A)));
}
