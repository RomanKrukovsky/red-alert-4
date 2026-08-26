// Copyright (c) Red Alert 4 project. Loads normalized JSON into ContentDatabase.
#include "RA4Content/BibleContentLoader.h"
#include "RA4Content/ContentDatabase.h"
#include "RA4Content/JsonParser.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

namespace RA4
{

namespace
{

// --- String → enum helpers -----------------------------------------------

FactionId ParseFactionId(const std::string& Name)
{
    if (Name == "СССР" || Name == "Soviet") return FactionId::Soviet;
    if (Name == "Альянс" || Name == "Alliance") return FactionId::Alliance;
    if (Name == "Восточная коалиция" || Name == "EasternCoalition") return FactionId::EasternCoalition;
    if (Name == "Хронолегион" || Name == "ChronoLegion") return FactionId::ChronoLegion;
    return FactionId::None;
}

ArmorClass ParseArmorClass(const std::string& Name)
{
    if (Name == "Лёгкая пехота") return ArmorClass::LightInfantry;
    if (Name == "Тяжёлая пехота") return ArmorClass::HeavyInfantry;
    if (Name == "Лёгкая техника") return ArmorClass::LightVehicle;
    if (Name == "Тяжёлая техника") return ArmorClass::HeavyVehicle;
    if (Name == "Осадная техника") return ArmorClass::SiegeVehicle;
    if (Name == "Воздушная") return ArmorClass::Air;
    if (Name == "Морская") return ArmorClass::Naval;
    if (Name == "Здания") return ArmorClass::Building;
    if (Name == "Щитовая") return ArmorClass::Shielded;
    return ArmorClass::LightInfantry;
}

WarheadClass ParseWarheadClass(const std::string& Name)
{
    if (Name == "Баллистический") return WarheadClass::Ballistic;
    if (Name == "Осколочный") return WarheadClass::Fragmentation;
    if (Name == "Бронебойный") return WarheadClass::ArmorPiercing;
    if (Name == "Осадный") return WarheadClass::Siege;
    if (Name == "Электрический") return WarheadClass::Electric;
    if (Name == "Плазменный") return WarheadClass::Plasma;
    if (Name == "Криогенный") return WarheadClass::Cryogenic;
    if (Name == "Темпоральный") return WarheadClass::Temporal;
    if (Name == "ПВО") return WarheadClass::AntiAir;
    return WarheadClass::Ballistic;
}

ProductionCategory ParseCategory(const std::string& Name)
{
    if (Name == "Пехота") return ProductionCategory::Infantry;
    if (Name == "Техника") return ProductionCategory::Vehicle;
    if (Name == "Авиация") return ProductionCategory::Aircraft;
    if (Name == "Флот") return ProductionCategory::Naval;
    if (Name == "Герой") return ProductionCategory::Infantry;
    if (Name == "Здание" || Name == "Оборона") return ProductionCategory::Structure;
    return ProductionCategory::Infantry;
}

TechTier ParseTechTier(const std::string& Name)
{
    if (Name == "T3" || Name == "Т3") return TechTier::T3;
    if (Name == "T2" || Name == "Т2") return TechTier::T2;
    if (Name == "T1" || Name == "Т1") return TechTier::T1;
    return TechTier::T0;
}

WarheadClass ParseWarheadFromWeaponString(const std::string& S)
{
    if (S.find("баллистический") != std::string::npos || S.find("Ballistic") != std::string::npos) return WarheadClass::Ballistic;
    if (S.find("осколочный") != std::string::npos || S.find("Fragmentation") != std::string::npos) return WarheadClass::Fragmentation;
    if (S.find("бронебойный") != std::string::npos) return WarheadClass::ArmorPiercing;
    if (S.find("осадный") != std::string::npos || S.find("Siege") != std::string::npos) return WarheadClass::Siege;
    if (S.find("электрический") != std::string::npos || S.find("Electric") != std::string::npos) return WarheadClass::Electric;
    if (S.find("плазменный") != std::string::npos || S.find("Plasma") != std::string::npos) return WarheadClass::Plasma;
    if (S.find("криогенный") != std::string::npos) return WarheadClass::Cryogenic;
    if (S.find("темпоральный") != std::string::npos || S.find("Temporal") != std::string::npos) return WarheadClass::Temporal;
    if (S.find("ПВО") != std::string::npos || S.find("AntiAir") != std::string::npos) return WarheadClass::AntiAir;
    if (S.find("термобарический") != std::string::npos) return WarheadClass::Siege;
    if (S.find("ракет") != std::string::npos) return WarheadClass::ArmorPiercing;
    return WarheadClass::Ballistic;
}

MovementLayer ParseMovementLayer(const std::string& Category, const std::string& Armor)
{
    if (Category == "Авиация") return MovementLayer::Air;
    if (Category == "Флот") return MovementLayer::Naval;
    if (Category == "Пехота" || Category == "Герой") return MovementLayer::Infantry;
    if (Armor == "Лёгкая техника") return MovementLayer::Wheeled;
    if (Armor == "Тяжёлая техника" || Armor == "Осадная техника") return MovementLayer::Tracked;
    return MovementLayer::Infantry;
}

FactionResourceType ParseFactionResourceType(const std::string& Name)
{
    if (Name == "Мобилизация" || Name == "Mobilization") return FactionResourceType::Mobilization;
    if (Name == "Разведданные" || Name == "Intelligence") return FactionResourceType::Intelligence;
    if (Name == "Синхронизация" || Name == "Synchronization") return FactionResourceType::Synchronization;
    if (Name == "Темпоральная стабильность" || Name == "TemporalStability") return FactionResourceType::TemporalStability;
    return FactionResourceType::None;
}

int32_t ParseInt(const std::string& S)
{
    std::string Clean;
    for (char C : S)
    {
        if (std::isdigit(static_cast<unsigned char>(C)) || C == '-')
        {
            Clean.push_back(C);
        }
        else if (!Clean.empty() && Clean.back() != '-')
        {
            break;
        }
    }
    if (Clean.empty()) return 0;
    return std::atoi(Clean.c_str());
}

double ParseDouble(const std::string& S)
{
    std::string Clean;
    for (char C : S)
    {
        if (std::isdigit(static_cast<unsigned char>(C)) || C == '.' || C == '-')
        {
            Clean.push_back(C);
        }
        else if (!Clean.empty() && Clean.back() != '-')
        {
            break;
        }
    }
    if (Clean.empty()) return 0.0;
    return std::atof(Clean.c_str());
}

std::string CleanId(const std::string& S)
{
    std::string Result;
    for (char C : S)
    {
        if (C != '`') Result.push_back(C);
    }
    return Result;
}

void ParseUnitJson(const Json::Value& UnitVal, FactionId Faction, ContentDatabase& Db)
{
    const Json::Value* IdVal = UnitVal.Find("id");
    std::string UnitId = IdVal ? CleanId(IdVal->AsString()) : "";
    if (UnitId.empty())
    {
        if (const Json::Value* NameVal = UnitVal.Find("name"))
        {
            UnitId = NameVal->AsString();
        }
        else return;
    }

    EntityDef E;
    VoiceSetDef VoiceSet;

    E.Id = MakeContentId(UnitId.c_str());
    E.Name = "unit." + UnitId;
    E.DisplayNameKey = "RA4.Unit." + UnitId + ".Name";
    E.Kind = EntityKind::Unit;
    E.Faction = Faction;

    if (const Json::Value* FacVal = UnitVal.Find("faction"))
    {
        FactionId ReadFac = ParseFactionId(FacVal->AsString());
        if (ReadFac != FactionId::None) E.Faction = ReadFac;
    }

    // Direct fields if present
    if (const Json::Value* Cost = UnitVal.Find("cost")) E.Production.Cost = ParseInt(Cost->AsString());
    if (const Json::Value* BT = UnitVal.Find("buildTime")) E.Production.BuildTimeTicks = ParseInt(BT->AsString()) * 20;
    if (const Json::Value* CL = UnitVal.Find("commandLimit")) E.Production.CommandLimit = ParseInt(CL->AsString());
    if (const Json::Value* HP = UnitVal.Find("hp")) E.MaxHealth = ParseInt(HP->AsString());
    if (const Json::Value* Armor = UnitVal.Find("armor")) E.Armor = ParseArmorClass(Armor->AsString());
    if (const Json::Value* Speed = UnitVal.Find("speed")) E.Unit.MaxSpeed = Fixed::FromInt(ParseInt(Speed->AsString()));
    if (const Json::Value* Cat = UnitVal.Find("category")) E.Production.Category = ParseCategory(Cat->AsString());

    // Parse params dictionary if present
    int32_t BibleRange = 0;
    int32_t BibleDps = 0;
    std::string WeaponString;
    std::string TierString;
    if (const Json::Value* Params = UnitVal.Find("params"))
    {
        const auto& Obj = Params->AsObject();
        for (const auto& [Key, Val] : Obj)
        {
            const std::string& StrVal = Val.AsString();
            if (Key == "Категория")
            {
                E.Production.Category = ParseCategory(StrVal);
            }
            else if (Key == "Стоимость")
            {
                E.Production.Cost = ParseInt(StrVal);
            }
            else if (Key == "Время производства")
            {
                E.Production.BuildTimeTicks = ParseInt(StrVal) * 20;
            }
            else if (Key == "Командный лимит")
            {
                E.Production.CommandLimit = ParseInt(StrVal);
            }
            else if (Key == "HP")
            {
                E.MaxHealth = ParseInt(StrVal);
            }
            else if (Key == "Тип брони")
            {
                E.Armor = ParseArmorClass(StrVal);
            }
            else if (Key == "Скорость")
            {
                E.Unit.MaxSpeed = Fixed::FromInt(static_cast<int32_t>(ParseDouble(StrVal) * 100));
            }
            else if (Key == "Дальность")
            {
                BibleRange = ParseInt(StrVal);
            }
            else if (Key == "Ориентировочный DPS")
            {
                BibleDps = ParseInt(StrVal);
            }
            else if (Key == "Основное оружие")
            {
                WeaponString = StrVal;
            }
            else if (Key == "Технологический уровень")
            {
                TierString = StrVal;
            }
        }
    }
    if (!TierString.empty())
    {
        E.Production.Tier = ParseTechTier(TierString);
    }

    // Set movement layer
    std::string CatStr = "Пехота";
    if (E.Production.Category == ProductionCategory::Vehicle) CatStr = "Техника";
    else if (E.Production.Category == ProductionCategory::Aircraft) CatStr = "Авиация";
    else if (E.Production.Category == ProductionCategory::Naval) CatStr = "Флот";

    std::string ArmorStr = "Лёгкая пехота";
    if (E.Armor == ArmorClass::HeavyInfantry) ArmorStr = "Тяжёлая пехота";
    else if (E.Armor == ArmorClass::LightVehicle) ArmorStr = "Лёгкая техника";
    else if (E.Armor == ArmorClass::HeavyVehicle) ArmorStr = "Тяжёлая техника";
    else if (E.Armor == ArmorClass::SiegeVehicle) ArmorStr = "Осадная техника";

    E.Unit.Layer = ParseMovementLayer(CatStr, ArmorStr);
    if (E.Unit.MaxSpeed.Raw == 0)
    {
        // Default speed per category if bible omitted
        if (E.Production.Category == ProductionCategory::Naval) E.Unit.MaxSpeed = Fixed::FromInt(500);
        else if (E.Production.Category == ProductionCategory::Aircraft) E.Unit.MaxSpeed = Fixed::FromInt(900);
        else if (E.Production.Category == ProductionCategory::Vehicle) E.Unit.MaxSpeed = Fixed::FromInt(450);
        else E.Unit.MaxSpeed = Fixed::FromInt(400);
    }
    E.VisionRange = Fixed::FromInt((BibleRange > 0 ? BibleRange + 2 : 8) * 100);
    if (E.MaxHealth == 100 && E.Production.Cost > 0)
    {
        // Fallback: keep bible HP, already parsed
    }

    // Create per-unit weapon from bible DPS/range/warhead if combat unit
    if (BibleDps > 0 && BibleRange > 0)
    {
        WeaponDef W;
        W.Id = MakeContentId(("weapon." + UnitId).c_str());
        W.Name = "weapon." + UnitId;
        W.Warhead = ParseWarheadFromWeaponString(WeaponString);
        // DPS is per-second; cooldown 20 ticks = 1 sec, so damage = DPS
        W.Damage = std::max(5, BibleDps);
        W.MaxRange = Fixed::FromInt(BibleRange * 100);
        W.MinRange = Fixed::FromInt(0);
        // Cooldown tuned by category: arty slower, infantry faster
        if (E.Production.Category == ProductionCategory::Vehicle && E.Armor == ArmorClass::SiegeVehicle) W.CooldownTicks = 60;
        else if (E.Production.Category == ProductionCategory::Naval) W.CooldownTicks = 30;
        else if (E.Production.Category == ProductionCategory::Aircraft) W.CooldownTicks = 25;
        else W.CooldownTicks = 20;
        // Naval/air can hit both, anti-air filtered elsewhere
        W.bCanTargetGround = true;
        W.bCanTargetAir = (E.Production.Category == ProductionCategory::Aircraft || WeaponString.find("ПВО") != std::string::npos);
        if (E.Production.Category == ProductionCategory::Naval) W.bCanTargetGround = true;
        W.bRequiresTurretAligned = false;
        Db.AddWeapon(W);
        E.Weapon = W.Id;
        // Role hints for AI
        if (E.Armor == ArmorClass::SiegeVehicle) E.Roles = EntityRole::Combat | EntityRole::Artillery;
        else if (WeaponString.find("ПВО") != std::string::npos) E.Roles = EntityRole::Combat | EntityRole::AntiAir;
        else E.Roles = EntityRole::Combat;
    }

    // Infer producer building from category + faction for buildability
    auto ProducerFor = [&](FactionId Fac, ProductionCategory Cat) -> std::string {
        if (Cat == ProductionCategory::Infantry) {
            if (Fac == FactionId::Soviet) return "Казарма мобилизации";
            if (Fac == FactionId::Alliance) return "Казарма";
            if (Fac == FactionId::EasternCoalition) return "Казарма";
            if (Fac == FactionId::ChronoLegion) return "Казарма";
        } else if (Cat == ProductionCategory::Vehicle) {
            if (Fac == FactionId::Soviet) return "Тяжёлый завод";
            if (Fac == FactionId::Alliance) return "Военный завод";
            if (Fac == FactionId::EasternCoalition) return "Военный завод";
            if (Fac == FactionId::ChronoLegion) return "Военный завод";
        } else if (Cat == ProductionCategory::Aircraft) {
            if (Fac == FactionId::Soviet) return "Аэродром дальней авиации";
            return "Аэродром";
        } else if (Cat == ProductionCategory::Naval) {
            if (Fac == FactionId::Soviet) return "Военно-морской док";
            return "Военно-морской док";
        }
        return "";
    };
    std::string ProducerName = ProducerFor(E.Faction, E.Production.Category);
    if (!ProducerName.empty()) {
        ContentId ProducerId = MakeContentId(ProducerName.c_str());
        // Also try the default-content english id as fallback
        E.Production.ProducedBy = {ProducerId};
        // Keep tier-based prerequisites minimal: T1 needs nothing else, T2 needs radar, T3 needs tech center
        if (E.Production.Tier == TechTier::T2) {
            std::string RadarName = (E.Faction == FactionId::Soviet ? "Командный радар" : "Радар");
            E.Production.Prerequisites = {MakeContentId(RadarName.c_str())};
        } else if (E.Production.Tier == TechTier::T3) {
            E.Production.Prerequisites = {MakeContentId(ProducerName.c_str())};
        }
    }

    Db.AddEntity(E);

    // Add default voice lines for unit
    VoiceSet.UnitId = E.Id;
    VoiceSet.VoiceId = UnitId;
    VoiceSet.Lines.push_back({ "Voice.Selected", "Готов к бою!", "", 10 });
    VoiceSet.Lines.push_back({ "Voice.Move", "Выполняю!", "", 10 });
    VoiceSet.Lines.push_back({ "Voice.Attack", "Огонь на поражение!", "", 10 });
    VoiceSet.Lines.push_back({ "Voice.EnemyDestroyed", "Цель уничтожена!", "", 10 });
    VoiceSet.Lines.push_back({ "Voice.Damaged", "Под обстрелом!", "", 10 });
    VoiceSet.Lines.push_back({ "Voice.CriticalDamage", "Критический урон!", "", 10 });
    VoiceSet.Lines.push_back({ "Voice.Death", "А-а-а!", "", 10 });
    VoiceSet.Lines.push_back({ "Voice.CannotComply", "Невозможно выполнить!", "", 10 });
    Db.AddVoiceSet(VoiceSet);
}

void ParseBuildingJson(const Json::Value& BVal, FactionId Faction, ContentDatabase& Db)
{
    const Json::Value* IdVal = BVal.Find("id");
    std::string BldId = IdVal ? CleanId(IdVal->AsString()) : "";
    if (BldId.empty())
    {
        if (const Json::Value* NameVal = BVal.Find("name"))
        {
            BldId = NameVal->AsString();
        }
        else return;
    }

    EntityDef E;
    E.Id = MakeContentId(BldId.c_str());
    E.Name = "building." + BldId;
    E.DisplayNameKey = "RA4.Building." + BldId + ".Name";
    E.Kind = EntityKind::Building;
    E.Faction = Faction;
    E.Armor = ArmorClass::Building;

    if (const Json::Value* Cost = BVal.Find("cost")) E.Production.Cost = ParseInt(Cost->AsString());
    if (const Json::Value* BT = BVal.Find("build_time_sec")) E.Production.BuildTimeTicks = ParseInt(BT->AsString()) * 20;
    if (const Json::Value* BT2 = BVal.Find("buildTime")) E.Production.BuildTimeTicks = ParseInt(BT2->AsString()) * 20;
    if (const Json::Value* HP = BVal.Find("hp")) E.MaxHealth = ParseInt(HP->AsString());

    if (const Json::Value* Params = BVal.Find("params"))
    {
        const auto& Obj = Params->AsObject();
        for (const auto& [Key, Val] : Obj)
        {
            const std::string& StrVal = Val.AsString();
            if (Key == "Стоимость") E.Production.Cost = ParseInt(StrVal);
            else if (Key == "Время строительства") E.Production.BuildTimeTicks = ParseInt(StrVal) * 20;
            else if (Key == "HP") E.MaxHealth = ParseInt(StrVal);
            else if (Key == "Энергия" || Key == "Энергопотребление" || Key == "Базовая мощность")
            {
                int32_t PwrVal = ParseInt(StrVal);
                if (StrVal.find('+') != std::string::npos || PwrVal > 0)
                {
                    E.Building.PowerProduced = PwrVal;
                }
                else
                {
                    E.Building.PowerConsumed = std::abs(PwrVal);
                }
            }
        }
    }

    if (const Json::Value* Power = BVal.Find("power"))
    {
        const std::string Pwr = Power->AsString();
        int32_t PwrVal = ParseInt(Pwr);
        if (Pwr.find('+') != std::string::npos || PwrVal > 0)
        {
            E.Building.PowerProduced = PwrVal;
        }
        else
        {
            E.Building.PowerConsumed = std::abs(PwrVal);
        }
    }

    // Purpose-driven flags for RA3 parity
    std::string PurposeCheck;
    if (const Json::Value* Purp = BVal.Find("purpose")) PurposeCheck = Purp->AsString();
    // Detect naval yard / airfield / superweapon by name/purpose
    const std::string NameCheck = BldId + PurposeCheck;
    if (NameCheck.find("док") != std::string::npos || PurposeCheck.find("Корабли") != std::string::npos)
    {
        E.Building.bWaterOnly = true;
        E.Building.bProvidesBuildRadius = true;
        E.Building.BuildRadius = Fixed::FromInt(1200);
        if (E.Building.FootprintX == 1) { E.Building.FootprintX = 3; E.Building.FootprintY = 3; }
    }
    if (NameCheck.find("Аэродром") != std::string::npos || PurposeCheck.find("посадочных места") != std::string::npos)
    {
        E.Building.bProvidesBuildRadius = true;
        E.Building.BuildRadius = Fixed::FromInt(1200);
        if (E.Building.FootprintX == 1) { E.Building.FootprintX = 3; E.Building.FootprintY = 2; }
    }
    if (PurposeCheck.find("неуязвимой") != std::string::npos || PurposeCheck.find("Железный купол") != std::string::npos)
    {
        E.Building.SuperweaponRechargeTicks = 6 * 60 * 20;
        E.Building.SuperweaponDamage = 0;
        E.Building.SuperweaponRadius = Fixed::FromInt(800);
        E.Building.SuperweaponWarhead = WarheadClass::Siege;
    }
    if (PurposeCheck.find("ракетный удар") != std::string::npos || PurposeCheck.find("Каратель") != std::string::npos)
    {
        E.Building.SuperweaponRechargeTicks = 8 * 60 * 20;
        E.Building.SuperweaponDamage = 900;
        E.Building.SuperweaponRadius = Fixed::FromInt(600);
        E.Building.SuperweaponWarhead = WarheadClass::Siege;
    }
    if (PurposeCheck.find("Мини-карта") != std::string::npos || NameCheck.find("радар") != std::string::npos || NameCheck.find("Радар") != std::string::npos)
    {
        E.Building.bIsRadar = true;
    }
    if (E.Building.PowerProduced == 0 && E.Building.PowerConsumed == 0)
    {
        E.Building.PowerConsumed = 50;
    }
    if (E.Building.FootprintX == 1 && E.Building.FootprintY == 1 && E.Kind == EntityKind::Building)
    {
        // Default footprint for bible buildings lacking explicit size
        if (PurposeCheck.find("Техника") != std::string::npos || NameCheck.find("завод") != std::string::npos) { E.Building.FootprintX = 3; E.Building.FootprintY = 3; }
        else if (NameCheck.find("Казарма") != std::string::npos) { E.Building.FootprintX = 2; E.Building.FootprintY = 2; }
        else if (NameCheck.find("электростанция") != std::string::npos) { E.Building.FootprintX = 2; E.Building.FootprintY = 2; }
    }

    Db.AddEntity(E);
}

} // anonymous namespace

bool LoadBibleContent(ContentDatabase& Db, const std::string& JsonFilePath, std::vector<std::string>& OutErrors)
{
    std::ifstream File(JsonFilePath);
    if (!File.is_open())
    {
        std::string Fallback = "../" + JsonFilePath;
        File.open(Fallback);
        if (!File.is_open())
        {
            Fallback = "../../" + JsonFilePath;
            File.open(Fallback);
        }
    }
    if (!File.is_open())
    {
        OutErrors.push_back("Failed to open JSON file: " + JsonFilePath);
        return false;
    }

    std::stringstream Buffer;
    Buffer << File.rdbuf();
    std::string Content = Buffer.str();

    std::string Err;
    Json::Value Root;
    if (!Json::Parse(Content, Root, Err))
    {
        OutErrors.push_back("JSON parse error: " + Err);
        return false;
    }

    // Load damage matrix (check root, combat, or economy)
    const Json::Value* DM = Root.Find("damageMatrix");
    if (!DM && Root.Find("combat"))
    {
        DM = Root.Find("combat")->Find("damageMatrix");
    }
    if (!DM && Root.Find("economy"))
    {
        DM = Root.Find("economy")->Find("damageMatrix");
    }

    if (DM && DM->IsArray())
    {
        DamageMatrixDef Dm;
        constexpr ArmorClass kArmorCols[] = {
            ArmorClass::LightInfantry,
            ArmorClass::HeavyInfantry,
            ArmorClass::LightVehicle,
            ArmorClass::HeavyVehicle,
            ArmorClass::Building,
            ArmorClass::Air
        };

        for (const auto& RowVal : DM->AsArray())
        {
            if (!RowVal.IsArray()) continue;
            const auto& Row = RowVal.AsArray();
            if (Row.size() < 2) continue;
            WarheadClass Wh = ParseWarheadClass(Row[0].AsString());
            for (size_t a = 1; a < Row.size() && (a - 1) < (sizeof(kArmorCols) / sizeof(kArmorCols[0])); ++a)
            {
                ArmorClass Arm = kArmorCols[a - 1];
                double Mult = ParseDouble(Row[a].AsString());
                int32_t MultiplierThousandths = static_cast<int32_t>(std::round(Mult * 1000.0));
                int32_t MultiplierPercent = static_cast<int32_t>(std::round(Mult * 100.0));
                Dm.Multipliers[int32_t(Wh)][int32_t(Arm)] = MultiplierThousandths;
                Db.SetDamageMultiplier(Wh, Arm, MultiplierPercent);
                if (Arm == ArmorClass::SiegeVehicle)
                {
                    Dm.Multipliers[int32_t(Wh)][int32_t(ArmorClass::Building)] = MultiplierThousandths;
                    Db.SetDamageMultiplier(Wh, ArmorClass::Building, MultiplierPercent);
                }
            }
        }
        Db.SetDamageMatrix(Dm);
    }

    // Load veterancy
    {
        VeterancyDef Vet;
        Vet.Levels[int32_t(VeterancyRank::Recruit)] = {1, 0, 0, 0, false, false};
        Vet.Levels[int32_t(VeterancyRank::Veteran)] = {1, 10, 8, 1, false, false};
        Vet.Levels[int32_t(VeterancyRank::Elite)] = {2, 10, 10, 2, true, false};
        Vet.Levels[int32_t(VeterancyRank::Heroic)] = {5, 10, 10, 3, true, true};
        Db.SetVeterancy(Vet);
    }

    // Load factions
    if (const Json::Value* Factions = Root.Find("factions"))
    {
        for (const auto& FactionVal : Factions->AsArray())
        {
            FactionDef F;
            F.Id = ParseFactionId(FactionVal.Find("name")->AsString());
            F.Name = FactionVal.Find("name")->AsString();
            F.StartingCredits = 10000;
            F.StartingCommandLimit = 50;
            F.MaxCommandLimit = 200;

            if (const Json::Value* FR = FactionVal.Find("factionResource"))
            {
                if (const Json::Value* Name = FR->Find("text"))
                {
                    F.UniqueResourceName = Name->AsString();
                }
            }

            Db.AddFaction(F);

            if (FactionVal.Find("factionResource") != nullptr)
            {
                FactionResourceDef Res;
                Res.Faction = F.Id;
                Res.Name = F.UniqueResourceName;
                Res.Type = ParseFactionResourceType(Res.Name);
                Res.Min = 0;
                Res.Max = 100;
                Db.SetFactionResource(Res);
            }

            // Load EVA lines
            if (const Json::Value* EVA = FactionVal.Find("eva"))
            {
                for (const auto& EvaVal : EVA->AsArray())
                {
                    EvaLineDef Eva;
                    Eva.Faction = F.Name;
                    if (const Json::Value* Event = EvaVal.Find("event"))
                        Eva.EventTag = "EVA." + Event->AsString();
                    if (const Json::Value* Line = EvaVal.Find("line"))
                        Eva.TextRu = Line->AsString();
                    Eva.Priority = 50;
                    Db.AddEvaLine(Eva);
                }
            }

            // Load heroes, units & buildings within faction
            if (const Json::Value* Heroes = FactionVal.Find("heroes"))
            {
                for (const auto& HeroVal : Heroes->AsArray())
                {
                    ParseUnitJson(HeroVal, F.Id, Db);
                }
            }
            if (const Json::Value* FUnits = FactionVal.Find("units"))
            {
                for (const auto& UnitVal : FUnits->AsArray())
                {
                    ParseUnitJson(UnitVal, F.Id, Db);
                }
            }
            if (const Json::Value* FBuildings = FactionVal.Find("buildings"))
            {
                for (const auto& BVal : FBuildings->AsArray())
                {
                    ParseBuildingJson(BVal, F.Id, Db);
                }
            }
            if (const Json::Value* FSummary = FactionVal.Find("unitSummary"))
            {
                for (const auto& UnitVal : FSummary->AsArray())
                {
                    ParseUnitJson(UnitVal, F.Id, Db);
                }
            }
        }
    }

    // Load faction resources from section 3
    if (const Json::Value* FR = Root.Find("factionResources"))
    {
        for (const auto& FRVal : FR->AsArray())
        {
            const auto& Arr = FRVal.AsArray();
            if (Arr.size() < 4) continue;
            FactionId Fac = ParseFactionId(Arr[0].AsString());
            FactionResourceDef Res;
            Res.Faction = Fac;
            Res.Name = Arr[1].AsString();
            Res.Type = ParseFactionResourceType(Res.Name);
            Res.Min = 0;
            Res.Max = 100;
            Res.AccrualRules.push_back(Arr[2].AsString());
            Res.SpendRules.push_back(Arr[3].AsString());
            Db.SetFactionResource(Res);
        }
    }

    // Load buildings at root level if any
    if (const Json::Value* RootBuildings = Root.Find("buildings"))
    {
        for (const auto& BVal : RootBuildings->AsArray())
        {
            FactionId Fac = FactionId::None;
            if (const Json::Value* FacVal = BVal.Find("faction"))
            {
                Fac = ParseFactionId(FacVal->AsString());
            }
            ParseBuildingJson(BVal, Fac, Db);
        }
    }

    // Load units at root level if any
    if (const Json::Value* Units = Root.Find("units"))
    {
        for (const auto& UnitVal : Units->AsArray())
        {
            ParseUnitJson(UnitVal, FactionId::None, Db);
        }
    }

    return true;
}

bool ValidateBibleContent(const ContentDatabase& Db, std::vector<std::string>& OutErrors)
{
    if (Db.GetEntities().empty())
    {
        OutErrors.push_back("ContentDatabase has no entities loaded");
        return false;
    }
    return true;
}

} // namespace RA4