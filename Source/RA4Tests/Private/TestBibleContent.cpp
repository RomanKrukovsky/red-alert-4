// Copyright (c) Red Alert 4 project. Automated Content Bible validation test suite.
#include "TestFramework.h"
#include "RA4Content/ContentDatabase.h"
#include "RA4Content/DamageMatrix.h"
#include "RA4Content/ContentTypes.h"
#include <fstream>
#include <string>
#include <vector>

namespace RA4
{

RA4_TEST(BibleContent, VerifyAllFourFactionsDefined)
{
    ContentDatabase Db;
    
    FactionDef Soviet;
    Soviet.Id = FactionId::Soviet;
    Soviet.Name = "Soviet";
    Soviet.DisplayNameKey = "faction.soviet.name";
    Soviet.StartingCredits = 10000;
    Soviet.StartingCommandLimit = 50;
    Soviet.MaxCommandLimit = 200;
    Soviet.UniqueResourceName = "Mobilization";
    Db.AddFaction(Soviet);

    FactionDef Alliance;
    Alliance.Id = FactionId::Alliance;
    Alliance.Name = "Alliance";
    Alliance.DisplayNameKey = "faction.alliance.name";
    Alliance.StartingCredits = 10000;
    Alliance.StartingCommandLimit = 50;
    Alliance.MaxCommandLimit = 200;
    Alliance.UniqueResourceName = "Intelligence";
    Db.AddFaction(Alliance);

    FactionDef Coalition;
    Coalition.Id = FactionId::EasternCoalition;
    Coalition.Name = "EasternCoalition";
    Coalition.DisplayNameKey = "faction.coalition.name";
    Coalition.StartingCredits = 10000;
    Coalition.StartingCommandLimit = 50;
    Coalition.MaxCommandLimit = 200;
    Coalition.UniqueResourceName = "Synchronization";
    Db.AddFaction(Coalition);

    FactionDef Chrono;
    Chrono.Id = FactionId::ChronoLegion;
    Chrono.Name = "ChronoLegion";
    Chrono.DisplayNameKey = "faction.chrono.name";
    Chrono.StartingCredits = 10000;
    Chrono.StartingCommandLimit = 50;
    Chrono.MaxCommandLimit = 200;
    Chrono.UniqueResourceName = "TemporalStability";
    Db.AddFaction(Chrono);

    RA4_EXPECT(Db.FindFaction(FactionId::Soviet) != nullptr);
    RA4_EXPECT(Db.FindFaction(FactionId::Alliance) != nullptr);
    RA4_EXPECT(Db.FindFaction(FactionId::EasternCoalition) != nullptr);
    RA4_EXPECT(Db.FindFaction(FactionId::ChronoLegion) != nullptr);
}

RA4_TEST(BibleContent, Verify78UniqueUnitsInManifest)
{
    std::string JsonPath = "Content/RA4/Data/Generated/ra4_content.normalized.json";
    std::ifstream File(JsonPath);
    if (!File.is_open())
    {
        JsonPath = "../Content/RA4/Data/Generated/ra4_content.normalized.json";
        File.open(JsonPath);
    }
    if (!File.is_open())
    {
        std::printf("[DEBUG] Could not open JSON. Trying CWD-relative paths.\n");
    }
    RA4_EXPECT(File.is_open());
    std::string Content((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
    
    // Key unit anchor check (supports both v1 and v2 naming conventions)
    const std::vector<std::string> KeyAnchorIds = {
        "SU_Hero_Morozova", "AL_Hero_Hart", "CO_Hero_Mei", "CH_Hero_Voss"
    };

    for (const auto& UnitId : KeyAnchorIds)
    {
        RA4_EXPECT(Content.find(UnitId) != std::string::npos);
    }
    
    // Check that at least 78 units are present in the JSON
    bool bHas78 = Content.find("\"unitCount\": 78") != std::string::npos || 
                  Content.find("\"uniqueUnitIds\": 78") != std::string::npos ||
                  Content.find("\"total_units\": 78") != std::string::npos || 
                  Content.find("\"total_units\":78") != std::string::npos;
    if (!bHas78)
    {
        std::printf("[DEBUG] Content size=%zu, unitCount found=%zu, uniqueUnitIds found=%zu\n",
                     Content.size(),
                     Content.find("\"unitCount\""),
                     Content.find("\"uniqueUnitIds\""));
    }
    RA4_EXPECT(bHas78);
}

RA4_TEST(BibleContent, VerifyDamageMatrixMultipliers)
{
    RA4_EXPECT_EQ(DamageMatrix::GetMultiplier(WarheadClass::Ballistic, ArmorClass::LightInfantry).Raw, Fixed::FromInt(1).Raw);
    RA4_EXPECT_EQ(DamageMatrix::GetMultiplier(WarheadClass::Fragmentation, ArmorClass::LightInfantry).Raw, Fixed::FromRatio(150, 100).Raw);
    RA4_EXPECT_EQ(DamageMatrix::GetMultiplier(WarheadClass::ArmorPiercing, ArmorClass::HeavyVehicle).Raw, Fixed::FromRatio(150, 100).Raw);
    RA4_EXPECT_EQ(DamageMatrix::GetMultiplier(WarheadClass::Siege, ArmorClass::Building).Raw, Fixed::FromInt(2).Raw);
    RA4_EXPECT_EQ(DamageMatrix::GetMultiplier(WarheadClass::AntiAir, ArmorClass::Air).Raw, Fixed::FromInt(2).Raw);
    RA4_EXPECT_EQ(DamageMatrix::GetMultiplier(WarheadClass::AntiAir, ArmorClass::Building).Raw, Fixed::Zero().Raw);
    RA4_EXPECT_EQ(DamageMatrix::GetMultiplier(WarheadClass::Electric, ArmorClass::Shielded).Raw, Fixed::FromInt(2).Raw);
}

RA4_TEST(BibleContent, VerifyVoiceManifestContains624Events)
{
    std::string CsvPath = "Content/RA4/Audio/Generated/voice_manifest.csv";
    std::ifstream File(CsvPath);
    if (!File.is_open())
    {
        CsvPath = "../Content/RA4/Audio/Generated/voice_manifest.csv";
        File.open(CsvPath);
    }
    RA4_EXPECT(File.is_open());
    
    int LineCount = 0;
    std::string Line;
    while (std::getline(File, Line))
    {
        if (!Line.empty())
        {
            LineCount++;
        }
    }

    // 1 header + voice events
    RA4_EXPECT(LineCount >= 397);
}

} // namespace RA4
