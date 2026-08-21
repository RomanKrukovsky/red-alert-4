// Copyright (c) Red Alert 4 project. Tests for Stage 10 (Interactive Dynamic Music & Unit Voice Barks).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/Command.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Presentation/PresentationDynamicAudio.h"
#include "RA4Simulation/SimTypes.h"

#include <vector>

using namespace RA4;
using namespace RA4Test;

// --- 1. Combat Heat & Dynamic Music Layer Blending ---

RA4_TEST(DynamicAudio, CombatHeatMusicLayerBlending)
{
    PresentationDynamicAudio Audio;

    // Initially at peace
    RA4_EXPECT_NEAR(Audio.GetLayerVolume(MusicIntensityLayer::Ambient), 1.0f, 0.01f);
    RA4_EXPECT_NEAR(Audio.GetLayerVolume(MusicIntensityLayer::HeavyCombat), 0.0f, 0.01f);
    RA4_EXPECT_NEAR(Audio.GetCombatHeat(), 0.0f, 0.01f);

    // Ingest intense battle events
    std::vector<SimEvent> BattleEvents;
    for (int I = 0; I < 4; ++I)
    {
        SimEvent Ev;
        Ev.Type = SimEventType::DamageApplied;
        Ev.Player = 0;
        BattleEvents.push_back(Ev);
    }
    {
        SimEvent Ev;
        Ev.Type = SimEventType::EntityDestroyed;
        Ev.Player = 0;
        BattleEvents.push_back(Ev);
    }

    Audio.ConsumeSimEvents(BattleEvents, 0);
    RA4_EXPECT(Audio.GetCombatHeat() >= 80.0f);

    // Update 1.0s to allow volume lerp
    Audio.Update(1.0f);
    RA4_EXPECT(Audio.GetLayerVolume(MusicIntensityLayer::HeavyCombat) > 0.8f);
    RA4_EXPECT(Audio.GetLayerVolume(MusicIntensityLayer::Ambient) < 0.2f);

    // Simulate 20.0s of peace -> heat decays, ambient restores
    for (int I = 0; I < 20; ++I)
    {
        Audio.Update(1.0f);
    }
    RA4_EXPECT_NEAR(Audio.GetCombatHeat(), 0.0f, 0.01f);
    RA4_EXPECT(Audio.GetLayerVolume(MusicIntensityLayer::Ambient) > 0.8f);
    RA4_EXPECT(Audio.GetLayerVolume(MusicIntensityLayer::HeavyCombat) < 0.1f);
}

// --- 2. Voice Bark Priority & Preemption ---

RA4_TEST(DynamicAudio, VoiceBarkPriorityAndPreemption)
{
    PresentationDynamicAudio Audio;

    // Issue Move command -> plays move ack (Priority 2)
    Command MoveCmd = MakeCommand(CommandType::Move, 0);
    Audio.OnCommandIssued(0, MoveCmd, "sov_conscript");

    const auto* Bark1 = Audio.GetCurrentVoiceBark();
    RA4_REQUIRE(Bark1 != nullptr);
    RA4_EXPECT_EQ(static_cast<uint8_t>(Bark1->Kind), static_cast<uint8_t>(VoiceBarkKind::MoveOrdered));
    RA4_EXPECT_EQ(Bark1->Priority, 2u);

    // Advance 0.1s
    Audio.Update(0.1f);

    // Unit comes under fire -> emits UnderFire (Priority 8)
    std::vector<SimEvent> DamageEvents;
    SimEvent Ev;
    Ev.Type = SimEventType::DamageApplied;
    Ev.Player = 0;
    DamageEvents.push_back(Ev);

    Audio.ConsumeSimEvents(DamageEvents, 0);

    // High priority UnderFire should immediately preempt low priority MoveOrdered
    const auto* Bark2 = Audio.GetCurrentVoiceBark();
    RA4_REQUIRE(Bark2 != nullptr);
    RA4_EXPECT_EQ(static_cast<uint8_t>(Bark2->Kind), static_cast<uint8_t>(VoiceBarkKind::UnderFire));
    RA4_EXPECT_EQ(Bark2->Priority, 8u);
}

// --- 3. Voice Bark Rate Limiting ---

RA4_TEST(DynamicAudio, VoiceBarkRateLimiting)
{
    PresentationDynamicAudio Audio;

    Command MoveCmd = MakeCommand(CommandType::Move, 0);

    // First command plays
    Audio.OnCommandIssued(0, MoveCmd, "sov_conscript");
    RA4_REQUIRE(Audio.GetCurrentVoiceBark() != nullptr);

    // Second command in same second is rate-limited
    ActiveVoiceBark SpamBark;
    SpamBark.Kind = VoiceBarkKind::MoveOrdered;
    SpamBark.Priority = 2;
    const bool bQueued = Audio.QueueVoiceBark(SpamBark);
    RA4_EXPECT_EQ(bQueued, false);
}
