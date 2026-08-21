// Copyright (c) Red Alert 4 project. Tests for Stage 9 (Procedural Vehicle Animation & Niagara VFX Driver).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Presentation/PresentationAnimationFX.h"
#include "RA4Simulation/SimTypes.h"

#include <cmath>
#include <vector>

using namespace RA4;
using namespace RA4Test;

// --- 1. Turret Independent Aiming Interpolation ---

RA4_TEST(AnimationFX, TurretAimingInterpolation)
{
    TurretAimState Turret;
    const Vec2 VehiclePos(Fixed::FromInt(1000), Fixed::FromInt(1000));
    const float VehicleYaw = 0.0f; // Facing East (0 deg)
    const Vec2 TargetPos(Fixed::FromInt(1000), Fixed::FromInt(2000)); // Target North (+Y is 90 deg)

    Turret.SetTargetAim(VehiclePos, VehicleYaw, TargetPos);
    RA4_EXPECT_NEAR(Turret.TargetYawDegrees, 90.0f, 0.01f);
    RA4_EXPECT_NEAR(Turret.CurrentYawDegrees, 0.0f, 0.01f);

    // Update 0.25s at 180 deg/s -> should advance 45 deg
    Turret.Update(0.25f, 180.0f);
    RA4_EXPECT_NEAR(Turret.CurrentYawDegrees, 45.0f, 0.01f);

    // Update another 0.25s -> reaches target 90 deg
    Turret.Update(0.25f, 180.0f);
    RA4_EXPECT_NEAR(Turret.CurrentYawDegrees, 90.0f, 0.01f);
}

// --- 2. Barrel Recoil Spring-Damper Physics ---

RA4_TEST(AnimationFX, BarrelRecoilSpringDamper)
{
    BarrelRecoilState Recoil;
    RA4_EXPECT_NEAR(Recoil.RecoilOffset, 0.0f, 0.001f);

    // Fire weapon impulse
    Recoil.Fire(1.0f);
    RA4_EXPECT_NEAR(Recoil.RecoilOffset, 1.0f, 0.001f);

    // Update 0.05s -> spring damping pulls barrel forward
    Recoil.Update(0.05f);
    RA4_EXPECT(Recoil.RecoilOffset < 1.0f);
    RA4_EXPECT(Recoil.RecoilOffset > 0.0f);

    // Simulate 0.5s total -> should settle at 0.0f resting state
    for (int I = 0; I < 10; ++I)
    {
        Recoil.Update(0.05f);
    }
    RA4_EXPECT_NEAR(Recoil.RecoilOffset, 0.0f, 0.01f);
}

// --- 3. Differential Tread Track UV Scroll ---

RA4_TEST(AnimationFX, TreadTrackDifferentialScroll)
{
    TreadTrackScrollState Treads;

    // Moving straight at 100 units/sec for 1.0s
    Treads.Update(100.0f, 0.0f, 40.0f, 1.0f);
    RA4_EXPECT(Treads.LeftTrackUV > 0.0f);
    RA4_EXPECT_NEAR(Treads.LeftTrackUV, Treads.RightTrackUV, 0.001f);


    const float PrevRight = Treads.RightTrackUV;

    // Turning right in place (0 forward speed, +90 deg/s angular speed)
    Treads.Update(0.0f, 90.0f, 40.0f, 1.0f);

    // Left track must scroll forward more than right track during right turn
    RA4_EXPECT(Treads.RightTrackUV > PrevRight);

}

// --- 4. Niagara Particle Event Consumption ---

RA4_TEST(AnimationFX, NiagaraEventConsumption)
{
    PresentationAnimationFX FX;

    std::vector<SimEvent> Events;
    {
        SimEvent Ev;
        Ev.Type = SimEventType::WeaponFired;
        Ev.Entity = EntityId(1, 0);
        Ev.Location = Vec2(Fixed::FromInt(500), Fixed::FromInt(500));
        Events.push_back(Ev);
    }
    {
        SimEvent Ev;
        Ev.Type = SimEventType::ProjectileImpact;
        Ev.Location = Vec2(Fixed::FromInt(1200), Fixed::FromInt(1200));
        Events.push_back(Ev);
    }

    FX.ConsumeSimEvents(Events);

    auto Requests = FX.PopQueuedParticleRequests();
    RA4_REQUIRE(Requests.size() == 2u);
    RA4_EXPECT_EQ(static_cast<uint8_t>(Requests[0].Kind), static_cast<uint8_t>(NiagaraParticleEffectKind::MuzzleFlash));
    RA4_EXPECT_EQ(static_cast<uint8_t>(Requests[1].Kind), static_cast<uint8_t>(NiagaraParticleEffectKind::ProjectileImpactCrater));

    // Queue must be empty after popping
    RA4_EXPECT_EQ(FX.PopQueuedParticleRequests().size(), 0u);
}
