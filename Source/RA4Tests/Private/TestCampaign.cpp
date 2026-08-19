// Copyright (c) Red Alert 4 project. Tests for campaign database and mission graphs.
#include "TestFramework.h"

#include "CampaignDatabase.h"

using namespace RA4;

RA4_TEST(Campaign, VerifyAllFourChaptersAnd38MissionsExist)
{
    CampaignDatabase Db;
    const std::vector<CampaignChapterDef>& Chapters = Db.GetChapters();
    RA4_REQUIRE(Chapters.size() == 4);

    const CampaignChapterDef* Sov = Db.FindChapter(FactionId::Soviet);
    RA4_REQUIRE(Sov != nullptr);
    RA4_EXPECT_EQ(Sov->Missions.size(), size_t(10));

    const CampaignChapterDef* All = Db.FindChapter(FactionId::Alliance);
    RA4_REQUIRE(All != nullptr);
    RA4_EXPECT_EQ(All->Missions.size(), size_t(10));

    const CampaignChapterDef* Eac = Db.FindChapter(FactionId::EasternCoalition);
    RA4_REQUIRE(Eac != nullptr);
    RA4_EXPECT_EQ(Eac->Missions.size(), size_t(10));

    const CampaignChapterDef* Chro = Db.FindChapter(FactionId::ChronoLegion);
    RA4_REQUIRE(Chro != nullptr);
    RA4_EXPECT_EQ(Chro->Missions.size(), size_t(8));
    RA4_EXPECT(Chro->bIsSecretCampaign);
}

RA4_TEST(Campaign, VerifySokolovDemonstrationCutscene)
{
    CampaignDatabase Db;
    const CutsceneSequenceDef Cutscene = Db.GetSokolovDemonstrationCutscene();

    RA4_EXPECT(Cutscene.CutsceneId == "cutscene.sov.briefing_sokolov_ward");
    RA4_REQUIRE(Cutscene.DialogueLines.size() >= 2);
    RA4_EXPECT(Cutscene.DialogueLines[0].AudioEventId == "vo.sokolov.madam_president");
    RA4_EXPECT(Cutscene.DialogueLines[0].TextKey == "dialogue.sokolov.madam_president_speech");
}

#include "CampaignProfile.h"

RA4_TEST(Campaign, ProfileRecordsVictoriesAndComputesStars)
{
    CampaignProfile Profile;
    RA4_EXPECT_EQ(Profile.GetCompletedMissionCount(), 0);
    RA4_EXPECT_EQ(Profile.GetTotalStars(), 0);

    // Record a 1-star victory (primary only, slower than par time)
    Profile.RecordMissionVictory("sov_01", 1200, false, 800, 1700000000);
    RA4_EXPECT(Profile.IsMissionCompleted("sov_01"));
    RA4_EXPECT_EQ(Profile.GetCompletedMissionCount(), 1);
    const MissionRecord* R1 = Profile.GetMissionRecord("sov_01");
    RA4_REQUIRE(R1 != nullptr);
    RA4_EXPECT_EQ(R1->StarsEarned, 1);
    RA4_EXPECT_EQ(R1->BestCompletionTick, 1200u);

    // Replay faster (beats par time and completes secondary objectives -> 3 stars)
    Profile.RecordMissionVictory("sov_01", 650, true, 800, 1700000100);
    const MissionRecord* R2 = Profile.GetMissionRecord("sov_01");
    RA4_REQUIRE(R2 != nullptr);
    RA4_EXPECT_EQ(R2->StarsEarned, 3);
    RA4_EXPECT_EQ(R2->BestCompletionTick, 650u);
    RA4_EXPECT(R2->bSecondaryObjectivesDone);
    RA4_EXPECT_EQ(Profile.GetTotalStars(), 3);
}

RA4_TEST(Campaign, ProfileJsonSerializationRoundTrip)
{
    CampaignProfile Original;
    Original.RecordMissionVictory("sov_01", 600, true, 800, 1700000001);
    Original.RecordMissionVictory("all_01", 950, false, 900, 1700000002);

    std::string Json;
    RA4_REQUIRE(Original.SerializeJson(Json));
    RA4_EXPECT(!Json.empty());

    CampaignProfile Restored;
    RA4_REQUIRE(Restored.DeserializeJson(Json));
    RA4_EXPECT_EQ(Restored.GetCompletedMissionCount(), 2);
    RA4_EXPECT_EQ(Restored.GetTotalStars(), Original.GetTotalStars());

    const MissionRecord* R_Sov = Restored.GetMissionRecord("sov_01");
    RA4_REQUIRE(R_Sov != nullptr);
    RA4_EXPECT_EQ(R_Sov->BestCompletionTick, 600u);
    RA4_EXPECT(R_Sov->bSecondaryObjectivesDone);
    RA4_EXPECT_EQ(R_Sov->StarsEarned, 3);

    const MissionRecord* R_All = Restored.GetMissionRecord("all_01");
    RA4_REQUIRE(R_All != nullptr);
    RA4_EXPECT_EQ(R_All->BestCompletionTick, 950u);
    RA4_EXPECT(!R_All->bSecondaryObjectivesDone);
    RA4_EXPECT_EQ(R_All->StarsEarned, 1);
}
