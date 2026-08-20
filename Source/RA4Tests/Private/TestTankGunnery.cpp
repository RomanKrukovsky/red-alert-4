// Copyright (c) Red Alert 4 project.
//
// Tests for the direct-control gunnery model. These pin the behaviour that makes
// a tank feel like a tank rather than an RTS unit with a camera inside it: that
// standing still is rewarded, that driving is punished, that where you are hit
// matters, and that angling the hull is a defence.
//
// Every function under test is pure integer arithmetic, so these run headlessly
// and are exact -- no tolerances, no "approximately".
#include "TestFramework.h"

#include "RA4Simulation/TankGunnery.h"

using namespace RA4;

namespace
{
GunneryDef MediumTankGun()
{
    GunneryDef G;
    G.AimedDispersionMrad = 30;
    G.MoveBloomMrad = 90;
    G.HullTurnBloomMrad = 70;
    G.TurretTurnBloomMrad = 50;
    G.FireBloomMrad = 60;
    G.ConvergePerMille = 900;
    G.PenetrationMm = 100;
    G.PenetrationVariancePerMille = 250;
    return G;
}
}  // namespace

// --- Dispersion -------------------------------------------------------------

RA4_TEST(Gunnery, StandingStillTightensTheCone)
{
    const GunneryDef Gun = MediumTankGun();
    GunneryMotion Still;   // everything zero

    int32_t Cone = 300;   // wide, as if the tank has just braked
    const int32_t Start = Cone;

    for (int i = 0; i < 40; ++i)
    {
        Cone = UpdateDispersionMrad(Gun, Still, Cone);
    }

    RA4_EXPECT(Cone < Start);
    // Two seconds of holding still must actually get you to a shot worth taking,
    // otherwise the loop the whole design rests on is not playable.
    RA4_EXPECT(IsAimed(Gun, Cone));
    // And it never goes below what the gun is capable of.
    RA4_EXPECT(Cone >= Gun.AimedDispersionMrad);
}

RA4_TEST(Gunnery, DrivingHoldsTheConeOpen)
{
    const GunneryDef Gun = MediumTankGun();
    GunneryMotion Driving;
    Driving.SpeedPerMille = 1000;   // flat out

    int32_t Cone = Gun.AimedDispersionMrad;
    for (int i = 0; i < 60; ++i)
    {
        Cone = UpdateDispersionMrad(Gun, Driving, Cone);
    }

    // However long you drive, the cone sits at the floor for driving and no
    // tighter. You cannot out-wait your own throttle.
    RA4_EXPECT(Cone == DispersionFloorMrad(Gun, Driving));
    RA4_EXPECT(!IsAimed(Gun, Cone));
    RA4_EXPECT(Cone > Gun.AimedDispersionMrad);
}

RA4_TEST(Gunnery, DisturbancesAddRatherThanMax)
{
    const GunneryDef Gun = MediumTankGun();

    GunneryMotion DriveOnly;
    DriveOnly.SpeedPerMille = 1000;
    GunneryMotion TraverseOnly;
    TraverseOnly.TurretTurnPerMille = 1000;
    GunneryMotion Both;
    Both.SpeedPerMille = 1000;
    Both.TurretTurnPerMille = 1000;

    const int32_t A = DispersionFloorMrad(Gun, DriveOnly);
    const int32_t B = DispersionFloorMrad(Gun, TraverseOnly);
    const int32_t C = DispersionFloorMrad(Gun, Both);

    // Driving while traversing is worse than either alone -- that is the whole
    // reason a player stops the hull before swinging the turret onto a target.
    RA4_EXPECT(C > A);
    RA4_EXPECT(C > B);
    RA4_EXPECT(C == A + B - Gun.AimedDispersionMrad);
}

RA4_TEST(Gunnery, FiringOpensTheConeSoBurstsAreWorse)
{
    const GunneryDef Gun = MediumTankGun();
    GunneryMotion Still;
    GunneryMotion Firing;
    Firing.bFiredThisTick = true;

    const int32_t Settled = Gun.AimedDispersionMrad;
    const int32_t AfterShot = UpdateDispersionMrad(Gun, Firing, Settled);

    RA4_EXPECT(AfterShot == Settled + Gun.FireBloomMrad);
    RA4_EXPECT(!IsAimed(Gun, AfterShot));

    // And it settles again afterwards, so a patient second shot is a good one.
    int32_t Cone = AfterShot;
    for (int i = 0; i < 40; ++i)
    {
        Cone = UpdateDispersionMrad(Gun, Still, Cone);
    }
    RA4_EXPECT(IsAimed(Gun, Cone));
}

// --- Facing -----------------------------------------------------------------

RA4_TEST(Gunnery, FacingSplitsIntoFrontSideRear)
{
    constexpr int32_t North = 0;

    // Shot arriving from straight ahead.
    RA4_EXPECT(FacingForImpact(North, 0) == HitFacing::Front);
    // From directly behind.
    RA4_EXPECT(FacingForImpact(North, kAngleTurn / 2) == HitFacing::Rear);
    // From either flank.
    RA4_EXPECT(FacingForImpact(North, kAngleTurn / 4) == HitFacing::Side);
    RA4_EXPECT(FacingForImpact(North, (3 * kAngleTurn) / 4) == HitFacing::Side);

    // The front arc is wide on purpose: a hull angled up to 45 degrees still
    // presents its front plate, which is what makes angling a skill rather than
    // a coin flip.
    RA4_EXPECT(FacingForImpact(North, kAngleTurn / 8) == HitFacing::Front);
    RA4_EXPECT(FacingForImpact(North, -kAngleTurn / 8) == HitFacing::Front);
}

RA4_TEST(Gunnery, RearIsSofterThanFront)
{
    ArmorDef Armor;
    RA4_EXPECT(Armor.ForFacing(HitFacing::Front) > Armor.ForFacing(HitFacing::Side));
    RA4_EXPECT(Armor.ForFacing(HitFacing::Side) > Armor.ForFacing(HitFacing::Rear));
}

// --- Effective thickness ----------------------------------------------------

RA4_TEST(Gunnery, ObliqueHitsMeetMoreMetal)
{
    constexpr int32_t Base = 100;

    const int32_t Square = EffectiveArmorMm(Base, 0);
    const int32_t Angled30 = EffectiveArmorMm(Base, (kAngleTurn * 30) / 360);
    const int32_t Angled60 = EffectiveArmorMm(Base, (kAngleTurn * 60) / 360);

    // Straight on, the plate is what it says on the tin.
    RA4_EXPECT(Square == Base);
    // The more oblique, the more metal in the way -- monotonically.
    RA4_EXPECT(Angled30 > Square);
    RA4_EXPECT(Angled60 > Angled30);
    // 60 degrees doubles the plate, which is the textbook figure and the number
    // the rest of the balance is reasoned against.
    RA4_EXPECT(Angled60 >= 195 && Angled60 <= 205);
}

RA4_TEST(Gunnery, GrazingHitsDoNotOverflow)
{
    // cos approaches zero at 90 degrees and the true effective thickness
    // approaches infinity. Clamped, or the arithmetic downstream stops meaning
    // anything.
    const int32_t Grazing = EffectiveArmorMm(100, (kAngleTurn * 89) / 360);
    RA4_EXPECT(Grazing > 0);
    RA4_EXPECT(Grazing <= 1000);
}

// --- Impact -----------------------------------------------------------------

RA4_TEST(Gunnery, SteepAnglesRicochetRegardlessOfGun)
{
    GunneryDef Monster = MediumTankGun();
    Monster.PenetrationMm = 10000;   // absurd on purpose

    const int32_t Steep = (kAngleTurn * 70) / 360;
    // Even a gun that could shoot through anything skids off a plate met at 70
    // degrees. This is what rewards angling against a superior gun.
    RA4_EXPECT(ResolveImpact(Monster, 50, Steep, 999) == ImpactResult::Ricochet);
    RA4_EXPECT(ResolveImpact(Monster, 50, Steep, 0) == ImpactResult::Ricochet);
}

RA4_TEST(Gunnery, PenetrationDependsOnWhereYouHit)
{
    const GunneryDef Gun = MediumTankGun();   // 100 mm nominal
    ArmorDef Armor;                            // 90 front, 45 side, 30 rear
    constexpr int32_t Square = 0;
    constexpr int32_t MedianRoll = 500;

    // Square onto the front of a well-armoured tank is a marginal shot.
    const ImpactResult Front =
        ResolveImpact(Gun, Armor.ForFacing(HitFacing::Front), Square, MedianRoll);
    // The side is not marginal at all.
    const ImpactResult Side =
        ResolveImpact(Gun, Armor.ForFacing(HitFacing::Side), Square, MedianRoll);
    const ImpactResult Rear =
        ResolveImpact(Gun, Armor.ForFacing(HitFacing::Rear), Square, MedianRoll);

    RA4_EXPECT(Front == ImpactResult::Penetration);   // 100 >= 90, just
    RA4_EXPECT(Side == ImpactResult::Penetration);
    RA4_EXPECT(Rear == ImpactResult::Penetration);

    // Now angle that same front plate 45 degrees: 90 mm becomes about 127 mm and
    // the same gun stops going through it.
    const int32_t Angled = (kAngleTurn * 45) / 360;
    RA4_EXPECT(ResolveImpact(Gun, Armor.ForFacing(HitFacing::Front), Angled, MedianRoll)
               == ImpactResult::Bounce);
}

RA4_TEST(Gunnery, TheRollDecidesMarginalShots)
{
    const GunneryDef Gun = MediumTankGun();   // 100 mm +/- 25%
    constexpr int32_t Square = 0;
    // A plate right at the edge of what this gun manages: 110 mm sits inside the
    // variance band, so the shell decides it and two identical shots differ.
    constexpr int32_t Marginal = 110;

    RA4_EXPECT(ResolveImpact(Gun, Marginal, Square, 999) == ImpactResult::Penetration);
    RA4_EXPECT(ResolveImpact(Gun, Marginal, Square, 0) == ImpactResult::Bounce);

    // Well outside the band in either direction, the roll cannot save or damn it.
    RA4_EXPECT(ResolveImpact(Gun, 40, Square, 0) == ImpactResult::Penetration);
    RA4_EXPECT(ResolveImpact(Gun, 300, Square, 999) == ImpactResult::Bounce);
}

// --- Scatter ----------------------------------------------------------------

RA4_TEST(Gunnery, ScatterGrowsWithConeAndRange)
{
    const Fixed Near = Fixed::FromInt(500);
    const Fixed Far = Fixed::FromInt(2000);

    const Fixed TightNear = ShotOffsetAtRange(30, Near, 1000);
    const Fixed WideNear = ShotOffsetAtRange(300, Near, 1000);
    const Fixed TightFar = ShotOffsetAtRange(30, Far, 1000);

    // A wider cone throws the shell further off at the same range.
    RA4_EXPECT(WideNear > TightNear);
    // The same cone throws it further off at longer range -- which is why a
    // sniping shot needs a tighter cone than a point-blank one.
    RA4_EXPECT(TightFar > TightNear);
    // Four times the range, four times the offset: the relationship is linear,
    // which is the entire point of quoting accuracy in milliradians.
    RA4_EXPECT(TightFar.Raw == TightNear.Raw * 4);

    // A centred roll puts the shell on the crosshair, and the sign of the roll
    // decides which side it lands.
    RA4_EXPECT(ShotOffsetAtRange(300, Near, 0) == Fixed::FromInt(0));
    RA4_EXPECT(ShotOffsetAtRange(300, Near, -1000) < Fixed::FromInt(0));
}

RA4_TEST(Gunnery, AimedShotLandsCloserThanSnapShot)
{
    const GunneryDef Gun = MediumTankGun();
    const Fixed Range = Fixed::FromInt(1500);

    GunneryMotion Driving;
    Driving.SpeedPerMille = 1000;
    Driving.TurretTurnPerMille = 1000;

    // Snap shot: everything moving.
    int32_t Snap = Gun.AimedDispersionMrad;
    for (int i = 0; i < 20; ++i)
    {
        Snap = UpdateDispersionMrad(Gun, Driving, Snap);
    }

    // Aimed shot: braked and settled.
    GunneryMotion Still;
    int32_t Aimed = Snap;
    for (int i = 0; i < 60; ++i)
    {
        Aimed = UpdateDispersionMrad(Gun, Still, Aimed);
    }

    const Fixed SnapMiss = ShotOffsetAtRange(Snap, Range, 1000);
    const Fixed AimedMiss = ShotOffsetAtRange(Aimed, Range, 1000);

    // The payoff for waiting, stated as a test so it cannot be balanced away by
    // accident: a settled shot is at least three times tighter than a snap shot.
    RA4_EXPECT(AimedMiss < SnapMiss);
    RA4_EXPECT(SnapMiss.Raw >= AimedMiss.Raw * 3);
}
