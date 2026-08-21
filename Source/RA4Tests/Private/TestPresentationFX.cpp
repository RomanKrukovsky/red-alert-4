// Copyright (c) Red Alert 4 project. Tests for Stage 4 (Graphics, VFX, Spatial Audio & Presentation).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Presentation/PresentationTracerFX.h"
#include "RA4Presentation/PresentationAudioManager.h"
#include "RA4Simulation/SimWorld.h"

#include <memory>
#include <vector>

using namespace RA4;
using namespace RA4Test;
using namespace RA4::Presentation;

// --- 1. Tracer Lifecycle & Ballistic Interpolation ---

RA4_TEST(PresentationFX, TracerLifecycleAndInterpolation)
{
    PresentationTracerFX FX;

    const Vec2 From = Vec2(Fixed::FromInt(0), Fixed::FromInt(0));
    const Vec2 To = Vec2(Fixed::FromInt(4000), Fixed::FromInt(0));

    const uint32_t TracerId = FX.SpawnTracer(TracerType::StandardBullet, From, To, /*Speed*/ 4000.0f);
    RA4_EXPECT(TracerId > 0);
    RA4_REQUIRE(FX.GetActiveTracers().size() == 1u);

    // Initial state
    RA4_EXPECT_EQ(FX.GetActiveTracers()[0].ProgressAlpha, 0.0f);
    RA4_EXPECT(FX.GetActiveTracers()[0].bAlive);

    // Advance 0.5s -> tracer travels 2000cm (50% progress)
    FX.Tick(0.5f);
    RA4_REQUIRE(FX.GetActiveTracers().size() == 1u);
    RA4_EXPECT(FX.GetActiveTracers()[0].ProgressAlpha >= 0.49f && FX.GetActiveTracers()[0].ProgressAlpha <= 0.51f);
    RA4_EXPECT(FX.GetActiveTracers()[0].CurrentPos.X >= Fixed::FromInt(1950) &&
               FX.GetActiveTracers()[0].CurrentPos.X <= Fixed::FromInt(2050));

    // Advance another 0.6s -> tracer reaches target and is retired
    FX.Tick(0.6f);
    RA4_EXPECT_EQ(FX.GetActiveTracers().size(), 0u);
}

// --- 2. Sustained Tesla & Laser Beam Decay ---

RA4_TEST(PresentationFX, TeslaAndLaserBeamDecay)
{
    PresentationTracerFX FX;

    const Vec2 From = Vec2(Fixed::FromInt(100), Fixed::FromInt(100));
    const Vec2 To = Vec2(Fixed::FromInt(500), Fixed::FromInt(500));

    FX.SpawnBeam(TracerType::TeslaArc, From, To, /*Duration*/ 0.4f, /*Width*/ 12.0f);
    RA4_REQUIRE(FX.GetActiveTracers().size() == 1u);
    RA4_EXPECT(FX.GetActiveTracers()[0].bIsBeam);

    // Halfway
    FX.Tick(0.2f);
    RA4_REQUIRE(FX.GetActiveTracers().size() == 1u);

    // Expired
    FX.Tick(0.25f);
    RA4_EXPECT_EQ(FX.GetActiveTracers().size(), 0u);
}

// --- 3. Impact Decal Spawning & Alpha Fade ---

RA4_TEST(PresentationFX, ImpactDecalSpawningAndFade)
{
    PresentationTracerFX FX;
    const Vec2 Pos = Vec2(Fixed::FromInt(1200), Fixed::FromInt(800));

    FX.SpawnImpact(Pos, WarheadClass::Ballistic, 120.0f, /*Duration*/ 2.0f);
    RA4_REQUIRE(FX.GetActiveImpacts().size() == 1u);
    RA4_EXPECT_EQ(FX.GetActiveImpacts()[0].Alpha, 1.0f);

    // Tick 1.0s (50% elapsed) -> Alpha is ~0.5
    FX.Tick(1.0f);
    RA4_REQUIRE(FX.GetActiveImpacts().size() == 1u);
    RA4_EXPECT(FX.GetActiveImpacts()[0].Alpha >= 0.49f && FX.GetActiveImpacts()[0].Alpha <= 0.51f);

    // Tick another 1.1s -> Alpha reaches 0.0 and impact is purged
    FX.Tick(1.1f);
    RA4_EXPECT_EQ(FX.GetActiveImpacts().size(), 0u);
}

// --- 4. Spatial Audio Attenuation & Stereo Panning ---

RA4_TEST(PresentationFX, SpatialAudioDistanceAttenuationAndStereoPan)
{
    PresentationAudioManager Audio;
    // Listener focused at (0, 0), facing North (0 deg), elevation 1500cm
    Audio.UpdateListener(Vec2(Fixed::FromInt(0), Fixed::FromInt(0)), /*Facing*/ 0.0f, /*Height*/ 1500.0f);

    // 1. Center sound directly under camera
    float VolCenter = 0.0f, PanCenter = 0.0f;
    Audio.CalculateSpatialParameters(Vec2(Fixed::FromInt(0), Fixed::FromInt(0)), 5000.0f, VolCenter, PanCenter);
    RA4_EXPECT_EQ(VolCenter, 1.0f);
    RA4_EXPECT_EQ(PanCenter, 0.0f);

    // 2. Sound to the right (+X)
    float VolRight = 0.0f, PanRight = 0.0f;
    Audio.CalculateSpatialParameters(Vec2(Fixed::FromInt(1000), Fixed::FromInt(0)), 5000.0f, VolRight, PanRight);
    RA4_EXPECT(VolRight > 0.0f && VolRight < 1.0f);
    RA4_EXPECT(PanRight > 0.4f && PanRight < 0.6f); // 1000/2000 = 0.5

    // 3. Sound to the left (-X)
    float VolLeft = 0.0f, PanLeft = 0.0f;
    Audio.CalculateSpatialParameters(Vec2(Fixed::FromInt(-1000), Fixed::FromInt(0)), 5000.0f, VolLeft, PanLeft);
    RA4_EXPECT(PanLeft < -0.4f && PanLeft > -0.6f);

    // 4. Sound at edge of falloff (5000cm away)
    float VolFar = 0.0f, PanFar = 0.0f;
    Audio.CalculateSpatialParameters(Vec2(Fixed::FromInt(5000), Fixed::FromInt(0)), 5000.0f, VolFar, PanFar);
    RA4_EXPECT_EQ(VolFar, 0.0f);
}

// --- 5. Voice Line Priority Queue and Cooldown ---

RA4_TEST(PresentationFX, VoiceLinePriorityQueueAndCooldown)
{
    PresentationAudioManager Audio;

    const ContentId TankDef = MakeContentId("unit.test_tank");

    // Regular movement voice bark (priority 50)
    Audio.RequestVoiceBark(0, TankDef, SoundCategory::Voice_UnitAcknowledge, "voice.tank.move", 50);

    // Immediate duplicate bark is rejected due to active cooldown
    Audio.RequestVoiceBark(0, TankDef, SoundCategory::Voice_UnitAcknowledge, "voice.tank.move", 50);

    // Critical commander alert (priority 95)
    Audio.RequestVoiceBark(0, TankDef, SoundCategory::Voice_CommanderAlert, "voice.base.under_attack", 95);

    PendingVoiceBark FirstBark;
    RA4_REQUIRE(Audio.GetNextVoiceBark(FirstBark));
    // Higher priority alert pops FIRST even though it was requested second!
    RA4_EXPECT_EQ(FirstBark.Priority, 95);
    RA4_EXPECT(FirstBark.VoiceKey == "voice.base.under_attack");

    PendingVoiceBark SecondBark;
    RA4_REQUIRE(Audio.GetNextVoiceBark(SecondBark));
    RA4_EXPECT_EQ(SecondBark.Priority, 50);
    RA4_EXPECT(SecondBark.VoiceKey == "voice.tank.move");

    // Queue is now empty (duplicate was rejected)
    PendingVoiceBark ThirdBark;
    RA4_EXPECT(!Audio.GetNextVoiceBark(ThirdBark));
}

// --- 6. SimEvents Consumption Integration ---

RA4_TEST(PresentationFX, SimEventsConsumptionIntegration)
{
    PresentationTracerFX TracerFX;
    PresentationAudioManager AudioManager;
    AudioManager.UpdateListener(Vec2(Fixed::FromInt(0), Fixed::FromInt(0)));

    std::vector<SimEvent> Events;

    // Simulate WeaponFired event
    {
        SimEvent Ev;
        Ev.Type = SimEventType::WeaponFired;
        Ev.Location = Vec2(Fixed::FromInt(1500), Fixed::FromInt(500));
        Events.push_back(Ev);
    }

    // Simulate DamageApplied event
    {
        SimEvent Ev;
        Ev.Type = SimEventType::DamageApplied;
        Ev.Location = Vec2(Fixed::FromInt(1500), Fixed::FromInt(500));
        Events.push_back(Ev);
    }

    // Simulate CoopPingEmitted event
    {
        SimEvent Ev;
        Ev.Type = SimEventType::CoopPingEmitted;
        Ev.Location = Vec2(Fixed::FromInt(2000), Fixed::FromInt(2000));
        Events.push_back(Ev);
    }

    SimWorld DummyWorld;

    TracerFX.ConsumeSimEvents(Events, DummyWorld);
    AudioManager.ConsumeSimEvents(Events, DummyWorld, /*LocalPlayer*/ 0);

    // Verify Tracers and Muzzle flashes spawned
    RA4_EXPECT_EQ(TracerFX.GetActiveTracers().size(), 1u);
    RA4_EXPECT_EQ(TracerFX.GetActiveMuzzleFlashes().size(), 1u);
    RA4_EXPECT_EQ(TracerFX.GetActiveImpacts().size(), 1u);

    // Verify Audio cues generated
    RA4_EXPECT(AudioManager.GetActiveSoundCues().size() >= 3u);
}
