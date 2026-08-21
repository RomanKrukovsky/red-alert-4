// Copyright (c) Scarlet Horizon project.

#include "RA4FactionData.h"

#define LOCTEXT_NAMESPACE "RA4FactionData"

const FRA4FactionDataRegistry& FRA4FactionDataRegistry::Get()
{
    static FRA4FactionDataRegistry Instance;
    return Instance;
}

FRA4FactionDataRegistry::FRA4FactionDataRegistry()
{
    PopulateRegistry();
}

FLinearColor FRA4FactionDataRegistry::GetBlocPrimaryColor(ERA4FactionTheme Theme)
{
    switch (Theme)
    {
        case ERA4FactionTheme::EurasianPact:
            return FLinearColor(0.28f, 0.08f, 0.38f, 1.0f); // Deep Plum / Purple
        case ERA4FactionTheme::AtlanticAlliance:
            return FLinearColor(0.08f, 0.28f, 0.65f, 1.0f); // Cobalt Blue
        case ERA4FactionTheme::EasternCoalition:
            return FLinearColor(0.06f, 0.40f, 0.26f, 1.0f); // Deep Emerald
        case ERA4FactionTheme::PacificPact:
            return FLinearColor(0.04f, 0.48f, 0.58f, 1.0f); // Marine Turquoise
        case ERA4FactionTheme::Independent:
            return FLinearColor(0.72f, 0.52f, 0.18f, 1.0f); // Amber / Sand Gold
        default:
            return FLinearColor(0.28f, 0.08f, 0.38f, 1.0f);
    }
}

FLinearColor FRA4FactionDataRegistry::GetBlocAccentColor(ERA4FactionTheme Theme)
{
    switch (Theme)
    {
        case ERA4FactionTheme::EurasianPact:
            return FLinearColor(0.68f, 0.28f, 0.88f, 1.0f); // Bright Purple / Violet Accent
        case ERA4FactionTheme::AtlanticAlliance:
            return FLinearColor(0.35f, 0.70f, 0.98f, 1.0f); // Ice Blue
        case ERA4FactionTheme::EasternCoalition:
            return FLinearColor(0.88f, 0.72f, 0.22f, 1.0f); // Warm Jade Gold
        case ERA4FactionTheme::PacificPact:
            return FLinearColor(0.95f, 0.42f, 0.32f, 1.0f); // Coral Warning Accent
        case ERA4FactionTheme::Independent:
            return FLinearColor(0.22f, 0.78f, 0.72f, 1.0f); // Drone Cyan Accent
        default:
            return FLinearColor(0.68f, 0.28f, 0.88f, 1.0f);
    }
}

FLinearColor FRA4FactionDataRegistry::GetBlocGlowColor(ERA4FactionTheme Theme)
{
    switch (Theme)
    {
        case ERA4FactionTheme::EurasianPact:
            return FLinearColor(0.82f, 0.35f, 0.95f, 1.0f);
        case ERA4FactionTheme::AtlanticAlliance:
            return FLinearColor(0.45f, 0.82f, 1.00f, 1.0f);
        case ERA4FactionTheme::EasternCoalition:
            return FLinearColor(0.25f, 0.85f, 0.55f, 1.0f);
        case ERA4FactionTheme::PacificPact:
            return FLinearColor(0.30f, 0.90f, 0.95f, 1.0f);
        case ERA4FactionTheme::Independent:
            return FLinearColor(0.95f, 0.70f, 0.25f, 1.0f);
        default:
            return FLinearColor(0.82f, 0.35f, 0.95f, 1.0f);
    }
}

void FRA4FactionDataRegistry::PopulateRegistry()
{
    Blocs.Reset();

    // =========================================================================
    // 1. ЕВРАЗИЙСКИЙ ПАКТ (EURASIAN PACT)
    // =========================================================================
    {
        FRA4BlocInfo Bloc;
        Bloc.BlocId = ERA4FactionTheme::EurasianPact;
        // Континентальный щит: тяжёлый левый рельс и массивное основание.
        Bloc.LayoutWeight = 2.30f;
        Bloc.PanelDensity = ERA4PanelRole::Hero;
        Bloc.FrameRail = FMargin(6.0f, 1.5f, 1.5f, 4.0f);
        Bloc.DisplayName = LOCTEXT("Bloc_Eurasia", "Евразийский пакт");
        Bloc.Motto = LOCTEXT("Motto_Eurasia", "Единство. Технология. Суверенитет.");
        Bloc.Description = LOCTEXT("Desc_Eurasia", "Континентальный щит и глубинная оборона. Индустриальная мощь, эшелонированная ПВО и контроль ключевых направлений.");
        Bloc.KeyAdvantages = {
            LOCTEXT("Adv_Eurasia_1", "Мощная ПВО и плотная сеть радаров перехвата"),
            LOCTEXT("Adv_Eurasia_2", "Тяжёлая броня защищённых подразделений"),
            LOCTEXT("Adv_Eurasia_3", "Индустриальная глубина и быстрое развёртывание")
        };
        Bloc.ControlledRegions = 23;
        Bloc.ActivePersonnel = 1248000;
        Bloc.ReadinessRatio = 0.88f;
        Bloc.PrimaryColor = GetBlocPrimaryColor(ERA4FactionTheme::EurasianPact);
        Bloc.GlowColor = GetBlocGlowColor(ERA4FactionTheme::EurasianPact);

        // Russia (Leader)
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("RU");
            Country.LayoutWeight = 2.20f;
            Country.DisplayName = LOCTEXT("Country_RU", "Россия");
            Country.Specialization = LOCTEXT("Spec_RU", "Тяжёлая война · РЭБ · ПВО");
            Country.BaseHeadquarters = LOCTEXT("HQ_RU", "Полевой командный пункт «Рубеж-К»");
            Country.LoreDescription = LOCTEXT("Lore_RU", "Опора Пакта и ударная сила на континенте. Масштабные бронетанковые объединения, развитые средства РЭБ и эшелонированная ПВО создают непреодолимый щит.");
            Country.BlocTheme = ERA4FactionTheme::EurasianPact;
            Country.PrimaryColor = FLinearColor(0.32f, 0.08f, 0.44f, 1.0f);
            Country.AccentColor = FLinearColor(0.72f, 0.28f, 0.92f, 1.0f);
            Country.FirepowerRating = 0.95f;
            Country.ArmorRating = 0.92f;
            Country.MobilityRating = 0.58f;
            Country.TechRating = 0.88f;
            Country.bUnlocked = true;

            // Doctrine 1
            FRA4DoctrineInfo Doc1;
            Doc1.DoctrineId = TEXT("RU_DOC_EW_Missile");
            Doc1.CombatPhilosophy = LOCTEXT("Phil_RU_DOC_EW_Missile", "Ослепить, затем накрыть залпом");
            Doc1.SignatureSuperweapon = LOCTEXT("Super_RU_DOC_EW_Missile", "Ракетная шахта «Каратель» — стратегический ракетный удар");
            Doc1.DisplayName = LOCTEXT("Doc_RU_1", "РЭБ и ракетные войска");
            Doc1.Description = LOCTEXT("Doc_RU_1_Desc", "Подавление каналов связи противника, срыв высокоточного наведения и массированные термобарические ракетные удары.");
            Doc1.ModifiedUnits = { LOCTEXT("RU_U1", "Машина РЭБ «Громобой»"), LOCTEXT("RU_U2", "РСЗО «Зарево»"), LOCTEXT("RU_U3", "ЗРК «Заслон»") };
            Doc1.KeyAbilities = { LOCTEXT("RU_A1", "ЭМИ-удар"), LOCTEXT("RU_A2", "Огненный сектор"), LOCTEXT("RU_A3", "Подавление радара") };
            Doc1.SignatureUnit = LOCTEXT("RU_Sig1", "ЭМП-7 «Громобой»");
            Country.Doctrines.Add(Doc1);

            // Doctrine 2
            FRA4DoctrineInfo Doc2;
            Doc2.DoctrineId = TEXT("RU_DOC_HeavyBreakthrough");
            Doc2.CombatPhilosophy = LOCTEXT("Phil_RU_DOC_HeavyBreakthrough", "Броня решает там, где манёвр бесполезен");
            Doc2.SignatureSuperweapon = LOCTEXT("Super_RU_DOC_HeavyBreakthrough", "Осадный режим ТТП-11 «Воевода»");
            Doc2.DisplayName = LOCTEXT("Doc_RU_2", "Тяжёлый прорыв");
            Doc2.Description = LOCTEXT("Doc_RU_2_Desc", "Сверхтяжёлые танки прорыва, осадная трансформация и мощнейшая лобовая защита для преодоления любых укреплений.");
            Doc2.ModifiedUnits = { LOCTEXT("RU_U4", "Танк прорыва «Воевода»"), LOCTEXT("RU_U5", "ОБТ «Гранит»"), LOCTEXT("RU_U6", "Штурмовик «Запал»") };
            Doc2.KeyAbilities = { LOCTEXT("RU_A4", "Осадный режим"), LOCTEXT("RU_A5", "Броневой таран"), LOCTEXT("RU_A6", "Термобарический штурм") };
            Doc2.SignatureUnit = LOCTEXT("RU_Sig2", "ТТП-11 «Воевода»");
            Country.Doctrines.Add(Doc2);

            // Doctrine 3
            FRA4DoctrineInfo Doc3;
            Doc3.DoctrineId = TEXT("RU_DOC_ArcticGroup");
            Doc3.CombatPhilosophy = LOCTEXT("Phil_RU_DOC_ArcticGroup", "Рельеф и мороз работают на нас");
            Doc3.SignatureSuperweapon = LOCTEXT("Super_RU_DOC_ArcticGroup", "Автономный ракетный комплекс «Полярный»");
            Doc3.DisplayName = LOCTEXT("Doc_RU_3", "Арктическая группировка");
            Doc3.Description = LOCTEXT("Doc_RU_3_Desc", "Высокая проходимость по заснеженному и пересечённому рельефу, автономные ракетные комплексы и северное патрулирование.");
            Doc3.ModifiedUnits = { LOCTEXT("RU_U7", "Амфибия «Север»"), LOCTEXT("RU_U8", "Ракетный комплекс «Полярный»") };
            Doc3.KeyAbilities = { LOCTEXT("RU_A7", "Арктический марш"), LOCTEXT("RU_A8", "Полярный дозор") };
            Doc3.SignatureUnit = LOCTEXT("RU_Sig3", "ТРК «Полярный рубеж»");
            Country.Doctrines.Add(Doc3);

            Bloc.Countries.Add(Country);
        }

        // Belarus
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("BY");
            Country.LayoutWeight = 1.05f;
            Country.DisplayName = LOCTEXT("Country_BY", "Беларусь");
            Country.Specialization = LOCTEXT("Spec_BY", "Скрытность · ПВО · РЭБ");
            Country.BaseHeadquarters = LOCTEXT("HQ_BY", "Лесной командный узел «Пуща»");
            Country.LoreDescription = LOCTEXT("Lore_BY", "Маскировка позиций, засады в лесистой местности и мобильные радиоэлектронные помехи.");
            Country.BlocTheme = ERA4FactionTheme::EurasianPact;
            Country.PrimaryColor = FLinearColor(0.24f, 0.12f, 0.32f, 1.0f);
            Country.AccentColor = FLinearColor(0.58f, 0.32f, 0.75f, 1.0f);
            Country.FirepowerRating = 0.70f;
            Country.ArmorRating = 0.68f;
            Country.MobilityRating = 0.82f;
            Country.TechRating = 0.80f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc;
            Doc.DoctrineId = TEXT("BY_DOC_StealthEW");
            Doc.CombatPhilosophy = LOCTEXT("Phil_BY_DOC_StealthEW", "Противник не бьёт по тому, чего не видит");
            Doc.SignatureSuperweapon = LOCTEXT("Super_BY_DOC_StealthEW", "Ложные сигнатуры и радиоэлектронный купол");
            Doc.DisplayName = LOCTEXT("Doc_BY_1", "Комплексная маскировка и РЭБ");
            Doc.Description = LOCTEXT("Doc_BY_1_Desc", "Скрытное развёртывание подразделений и активные радиоэлектронные ловушки.");
            Doc.ModifiedUnits = { LOCTEXT("BY_U1", "Мобильный комплекс «Пеленг»"), LOCTEXT("BY_U2", "ЗРК «Оса-М2»") };
            Doc.SignatureUnit = LOCTEXT("BY_Sig", "РЛС «Пеленг-М»");
            Country.Doctrines.Add(Doc);

            Bloc.Countries.Add(Country);
        }

        // Kazakhstan
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("KZ");
            Country.LayoutWeight = 1.05f;
            Country.DisplayName = LOCTEXT("Country_KZ", "Казахстан");
            Country.Specialization = LOCTEXT("Spec_KZ", "Ракетные войска · Дальний удар · Мобильность");
            Country.BaseHeadquarters = LOCTEXT("HQ_KZ", "Степной пусковой штаб «Сарыарка»");
            Country.LoreDescription = LOCTEXT("Lore_KZ", "Мобильные степные ракетные соединения, сверхдальние огневые рубежи и скоростные контрудары.");
            Country.BlocTheme = ERA4FactionTheme::EurasianPact;
            Country.PrimaryColor = FLinearColor(0.20f, 0.15f, 0.38f, 1.0f);
            Country.AccentColor = FLinearColor(0.48f, 0.42f, 0.88f, 1.0f);
            Country.FirepowerRating = 0.86f;
            Country.ArmorRating = 0.62f;
            Country.MobilityRating = 0.90f;
            Country.TechRating = 0.76f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc;
            Doc.DoctrineId = TEXT("KZ_DOC_SteppeMissile");
            Doc.CombatPhilosophy = LOCTEXT("Phil_KZ_DOC_SteppeMissile", "Дальний рубеж вместо ближнего боя");
            Doc.SignatureSuperweapon = LOCTEXT("Super_KZ_DOC_SteppeMissile", "Сверхдальний залп степного дивизиона");
            Doc.DisplayName = LOCTEXT("Doc_KZ_1", "Степной дальний удар");
            Doc.Description = LOCTEXT("Doc_KZ_1_Desc", "Сверхдальние высокоточные ракетные комплексы на колёсном шасси высокой проходимости.");
            Doc.ModifiedUnits = { LOCTEXT("KZ_U1", "ОТРК «Байтерек»"), LOCTEXT("KZ_U2", "Колёсный БТР «Арлан»") };
            Doc.SignatureUnit = LOCTEXT("KZ_Sig", "ОТРК «Байтерек-С»");
            Country.Doctrines.Add(Doc);

            Bloc.Countries.Add(Country);
        }

        Blocs.Add(Bloc);
    }

    // =========================================================================
    // 2. АТЛАНТИЧЕСКИЙ АЛЬЯНС (ATLANTIC ALLIANCE)
    // =========================================================================
    {
        FRA4BlocInfo Bloc;
        Bloc.BlocId = ERA4FactionTheme::AtlanticAlliance;
        // Воздушно-морская сеть: тонкий аэродинамический верхний рельс.
        Bloc.LayoutWeight = 1.30f;
        Bloc.PanelDensity = ERA4PanelRole::Standard;
        Bloc.FrameRail = FMargin(1.0f, 4.0f, 1.0f, 1.0f);
        Bloc.DisplayName = LOCTEXT("Bloc_Atlantic", "Атлантический альянс");
        Bloc.Motto = LOCTEXT("Motto_Atlantic", "Сила закона. Превосходство сети.");
        Bloc.Description = LOCTEXT("Desc_Atlantic", "Воздушно-морское превосходство, глобальное экспедиционное развёртывание и единая информационно-боевая сеть.");
        Bloc.KeyAdvantages = {
            LOCTEXT("Adv_Atlantic_1", "Господство в воздухе и палубная авиация"),
            LOCTEXT("Adv_Atlantic_2", "Загоризонтная разведка и высокоточные удары"),
            LOCTEXT("Adv_Atlantic_3", "Сетецентрическое управление боем")
        };
        Bloc.ControlledRegions = 28;
        Bloc.ActivePersonnel = 1420000;
        Bloc.ReadinessRatio = 0.94f;
        Bloc.PrimaryColor = GetBlocPrimaryColor(ERA4FactionTheme::AtlanticAlliance);
        Bloc.GlowColor = GetBlocGlowColor(ERA4FactionTheme::AtlanticAlliance);

        // USA
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("US");
            Country.LayoutWeight = 2.20f;
            Country.DisplayName = LOCTEXT("Country_US", "США");
            Country.Specialization = LOCTEXT("Spec_US", "Авиация · Разведка · Точный удар");
            Country.BaseHeadquarters = LOCTEXT("HQ_US", "Мобильный оперативный центр «Vanguard»");
            Country.LoreDescription = LOCTEXT("Lore_US", "Основа экспедиционных сил Альянса. Авианосные группы, стелс-истребители 6-го поколения и система сетевого взаимодействия.");
            Country.BlocTheme = ERA4FactionTheme::AtlanticAlliance;
            Country.PrimaryColor = FLinearColor(0.08f, 0.28f, 0.65f, 1.0f);
            Country.AccentColor = FLinearColor(0.35f, 0.70f, 0.98f, 1.0f);
            Country.FirepowerRating = 0.88f;
            Country.ArmorRating = 0.78f;
            Country.MobilityRating = 0.92f;
            Country.TechRating = 0.98f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc1;
            Doc1.DoctrineId = TEXT("US_DOC_AirLandBattle");
            Doc1.CombatPhilosophy = LOCTEXT("Phil_US_DOC_AirLandBattle", "Небо готовит удар, земля закрепляет");
            Doc1.SignatureSuperweapon = LOCTEXT("Super_US_DOC_AirLandBattle", "Воздушно-наземный координированный удар");
            Doc1.DisplayName = LOCTEXT("Doc_US_1", "AirLand Battle");
            Doc1.Description = LOCTEXT("Doc_US_1_Desc", "Глубокая интеграция штурмовой авиации и наземных механизированных бригад.");
            Doc1.ModifiedUnits = { LOCTEXT("US_U1", "Стелс-штурмовик F-35"), LOCTEXT("US_U2", "ОБТ «АбрамсX»") };
            Doc1.SignatureUnit = LOCTEXT("US_Sig1", "F-35C Lightning II");
            Country.Doctrines.Add(Doc1);

            FRA4DoctrineInfo Doc2;
            Doc2.DoctrineId = TEXT("US_DOC_Expeditionary");
            Doc2.CombatPhilosophy = LOCTEXT("Phil_US_DOC_Expeditionary", "Сила приходит с моря в любую точку");
            Doc2.SignatureSuperweapon = LOCTEXT("Super_US_DOC_Expeditionary", "Развёртывание экспедиционной группы");
            Doc2.DisplayName = LOCTEXT("Doc_US_2", "Экспедиционные силы");
            Doc2.Description = LOCTEXT("Doc_US_2_Desc", "Мгновенное развёртывание баз и морской пехоты в любой точке океанского театра.");
            Doc2.ModifiedUnits = { LOCTEXT("US_U3", "Авианосец класса «Форд»"), LOCTEXT("US_U4", "Конвертоплан V-280") };
            Doc2.SignatureUnit = LOCTEXT("US_Sig2", "АВ «Свобода» (CVN-81)");
            Country.Doctrines.Add(Doc2);

            FRA4DoctrineInfo Doc3;
            Doc3.DoctrineId = TEXT("US_DOC_NetworkWarfare");
            Doc3.CombatPhilosophy = LOCTEXT("Phil_US_DOC_NetworkWarfare", "Информация опережает огонь");
            Doc3.SignatureSuperweapon = LOCTEXT("Super_US_DOC_NetworkWarfare", "Полное раскрытие театра на 8 секунд");
            Doc3.DisplayName = LOCTEXT("Doc_US_3", "Сетевая война");
            Doc3.Description = LOCTEXT("Doc_US_3_Desc", "Спутниковое целеуказание, лазерные системы ПРО и автономные разведывательные БПЛА.");
            Doc3.ModifiedUnits = { LOCTEXT("US_U5", "Лазерный комплекс «Гелиос»"), LOCTEXT("US_U6", "Стратегический БПЛА «RQ-180»") };
            Doc3.SignatureUnit = LOCTEXT("US_Sig3", "Лазерная платформа «Гелиос-2»");
            Country.Doctrines.Add(Doc3);

            Bloc.Countries.Add(Country);
        }

        // Great Britain, France, Germany, Poland, Ukraine...
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("GB");
            Country.LayoutWeight = 1.30f;
            Country.DisplayName = LOCTEXT("Country_GB", "Великобритания");
            Country.Specialization = LOCTEXT("Spec_GB", "Морская блокада · Спецоперации");
            Country.BaseHeadquarters = LOCTEXT("HQ_GB", "Флотский штаб «Admiralty»");
            Country.BlocTheme = ERA4FactionTheme::AtlanticAlliance;
            Country.PrimaryColor = FLinearColor(0.10f, 0.22f, 0.52f, 1.0f);
            Country.AccentColor = FLinearColor(0.40f, 0.65f, 0.95f, 1.0f);
            Country.FirepowerRating = 0.82f;
            Country.ArmorRating = 0.74f;
            Country.MobilityRating = 0.85f;
            Country.TechRating = 0.90f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc;
            Doc.DoctrineId = TEXT("GB_DOC_NavalSAS");
            Doc.CombatPhilosophy = LOCTEXT("Phil_GB_DOC_NavalSAS", "Точечный рейд вместо фронтального боя");
            Doc.SignatureSuperweapon = LOCTEXT("Super_GB_DOC_NavalSAS", "Скрытная высадка спецподразделения");
            Doc.DisplayName = LOCTEXT("Doc_GB_1", "Королевский флот и SAS");
            Doc.Description = LOCTEXT("Doc_GB_1_Desc", "Эсминцы ПРО и диверсионные группы глубокого проникновения.");
            Country.Doctrines.Add(Doc);
            Bloc.Countries.Add(Country);
        }

        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("FR");
            Country.LayoutWeight = 1.30f;
            Country.DisplayName = LOCTEXT("Country_FR", "Франция");
            Country.Specialization = LOCTEXT("Spec_FR", "Быстрое развёртывание · Ракетный щит");
            Country.BaseHeadquarters = LOCTEXT("HQ_FR", "Экспедиционный штаб «Citadelle»");
            Country.BlocTheme = ERA4FactionTheme::AtlanticAlliance;
            Country.PrimaryColor = FLinearColor(0.12f, 0.25f, 0.58f, 1.0f);
            Country.AccentColor = FLinearColor(0.45f, 0.72f, 0.98f, 1.0f);
            Country.FirepowerRating = 0.84f;
            Country.ArmorRating = 0.70f;
            Country.MobilityRating = 0.90f;
            Country.TechRating = 0.88f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc;
            Doc.DoctrineId = TEXT("FR_DOC_Vanguard");
            Doc.CombatPhilosophy = LOCTEXT("Phil_FR_DOC_Vanguard", "Скорость важнее толщины брони");
            Doc.SignatureSuperweapon = LOCTEXT("Super_FR_DOC_Vanguard", "Стремительный охват авангарда");
            Doc.DisplayName = LOCTEXT("Doc_FR_1", "Легкобронированный авангард");
            Doc.Description = LOCTEXT("Doc_FR_1_Desc", "Колёсная бронетехника «Ягуар» и маневренная тактическая авиация «Рафаль».");
            Country.Doctrines.Add(Doc);
            Bloc.Countries.Add(Country);
        }

        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("DE");
            Country.LayoutWeight = 1.10f;
            Country.DisplayName = LOCTEXT("Country_DE", "Германия");
            Country.Specialization = LOCTEXT("Spec_DE", "Высокоточная броня · Системная логистика");
            Country.BaseHeadquarters = LOCTEXT("HQ_DE", "Логистический штаб «Bollwerk»");
            Country.BlocTheme = ERA4FactionTheme::AtlanticAlliance;
            Country.PrimaryColor = FLinearColor(0.15f, 0.24f, 0.48f, 1.0f);
            Country.AccentColor = FLinearColor(0.38f, 0.62f, 0.90f, 1.0f);
            Country.FirepowerRating = 0.89f;
            Country.ArmorRating = 0.88f;
            Country.MobilityRating = 0.72f;
            Country.TechRating = 0.92f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc;
            Doc.DoctrineId = TEXT("DE_DOC_PanzerLogistics");
            Doc.CombatPhilosophy = LOCTEXT("Phil_DE_DOC_PanzerLogistics", "Клин, который снабжается на ходу");
            Doc.SignatureSuperweapon = LOCTEXT("Super_DE_DOC_PanzerLogistics", "Полевой ремонт бронегруппы на марше");
            Doc.DisplayName = LOCTEXT("Doc_DE_1", "Танковый клин «Леопард-3»");
            Doc.Description = LOCTEXT("Doc_DE_1_Desc", "Тяжёлая бронетехника высокой точности с автоматической перезарядкой.");
            Country.Doctrines.Add(Doc);
            Bloc.Countries.Add(Country);
        }

        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("PL");
            Country.LayoutWeight = 0.75f;
            Country.DisplayName = LOCTEXT("Country_PL", "Польша");
            Country.Specialization = LOCTEXT("Spec_PL", "Эшелонированная оборона · БПЛА");
            Country.BaseHeadquarters = LOCTEXT("HQ_PL", "Опорный узел обороны «Wisła»");
            Country.BlocTheme = ERA4FactionTheme::AtlanticAlliance;
            Country.PrimaryColor = FLinearColor(0.16f, 0.20f, 0.45f, 1.0f);
            Country.AccentColor = FLinearColor(0.42f, 0.68f, 0.96f, 1.0f);
            Country.FirepowerRating = 0.85f;
            Country.ArmorRating = 0.82f;
            Country.MobilityRating = 0.78f;
            Country.TechRating = 0.84f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc;
            Doc.DoctrineId = TEXT("PL_DOC_DefenseDrones");
            Doc.CombatPhilosophy = LOCTEXT("Phil_PL_DOC_DefenseDrones", "Держать рубеж и выбивать технику");
            Doc.SignatureSuperweapon = LOCTEXT("Super_PL_DOC_DefenseDrones", "Противотанковый дроновый заслон");
            Doc.DisplayName = LOCTEXT("Doc_PL_1", "Противотанковый вал и дроны");
            Doc.Description = LOCTEXT("Doc_PL_1_Desc", "Плотные противотанковые засады и барражирующие боеприпасы.");
            Country.Doctrines.Add(Doc);
            Bloc.Countries.Add(Country);
        }

        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("UA");
            Country.LayoutWeight = 0.75f;
            Country.DisplayName = LOCTEXT("Country_UA", "Украина");
            Country.Specialization = LOCTEXT("Spec_UA", "Роевые дроны · Мобильная оборона");
            Country.BaseHeadquarters = LOCTEXT("HQ_UA", "Сетевой узел управления «Тризуб»");
            Country.BlocTheme = ERA4FactionTheme::AtlanticAlliance;
            Country.PrimaryColor = FLinearColor(0.06f, 0.28f, 0.58f, 1.0f);
            Country.AccentColor = FLinearColor(0.85f, 0.75f, 0.15f, 1.0f);
            Country.FirepowerRating = 0.82f;
            Country.ArmorRating = 0.64f;
            Country.MobilityRating = 0.94f;
            Country.TechRating = 0.86f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc;
            Doc.DoctrineId = TEXT("UA_DOC_DroneSwarm");
            Doc.CombatPhilosophy = LOCTEXT("Phil_UA_DOC_DroneSwarm", "Дешёвый рой против дорогой цели");
            Doc.SignatureSuperweapon = LOCTEXT("Super_UA_DOC_DroneSwarm", "Массированный залёт FPV-роя");
            Doc.DisplayName = LOCTEXT("Doc_UA_1", "FPV-рои и маневренная сеть");
            Doc.Description = LOCTEXT("Doc_UA_1_Desc", "Насыщение поля боя дешёвыми ударными FPV-дронами и высокоточная артиллерия.");
            Country.Doctrines.Add(Doc);
            Bloc.Countries.Add(Country);
        }

        Blocs.Add(Bloc);
    }

    // =========================================================================
    // 3. ВОСТОЧНАЯ КОАЛИЦИЯ (EASTERN COALITION)
    // =========================================================================
    {
        FRA4BlocInfo Bloc;
        Bloc.BlocId = ERA4FactionTheme::EasternCoalition;
        // Производственный контур: плотная сетка и промышленное основание.
        Bloc.LayoutWeight = 1.30f;
        Bloc.PanelDensity = ERA4PanelRole::DenseHUD;
        Bloc.FrameRail = FMargin(2.5f, 2.5f, 2.5f, 6.0f);
        Bloc.DisplayName = LOCTEXT("Bloc_Eastern", "Восточная коалиция");
        Bloc.Motto = LOCTEXT("Motto_Eastern", "Великое объединение. Индустрия победы.");
        Bloc.Description = LOCTEXT("Desc_Eastern", "Колоссальные производственные мощности, автоматизация заводов, рои боевых БПЛА и гиперзвуковые ракетные комплексы.");
        Bloc.KeyAdvantages = {
            LOCTEXT("Adv_Eastern_1", "Массовое автоматизированное производство"),
            LOCTEXT("Adv_Eastern_2", "Автономные рои ударных БПЛА и дронов-камикадзе"),
            LOCTEXT("Adv_Eastern_3", "Зоны воспрещения доступа A2/AD и ракетные рубежи")
        };
        Bloc.ControlledRegions = 32;
        Bloc.ActivePersonnel = 2450000;
        Bloc.ReadinessRatio = 0.96f;
        Bloc.PrimaryColor = GetBlocPrimaryColor(ERA4FactionTheme::EasternCoalition);
        Bloc.GlowColor = GetBlocGlowColor(ERA4FactionTheme::EasternCoalition);

        // China
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("CN");
            Country.LayoutWeight = 2.20f;
            Country.DisplayName = LOCTEXT("Country_CN", "Китай");
            Country.Specialization = LOCTEXT("Spec_CN", "Массовое производство · Ракеты · Дроны");
            Country.BaseHeadquarters = LOCTEXT("HQ_CN", "Умный командный завод «Тяньмэнь»");
            Country.LoreDescription = LOCTEXT("Lore_CN", "Промышленный гигант и технологический лидер Коалиции. Автоматизированные «умные заводы», массовая бронетехника и гиперзвуковые ракетные рубежи.");
            Country.BlocTheme = ERA4FactionTheme::EasternCoalition;
            Country.PrimaryColor = FLinearColor(0.06f, 0.40f, 0.26f, 1.0f);
            Country.AccentColor = FLinearColor(0.88f, 0.72f, 0.22f, 1.0f);
            Country.FirepowerRating = 0.92f;
            Country.ArmorRating = 0.85f;
            Country.MobilityRating = 0.76f;
            Country.TechRating = 0.94f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc1;
            Doc1.DoctrineId = TEXT("CN_DOC_MassMechanization");
            Doc1.CombatPhilosophy = LOCTEXT("Phil_CN_DOC_MassMechanization", "Масштаб производства как оружие");
            Doc1.SignatureSuperweapon = LOCTEXT("Super_CN_DOC_MassMechanization", "Ускоренный выпуск бронетехники");
            Doc1.DisplayName = LOCTEXT("Doc_CN_1", "Массовая механизация");
            Doc1.Description = LOCTEXT("Doc_CN_1_Desc", "Ускоренный конвейерный выпуск тяжёлых танков Тип-99B и БМП при сниженной себестоимости.");
            Doc1.ModifiedUnits = { LOCTEXT("CN_U1", "Танк Тип-99B"), LOCTEXT("CN_U2", "БМП Тип-08") };
            Doc1.SignatureUnit = LOCTEXT("CN_Sig1", "ОБТ Тип-99B «Дракон»");
            Country.Doctrines.Add(Doc1);

            FRA4DoctrineInfo Doc2;
            Doc2.DoctrineId = TEXT("CN_DOC_DroneWarfare");
            Doc2.CombatPhilosophy = LOCTEXT("Phil_CN_DOC_DroneWarfare", "Сеть видит и бьёт как единое целое");
            Doc2.SignatureSuperweapon = LOCTEXT("Super_CN_DOC_DroneWarfare", "Полная синхронизация боевых узлов");
            Doc2.DisplayName = LOCTEXT("Doc_CN_2", "Беспилотная война");
            Doc2.Description = LOCTEXT("Doc_CN_2_Desc", "Роевые квадрокоптеры, мобильные фабрики дронов и автономная воздушная поддержка.");
            Doc2.ModifiedUnits = { LOCTEXT("CN_U3", "Рой дронов «Шэньлун»"), LOCTEXT("CN_U4", "Мобильная фабрика БПЛА") };
            Doc2.SignatureUnit = LOCTEXT("CN_Sig2", "Комплекс «Шэньлун-4»");
            Country.Doctrines.Add(Doc2);

            FRA4DoctrineInfo Doc3;
            Doc3.DoctrineId = TEXT("CN_DOC_MissileZoneDefense");
            Doc3.CombatPhilosophy = LOCTEXT("Phil_CN_DOC_MissileZoneDefense", "Зона, куда противнику невыгодно входить");
            Doc3.SignatureSuperweapon = LOCTEXT("Super_CN_DOC_MissileZoneDefense", "Гиперзвуковой зональный залп");
            Doc3.DisplayName = LOCTEXT("Doc_CN_3", "Ракетно-зональная оборона");
            Doc3.Description = LOCTEXT("Doc_CN_3_Desc", "Зоны воспрещения A2/AD, гиперзвуковые комплексы «Дунфэн» и дальний перехват кораблей.");
            Doc3.ModifiedUnits = { LOCTEXT("CN_U5", "Комплекс «Дунфэн-26»"), LOCTEXT("CN_U6", "ЗРК HQ-9B") };
            Doc3.SignatureUnit = LOCTEXT("CN_Sig3", "ГПРК DF-26D");
            Country.Doctrines.Add(Doc3);

            Bloc.Countries.Add(Country);
        }

        // DPRK
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("KP");
            Country.LayoutWeight = 1.10f;
            Country.DisplayName = LOCTEXT("Country_KP", "КНДР");
            Country.Specialization = LOCTEXT("Spec_KP", "Подземные арсеналы · Массовая артиллерия");
            Country.BaseHeadquarters = LOCTEXT("HQ_KP", "Скальный командный бункер «Пэкту»");
            Country.LoreDescription = LOCTEXT("Lore_KP", "Глубокие скальные бункеры, дальнобойные тяжелые САУ «Коксан» и массированные ракетные залпы.");
            Country.BlocTheme = ERA4FactionTheme::EasternCoalition;
            Country.PrimaryColor = FLinearColor(0.08f, 0.32f, 0.22f, 1.0f);
            Country.AccentColor = FLinearColor(0.82f, 0.65f, 0.18f, 1.0f);
            Country.FirepowerRating = 0.90f;
            Country.ArmorRating = 0.72f;
            Country.MobilityRating = 0.55f;
            Country.TechRating = 0.70f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc;
            Doc.DoctrineId = TEXT("KP_DOC_ArtilleryStorm");
            Doc.CombatPhilosophy = LOCTEXT("Phil_KP_DOC_ArtilleryStorm", "Плотность огня выше точности");
            Doc.SignatureSuperweapon = LOCTEXT("Super_KP_DOC_ArtilleryStorm", "Массированный артиллерийский шторм");
            Doc.DisplayName = LOCTEXT("Doc_KP_1", "Артиллерийский шторм");
            Doc.Description = LOCTEXT("Doc_KP_1_Desc", "Сверхдальние артиллерийские удары из защищённых укрытий.");
            Country.Doctrines.Add(Doc);
            Bloc.Countries.Add(Country);
        }

        Blocs.Add(Bloc);
    }

    // =========================================================================
    // 4. ТИХООКЕАНСКИЙ ПАКТ (PACIFIC PACT)
    // =========================================================================
    {
        FRA4BlocInfo Bloc;
        Bloc.BlocId = ERA4FactionTheme::PacificPact;
        // Островная оборона: лёгкая рамка со смещением к морскому краю.
        Bloc.LayoutWeight = 0.95f;
        Bloc.PanelDensity = ERA4PanelRole::Compact;
        Bloc.FrameRail = FMargin(1.0f, 1.0f, 3.5f, 1.0f);
        Bloc.DisplayName = LOCTEXT("Bloc_Pacific", "Тихоокеанский пакт");
        Bloc.Motto = LOCTEXT("Motto_Pacific", "Оборона рубежей. Роботизация будущего.");
        Bloc.Description = LOCTEXT("Desc_Pacific", "Морская и островная оборона, передовая автономная робототехника, лазерные комплексы и мобильные экспедиционные группы.");
        Bloc.KeyAdvantages = {
            LOCTEXT("Adv_Pacific_1", "Автономные боевые шагоходы и дроны поддержки"),
            LOCTEXT("Adv_Pacific_2", "Эшелонированное островное ПВО и лазерные щиты"),
            LOCTEXT("Adv_Pacific_3", "Высокоскоростной москитный и эскортный флот")
        };
        Bloc.ControlledRegions = 19;
        Bloc.ActivePersonnel = 850000;
        Bloc.ReadinessRatio = 0.92f;
        Bloc.PrimaryColor = GetBlocPrimaryColor(ERA4FactionTheme::PacificPact);
        Bloc.GlowColor = GetBlocGlowColor(ERA4FactionTheme::PacificPact);

        // Japan
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("JP");
            Country.LayoutWeight = 2.20f;
            Country.DisplayName = LOCTEXT("Country_JP", "Япония");
            Country.Specialization = LOCTEXT("Spec_JP", "Робототехника · ПВО · Морская война");
            Country.BaseHeadquarters = LOCTEXT("HQ_JP", "Островной командный терминал «Кайган»");
            Country.LoreDescription = LOCTEXT("Lore_JP", "Технологический авангард островной обороны. Автономные боевые роботы «Кайган», лазерные батареи перехвата и скоростные перехватчики.");
            Country.BlocTheme = ERA4FactionTheme::PacificPact;
            Country.PrimaryColor = FLinearColor(0.04f, 0.48f, 0.58f, 1.0f);
            Country.AccentColor = FLinearColor(0.95f, 0.42f, 0.32f, 1.0f);
            Country.FirepowerRating = 0.84f;
            Country.ArmorRating = 0.72f;
            Country.MobilityRating = 0.95f;
            Country.TechRating = 0.99f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc1;
            Doc1.DoctrineId = TEXT("JP_DOC_IslandDefense");
            Doc1.CombatPhilosophy = LOCTEXT("Phil_JP_DOC_IslandDefense", "Каждый остров — укреплённый рубеж");
            Doc1.SignatureSuperweapon = LOCTEXT("Super_JP_DOC_IslandDefense", "Многослойный лазерный перехват");
            Doc1.DisplayName = LOCTEXT("Doc_JP_1", "Оборона островов");
            Doc1.Description = LOCTEXT("Doc_JP_1_Desc", "Береговые лазерные батареи, сверхдальние комплексы ПВО «Акацуки» и защитные энергокупола.");
            Doc1.ModifiedUnits = { LOCTEXT("JP_U1", "Лазерный комплекс «Кагами»"), LOCTEXT("JP_U2", "ЗРК «Акацуки»") };
            Doc1.SignatureUnit = LOCTEXT("JP_Sig1", "Лазерная батарея «Кагами-3»");
            Country.Doctrines.Add(Doc1);

            FRA4DoctrineInfo Doc2;
            Doc2.DoctrineId = TEXT("JP_DOC_RoboticForces");
            Doc2.CombatPhilosophy = LOCTEXT("Phil_JP_DOC_RoboticForces", "Машина идёт туда, где рискует человек");
            Doc2.SignatureSuperweapon = LOCTEXT("Super_JP_DOC_RoboticForces", "Автономное звено роботов «Кайган»");
            Doc2.DisplayName = LOCTEXT("Doc_JP_2", "Роботизированные силы");
            Doc2.Description = LOCTEXT("Doc_JP_2_Desc", "Автономные двуногие платформы «Кайган», ремонтные нанодроны и автономные перехватчики.");
            Doc2.ModifiedUnits = { LOCTEXT("JP_U3", "Шагоход «Кайган»"), LOCTEXT("JP_U4", "Дрон РЭБ «Цуру»") };
            Doc2.SignatureUnit = LOCTEXT("JP_Sig2", "Боевой робот «Кайган-II»");
            Country.Doctrines.Add(Doc2);

            FRA4DoctrineInfo Doc3;
            Doc3.DoctrineId = TEXT("JP_DOC_NavalExpedition");
            Doc3.CombatPhilosophy = LOCTEXT("Phil_JP_DOC_NavalExpedition", "Море соединяет, а не разделяет");
            Doc3.SignatureSuperweapon = LOCTEXT("Super_JP_DOC_NavalExpedition", "Морская экспедиционная переброска");
            Doc3.DisplayName = LOCTEXT("Doc_JP_3", "Морская экспедиция");
            Doc3.Description = LOCTEXT("Doc_JP_3_Desc", "Ракетные эсминцы класса «Майя», перехватчики «Сирокко» и мобильные вертолётоносцы.");
            Doc3.ModifiedUnits = { LOCTEXT("JP_U5", "Эсминец «Майя»"), LOCTEXT("JP_U6", "Перехватчик «Сирокко»") };
            Doc3.SignatureUnit = LOCTEXT("JP_Sig3", "Истребитель «Сирокко»");
            Country.Doctrines.Add(Doc3);

            Bloc.Countries.Add(Country);
        }

        // South Korea
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("KR");
            Country.LayoutWeight = 1.15f;
            Country.DisplayName = LOCTEXT("Country_KR", "Южная Корея");
            Country.Specialization = LOCTEXT("Spec_KR", "Сетевая артиллерия · Экзоскелеты");
            Country.BaseHeadquarters = LOCTEXT("HQ_KR", "Сетевой командный центр «Ханган»");
            Country.BlocTheme = ERA4FactionTheme::PacificPact;
            Country.PrimaryColor = FLinearColor(0.06f, 0.42f, 0.52f, 1.0f);
            Country.AccentColor = FLinearColor(0.92f, 0.48f, 0.38f, 1.0f);
            Country.FirepowerRating = 0.88f;
            Country.ArmorRating = 0.76f;
            Country.MobilityRating = 0.86f;
            Country.TechRating = 0.94f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc;
            Doc.DoctrineId = TEXT("KR_DOC_NetworkArtillery");
            Doc.CombatPhilosophy = LOCTEXT("Phil_KR_DOC_NetworkArtillery", "Связанная батарея бьёт первой");
            Doc.SignatureSuperweapon = LOCTEXT("Super_KR_DOC_NetworkArtillery", "Сетевой контрбатарейный залп K9");
            Doc.DisplayName = LOCTEXT("Doc_KR_1", "Сетевая САУ K9 и экзопехота");
            Doc.Description = LOCTEXT("Doc_KR_1_Desc", "Автоматизированные батареи САУ K9A2 и штурмовая пехота в силовых экзоскелетах.");
            Country.Doctrines.Add(Doc);
            Bloc.Countries.Add(Country);
        }

        // Australia
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("AU");
            Country.LayoutWeight = 1.00f;
            Country.DisplayName = LOCTEXT("Country_AU", "Австралия");
            Country.Specialization = LOCTEXT("Spec_AU", "Дальняя разведка · Океанский флот");
            Country.BaseHeadquarters = LOCTEXT("HQ_AU", "Прибрежная база патрулирования «Reef»");
            Country.BlocTheme = ERA4FactionTheme::PacificPact;
            Country.PrimaryColor = FLinearColor(0.08f, 0.45f, 0.48f, 1.0f);
            Country.AccentColor = FLinearColor(0.90f, 0.55f, 0.30f, 1.0f);
            Country.FirepowerRating = 0.80f;
            Country.ArmorRating = 0.70f;
            Country.MobilityRating = 0.92f;
            Country.TechRating = 0.88f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc;
            Doc.DoctrineId = TEXT("AU_DOC_OceanPatrol");
            Doc.CombatPhilosophy = LOCTEXT("Phil_AU_DOC_OceanPatrol", "Океан контролируется без экипажа");
            Doc.SignatureSuperweapon = LOCTEXT("Super_AU_DOC_OceanPatrol", "Патруль беспилотных субмарин");
            Doc.DisplayName = LOCTEXT("Doc_AU_1", "Океанский дозор и беспилотные субмарины");
            Doc.Description = LOCTEXT("Doc_AU_1_Desc", "Дальний радарный контроль морских путей и автономные подводные дроны.");
            Country.Doctrines.Add(Doc);
            Bloc.Countries.Add(Country);
        }

        Blocs.Add(Bloc);
    }

    // =========================================================================
    // 5. НЕЗАВИСИМЫЕ ДЕРЖАВЫ (INDEPENDENT POWERS)
    // Внимание: это категория выбора самостоятельных стран, а не единый блок!
    // =========================================================================
    {
        FRA4BlocInfo Bloc;
        Bloc.BlocId = ERA4FactionTheme::Independent;
        // Не союз: рамка намеренно разорвана по вертикали, единого щита нет.
        Bloc.LayoutWeight = 0.95f;
        Bloc.PanelDensity = ERA4PanelRole::Compact;
        Bloc.FrameRail = FMargin(3.0f, 1.0f, 3.0f, 1.0f);
        Bloc.DisplayName = LOCTEXT("Bloc_Independent", "Независимые державы");
        Bloc.Motto = LOCTEXT("Motto_Independent", "Категория выбора • Не является союзом");
        Bloc.bIsCategoryOnly = true;
        Bloc.Description = LOCTEXT("Desc_Independent", "Самостоятельные региональные государства с уникальными национальными доктринами, собственной палитрой и независимой внешней политикой.");
        Bloc.KeyAdvantages = {
            LOCTEXT("Adv_Indep_1", "Уникальный национальный состав вооружения"),
            LOCTEXT("Adv_Indep_2", "Асимметричные тактики и адаптированные технологии"),
            LOCTEXT("Adv_Indep_3", "Полная независимость от блоковых обязательств")
        };
        Bloc.ControlledRegions = 35;
        Bloc.ActivePersonnel = 1800000;
        Bloc.ReadinessRatio = 0.85f;
        Bloc.PrimaryColor = GetBlocPrimaryColor(ERA4FactionTheme::Independent);
        Bloc.GlowColor = GetBlocGlowColor(ERA4FactionTheme::Independent);

        // Iran
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("IR");
            Country.LayoutWeight = 1.80f;
            Country.DisplayName = LOCTEXT("Country_IR", "Иран");
            Country.Specialization = LOCTEXT("Spec_IR", "Ракеты · БПЛА · Асимметричная война");
            Country.BaseHeadquarters = LOCTEXT("HQ_IR", "Мобильный узел «Мираж»");
            Country.LoreDescription = LOCTEXT("Lore_IR", "Разрозненные мобильные ракетные группы превращают сложный горный рельеф, маскировку и ложные сигналы в единую сеть сдерживания.");
            Country.BlocTheme = ERA4FactionTheme::Independent;
            Country.PrimaryColor = FLinearColor(0.72f, 0.48f, 0.16f, 1.0f); // Desert Ochre / Amber
            Country.AccentColor = FLinearColor(0.20f, 0.78f, 0.72f, 1.0f); // Drone Cyan
            Country.FirepowerRating = 0.86f;
            Country.ArmorRating = 0.60f;
            Country.MobilityRating = 0.88f;
            Country.TechRating = 0.82f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc1;
            Doc1.DoctrineId = TEXT("IR_DOC_MissileForces");
            Doc1.CombatPhilosophy = LOCTEXT("Phil_IR_DOC_MissileForces", "Насыщение важнее одиночного удара");
            Doc1.SignatureSuperweapon = LOCTEXT("Super_IR_DOC_MissileForces", "Сатурационный ракетный залп");
            Doc1.DisplayName = LOCTEXT("Doc_IR_1", "Ракетные войска");
            Doc1.Description = LOCTEXT("Doc_IR_1_Desc", "Мобильные замаскированные пусковые установки «Хейбар» и залповый сатурационный удар.");
            Doc1.ModifiedUnits = { LOCTEXT("IR_U1", "СПУ «Хейбар Шекан»"), LOCTEXT("IR_U2", "Мобильный узел «Мираж»") };
            Doc1.KeyAbilities = { LOCTEXT("IR_A1", "Сатурационный залп"), LOCTEXT("IR_A2", "Ложная позиция") };
            Doc1.SignatureUnit = LOCTEXT("IR_Sig1", "СПУ «Хейбар-2»");
            Country.Doctrines.Add(Doc1);

            FRA4DoctrineInfo Doc2;
            Doc2.DoctrineId = TEXT("IR_DOC_DroneAsymmetric");
            Doc2.CombatPhilosophy = LOCTEXT("Phil_IR_DOC_DroneAsymmetric", "Рассеяться, ударить, сменить позицию");
            Doc2.SignatureSuperweapon = LOCTEXT("Super_IR_DOC_DroneAsymmetric", "Дроновый рой и ложные позиции");
            Doc2.DisplayName = LOCTEXT("Doc_IR_2", "БПЛА и асимметричная война");
            Doc2.Description = LOCTEXT("Doc_IR_2_Desc", "Рои малозаметных дронов-камикадзе «Шахед», скрытные подземные укрытия и скоростные удары.");
            Doc2.ModifiedUnits = { LOCTEXT("IR_U3", "Дрон-камикадзе «Шахед-136»"), LOCTEXT("IR_U4", "Разведчик «Мохаджер-6»") };
            Doc2.KeyAbilities = { LOCTEXT("IR_A3", "Роевая атака"), LOCTEXT("IR_A4", "Скрытное развёртывание") };
            Doc2.SignatureUnit = LOCTEXT("IR_Sig2", "БПЛА «Шахед-136М»");
            Country.Doctrines.Add(Doc2);

            Bloc.Countries.Add(Country);
        }

        // Israel
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("IL");
            Country.LayoutWeight = 1.10f;
            Country.DisplayName = LOCTEXT("Country_IL", "Израиль");
            Country.Specialization = LOCTEXT("Spec_IL", "Разведка и превентивный удар · ПВО");
            Country.BaseHeadquarters = LOCTEXT("HQ_IL", "Заглублённый центр управления «Мигдаль»");
            Country.LoreDescription = LOCTEXT("Lore_IL", "Многоуровневая эшелонированная система ПВО («Железный купол», «Праща Давида»), танки «Меркава Mk.5» с КАЗ «Трофи» и точечные превентивные удары.");
            Country.BlocTheme = ERA4FactionTheme::Independent;
            Country.PrimaryColor = FLinearColor(0.20f, 0.38f, 0.62f, 1.0f); // Slate Iron Blue
            Country.AccentColor = FLinearColor(0.92f, 0.72f, 0.20f, 1.0f); // Amber Gold
            Country.FirepowerRating = 0.90f;
            Country.ArmorRating = 0.88f;
            Country.MobilityRating = 0.75f;
            Country.TechRating = 0.96f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc1;
            Doc1.DoctrineId = TEXT("IL_DOC_LayeredDefense");
            Doc1.CombatPhilosophy = LOCTEXT("Phil_IL_DOC_LayeredDefense", "Перехват на каждом эшелоне");
            Doc1.SignatureSuperweapon = LOCTEXT("Super_IL_DOC_LayeredDefense", "Многоуровневый купол ПВО");
            Doc1.DisplayName = LOCTEXT("Doc_IL_1", "Многоуровневая оборона и ПВО");
            Doc1.Description = LOCTEXT("Doc_IL_1_Desc", "Комплексы «Железный купол», активная защита бронетехники «Трофи» и лазерный перехват «Железный луч».");
            Doc1.ModifiedUnits = { LOCTEXT("IL_U1", "ОБТ «Меркава Mk.5»"), LOCTEXT("IL_U2", "Батарея «Железный купол»") };
            Doc1.SignatureUnit = LOCTEXT("IL_Sig1", "ОБТ «Меркава-5 Барак»");
            Country.Doctrines.Add(Doc1);

            Bloc.Countries.Add(Country);
        }

        // Saudi Arabia
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("SA");
            Country.LayoutWeight = 1.00f;
            Country.DisplayName = LOCTEXT("Country_SA", "Саудовская Аравия");
            Country.Specialization = LOCTEXT("Spec_SA", "Тяжёлая экспедиционная техника · Защита инфраструктуры");
            Country.BaseHeadquarters = LOCTEXT("HQ_SA", "Пустынный командный комплекс «Наджд»");
            Country.BlocTheme = ERA4FactionTheme::Independent;
            Country.PrimaryColor = FLinearColor(0.68f, 0.58f, 0.32f, 1.0f); // Desert Sand Tan
            Country.AccentColor = FLinearColor(0.18f, 0.68f, 0.38f, 1.0f); // Emerald
            Country.FirepowerRating = 0.82f;
            Country.ArmorRating = 0.80f;
            Country.MobilityRating = 0.78f;
            Country.TechRating = 0.84f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc;
            Doc.DoctrineId = TEXT("SA_DOC_HeavyDesert");
            Doc.CombatPhilosophy = LOCTEXT("Phil_SA_DOC_HeavyDesert", "Пустыня прощает только тяжёлую броню");
            Doc.SignatureSuperweapon = LOCTEXT("Super_SA_DOC_HeavyDesert", "Тяжёлый пустынный бронекулак");
            Doc.DisplayName = LOCTEXT("Doc_SA_1", "Тяжёлый пустынный корпус");
            Doc.Description = LOCTEXT("Doc_SA_1_Desc", "Мобильная бронетехника с усиленным охлаждением и охрана месторождений.");
            Country.Doctrines.Add(Doc);
            Bloc.Countries.Add(Country);
        }

        // Brazil
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("BR");
            Country.LayoutWeight = 1.00f;
            Country.DisplayName = LOCTEXT("Country_BR", "Бразилия");
            Country.Specialization = LOCTEXT("Spec_BR", "Речная маневренность · Региональный доминион");
            Country.BaseHeadquarters = LOCTEXT("HQ_BR", "Речной командный понтон «Амазонас»");
            Country.BlocTheme = ERA4FactionTheme::Independent;
            Country.PrimaryColor = FLinearColor(0.12f, 0.52f, 0.24f, 1.0f); // Tropical Forest Green
            Country.AccentColor = FLinearColor(0.92f, 0.78f, 0.12f, 1.0f); // Sun Gold
            Country.FirepowerRating = 0.78f;
            Country.ArmorRating = 0.65f;
            Country.MobilityRating = 0.92f;
            Country.TechRating = 0.80f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc;
            Doc.DoctrineId = TEXT("BR_DOC_RiverJungle");
            Doc.CombatPhilosophy = LOCTEXT("Phil_BR_DOC_RiverJungle", "Река — это дорога, лес — это укрытие");
            Doc.SignatureSuperweapon = LOCTEXT("Super_BR_DOC_RiverJungle", "Речная десантная переброска");
            Doc.DisplayName = LOCTEXT("Doc_BR_1", "Речная и лесная маневренность");
            Doc.Description = LOCTEXT("Doc_BR_1_Desc", "Бронекатера, амфибийная лёгкая техника «Гуарани» и скрытные лесные засады.");
            Country.Doctrines.Add(Doc);
            Bloc.Countries.Add(Country);
        }

        // India
        {
            FRA4CountryInfo Country;
            Country.CountryId = TEXT("IN");
            Country.LayoutWeight = 1.15f;
            Country.DisplayName = LOCTEXT("Country_IN", "Индия");
            Country.Specialization = LOCTEXT("Spec_IN", "Горная оборона · Комбинированный арсенал");
            Country.BaseHeadquarters = LOCTEXT("HQ_IN", "Горный командный комплекс «Гималай»");
            Country.BlocTheme = ERA4FactionTheme::Independent;
            Country.PrimaryColor = FLinearColor(0.82f, 0.42f, 0.14f, 1.0f); // Saffron Amber
            Country.AccentColor = FLinearColor(0.12f, 0.22f, 0.52f, 1.0f); // Deep Blue
            Country.FirepowerRating = 0.88f;
            Country.ArmorRating = 0.82f;
            Country.MobilityRating = 0.70f;
            Country.TechRating = 0.84f;
            Country.bUnlocked = true;

            FRA4DoctrineInfo Doc;
            Doc.DoctrineId = TEXT("IN_DOC_MountainArsenal");
            Doc.CombatPhilosophy = LOCTEXT("Phil_IN_DOC_MountainArsenal", "Высота даёт и защиту, и дальность");
            Doc.SignatureSuperweapon = LOCTEXT("Super_IN_DOC_MountainArsenal", "Горный ударный корпус");
            Doc.DisplayName = LOCTEXT("Doc_IN_1", "Горные ударные корпуса");
            Doc.Description = LOCTEXT("Doc_IN_1_Desc", "Высокогорная артиллерия и сверхзвуковые ракеты «БраМос».");
            Country.Doctrines.Add(Doc);
            Bloc.Countries.Add(Country);
        }

        Blocs.Add(Bloc);
    }
}

const FRA4BlocInfo* FRA4FactionDataRegistry::FindBloc(ERA4FactionTheme Theme) const
{
    for (const FRA4BlocInfo& Bloc : Blocs)
    {
        if (Bloc.BlocId == Theme)
        {
            return &Bloc;
        }
    }
    return Blocs.Num() > 0 ? &Blocs[0] : nullptr;
}

const FRA4CountryInfo* FRA4FactionDataRegistry::FindCountry(FName CountryId) const
{
    for (const FRA4BlocInfo& Bloc : Blocs)
    {
        for (const FRA4CountryInfo& Country : Bloc.Countries)
        {
            if (Country.CountryId == CountryId)
            {
                return &Country;
            }
        }
    }
    return nullptr;
}

const FRA4CountryInfo* FRA4FactionDataRegistry::FindDefaultCountryForBloc(ERA4FactionTheme Theme) const
{
    const FRA4BlocInfo* Bloc = FindBloc(Theme);
    if (Bloc && Bloc->Countries.Num() > 0)
    {
        return &Bloc->Countries[0];
    }
    return nullptr;
}

#undef LOCTEXT_NAMESPACE
