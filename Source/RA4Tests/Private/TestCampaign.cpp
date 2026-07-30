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
