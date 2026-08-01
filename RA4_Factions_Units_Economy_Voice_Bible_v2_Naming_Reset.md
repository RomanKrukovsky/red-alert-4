# RA4 — polnyy dizayn fraktsiy, ekonomiki, yunitov i ozvuchki
**Status dokumenta:** bazovaya production-bibliya for realizatsii v Unreal Engine 5.  
**Version:** 2.0 — Naming Reset  
**Yazyk:** russkiy  
**Naznachenie:** edinyy istochnik istiny for geymdizayna, Data Assets, balansa, UI, AI, ozvuchki EVA i yunitov.  
**Rabochie Factions:** Soviet Union, Alliance, Vostochnaya Coalition, Khronolegion.  
**Primechanie po IP:** Version 2.0 provodit polnyy naming reset rostera: pryamye i slishkom uznavaemye nazvaniya iz Command & Conquer: Red Alert i Generals udaleny iz otobrazhaemykh imyon i Stable ID. Novye oboznacheniya vdokhnovleny realnymi rossiysko-sovetskimi, amerikanskimi/natovskimi, kitayskimi, yaponskimi i indiyskimi traditsiyami voennoy nomenklatury, no ne kopiruyut konkretnye seriynye obraztsy odin v odin. Starye ID sokhraneny only v tablitse LegacyAliases for migratsii.

## 0. Kratkaya ideya igry
RA4 — bystraya asimmetrichnaya RTS o globalnoy voyne chetyryokh tekhnologicheskikh blokov. Match stroitsya vokrug klassicheskoy formuly: razvernut bazu, zakhvatit ekonomiku karty, razvedat protivnika, pereyti v T2, sorvat ego tekhnologicheskiy skachok, a zatem zakonchit igru kombinirovannym udarom armii, aviatsii, flota i strategicheskikh sposobnostey. Kazhdaya fraktsiya obyazana imet ponyatnuyu silnuyu storonu, realnuyu slabost i sobstvennyy ritm ekonomiki. Ni odna fraktsiya ne dolzhna vyigryvat only za schyot bolee vysokikh chisel.

## 0.1. Sistema imenovaniya versii 2.0
Novaya nomenklatura otdelyaet Project ot starykh rosterov Command & Conquer i odnovremenno delaet armii pokhozhimi na produktsiyu raznykh voenno-promyshlennykh shkol. Soviet Union ispolzuet russkoyazychnye indeksy i voyskovye prozvishcha; Alliance — amerikanskie i natovskie bukvenno-tsifrovye oboznacheniya s korotkimi angloyazychnymi callsign; Vostochnaya Coalition — smeshannuyu kitaysko-yaponsko-indiyskuyu sistemu programm i kulturnykh imyon; Khronolegion — kody zakrytykh temporalnykh proektov. Vnutrenniy Stable ID bolshe ne dolzhen soderzhat staroe rabochee imya. Starye ID dopustimy only kak `LegacyAliases` for odnorazovoy migratsii uzhe sozdannykh assetov, ssylok, testov, repleev i SaveGame.

### 0.1.1. Pravila
1. Odno boevoe imya prinadlezhit only odnomu yunitu.
2. Nazvanie ne dolzhno sovpadat s yunitom Command & Conquer v toy zhe roli.
3. Realnye indeksy i nazvaniya ispolzuyutsya lish kak printsip postroeniya; konkretnyy realnyy obrazets ne kopiruetsya odin v odin.
4. `UnitId`, `VoiceId`, imya Primary Data Asset i Gameplay Tags stroyatsya ot novogo Stable ID.
5. Russkaya lokalizatsiya mozhet proiznosit transliterirovannyy callsign Alyansa, no iskhodnoe latinskoe imya sokhranyaetsya v UI i entsiklopedii.
6. after migratsii starye imena ne otobrazhayutsya igroku i ne ispolzuyutsya with sozdanii novogo kontenta.

## 0.2. Karta migratsii LegacyAliases
| Fraktsiya | Staroe rabochee imya | Staryy Stable ID | Novoe polnoe imya | Novyy Stable ID |
| --- | --- | --- | --- | --- |
| Soviet Union | Conscript | `SU_Conscript` | Motostrelok MS-12 «Rubezh» | `SU_RubezhRifleman` |
| Soviet Union | Grenadyor | `SU_Grenadier` | Shturmovik OSh-4 «Zapal» | `SU_ZapalGrenadier` |
| Soviet Union | Zenitchik | `SU_FlakTrooper` | Zenitnyy raschyot PZK-9 «Zaslon» | `SU_ZaslonAATeam` |
| Soviet Union | Boevoy Engineer | `SU_Engineer` | Engineer-sapyor IS-3 «Master» | `SU_MasterEngineer` |
| Soviet Union | Shturmovik «Grom» | `SU_ShockTrooper` | Elektroshturmovik ESh-8 «Razryad» | `SU_RazryadTrooper` |
| Soviet Union | Komissar svyazi | `SU_Commissar` | Ofitser svyazi KS-6 «Vektor» | `SU_VektorOfficer` |
| Soviet Union | Harvester «Bogatyr» | `SU_Harvester` | Gornorudnaya mashina GRM-8 «Bogatyr» | `SU_BogatyrOreCarrier` |
| Soviet Union | Razvedmashina «Serp» | `SU_SickleScout` | Boevaya razvedmashina BRM-27 «Rys» | `SU_RysScout` |
| Soviet Union | Tank «Molot» | `SU_HammerTank` | Osnovnoy Tank OBT-92 «Granit» | `SU_GranitMBT` |
| Soviet Union | RSZO «Buratino» | `SU_Buratino` | Termobaricheskaya RSZO TRS-18 «Zarevo» | `SU_ZarevoMLRS` |
| Soviet Union | Tesla-taran «Perun» | `SU_TeslaRam` | Elektrotaran ETM-7 «Gromoboy» | `SU_GromoboyRam` |
| Soviet Union | Tank «Apokalipsis» | `SU_Apocalypse` | Tyazhyolyy Tank proryva TTP-11 «Voevoda» | `SU_VoevodaHeavyTank` |
| Soviet Union | Istrebitel MiG-41 | `SU_MiG41` | Istrebitel I-47 «Krechet» | `SU_KrechetInterceptor` |
| Soviet Union | Shturmovik «Khind-M» | `SU_Hind` | Shturmovoy vertolyot ShV-38 «Korshun» | `SU_KorshunGunship` |
| Soviet Union | Dirizhabl «Kirovets» | `SU_Kirov` | Tyazhyolyy dirizhabl TDA-8 «Gromada» | `SU_GromadaAirship` |
| Soviet Union | Kater «Skat» | `SU_StingrayBoat` | Boevoy kater BK-27 «Buran» | `SU_BuranPatrolBoat` |
| Soviet Union | Podlodka «Tayfun» | `SU_TyphoonSub` | Udarnaya podlodka UPL-90 «Morok» | `SU_MorokSubmarine` |
| Soviet Union | Raketnyy kreyser «Drednout» | `SU_Dreadnought` | Raketnyy kreyser RKR-44 «Svyatogor» | `SU_SvyatogorCruiser` |
| Soviet Union | Mayor Elena Morozova | `SU_Hero_Morozova` | Mayor Elena Morozova | `SU_Hero_Morozova` |
| Alliance | Mirotvorets | `AL_Peacekeeper` | Strelok M6 «Sentinel» | `AL_SentinelRifleman` |
| Alliance | Raschyot «Dzhavelin» | `AL_Javelin` | Raketnyy raschyot FGM-31 «Lancer» | `AL_LancerTeam` |
| Alliance | Polevoy Engineer | `AL_Engineer` | Polevoy Engineer E-4 | `AL_FieldEngineer` |
| Alliance | Sledopyt | `AL_Pathfinder` | Snayper R-9 «Longwatch» | `AL_LongwatchSniper` |
| Alliance | Polevoy medik | `AL_Medic` | Polevoy medik M-12 «Lifeline» | `AL_LifelineMedic` |
| Alliance | Kriospetsialist | `AL_CryoSpecialist` | Spetsialist C-7 «Frostline» | `AL_FrostlineSpecialist` |
| Alliance | Harvester «Prospektor» | `AL_Prospector` | Dobyvayushchaya platforma M88 «Pioneer» | `AL_PioneerHarvester` |
| Alliance | Razvedchik «Shakal» | `AL_Jackal` | Razvedmashina LAV-41 «Kestrel» | `AL_KestrelScout` |
| Alliance | Tank «Gardian» | `AL_Guardian` | Osnovnoy Tank M14 «Bulwark» | `AL_BulwarkMBT` |
| Alliance | Relsovaya SAU «Afina» | `AL_Athena` | Relsovaya SAU XM190 «Oracle» | `AL_OracleArtillery` |
| Alliance | Tank «Mirazh» | `AL_Mirage` | Maskirovochnyy Tank XM27 «Refraction» | `AL_RefractionTank` |
| Alliance | Mobilnyy shchit «Egida» | `AL_AegisShield` | Mobilnyy shchit M46 «Ward» | `AL_WardShieldCarrier` |
| Alliance | Tank «Paladin» | `AL_Paladin` | Tyazhyolyy Tank M70 «Citadel» | `AL_CitadelTank` |
| Alliance | Istrebitel «Falkon» | `AL_Falcon` | Istrebitel F/A-48 «Shrike» | `AL_ShrikeInterceptor` |
| Alliance | VTOL «Kharrier» | `AL_Harrier` | VTOL AV-27 «Vector» | `AL_VectorVTOL` |
| Alliance | Stels-bombardirovshchik «Spektr» | `AL_Specter` | Stels-bombardirovshchik B-39 «Nightveil» | `AL_NightveilBomber` |
| Alliance | Gidrofoyl «Riptayd» | `AL_Hydrofoil` | Gidrofoyl PHM-22 «Manta» | `AL_MantaPatrolCraft` |
| Alliance | Esminets «Triton» | `AL_Triton` | Esminets DDG-31 «Resolute» | `AL_ResoluteDestroyer` |
| Alliance | Avianosets «Poseydon» | `AL_Poseidon` | Avianosets CVX-90 «Horizon» | `AL_HorizonCarrier` |
| Alliance | Agent Evelin Khart | `AL_Hero_Hart` | Agent Evelin Khart | `AL_Hero_Hart` |
| Vostochnaya Coalition | Avangard | `CO_Vanguard` | Strelok Tip 21 «Tsyanvey» | `CO_QianweiRifleman` |
| Vostochnaya Coalition | Kopeyshchik buri | `CO_StormLancer` | Protivotankovyy raschyot AT-8 «Vadzhra» | `CO_VajraLancer` |
| Vostochnaya Coalition | Tekhnik seti | `CO_Technician` | Tekhnik seti Tip 06 «Tsze» | `CO_JieTechnician` |
| Vostochnaya Coalition | Fazovyy strelok | `CO_PhaseArcher` | Fazovyy strelok QBS-19 «Shengun» | `CO_ShengongMarksman` |
| Vostochnaya Coalition | Nanitnyy medik | `CO_NaniteMedic` | Nanitnyy medik NM-7 «Sandzhivani» | `CO_SanjivaniMedic` |
| Vostochnaya Coalition | Pochyotnyy strazh | `CO_HonorGuard` | Pochyotnyy strazh HG-33 «Raksha» | `CO_RakshaGuard` |
| Vostochnaya Coalition | Harvester «Sobiratel» | `CO_Collector` | Dobyvayushchaya platforma GRP-12 «Yuan» | `CO_YuanCollector` |
| Vostochnaya Coalition | Shagokhod «Bogomol» | `CO_Mantis` | Razvedshagokhod Tip 17 «Kamakiri» | `CO_KamakiriWalker` |
| Vostochnaya Coalition | Tank «Nefrit» | `CO_JadeTank` | Osnovnoy Tank ZTZ-61 «Tsinlun» | `CO_QinglongMBT` |
| Vostochnaya Coalition | Artilleriya «Lotos» | `CO_LotusArtillery` | Artilleriya PHL-29 «Musson» | `CO_MonsoonArtillery` |
| Vostochnaya Coalition | Shchitovoy nositel «Bastion» | `CO_Bastion` | Shchitovoy nositel Tip 42 «Seymon» | `CO_SeimonShieldCarrier` |
| Vostochnaya Coalition | Shturmovoy shagokhod «Kirin» | `CO_Kirin` | Shturmovoy shagokhod MBT-X «Ayravata» | `CO_AiravataWalker` |
| Vostochnaya Coalition | Mobilnaya krepost «Nebesnyy dvorets» | `CO_CelestialFortress` | Mobilnaya krepost ZTD-90 «Tyanmen» | `CO_TianmenFortress` |
| Vostochnaya Coalition | Dron «Strizh» | `CO_SwiftDrone` | Razveddron UAV-12 «Kavasemi» | `CO_KawasemiDrone` |
| Vostochnaya Coalition | Shturmovik «Gromovoy zhuravl» | `CO_ThunderCrane` | Shturmovik Z-28 «Leykhe» | `CO_LeiheGunship` |
| Vostochnaya Coalition | Bombardirovshchik «Alyy feniks» | `CO_Vermilion` | Bombardirovshchik H-26 «Agnipaksha» | `CO_AgnipakshaBomber` |
| Vostochnaya Coalition | Korvet «Mech-ryba» | `CO_Swordfish` | Korvet Tip 32 «Kadzekiri» | `CO_KazekiriCorvette` |
| Vostochnaya Coalition | Relsovyy kreyser «Leviafan» | `CO_Leviathan` | Relsovyy kreyser Tip 81 «Syuanu» | `CO_XuanwuCruiser` |
| Vostochnaya Coalition | Podvodnyy avianosets «Dvorets drakona» | `CO_DragonPalace` | Podvodnyy avianosets SSGN-18 «Samudra» | `CO_SamudraCarrier` |
| Vostochnaya Coalition | Commander Mey Tszyan | `CO_Hero_Mei` | Commander Mey Tszyan | `CO_Hero_Mei` |
| Khronolegion | Strelok ekha | `CH_EchoRifleman` | Strelok ECHO-7 «Rezonans» | `CH_ResonanceRifleman` |
| Khronolegion | Fazovyy kopeyshchik | `CH_PhaseLancer` | Kopeyshchik PHASE-L9 «Prokol» | `CH_PunctureLancer` |
| Khronolegion | Temporalnyy Engineer | `CH_Engineer` | Engineer CSE-2 «Redaktor» | `CH_CausalityEngineer` |
| Khronolegion | Peremotchik | `CH_Rewinder` | Operator RWD-3 «Revers» | `CH_ReversalMedic` |
| Khronolegion | Snayper paradoksa | `CH_ParadoxSniper` | Snayper PDX-12 «Aporiya» | `CH_AporiaSniper` |
| Khronolegion | Nulevoy operativnik | `CH_NullOperative` | Operativnik NULL-12 «Tsenzor» | `CH_CensorOperative` |
| Khronolegion | Kvantovyy Harvester | `CH_QuantumHarvester` | Harvester QH-4 «Veroyatnik» | `CH_ProbabilistHarvester` |
| Khronolegion | Razvedchik «Mertsanie» | `CH_BlinkScout` | Razvedchik BLK-8 «Parallaks» | `CH_ParallaxScout` |
| Khronolegion | Tank «Kontinuum» | `CH_ContinuumTank` | Tank CT-21 «Liniya» | `CH_TimelineTank` |
| Khronolegion | Artilleriya zaderzhki | `CH_DelayArtillery` | Artilleriya LAG-16 «Delta» | `CH_DeltaDelayArtillery` |
| Khronolegion | Proektor stazisa | `CH_StasisProjector` | Proektor STS-5 «Pauza» | `CH_PauseProjector` |
| Khronolegion | Dvigatel epokhi | `CH_EpochEngine` | Tyazhyolyy Tank EPC-0 «Era» | `CH_EraEngine` |
| Khronolegion | Razlomnyy perekhvatchik | `CH_RiftInterceptor` | Perekhvatchik RFT-31 «Razryv» | `CH_GapInterceptor` |
| Khronolegion | Shturmovik «Posleobraz» | `CH_AfterimageGunship` | Shturmovik AFG-6 «Shleyf» | `CH_TrailGunship` |
| Khronolegion | Bombardirovshchik «Gorizont sobytiy» | `CH_EventHorizon` | Bombardirovshchik CRV-9 «Kriticheskaya tochka» | `CH_CriticalPointBomber` |
| Khronolegion | Fregat «Otmetka priliva» | `CH_Tidemark` | Fregat TMK-9 «Izobata» | `CH_IsobathFrigate` |
| Khronolegion | Podlodka «Bezdna» | `CH_AbyssWalker` | Podlodka ABY-14 «Batis» | `CH_BathysSubmarine` |
| Khronolegion | Kovcheg singulyarnosti | `CH_SingularityArk` | Kovcheg SGA-1 «Attraktor» | `CH_AttractorArk` |
| Khronolegion | Arkhivist Selena Voss | `CH_Hero_Voss` | Arkhivist Selena Voss | `CH_Hero_Voss` |

## 1. Globalnaya Economy
### 1.1. Osnovnye resursy
| Resurs | Naznachenie | Kak poluchaetsya | Klyuchevoe ogranichenie |
| --- | --- | --- | --- |
| Kredity | Stroitelstvo, Production, remont, sposobnosti | Rudnye polya, bogataya ruda, neytralnye neftyanye stantsii, razovye nagrady | Dobycha zavisit ot logistiki i zashchishchyonnosti marshruta |
| Energiya | Pitanie zdaniy, oborony, radara i nekotorykh yunitov | Elektrostantsii i vysokotekhnologichnye reaktory | with defitsite otklyuchayutsya prioritetnye sistemy |
| Komandnyy limit | Ogranichenie kolichestva boevykh edinits | HQ, kazarmy, zavody, logisticheskie uzly | Ne raskhoduetsya, a rezerviruetsya yunitami |
| Fraktsionnyy resurs | Opredelyaet unikalnyy stil armii | Otdelnaya mekhanika kazhdoy Factions | Ne zamenyaet kredity i ne dolzhen byt obyazatelnym for bazovykh deystviy |

### 1.2. Start matcha
| Parametr | Standart |
| --- | --- |
| Startovye kredity | 10 000 |
| Startovaya energiya | +100 svobodnoy moshchnosti after razvyortyvaniya shtaba |
| Startovyy komandnyy limit | 50 |
| Maksimalnyy komandnyy limit | 200 |
| Startovyy stroitel | 1 Mobilnyy komandnyy modul, razvorachivaemyy v HQ |
| Startovaya razvedka | 1 deshyovyy razvedchik or ekvivalent after postroyki kazarm/zavoda |
| Standartnaya dlitelnost matcha 1v1 | 18–30 minut |
| Standartnaya dlitelnost matcha 4v4 | 30–55 minut |

### 1.3. Ruda i dobycha
Obychnoe rudnoe pole soderzhit 45 000 kreditov. Bogatoe rudnoe pole soderzhit 75 000 kreditov i dayot +25% k skorosti zagruzki. Bazovyy gruz dobytchika — 1 200 kreditov. Polnaya zagruzka zanimaet 12–16 sekund, vygruzka — 4 sekundy, tipovoy tsikl with sredney distantsii dayot 35–45 kreditov v sekundu. Neftyanaya stantsiya prinosit 8 kreditov v sekundu vladeltsu i zakhvatyvaetsya inzhenerom. Unichtozhenie rudnogo polya nevozmozhno; karta dolzhna stimulirovat borbu za marshruty, a ne polnoe udalenie ekonomiki sopernika.

### 1.4. Stroitelstvo, otmena, prodazha i remont
| Deystvie | Pravilo |
| --- | --- |
| Otmena Buildings before 25% gotovnosti | Vozvrat 90% stoimosti |
| Otmena Buildings after 25% gotovnosti | Vozvrat 60% stoimosti |
| Otmena yunita v ocheredi | Vozvrat 80% neispolzovannoy stoimosti |
| Prodazha Buildings | Vozvrat 50% stoimosti i poyavlenie nebolshoy gruppy ekipazha |
| Remont Buildings | before 30% pervonachalnoy stoimosti za vosstanovlenie s 1 HP before 100% |
| Remont tekhniki | before 25% pervonachalnoy stoimosti za polnoe vosstanovlenie |
| Zakhvat Buildings | Engineer tratitsya; zakhvachennoe zdanie vremenno otklyucheno na 8 sekund |

### 1.5. Energosistema
Kazhdoe zdanie imeet Production or potreblenie energii. with defitsite sistema otklyuchaet obekty po prioritetam: dekorativnye i vspomogatelnye sistemy → radar i mini-karta → remont → vysokotekhnologichnoe Production → statsionarnaya oborona → superoruzhie. Igrok mozhet vruchnuyu izmenit prioritety. Bazovye kazarmy i dobycha nikogda polnostyu ne ostanavlivayutsya, no with tyazhyolom defitsite rabotayut na 50% skorosti.

### 1.6. Komandnyy limit
| Kategoriya | Stoimost limita |
| --- | --- |
| Obychnaya Infantry | 1 |
| Elitnaya/tyazhyolaya Infantry | 2 |
| Lyogkaya Vehicles | 3 |
| Osnovnoy Tank/artilleriya | 5 |
| Sverkhtyazhyolaya Vehicles | 8–10 |
| Istrebitel/vertolyot | 4 |
| Bombardirovshchik/vozdushnyy transport | 6 |
| Korabl | 5–10 |
| Hero | 8 |

### 1.7. Tekhnologicheskie urovni
| Uroven | Okno matcha | Chto otkryvaet |
| --- | --- | --- |
| T1 | 0–5 min | Bazovaya Economy, Infantry, razvedka, lyogkaya oborona |
| T2 | 4–12 min | Factory, radar, Aviation/Naval, spetsializirovannye kontr-Units |
| T3 | 9+ min | Tekhnologicheskiy tsentr, elitnye Units, tyazhyolaya artilleriya, superoruzhie |

## 2. Boevaya model
### 2.1. Tipy broni
| Tip | Primery | Silnye storony | Slabye storony |
| --- | --- | --- | --- |
| Lyogkaya Infantry | Strelki, inzhenery | Deshyovaya, rassredotochennaya | Oskolochnyy Damaged, ogon, podavlenie |
| Tyazhyolaya Infantry | Shturmoviki, spetsnaz | Vysokaya zhivuchest | Snaypery, plazma, tyazhyolye pulemyoty |
| Lyogkaya Vehicles | Razvedchiki, BTR | Skorost | PT-Weapons, miny |
| Tyazhyolaya Vehicles | Tanki, shagokhody | Lobovaya Armor | Bortovye ataki, Aviation, elektrichestvo |
| Osadnaya Vehicles | Artilleriya | Dalnost | Razvedchiki, Aviation, blizhniy boy |
| Vozdushnaya | Istrebiteli, bombardirovshchiki | Ignor relefa | PVO, perekhvatchiki |
| Morskaya | Korabli i podlodki | Vysokaya dalnost i ognevaya moshch | Aviation, torpedy, beregovye batarei |
| Buildings | Production i oborona | Bolshoy zapas HP | Osadnyy Damaged, diversii, superoruzhie |
| Shchitovaya | Coalition, Khronolegion | Pogloshchaet pervyy udar | Elektrichestvo, dlitelnyy fokus |

### 2.2. Tipy urona i bazovye mnozhiteli
| Damaged | Lyogkaya Infantry | Tyazhyolaya Infantry | Lyogkaya Vehicles | Tyazhyolaya Vehicles | Buildings | Vozdukh |
| --- | --- | --- | --- | --- | --- | --- |
| Ballisticheskiy | 1.0 | 0.8 | 0.6 | 0.35 | 0.25 | 0.0 |
| Oskolochnyy | 1.5 | 1.15 | 0.55 | 0.3 | 0.4 | 0.0 |
| Broneboynyy | 0.6 | 0.9 | 1.2 | 1.45 | 0.8 | 0.0 |
| Osadnyy | 0.8 | 0.8 | 1.0 | 1.15 | 1.7 | 0.0 |
| Elektricheskiy | 1.0 | 1.15 | 1.3 | 1.35 | 1.0 | 0.75 |
| Plazmennyy | 1.1 | 1.25 | 1.1 | 1.1 | 0.9 | 0.9 |
| Kriogennyy | 0.9 | 1.0 | 0.8 | 0.8 | 0.6 | 0.8 |
| Temporalnyy | 1.0 | 1.0 | 1.0 | 1.0 | 0.8 | 1.0 |
| PVO | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 1.5 |

### 2.3. Veteranstvo
| Rang | Trebovanie | Bonus |
| --- | --- | --- |
| Novobranets | Bazovyy | without bonusov |
| Veteran | 1.0 stoimosti unichtozhennykh tseley | +10% Damaged, +8% HP, uskorennoe vosstanovlenie |
| Elite | 2.5 stoimosti unichtozhennykh tseley | Eshchyo +10% Damaged, +10% HP, uluchshennaya Ability |
| Geroyskiy | 5.0 stoimosti unichtozhennykh tseley | Unikalnyy passivnyy effekt, vizualnaya otmetka, osobaya replika |

### 2.4. Universalnye komandy
all boevye Units podderzhivayut Move, Attack, Attack-Move, Stop, Hold Position, Patrol, Guard, Focus Fire i Retreat. Spetsializirovannye komandy dolzhny poyavlyatsya kontekstno. Prikaz ne dolzhen narushat rol yunita: artilleriya ne obyazana presledovat tsel v upor, PVO ne dolzhno pytatsya atakovat nazemnuyu tsel, a transport ne dolzhen avtomaticheski vysazhivat desant without yavnoy komandy.

## 3. Fraktsionnye resursy
| Fraktsiya | Resurs | Poluchenie | Effekt |
| --- | --- | --- | --- |
| Soviet Union | Mobilizatsiya, 0–100 | Damaged, poterya sobstvennykh voysk, uderzhanie peredovoy | Porogovye bonusy k proizvodstvu; aktivnye prikazy massovogo nastupleniya |
| Alliance | Razveddannye, 0–100 | Obnaruzhenie vraga, skanirovanie, unichtozhenie klyuchevykh tseley | Tochechnye skany, vzlom, uskorenie aviatsii i vysokotochnye udary |
| Vostochnaya Coalition | Sinkhronizatsiya, 0–100 | Svyazannye energoseti, postroeniya, sovmestnye ataki | Passivnye shchity, tochnost, uskorenie proizvodstva; padaet with razryve stroya |
| Khronolegion | Temporalnaya stabilnost, 0–100 | Medlennoe vosstanovlenie, zakhvat khronouzlov | Raskhoduetsya na teleportatsii i peremotku; nizhe 30 poyavlyayutsya sistemnye shtrafy |

## 4. Pravila ozvuchki
Kazhdyy yunit imeet sobstvennyy VoiceId. Ryadovye boevye edinitsy govoryat korotko i funktsionalno, elitnye i geroi — bolee kharakterno. Repliki ne dolzhny kopirovat frazy sushchestvuyushchikh igr. Bazovye kategorii: Selected, Move, Attack, Ability, Damaged, Elite, Idle, Death. for proizvodstva neobkhodimo sgenerirovat minimum po 2–4 varianta kazhdoy kategorii, no v etom dokumente dana kanonicheskaya pervaya liniya, zadayushchaya kharakter.

# Fraktsiya: Soviet Union
## Fraktsionnaya identichnost
Sverkhtyazhyolaya industrialnaya armiya, deshyovaya Infantry, silnaya lobovaya Attack, moshchnoe elektricheskoe i osadnoe vooruzhenie. Slabosti: nizkaya skorost, dorogaya razvedka, uyazvimost k obkhodu i tochechnym udaram.
## Fraktsionnyy resurs: Mobilizatsiya
0–24: without bonusa. 25–49: +5% skorost proizvodstva pekhoty. 50–74: +8% skorost tekhniki i +5% skorost dvizheniya ryadom so shtabom nastupleniya. 75–100: +10% Damaged tyazhyoloy tekhniki. Aktivnaya Ability «Obshchiy nazhim» stoit 50 ochkov i dayot vybrannoy gruppe +20% skorost i immunitet k podavleniyu na 12 sekund.
## Buildings i Economy Factions
| Zdanie | Tsena | Vremya, s | Energiya | Naznachenie |
| --- | --- | --- | --- | --- |
| Mobilnyy komandnyy modul | 5000 | 60 | 0 | Razvorachivaetsya v HQ, stroit bazovye Buildings, +20 limita |
| Krasnyy HQ | — | — | +100 | Stroitelnaya zona, Production inzhenerov i MKM |
| Teplovaya Power Plant | 800 | 18 | +120 | Deshyovaya energiya, with unichtozhenii vzryvaetsya |
| Rudnyy kombinat | 2400 | 45 | -20 | Vklyuchaet gornorudnuyu mashinu GRM-8 «Bogatyr» |
| Barracks mobilizatsii | 700 | 18 | -15 | Infantry T1–T2, +5 limita |
| Tyazhyolyy Factory | 2300 | 42 | -40 | Vehicles, +10 limita |
| Airfield dalney aviatsii | 1900 | 36 | -45 | 3 posadochnykh mesta, Aviation |
| Voenno-morskoy dok | 2100 | 42 | -45 | Korabli i remont flota |
| Komandnyy radar | 1500 | 30 | -60 | Mini-karta, dalnyaya razvedka, otkryvaet T2 |
| Nauchnyy kompleks «Grom» | 3600 | 62 | -100 | Otkryvaet T3 i elektricheskie tekhnologii |
| Pulemyotnyy dot | 700 | 15 | -15 | Protiv pekhoty |
| Zenitnaya bashnya «Shilka» | 950 | 20 | -25 | PVO |
| Katushka «Perun» | 1900 | 35 | -75 | Tyazhyolaya elektricheskaya oborona |
| Bunker peredovoy | 1100 | 25 | -10 | Vmeshchaet 5 pekhotintsev |
| Kompleks «Zheleznyy kupol» | 6000 | 90 | -180 | Na 12 sekund delaet oblast neuyazvimoy; perezaryadka 6 min |
| Raketnaya shakhta «Karatel» | 7000 | 110 | -220 | Strategicheskiy raketnyy udar; perezaryadka 8 min |

## EVA — kanonicheskie sistemnye repliki
| Sobytie | Replika |
| --- | --- |
| Start | Komandovanie razvyornuto. Promyshlennost zhdyot prikaza. |
| Nizkaya energiya | Energosistema peregruzhena. Production zamedlyaetsya. |
| Baza atakovana | Vrag atakuet nashi proizvodstvennye moshchnosti. |
| Yunit gotov | Boevaya edinitsa gotova k otpravke. |
| Superoruzhie vraga | Zafiksirovana podgotovka strategicheskogo udara. |
| Mobilizatsiya 100 | Mobilizatsiya zavershena. Armiya gotova k obshchemu nazhimu. |
| Pobeda | Soprotivlenie podavleno. Territoriya perekhodit under nash kontrol. |
| Porazhenie | Komandnaya set poteryana. Organizovannoe soprotivlenie prekrashcheno. |

## Svodnaya tablitsa yunitov
| Yunit | ID | Klass | Tir | Tsena | Vremya | Limit | HP | Armor | Skorost | Dalnost | DPS | Rol |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Motostrelok MS-12 «Rubezh» | SU_RubezhRifleman | Infantry | T1 | 150 | 6 | 1 | 110 | Lyogkaya Infantry | 4.5 | 6 | 9 | Deshyovaya lineynaya Infantry i zakhvat territorii |
| Shturmovik OSh-4 «Zapal» | SU_ZapalGrenadier | Infantry | T1 | 350 | 10 | 1 | 150 | Tyazhyolaya Infantry | 3.8 | 7 | 18 | Antipekhotnyy shturmovik i zachistka garnizonov |
| Zenitnyy raschyot PZK-9 «Zaslon» | SU_ZaslonAATeam | Infantry | T1 | 450 | 12 | 2 | 170 | Tyazhyolaya Infantry | 3.5 | 9 | 22 | Perenosnoe PVO i kontr lyogkoy tekhnike |
| Engineer-sapyor IS-3 «Master» | SU_MasterEngineer | Infantry | T1 | 500 | 15 | 1 | 100 | Lyogkaya Infantry | 3.7 | 0 | 0 | Zakhvat, remont i razminirovanie |
| Elektroshturmovik ESh-8 «Razryad» | SU_RazryadTrooper | Infantry | T2 | 850 | 20 | 2 | 260 | Tyazhyolaya Infantry | 4.0 | 8 | 34 | Elektricheskiy shturm tyazhyoloy pekhoty i tekhniki |
| Ofitser svyazi KS-6 «Vektor» | SU_VektorOfficer | Infantry | T2 | 900 | 22 | 2 | 210 | Tyazhyolaya Infantry | 3.8 | 7 | 16 | Podderzhka, moral i koordinatsiya |
| Gornorudnaya mashina GRM-8 «Bogatyr» | SU_BogatyrOreCarrier | Vehicles | T1 | 1400 | 28 | 4 | 1600 | Tyazhyolaya Vehicles | 2.5 | 0 | 0 | Tyazhyolyy Harvester s vysokoy zhivuchestyu |
| Boevaya razvedmashina BRM-27 «Rys» | SU_RysScout | Vehicles | T1 | 600 | 14 | 3 | 520 | Lyogkaya Vehicles | 8.2 | 6 | 15 | Razvedka, presledovanie inzhenerov, zakhvat flangov |
| Osnovnoy Tank OBT-92 «Granit» | SU_GranitMBT | Vehicles | T2 | 1200 | 26 | 5 | 1650 | Tyazhyolaya Vehicles | 4.3 | 8 | 46 | Osnovnoy boevoy Tank |
| Termobaricheskaya RSZO TRS-18 «Zarevo» | SU_ZarevoMLRS | Vehicles | T2 | 1600 | 34 | 5 | 900 | Osadnaya Vehicles | 3.2 | 16 | 58 | Osadnaya artilleriya i vyzhiganie ukrepleniy |
| Elektrotaran ETM-7 «Gromoboy» | SU_GromoboyRam | Vehicles | T3 | 2200 | 42 | 7 | 2100 | Tyazhyolaya Vehicles | 4.8 | 6 | 72 | Proryv shchitov i postroeniy |
| Tyazhyolyy Tank proryva TTP-11 «Voevoda» | SU_VoevodaHeavyTank | Vehicles | T3 | 3200 | 58 | 10 | 4200 | Tyazhyolaya Vehicles | 2.8 | 10 | 98 | Sverkhtyazhyolyy frontovoy Tank i PVO |
| Istrebitel I-47 «Krechet» | SU_KrechetInterceptor | Aviation | T2 | 1100 | 24 | 4 | 700 | Vozdushnaya | 12.0 | 12 | 54 | Perekhvat aviatsii i tochechnye raketnye udary |
| Shturmovoy vertolyot ShV-38 «Korshun» | SU_KorshunGunship | Aviation | T2 | 1500 | 32 | 5 | 1300 | Vozdushnaya | 7.0 | 7 | 60 | Podderzhka nazemnykh voysk i desant |
| Tyazhyolyy dirizhabl TDA-8 «Gromada» | SU_GromadaAirship | Aviation | T3 | 3000 | 60 | 10 | 5000 | Vozdushnaya | 2.0 | 5 | 130 | Strategicheskiy osadnyy bombardirovshchik |
| Boevoy kater BK-27 «Buran» | SU_BuranPatrolBoat | Naval | T1 | 750 | 16 | 4 | 700 | Morskaya | 8.0 | 7 | 22 | Bystryy kater PVO i okhoty na desant |
| Udarnaya podlodka UPL-90 «Morok» | SU_MorokSubmarine | Naval | T2 | 1700 | 34 | 6 | 1800 | Morskaya | 4.8 | 10 | 64 | Skrytaya okhota na krupnye korabli |
| Raketnyy kreyser RKR-44 «Svyatogor» | SU_SvyatogorCruiser | Naval | T3 | 3400 | 62 | 10 | 4600 | Morskaya | 2.8 | 22 | 120 | Dalnyaya osada poberezhya |
| Mayor Elena Morozova | SU_Hero_Morozova | Hero | T3 | 2600 | 50 | 8 | 900 | Tyazhyolaya Infantry | 5.0 | 10 | 70 | Hero-Commander tyazhyoloy pekhoty i elektricheskogo oruzhiya |

## Podrobnye kartochki yunitov
### 1. Motostrelok MS-12 «Rubezh» (`SU_RubezhRifleman`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_RubezhRifleman` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 150 |
| Vremya proizvodstva | 6 sek |
| Komandnyy limit | 1 |
| HP | 110 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 4.5 |
| Dalnost | 6 |
| Orientirovochnyy DPS | 9 |
| Prednaznachenie | Deshyovaya lineynaya Infantry i zakhvat territorii |
| Osnovnoe Weapons | Avtomat K-47, ballisticheskiy Damaged |
| Requirements | Barracks mobilizatsii |

#### Sposobnosti
- «Okopatsya»: 2 sek podgotovki, +35% zashchita i -40% skorost before otmeny

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Ochen dyoshev, bystro proizvoditsya, silyon massoy |
| Slabye storony | Slab protiv oskolochnogo urona i ognya |
| Pryamye kontrmery | Shturmoviki «Zapal», pulemyotnye tureli, shturmovaya Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Motostrelok MS-12 «Rubezh» na svyazi. |
| Move | Bezhim, poka doroga svobodna. |
| Attack | Tsel vizhu. Otkryvayu ogon. |
| Ability | V zemlyu! Derzhim pozitsiyu! |
| Damaged | Nas prizhali! |
| Elite | Teper my ne prosto popolnenie. |
| Idle | Obeshchali formu poteplee. |
| Death | Peredayte… pozitsiyu derzhali. |

### 2. Shturmovik OSh-4 «Zapal» (`SU_ZapalGrenadier`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_ZapalGrenadier` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 350 |
| Vremya proizvodstva | 10 sek |
| Komandnyy limit | 1 |
| HP | 150 |
| Tip broni | Tyazhyolaya Infantry |
| Skorost | 3.8 |
| Dalnost | 7 |
| Orientirovochnyy DPS | 18 |
| Prednaznachenie | Antipekhotnyy shturmovik i zachistka garnizonov |
| Osnovnoe Weapons | Avtomaticheskiy granatomyot, oskolochnyy Damaged |
| Requirements | Barracks |

#### Sposobnosti
- «Termobaricheskiy zaryad»: vybivaet garnizon i podzhigaet zdanie; 28 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Silnyy Damaged po gruppam i zdaniyam |
| Slabye storony | Nizkaya skorostrelnost, uyazvim for snayperov |
| Pryamye kontrmery | Snaypery, lyogkaya Vehicles, Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Granaty snaryazheny. |
| Move | Podoydyom na brosok. |
| Attack | Nakryvayu sektor! |
| Ability | Termobaricheskiy — vnutr! |
| Damaged | Oskolkami zadelo! |
| Elite | Teper popadayu s pervogo broska. |
| Idle | Glavnoe — ne pereputat sumki. |
| Death | Cheka… uzhe vydernuta… |

### 3. Zenitnyy raschyot PZK-9 «Zaslon» (`SU_ZaslonAATeam`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_ZaslonAATeam` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 450 |
| Vremya proizvodstva | 12 sek |
| Komandnyy limit | 2 |
| HP | 170 |
| Tip broni | Tyazhyolaya Infantry |
| Skorost | 3.5 |
| Dalnost | 9 |
| Orientirovochnyy DPS | 22 |
| Prednaznachenie | Perenosnoe PVO i kontr lyogkoy tekhnike |
| Osnovnoe Weapons | Fugasnaya zenitnaya pushka |
| Requirements | Barracks + radar |

#### Sposobnosti
- «Vozdushnaya zasada»: maskiruetsya na 6 sek i poluchaet +40% pervyy zalp

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Deshyovoe mobilnoe PVO |
| Slabye storony | Plokh protiv obychnoy pekhoty v blizhnem boyu |
| Pryamye kontrmery | Snaypery, artilleriya, tanki |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Nebo under kontrolem. |
| Move | Ishchu chistyy sektor. |
| Attack | Vysota podtverzhdena. Ogon! |
| Ability | Zatailis. Pust podletyat. |
| Damaged | Raschyot under obstrelom! |
| Elite | Ni odin bort ne uydyot. |
| Idle | Letyat krasivo. Padayut luchshe. |
| Death | Nebo… vashe… |

### 4. Engineer-sapyor IS-3 «Master» (`SU_MasterEngineer`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_MasterEngineer` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 500 |
| Vremya proizvodstva | 15 sek |
| Komandnyy limit | 1 |
| HP | 100 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 3.7 |
| Dalnost | 0 |
| Orientirovochnyy DPS | 0 |
| Prednaznachenie | Zakhvat, remont i razminirovanie |
| Osnovnoe Weapons | Ne vooruzhyon |
| Requirements | Barracks |

#### Sposobnosti
- «Polevoy remont»: vosstanavlivaet 300 HP tekhnike za 8 sek
- «Zakhvat»: zanimaet neytralnye i vrazheskie Buildings

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Klyuchevoy takticheskiy yunit |
| Slabye storony | Bezzashchiten, trebuet soprovozhdeniya |
| Pryamye kontrmery | Lyubaya Infantry i razvedchiki |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Instrument est. Plan by eshchyo. |
| Move | Doberus i pochinyu. |
| Attack | Ya ne strelok. Pokazhite obekt. |
| Ability | Seychas zavedyom etu razvalinu. |
| Damaged | Inzhenera prikroyte! |
| Elite | Ya pochinyu dazhe to, chego eshchyo ne postroili. |
| Idle | Po instruktsii eto dolzhno before rabotat. |
| Death | Skhema… byla vernoy… |

### 5. Elektroshturmovik ESh-8 «Razryad» (`SU_RazryadTrooper`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_RazryadTrooper` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 850 |
| Vremya proizvodstva | 20 sek |
| Komandnyy limit | 2 |
| HP | 260 |
| Tip broni | Tyazhyolaya Infantry |
| Skorost | 4.0 |
| Dalnost | 8 |
| Orientirovochnyy DPS | 34 |
| Prednaznachenie | Elektricheskiy shturm tyazhyoloy pekhoty i tekhniki |
| Osnovnoe Weapons | Ruchnoy dugovoy izluchatel |
| Requirements | Barracks + radar + Nauchnyy kompleks |

#### Sposobnosti
- «Peregruzka»: tsepnaya molniya po 4 tselyam; 30 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Vysokiy Damaged po tekhnike i shchitam |
| Slabye storony | Dorogoy, uyazvim for snayperov i osady |
| Pryamye kontrmery | Snaypery, artilleriya, Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Kontur zaryazhen. |
| Move | Tok poydyot za nami. |
| Attack | Razryad na tsel! |
| Ability | Peregruzka seti! |
| Damaged | Izolyatsiya probita! |
| Elite | Molniya slushaetsya menya. |
| Idle | Ne trogayte kabel. Poslednee Warning. |
| Death | Zazemlenie… ne srabotalo… |

### 6. Ofitser svyazi KS-6 «Vektor» (`SU_VektorOfficer`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_VektorOfficer` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 900 |
| Vremya proizvodstva | 22 sek |
| Komandnyy limit | 2 |
| HP | 210 |
| Tip broni | Tyazhyolaya Infantry |
| Skorost | 3.8 |
| Dalnost | 7 |
| Orientirovochnyy DPS | 16 |
| Prednaznachenie | Podderzhka, moral i koordinatsiya |
| Osnovnoe Weapons | Pistolet-pulemyot i komandnyy peredatchik |
| Requirements | Barracks + radar |

#### Sposobnosti
- «Prikaz №1»: soyuznaya Infantry v radiuse poluchaet +20% Damaged i immunitet k podavleniyu na 10 sek; 35 sek
- Passivno uskoryaet poluchenie Mobilizatsii

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Silno usilivaet pekhotnye massy |
| Slabye storony | Sam po sebe slab, prioritetnaya tsel |
| Pryamye kontrmery | Snaypery, artilleriya, diversanty |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Svyaz s frontom ustanovlena. |
| Move | Peredayu novyy rubezh. |
| Attack | Prikaz utverzhdyon. Unichtozhit. |
| Ability | Pervyy prikaz: ni shaga nazad! |
| Damaged | Kanal under ognyom! |
| Elite | Teper armiya slyshit menya without pomekh. |
| Idle | Molchanie v efire podozritelnee strelby. |
| Death | Komandovanie… prodolzhayte without menya. |

### 7. Gornorudnaya mashina GRM-8 «Bogatyr» (`SU_BogatyrOreCarrier`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_BogatyrOreCarrier` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 1400 |
| Vremya proizvodstva | 28 sek |
| Komandnyy limit | 4 |
| HP | 1600 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 2.5 |
| Dalnost | 0 |
| Orientirovochnyy DPS | 0 |
| Prednaznachenie | Tyazhyolyy Harvester s vysokoy zhivuchestyu |
| Osnovnoe Weapons | without oruzhiya |
| Requirements | Rudnyy kombinat |

#### Sposobnosti
- «Avariynaya Armor»: na 8 sek poluchaet -40% vkhodyashchego urona; 50 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Ochen prochnyy, bolshoy gruz |
| Slabye storony | Medlennyy i zametnyy |
| Pryamye kontrmery | PT-zasady, Aviation, miny |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Bogatyr gotov k reysu. |
| Move | Tyazhyolyy gruz idyot. |
| Attack | Oruzhiya no. Mogu pereekhat. |
| Ability | Zakryvayu bronevye shtorki. |
| Damaged | Obshivka derzhit! |
| Elite | Marshrut znayu luchshe generalov. |
| Idle | Ruda sama sebya ne privezyot. |
| Death | Gruz… ne dostavlen… |

### 8. Boevaya razvedmashina BRM-27 «Rys» (`SU_RysScout`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_RysScout` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 600 |
| Vremya proizvodstva | 14 sek |
| Komandnyy limit | 3 |
| HP | 520 |
| Tip broni | Lyogkaya Vehicles |
| Skorost | 8.2 |
| Dalnost | 6 |
| Orientirovochnyy DPS | 15 |
| Prednaznachenie | Razvedka, presledovanie inzhenerov, zakhvat flangov |
| Osnovnoe Weapons | Sparennyy pulemyot |
| Requirements | Tyazhyolyy Factory |

#### Sposobnosti
- «Pryzhok via prepyatstvie»: korotkiy pryzhok, 18 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Ochen bystraya, vidit skrytykh yunitov |
| Slabye storony | Slabaya Armor, nizkiy Damaged po tekhnike |
| Pryamye kontrmery | PT-Infantry, miny, tanki |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Rys vyshla na marshrut. |
| Move | Uzhe tam. |
| Attack | Srezaem khvost kolonne! |
| Ability | Pereprygivaem! |
| Damaged | Armor tonkaya, ne stoyte! |
| Elite | Ya vizhu flang ranshe radara. |
| Idle | Glavnoe — ne dognat sobstvennyy sled. |
| Death | Skorost… ne spasla… |

### 9. Osnovnoy Tank OBT-92 «Granit» (`SU_GranitMBT`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_GranitMBT` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1200 |
| Vremya proizvodstva | 26 sek |
| Komandnyy limit | 5 |
| HP | 1650 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 4.3 |
| Dalnost | 8 |
| Orientirovochnyy DPS | 46 |
| Prednaznachenie | Osnovnoy boevoy Tank |
| Osnovnoe Weapons | 125-mm broneboynaya pushka |
| Requirements | Tyazhyolyy Factory + radar |

#### Sposobnosti
- «Taran»: uskoryaetsya i otbrasyvaet lyogkuyu tekhniku; 26 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Vysokaya Armor i stabilnyy Damaged |
| Slabye storony | Medlennyy povorot, slabyy obzor |
| Pryamye kontrmery | PT-Aviation, obkhod, artilleriya |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Granit gotov. |
| Move | Gusenitsy — vperyod. |
| Attack | Razdrobit tsel. |
| Ability | Na taran! |
| Damaged | Lob derzhit, bort podstavili! |
| Elite | Stal nauchilas pobezhdat. |
| Idle | Tishe edesh — dolshe strelyaesh. |
| Death | Bashnya… zaklinila… |

### 10. Termobaricheskaya RSZO TRS-18 «Zarevo» (`SU_ZarevoMLRS`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_ZarevoMLRS` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1600 |
| Vremya proizvodstva | 34 sek |
| Komandnyy limit | 5 |
| HP | 900 |
| Tip broni | Osadnaya Vehicles |
| Skorost | 3.2 |
| Dalnost | 16 |
| Orientirovochnyy DPS | 58 |
| Prednaznachenie | Osadnaya artilleriya i vyzhiganie ukrepleniy |
| Osnovnoe Weapons | Termobaricheskie rakety |
| Requirements | Tyazhyolyy Factory + radar |

#### Sposobnosti
- «Ognennyy kvadrat»: zalp po bolshoy zone, ostavlyaet gorenie; 38 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Ogromnyy Damaged po zdaniyam i skopleniyam |
| Slabye storony | Ne mozhet strelyat vblizi, slabaya Armor |
| Pryamye kontrmery | Razvedchiki, Aviation, teleport |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Paket raket zaryazhen. |
| Move | Derzhim distantsiyu. |
| Attack | Raschyotnyy kvadrat podtverzhdyon. |
| Ability | Podzhigaem ves sektor. |
| Damaged | Puskovaya under ognyom! |
| Elite | Odin zalp — odin novyy gorizont. |
| Idle | Krasivo gorit only chuzhoe. |
| Death | Boekomplekt… seychas rvanyot… |

### 11. Elektrotaran ETM-7 «Gromoboy» (`SU_GromoboyRam`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_GromoboyRam` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 2200 |
| Vremya proizvodstva | 42 sek |
| Komandnyy limit | 7 |
| HP | 2100 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 4.8 |
| Dalnost | 6 |
| Orientirovochnyy DPS | 72 |
| Prednaznachenie | Proryv shchitov i postroeniy |
| Osnovnoe Weapons | Kontaktnyy elektricheskiy izluchatel |
| Requirements | Nauchnyy kompleks |

#### Sposobnosti
- «Razryad po zemle»: konusnyy elektricheskiy udar i kratkoe oglushenie tekhniki; 32 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Silnyy blizhniy boy, otklyuchaet shchity |
| Slabye storony | Korotkaya dalnost, uyazvim na podkhode |
| Pryamye kontrmery | Artilleriya, Aviation, miny |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Gromoboy zhdyot komandy. |
| Move | K kontaktu. |
| Attack | Zamykayu tsep! |
| Ability | Razryad v grunt! |
| Damaged | Katushki peregrevayutsya! |
| Elite | Groza teper idyot po zemle. |
| Idle | Sukhaya pogoda — vremennaya problema. |
| Death | Kontur… razomknut… |

### 12. Tyazhyolyy Tank proryva TTP-11 «Voevoda» (`SU_VoevodaHeavyTank`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_VoevodaHeavyTank` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 3200 |
| Vremya proizvodstva | 58 sek |
| Komandnyy limit | 10 |
| HP | 4200 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 2.8 |
| Dalnost | 10 |
| Orientirovochnyy DPS | 98 |
| Prednaznachenie | Sverkhtyazhyolyy frontovoy Tank i PVO |
| Osnovnoe Weapons | Dve tyazhyolye pushki i rakety PVO |
| Requirements | Nauchnyy kompleks + dva Tyazhyolykh zavoda |

#### Sposobnosti
- «Osadnyy rezhim»: -60% skorost, +30% dalnost i Armor; 4 sek razvyortyvaniya

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Chudovishchnaya zhivuchest, universalnost |
| Slabye storony | Ochen dorog, medlennyy, bolshaya tsel |
| Pryamye kontrmery | Sverkhdalnyaya artilleriya, EMP, massovaya Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Voevoda vstupaet v boy. |
| Move | Zemlya vyderzhit. |
| Attack | Steret koordinaty. |
| Ability | Perekhodim v osadnyy rezhim. |
| Damaged | Povrezhdenie prinyato. Prodolzhaem. |
| Elite | Teper eto ne Tank. Eto napravlenie fronta. |
| Idle | My ne opazdyvaem. Nas zhdut. |
| Death | Voevoda… ostavlyaet rubezh… |

### 13. Istrebitel I-47 «Krechet» (`SU_KrechetInterceptor`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_KrechetInterceptor` |
| Kategoriya | Aviation |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1100 |
| Vremya proizvodstva | 24 sek |
| Komandnyy limit | 4 |
| HP | 700 |
| Tip broni | Vozdushnaya |
| Skorost | 12.0 |
| Dalnost | 12 |
| Orientirovochnyy DPS | 54 |
| Prednaznachenie | Perekhvat aviatsii i tochechnye raketnye udary |
| Osnovnoe Weapons | Rakety vozdukh-vozdukh/vozdukh-zemlya |
| Requirements | Airfield + radar |

#### Sposobnosti
- «Forsazh»: +40% skorost na 6 sek; 25 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Vysokaya skorost, moshchnyy pervyy zalp |
| Slabye storony | Trebuet perezaryadki na aerodrome |
| Pryamye kontrmery | PVO, vozdushnye zasady |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Krechet na polose. |
| Move | Kurs prinyat. |
| Attack | Rakety soshli. |
| Ability | Forsazh! |
| Damaged | Poterya davleniya! |
| Elite | Nebo after tesnym. |
| Idle | Toplivo lyubit reshitelnykh. |
| Death | Katapulta… otkaz… |

### 14. Shturmovoy vertolyot ShV-38 «Korshun» (`SU_KorshunGunship`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_KorshunGunship` |
| Kategoriya | Aviation |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1500 |
| Vremya proizvodstva | 32 sek |
| Komandnyy limit | 5 |
| HP | 1300 |
| Tip broni | Vozdushnaya |
| Skorost | 7.0 |
| Dalnost | 7 |
| Orientirovochnyy DPS | 60 |
| Prednaznachenie | Podderzhka nazemnykh voysk i desant |
| Osnovnoe Weapons | Pushka i neupravlyaemye rakety |
| Requirements | Airfield |

#### Sposobnosti
- «Vysadka»: perevozit 6 pekhotintsev
- «Krug ognya»: zavisaet i usilivaet ogon na 8 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Silnyy po pekhote i lyogkoy tekhnike |
| Slabye storony | Uyazvim for PVO i istrebiteley |
| Pryamye kontrmery | PVO, istrebiteli |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Korshun gotov k vyletu. |
| Move | Idyom na maloy vysote. |
| Attack | Rabotaem po zemle! |
| Ability | Zakhodim na krug! |
| Damaged | Khvostovoy sektor povrezhdyon! |
| Elite | Infantry zovyot — my otvechaem. |
| Idle | V kabine pakhnet toplivom i pobedoy. |
| Death | Bort padaet… |

### 15. Tyazhyolyy dirizhabl TDA-8 «Gromada» (`SU_GromadaAirship`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_GromadaAirship` |
| Kategoriya | Aviation |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 3000 |
| Vremya proizvodstva | 60 sek |
| Komandnyy limit | 10 |
| HP | 5000 |
| Tip broni | Vozdushnaya |
| Skorost | 2.0 |
| Dalnost | 5 |
| Orientirovochnyy DPS | 130 |
| Prednaznachenie | Strategicheskiy osadnyy bombardirovshchik |
| Osnovnoe Weapons | Tyazhyolye svobodnopadayushchie bomby |
| Requirements | Nauchnyy kompleks + Airfield |

#### Sposobnosti
- «Polnyy gaz»: +60% skorost na 10 sek, zatem poluchaet 20% urona; 50 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Ogromnyy Damaged po baze, mnogo HP |
| Slabye storony | Krayne medlennyy, viden vsey karte with atake |
| Pryamye kontrmery | Massovoe PVO, istrebiteli |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Gromada v vozdukhe. |
| Move | Medlenno. Neotvratimo. |
| Attack | Otkryt bombolyuki. |
| Ability | Polnyy gaz. Dvigateli na predel. |
| Damaged | Obshivka gorit, kurs derzhim. |
| Elite | Goroda uznayut nas po teni. |
| Idle | Vysota khoroshaya. Mir kazhetsya tishe. |
| Death | Ballast… uzhe ne pomozhet… |

### 16. Boevoy kater BK-27 «Buran» (`SU_BuranPatrolBoat`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_BuranPatrolBoat` |
| Kategoriya | Naval |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 750 |
| Vremya proizvodstva | 16 sek |
| Komandnyy limit | 4 |
| HP | 700 |
| Tip broni | Morskaya |
| Skorost | 8.0 |
| Dalnost | 7 |
| Orientirovochnyy DPS | 22 |
| Prednaznachenie | Bystryy kater PVO i okhoty na desant |
| Osnovnoe Weapons | Avtopushka i lyogkie rakety |
| Requirements | Voenno-morskoy dok |

#### Sposobnosti
- «Elektroset»: stavit elektricheskuyu minu na vode; 25 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Bystryy, deshyovyy, polezen v razvedke |
| Slabye storony | Slab protiv krupnykh korabley |
| Pryamye kontrmery | Esmintsy, beregovye batarei |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Buran na vode. |
| Move | Rezhem volnu. |
| Attack | Tsel po pravomu bortu! |
| Ability | Set v vodu! |
| Damaged | Korpus prinimaet vodu! |
| Elite | More zapomnilo nash sled. |
| Idle | Shtil — eto prosto pauza. |
| Death | Otsek zatoplen… |

### 17. Udarnaya podlodka UPL-90 «Morok» (`SU_MorokSubmarine`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_MorokSubmarine` |
| Kategoriya | Naval |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1700 |
| Vremya proizvodstva | 34 sek |
| Komandnyy limit | 6 |
| HP | 1800 |
| Tip broni | Morskaya |
| Skorost | 4.8 |
| Dalnost | 10 |
| Orientirovochnyy DPS | 64 |
| Prednaznachenie | Skrytaya okhota na krupnye korabli |
| Osnovnoe Weapons | Tyazhyolye torpedy |
| Requirements | Dok + radar |

#### Sposobnosti
- «Bezzvuchnyy khod»: povyshennaya maskirovka na 12 sek; 35 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Silna protiv tyazhyologo flota |
| Slabye storony | Ne atakuet nazemnye tseli, uyazvima after obnaruzheniya |
| Pryamye kontrmery | Protivolodochnye korabli, Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Morok slushaet glubinu. |
| Move | Pogruzhaemsya. |
| Attack | Torpednyy rastvor otkryt. |
| Ability | Bezzvuchnyy khod. |
| Damaged | Prochnyy korpus deformirovan! |
| Elite | V more nas zamechayut slishkom pozdno. |
| Idle | Naverkhu shumyat. Zdes dumayut. |
| Death | Glubina… prinimaet… |

### 18. Raketnyy kreyser RKR-44 «Svyatogor» (`SU_SvyatogorCruiser`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_SvyatogorCruiser` |
| Kategoriya | Naval |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 3400 |
| Vremya proizvodstva | 62 sek |
| Komandnyy limit | 10 |
| HP | 4600 |
| Tip broni | Morskaya |
| Skorost | 2.8 |
| Dalnost | 22 |
| Orientirovochnyy DPS | 120 |
| Prednaznachenie | Dalnyaya osada poberezhya |
| Osnovnoe Weapons | Tyazhyolye krylatye rakety |
| Requirements | Dok + Nauchnyy kompleks |

#### Sposobnosti
- «Zagraditelnyy zalp»: 6 raket po shirokoy oblasti; 50 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Maksimalnaya morskaya dalnost |
| Slabye storony | Medlennyy, slab v blizhnem boyu i without okhrany |
| Pryamye kontrmery | Podlodki, Aviation, bystrye katera |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Svyatogor zhdyot koordinaty. |
| Move | Kreyser menyaet pozitsiyu. |
| Attack | Raketnyy zalp. |
| Ability | Zagraditelnyy ogon po sektoru. |
| Damaged | Paluba probita! |
| Elite | Bereg zakanchivaetsya tam, gde nachinayutsya nashi rakety. |
| Idle | More bolshoe. Dalnost bolshe. |
| Death | Pogreba… detoniruyut… |

### 19. Mayor Elena Morozova (`SU_Hero_Morozova`)
| Parametr | Value |
| --- | --- |
| Stable ID | `SU_Hero_Morozova` |
| Kategoriya | Hero |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 2600 |
| Vremya proizvodstva | 50 sek |
| Komandnyy limit | 8 |
| HP | 900 |
| Tip broni | Tyazhyolaya Infantry |
| Skorost | 5.0 |
| Dalnost | 10 |
| Orientirovochnyy DPS | 70 |
| Prednaznachenie | Hero-Commander tyazhyoloy pekhoty i elektricheskogo oruzhiya |
| Osnovnoe Weapons | Eksperimentalnaya Tesla-vintovka |
| Requirements | Nauchnyy kompleks |

#### Sposobnosti
- «Pole podavleniya»: vragi v oblasti teryayut skorost i tochnost; 40 sek
- «Komandnyy impuls»: soyuzniki mgnovenno poluchayut 20 Mobilizatsii; 60 sek
- Odin ekzemplyar

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Silnyy kontrol i podderzhka |
| Slabye storony | Prioritetnaya tsel, dorogaya |
| Pryamye kontrmery | Snaypery, fokus aviatsii, artilleriya |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Mayor Morozova. Dokladyvayte. |
| Move | Ya budu na peredovoy. |
| Attack | Etot uchastok fronta zakryvaem seychas. |
| Ability | Podavit ikh svyaz i Move. |
| Damaged | Tsarapina. Prikaz ne menyaetsya. |
| Elite | Segodnya front dvizhetsya vmeste so mnoy. |
| Idle | Generaly lyubyat karty. Ya predpochitayu mestnost. |
| Death | Prodolzhayte… nastuplenie… |

# Fraktsiya: Alliance
## Fraktsionnaya identichnost
Vysokotochnaya mobilnaya armiya s silnoy razvedkoy, aviatsiey, distantsionnym ognyom i tekhnologicheskimi kontrmerami. Slabosti: bolee nizkaya prochnost, zavisimost ot informatsii i pozitsionnogo kontrolya.
## Fraktsionnyy resurs: Razveddannye
Razveddannye generiruyutsya with pervom obnaruzhenii vrazheskikh obektov, podderzhanii razvedyvatelnogo kontakta i unichtozhenii vysokotsennykh tseley. 25 ochkov — orbitalnyy skan; 40 — podavlenie radara v oblasti; 60 — vysokotochnyy udar; 100 — globalnoe raskrytie karty na 8 sekund without snyatiya tumana navsegda.
## Buildings i Economy Factions
| Zdanie | Tsena | Vremya, s | Energiya | Naznachenie |
| --- | --- | --- | --- | --- |
| Mobilnyy uzel razvyortyvaniya | 5000 | 60 | 0 | Razvorachivaetsya v HQ, mozhet bystro svorachivatsya |
| Setevoy komandnyy tsentr | — | — | +110 | Stroitelnaya set, +20 limita |
| Kompaktnyy reaktor | 900 | 20 | +130 | Bezopasnaya energiya, malyy radius vzryva |
| Avtomatizirovannyy pererabotchik | 2500 | 44 | -20 | Vklyuchaet platformu M88 «Pioneer» |
| Takticheskaya Barracks | 750 | 18 | -15 | Infantry T1–T2, +5 limita |
| Modulnyy Factory | 2200 | 40 | -35 | Vehicles, +10 limita |
| Aviabaza «Nebesnaya liniya» | 1850 | 34 | -50 | 4 posadochnykh mesta |
| Okeanicheskiy dok | 2100 | 40 | -45 | Naval i morskie drony |
| Razvedyvatelnyy tsentr | 1450 | 28 | -55 | Radar, razveddannye, T2 |
| Laboratoriya prikladnoy fiziki | 3700 | 60 | -110 | T3, krio i maskirovka |
| Avtopushka «Strazh» | 750 | 16 | -15 | Protiv pekhoty i lyogkoy tekhniki |
| Raketnaya PVO «Kupol» | 1000 | 21 | -30 | Dalnee PVO |
| Prizmaticheskaya batareya | 2100 | 36 | -80 | Silna protiv tyazhyolykh tseley |
| Proektor shchita | 1800 | 32 | -85 | Pogloshchaet Damaged po oblasti |
| Set «Khronoevakuatsiya» | 6200 | 90 | -190 | Teleportiruet vybrannuyu gruppu domoy; 6 min |
| Orbitalnaya platforma «Zenit» | 7200 | 110 | -230 | Vysokotochnyy kineticheskiy udar; 8 min |

## EVA — kanonicheskie sistemnye repliki
| Sobytie | Replika |
| --- | --- |
| Start | Komandnaya set aktivna. all kanaly zashchishcheny. |
| Nizkaya energiya | Energeticheskiy rezerv nizhe bezopasnogo urovnya. |
| Baza atakovana | Obnaruzhena Attack na kriticheskuyu infrastrukturu. |
| Yunit gotov | Podrazdelenie zavershilo podgotovku. |
| Superoruzhie vraga | Strategicheskaya ugroza podtverzhdena. Idyot raschyot traektorii. |
| Razveddannye 100 | Polnyy paket razveddannykh sformirovan. |
| Pobeda | Tseli operatsii dostignuty. Poteri v dopustimykh predelakh. |
| Porazhenie | Set upravleniya razrushena. Operatsiya prekrashchena. |

## Svodnaya tablitsa yunitov
| Yunit | ID | Klass | Tir | Tsena | Vremya | Limit | HP | Armor | Skorost | Dalnost | DPS | Rol |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Strelok M6 «Sentinel» | AL_SentinelRifleman | Infantry | T1 | 220 | 7 | 1 | 120 | Lyogkaya Infantry | 5.0 | 6 | 11 | Universalnaya strelkovaya Infantry |
| Raketnyy raschyot FGM-31 «Lancer» | AL_LancerTeam | Infantry | T1 | 450 | 12 | 2 | 145 | Tyazhyolaya Infantry | 3.8 | 11 | 30 | Dalnyaya PT i PVO podderzhka |
| Polevoy Engineer E-4 | AL_FieldEngineer | Infantry | T1 | 550 | 15 | 1 | 105 | Lyogkaya Infantry | 4.2 | 0 | 0 | Zakhvat i bystryy remont |
| Snayper R-9 «Longwatch» | AL_LongwatchSniper | Infantry | T2 | 750 | 18 | 2 | 135 | Lyogkaya Infantry | 5.2 | 14 | 48 | Snayper, razvedka, obnaruzhenie skrytykh tseley |
| Polevoy medik M-12 «Lifeline» | AL_LifelineMedic | Infantry | T2 | 650 | 17 | 1 | 150 | Lyogkaya Infantry | 4.5 | 0 | 0 | Lechenie i snyatie negativnykh effektov |
| Spetsialist C-7 «Frostline» | AL_FrostlineSpecialist | Infantry | T3 | 1000 | 24 | 2 | 240 | Tyazhyolaya Infantry | 4.2 | 8 | 28 | Zamedlenie tyazhyolykh tseley i kontrol |
| Dobyvayushchaya platforma M88 «Pioneer» | AL_PioneerHarvester | Vehicles | T1 | 1450 | 27 | 4 | 1200 | Lyogkaya Vehicles | 4.0 | 0 | 0 | Bystryy Harvester i vremennyy forpost |
| Razvedmashina LAV-41 «Kestrel» | AL_KestrelScout | Vehicles | T1 | 700 | 15 | 3 | 430 | Lyogkaya Vehicles | 9.0 | 7 | 19 | Razvedka, pometka tseley i okhota na pekhotu |
| Osnovnoy Tank M14 «Bulwark» | AL_BulwarkMBT | Vehicles | T2 | 1250 | 25 | 5 | 1350 | Tyazhyolaya Vehicles | 5.2 | 9 | 43 | Mobilnyy osnovnoy Tank |
| Relsovaya SAU XM190 «Oracle» | AL_OracleArtillery | Vehicles | T2 | 1750 | 36 | 5 | 850 | Osadnaya Vehicles | 3.6 | 18 | 62 | Tochnaya dalnoboynaya artilleriya |
| Maskirovochnyy Tank XM27 «Refraction» | AL_RefractionTank | Vehicles | T3 | 1900 | 38 | 6 | 1200 | Tyazhyolaya Vehicles | 5.5 | 9 | 56 | Zasadnyy Tank i diversiya |
| Mobilnyy shchit M46 «Ward» | AL_WardShieldCarrier | Vehicles | T2 | 1600 | 34 | 5 | 1050 | Lyogkaya Vehicles | 4.4 | 0 | 0 | Podvizhnyy energeticheskiy shchit |
| Tyazhyolyy Tank M70 «Citadel» | AL_CitadelTank | Vehicles | T3 | 2800 | 52 | 9 | 3000 | Tyazhyolaya Vehicles | 4.0 | 11 | 84 | Tyazhyolyy Tank s aktivnoy zashchitoy |
| Istrebitel F/A-48 «Shrike» | AL_ShrikeInterceptor | Aviation | T2 | 1050 | 22 | 4 | 600 | Vozdushnaya | 13.0 | 13 | 50 | Chistyy vozdushnyy perekhvatchik |
| VTOL AV-27 «Vector» | AL_VectorVTOL | Aviation | T2 | 1450 | 30 | 5 | 950 | Vozdushnaya | 8.5 | 9 | 58 | Gibkiy shturmovik i tochechnyy udar |
| Stels-bombardirovshchik B-39 «Nightveil» | AL_NightveilBomber | Aviation | T3 | 2600 | 52 | 7 | 1500 | Vozdushnaya | 9.5 | 16 | 110 | Glubokiy tochechnyy udar po zdaniyam |
| Gidrofoyl PHM-22 «Manta» | AL_MantaPatrolCraft | Naval | T1 | 850 | 17 | 4 | 650 | Morskaya | 9.0 | 7 | 24 | Bystryy PVO-kater i razvedka |
| Esminets DDG-31 «Resolute» | AL_ResoluteDestroyer | Naval | T2 | 1900 | 37 | 7 | 2100 | Morskaya | 4.8 | 13 | 66 | Universalnyy korabl PVO i beregovogo ognya |
| Avianosets CVX-90 «Horizon» | AL_HorizonCarrier | Naval | T3 | 3600 | 65 | 10 | 3800 | Morskaya | 2.5 | 20 | 95 | Distantsionnaya morskaya aviatsionnaya platforma |
| Agent Evelin Khart | AL_Hero_Hart | Hero | T3 | 2500 | 48 | 8 | 750 | Lyogkaya Infantry | 5.8 | 15 | 82 | Hero razvedki, diversiy i tochechnykh ubiystv |

## Podrobnye kartochki yunitov
### 1. Strelok M6 «Sentinel» (`AL_SentinelRifleman`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_SentinelRifleman` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 220 |
| Vremya proizvodstva | 7 sek |
| Komandnyy limit | 1 |
| HP | 120 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 5.0 |
| Dalnost | 6 |
| Orientirovochnyy DPS | 11 |
| Prednaznachenie | Universalnaya strelkovaya Infantry |
| Osnovnoe Weapons | Modulnaya shturmovaya vintovka |
| Requirements | Takticheskaya Barracks |

#### Sposobnosti
- «Svetoshumovoy zaryad»: snizhaet tochnost vraga; 24 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Khoroshaya mobilnost i kontrol |
| Slabye storony | Dorozhe boytsa «Rubezh», slab protiv broni |
| Pryamye kontrmery | Oskolochnyy Damaged, Vehicles |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Strelok M6 «Sentinel» gotov. |
| Move | Perekhozhu k tochke. |
| Attack | Kontakt. Rabotayu. |
| Ability | Svetoshumovaya — poshla! |
| Damaged | under ognyom, no v stroyu. |
| Elite | Teper ya zadayu pravila kontakta. |
| Idle | Nazvanie optimistichnoe. Rabota — no. |
| Death | Sektor… ne uderzhan… |

### 2. Raketnyy raschyot FGM-31 «Lancer» (`AL_LancerTeam`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_LancerTeam` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 450 |
| Vremya proizvodstva | 12 sek |
| Komandnyy limit | 2 |
| HP | 145 |
| Tip broni | Tyazhyolaya Infantry |
| Skorost | 3.8 |
| Dalnost | 11 |
| Orientirovochnyy DPS | 30 |
| Prednaznachenie | Dalnyaya PT i PVO podderzhka |
| Osnovnoe Weapons | Upravlyaemaya raketa |
| Requirements | Barracks + razvedtsentr |

#### Sposobnosti
- «Lazernaya metka»: tsel poluchaet +20% urona ot vsekh soyuznikov 8 sek; 30 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Vysokaya dalnost, universalnaya raketa |
| Slabye storony | Trebuet soprovozhdeniya, medlennaya perezaryadka |
| Pryamye kontrmery | Snaypery, razvedchiki, artilleriya |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Lanser zakhvatil kanal. |
| Move | Nuzhna chistaya liniya. |
| Attack | Tsel podsvechena. Pusk. |
| Ability | Derzhu metku. Beyte seychas. |
| Damaged | Optika povrezhdena! |
| Elite | Ya vizhu slaboe mesto ranshe inzhenera. |
| Idle | Raketa dorogaya. Promakh — eshchyo dorozhe. |
| Death | Metka… poteryana… |

### 3. Polevoy Engineer E-4 (`AL_FieldEngineer`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_FieldEngineer` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 550 |
| Vremya proizvodstva | 15 sek |
| Komandnyy limit | 1 |
| HP | 105 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 4.2 |
| Dalnost | 0 |
| Orientirovochnyy DPS | 0 |
| Prednaznachenie | Zakhvat i bystryy remont |
| Osnovnoe Weapons | Ne vooruzhyon |
| Requirements | Barracks |

#### Sposobnosti
- «Remontnyy roy»: drony chinyat tekhniku na rasstoyanii; 35 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Chinit bezopasnee sovetskogo inzhenera |
| Slabye storony | Nizkaya zhivuchest |
| Pryamye kontrmery | Lyubaya boevaya edinitsa |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Inzhenernaya gruppa onlayn. |
| Move | Marshrut postroen. |
| Attack | U menya no boevogo paketa. |
| Ability | Remontnye drony — v rabotu. |
| Damaged | Zashchita kostyuma narushena! |
| Elite | Polomka — eto prosto nezavershyonnoe reshenie. |
| Idle | Proektirovali without dostupa k instruktsii. Klassika. |
| Death | Drony… zavershite remont… |

### 4. Snayper R-9 «Longwatch» (`AL_LongwatchSniper`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_LongwatchSniper` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 750 |
| Vremya proizvodstva | 18 sek |
| Komandnyy limit | 2 |
| HP | 135 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 5.2 |
| Dalnost | 14 |
| Orientirovochnyy DPS | 48 |
| Prednaznachenie | Snayper, razvedka, obnaruzhenie skrytykh tseley |
| Osnovnoe Weapons | Vysokotochnaya vintovka |
| Requirements | Barracks + razvedtsentr |

#### Sposobnosti
- «Skrytyy nablyudatel»: maskiruetsya nepodvizhno i uvelichivaet obzor; 4 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Ubivaet elitnuyu pekhotu, dayot razveddannye |
| Slabye storony | Slab protiv tekhniki i massovoy pekhoty |
| Pryamye kontrmery | Razvedchiki, artilleriya, Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Snayper R-9 «Longwatch» nablyudaet. |
| Move | Idu po myortvoy zone. |
| Attack | Odin vystrel. |
| Ability | Rastvoryayus v fone. |
| Damaged | Pozitsiya raskryta! |
| Elite | Ya uzhe videl ikh sleduyushchiy shag. |
| Idle | Tishina — luchshiy kamuflyazh. |
| Death | Kontakt… oborvan… |

### 5. Polevoy medik M-12 «Lifeline» (`AL_LifelineMedic`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_LifelineMedic` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 650 |
| Vremya proizvodstva | 17 sek |
| Komandnyy limit | 1 |
| HP | 150 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 4.5 |
| Dalnost | 0 |
| Orientirovochnyy DPS | 0 |
| Prednaznachenie | Lechenie i snyatie negativnykh effektov |
| Osnovnoe Weapons | Meditsinskie drony |
| Requirements | Barracks + razvedtsentr |

#### Sposobnosti
- «Stabilizatsiya»: vozvrashchaet soyuznoy pekhote 40% HP za 6 sek; 30 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Podderzhivaet doroguyu pekhotu |
| Slabye storony | Ne vooruzhyon, prioritetnaya tsel |
| Pryamye kontrmery | Snaypery, diversanty |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Meditsinskiy kanal otkryt. |
| Move | Idu k ranenym. |
| Attack | Ya lechu, a ne strelyayu. |
| Ability | Stabilizatsiya nachalas. |
| Damaged | Mediku nuzhna pomoshch! |
| Elite | Segodnya nikto ne ostayotsya na pole. |
| Idle | Luchshee lechenie — ne popadat under ogon. |
| Death | Aptechka… ryadom… |

### 6. Spetsialist C-7 «Frostline» (`AL_FrostlineSpecialist`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_FrostlineSpecialist` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 1000 |
| Vremya proizvodstva | 24 sek |
| Komandnyy limit | 2 |
| HP | 240 |
| Tip broni | Tyazhyolaya Infantry |
| Skorost | 4.2 |
| Dalnost | 8 |
| Orientirovochnyy DPS | 28 |
| Prednaznachenie | Zamedlenie tyazhyolykh tseley i kontrol |
| Osnovnoe Weapons | Kriogennyy izluchatel |
| Requirements | Laboratoriya fiziki |

#### Sposobnosti
- «Polnaya zamorozka»: obezdvizhivaet tsel na 4 sek; 34 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Kontroliruet dorogie tseli |
| Slabye storony | Nizkiy pryamoy Damaged, trebuet fokusa soyuznikov |
| Pryamye kontrmery | Snaypery, artilleriya |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Kriosistema stabilna. |
| Move | Temperaturu snizhaem po puti. |
| Attack | Okhlazhdayu tsel. |
| Ability | Polnaya zamorozka. |
| Damaged | Kontur khladagenta probit! |
| Elite | Absolyutnyy nol vsyo eshchyo nedostizhim. No my blizko. |
| Idle | Ne oblizyvayte oborudovanie. |
| Death | Temperatura… rastyot… |

### 7. Dobyvayushchaya platforma M88 «Pioneer» (`AL_PioneerHarvester`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_PioneerHarvester` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 1450 |
| Vremya proizvodstva | 27 sek |
| Komandnyy limit | 4 |
| HP | 1200 |
| Tip broni | Lyogkaya Vehicles |
| Skorost | 4.0 |
| Dalnost | 0 |
| Orientirovochnyy DPS | 0 |
| Prednaznachenie | Bystryy Harvester i vremennyy forpost |
| Osnovnoe Weapons | without oruzhiya |
| Requirements | Pererabotchik |

#### Sposobnosti
- «Razvernut forpost»: stanovitsya maloy remontno-stroitelnoy ploshchadkoy; povtornoe svorachivanie 10 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Gibkaya logistika, vysokaya skorost |
| Slabye storony | Menee prochnyy, chem Soviet Harvester |
| Pryamye kontrmery | Zasady, Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Payonir gotov k marshrutu. |
| Move | Optimiziruyu put. |
| Attack | Boevoy paket otsutstvuet. |
| Ability | Razvorachivayu polevoy uzel. |
| Damaged | Gruzovoy modul povrezhdyon! |
| Elite | Ya nakhozhu pribyl tam, gde drugie vidyat kamni. |
| Idle | Ruda ne redkaya. Vremya — redkoe. |
| Death | Marshrut… prervan… |

### 8. Razvedmashina LAV-41 «Kestrel» (`AL_KestrelScout`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_KestrelScout` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 700 |
| Vremya proizvodstva | 15 sek |
| Komandnyy limit | 3 |
| HP | 430 |
| Tip broni | Lyogkaya Vehicles |
| Skorost | 9.0 |
| Dalnost | 7 |
| Orientirovochnyy DPS | 19 |
| Prednaznachenie | Razvedka, pometka tseley i okhota na pekhotu |
| Osnovnoe Weapons | Impulsnaya avtopushka |
| Requirements | Modulnyy Factory |

#### Sposobnosti
- «Aktivnyy skan»: raskryvaet skrytye tseli; 20 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Samyy bystryy nazemnyy razvedchik |
| Slabye storony | Ochen khrupkiy |
| Pryamye kontrmery | PT-Infantry, miny |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Kestrel v seti. |
| Move | Prolozhu korotkiy put. |
| Attack | Tsel otmechena i atakovana. |
| Ability | Aktivnyy skan. |
| Damaged | Korpus ne rasschitan na eto! |
| Elite | Ya nakhozhu tseli eshchyo before zaprosa. |
| Idle | Stoyat na meste — ne moya spetsializatsiya. |
| Death | Signal… poteryan… |

### 9. Osnovnoy Tank M14 «Bulwark» (`AL_BulwarkMBT`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_BulwarkMBT` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1250 |
| Vremya proizvodstva | 25 sek |
| Komandnyy limit | 5 |
| HP | 1350 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 5.2 |
| Dalnost | 9 |
| Orientirovochnyy DPS | 43 |
| Prednaznachenie | Mobilnyy osnovnoy Tank |
| Osnovnoe Weapons | Kompozitnaya pushka |
| Requirements | Factory + razvedtsentr |

#### Sposobnosti
- «Tseleukazatel»: snizhaet bronyu tseli na 20% na 8 sek; 28 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Skorost i komandnaya sinergiya |
| Slabye storony | Nizhe HP, chem u «Granita» |
| Pryamye kontrmery | Tyazhyolye tanki, PT-Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Bulvark gotov prikryt. |
| Move | Derzhim temp. |
| Attack | Bronetsel podtverzhdena. |
| Ability | Snimayu zashchitnyy profil. |
| Damaged | Kompozit probit! |
| Elite | Ya ne derzhu liniyu. Ya dvigayu eyo. |
| Idle | Luchshiy shchit — pravilnaya distantsiya. |
| Death | Sistema zashchity… oflayn… |

### 10. Relsovaya SAU XM190 «Oracle» (`AL_OracleArtillery`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_OracleArtillery` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1750 |
| Vremya proizvodstva | 36 sek |
| Komandnyy limit | 5 |
| HP | 850 |
| Tip broni | Osadnaya Vehicles |
| Skorost | 3.6 |
| Dalnost | 18 |
| Orientirovochnyy DPS | 62 |
| Prednaznachenie | Tochnaya dalnoboynaya artilleriya |
| Osnovnoe Weapons | Relsovyy uskoritel |
| Requirements | Factory + razvedtsentr |

#### Sposobnosti
- «Sinkhronnyy zalp»: moshchnyy vystrel after 3 sek navedeniya; 36 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Tochnaya, pochti without razbrosa |
| Slabye storony | Khrupkaya, nuzhdaetsya v razvedke |
| Pryamye kontrmery | Razvedchiki, Aviation, teleport |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Orakul rasschityvaet reshenie. |
| Move | Menyayu ognevuyu pozitsiyu. |
| Attack | Rels zaryazhen. |
| Ability | Sinkhronnyy zalp via tri sekundy. |
| Damaged | Stabilizator povrezhdyon! |
| Elite | Uravnenie boya imeet odin otvet. |
| Idle | Tochnost nachinaetsya s terpeniya. |
| Death | Magnitnyy kontur… razrushen… |

### 11. Maskirovochnyy Tank XM27 «Refraction» (`AL_RefractionTank`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_RefractionTank` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 1900 |
| Vremya proizvodstva | 38 sek |
| Komandnyy limit | 6 |
| HP | 1200 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 5.5 |
| Dalnost | 9 |
| Orientirovochnyy DPS | 56 |
| Prednaznachenie | Zasadnyy Tank i diversiya |
| Osnovnoe Weapons | Teplovoy luch |
| Requirements | Laboratoriya fiziki |

#### Sposobnosti
- «Opticheskaya maskirovka»: v nepodvizhnosti stanovitsya nevidimym; pervyy vystrel +35% Damaged

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Silnyy pervyy udar i maskirovka |
| Slabye storony | Slab with dlitelnom obmene |
| Pryamye kontrmery | Skanery, oskolochnyy ogon, Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Refraktsiya uzhe na pozitsii. |
| Move | Smenyu fon. |
| Attack | Maskirovka snyata. Ogon. |
| Ability | Optika perestraivaetsya. |
| Damaged | Kontur maskirovki sorvan! |
| Elite | Oni strelyayut v to, chego uzhe no. |
| Idle | Derevya ne dokladyvayut po radio. |
| Death | Izobrazhenie… ischezaet… |

### 12. Mobilnyy shchit M46 «Ward» (`AL_WardShieldCarrier`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_WardShieldCarrier` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1600 |
| Vremya proizvodstva | 34 sek |
| Komandnyy limit | 5 |
| HP | 1050 |
| Tip broni | Lyogkaya Vehicles |
| Skorost | 4.4 |
| Dalnost | 0 |
| Orientirovochnyy DPS | 0 |
| Prednaznachenie | Podvizhnyy energeticheskiy shchit |
| Osnovnoe Weapons | Ne vooruzhyon |
| Requirements | Factory + razvedtsentr |

#### Sposobnosti
- «Proektsiya»: sozdayot napravlennyy shchit na 12 sek; 32 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Zashchishchaet artilleriyu i pekhotu |
| Slabye storony | Bezoruzhen, vysokoe energopotreblenie |
| Pryamye kontrmery | Flang, elektrichestvo, fokus |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Uord gotov k proektsii. |
| Move | Peremeshchayu zashchitnyy sektor. |
| Attack | Pryamogo vooruzheniya no. |
| Ability | Shchit razvyornut. |
| Damaged | Pole prosedaet! |
| Elite | Luchshiy vystrel vraga — tot, chto ne doshyol. |
| Idle | Shchit ne vyglyadit geroicheski. Zato working. |
| Death | Proektsiya… pogasla… |

### 13. Tyazhyolyy Tank M70 «Citadel» (`AL_CitadelTank`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_CitadelTank` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 2800 |
| Vremya proizvodstva | 52 sek |
| Komandnyy limit | 9 |
| HP | 3000 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 4.0 |
| Dalnost | 11 |
| Orientirovochnyy DPS | 84 |
| Prednaznachenie | Tyazhyolyy Tank s aktivnoy zashchitoy |
| Osnovnoe Weapons | Elektromagnitnaya pushka |
| Requirements | Laboratoriya + dva zavoda |

#### Sposobnosti
- «Aktivnaya zashchita»: perekhvatyvaet 6 raket/snaryadov; 40 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Silnyy dalniy boy, zashchita ot raket |
| Slabye storony | Dorog, khuzhe protiv massovoy pekhoty |
| Pryamye kontrmery | Oskolochnaya Infantry, blizhniy boy, EMP |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Tsitadel derzhit stroy. |
| Move | Tyazhyolyy modul v dvizhenii. |
| Attack | Elektromagnitnyy vystrel. |
| Ability | Aktivnaya zashchita vklyuchena. |
| Damaged | Perekhvatchiki ischerpany! |
| Elite | My vyigryvaem eshchyo before popadaniya. |
| Idle | Rytsarskiy kodeks obnovlyon before versii chetyre. |
| Death | Zashchitnyy kontur… ne otvechaet… |

### 14. Istrebitel F/A-48 «Shrike» (`AL_ShrikeInterceptor`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_ShrikeInterceptor` |
| Kategoriya | Aviation |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1050 |
| Vremya proizvodstva | 22 sek |
| Komandnyy limit | 4 |
| HP | 600 |
| Tip broni | Vozdushnaya |
| Skorost | 13.0 |
| Dalnost | 13 |
| Orientirovochnyy DPS | 50 |
| Prednaznachenie | Chistyy vozdushnyy perekhvatchik |
| Osnovnoe Weapons | Rakety vozdukh-vozdukh |
| Requirements | Aviabaza |

#### Sposobnosti
- «Perekhvat»: mgnovenno uskoryaetsya k vybrannoy vozdushnoy tseli; 24 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Luchshiy istrebitel po skorosti |
| Slabye storony | Pochti bespolezen po zemle |
| Pryamye kontrmery | PVO, chislennoe prevoskhodstvo |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Shrayk gotov k perekhvatu. |
| Move | Zanimayu eshelon. |
| Attack | Vozdushnaya tsel zakhvachena. |
| Ability | Perekhvat podtverzhdyon. |
| Damaged | Krylo povrezhdeno! |
| Elite | Nebo nachinaetsya s moego razresheniya. |
| Idle | Radar chist. Eto nenadolgo. |
| Death | Bort… poteryan… |

### 15. VTOL AV-27 «Vector» (`AL_VectorVTOL`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_VectorVTOL` |
| Kategoriya | Aviation |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1450 |
| Vremya proizvodstva | 30 sek |
| Komandnyy limit | 5 |
| HP | 950 |
| Tip broni | Vozdushnaya |
| Skorost | 8.5 |
| Dalnost | 9 |
| Orientirovochnyy DPS | 58 |
| Prednaznachenie | Gibkiy shturmovik i tochechnyy udar |
| Osnovnoe Weapons | Upravlyaemye rakety i pushka |
| Requirements | Aviabaza + razvedtsentr |

#### Sposobnosti
- «Vertikalnaya zasada»: zavisaet za relefom i poluchaet +25% pervyy zalp

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Tochnyy, mozhet zavisat |
| Slabye storony | Srednyaya zhivuchest, dorogaya perezaryadka |
| Pryamye kontrmery | PVO, istrebiteli |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Vektor na vertikalnoy tyage. |
| Move | Perekhozhu k tochke zavisaniya. |
| Attack | Paket na tsel. |
| Ability | Zavisanie. Skryvayu signaturu. |
| Damaged | Tyaga nestabilna! |
| Elite | Mne ne nuzhna polosa, chtoby izmenit boy. |
| Idle | Vertikalnyy vzlyot ekonomit vremya. Ne toplivo. |
| Death | Tyaga… padaet… |

### 16. Stels-bombardirovshchik B-39 «Nightveil» (`AL_NightveilBomber`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_NightveilBomber` |
| Kategoriya | Aviation |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 2600 |
| Vremya proizvodstva | 52 sek |
| Komandnyy limit | 7 |
| HP | 1500 |
| Tip broni | Vozdushnaya |
| Skorost | 9.5 |
| Dalnost | 16 |
| Orientirovochnyy DPS | 110 |
| Prednaznachenie | Glubokiy tochechnyy udar po zdaniyam |
| Osnovnoe Weapons | Korrektiruemye bomby |
| Requirements | Laboratoriya + aviabaza |

#### Sposobnosti
- «Rezhim teni»: ne obnaruzhivaetsya obychnym radarom before sbrosa; 45 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Skrytnyy, tochnyy, vysokiy Damaged |
| Slabye storony | Dorogoy i uyazvim after ataki |
| Pryamye kontrmery | Spetsradar, istrebiteli |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Naytveyl gotov k nevidimomu marshrutu. |
| Move | Vkhozhu v ten. |
| Attack | Tochka sbrosa podtverzhdena. |
| Ability | Signatura obnulena. |
| Damaged | Nas vidyat! |
| Elite | Luchshiy udar — tot, kotoryy nikto ne uspel zametit. |
| Idle | Otsutstvie na radare — tozhe prisutstvie. |
| Death | Stels… narushen navsegda… |

### 17. Gidrofoyl PHM-22 «Manta» (`AL_MantaPatrolCraft`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_MantaPatrolCraft` |
| Kategoriya | Naval |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 850 |
| Vremya proizvodstva | 17 sek |
| Komandnyy limit | 4 |
| HP | 650 |
| Tip broni | Morskaya |
| Skorost | 9.0 |
| Dalnost | 7 |
| Orientirovochnyy DPS | 24 |
| Prednaznachenie | Bystryy PVO-kater i razvedka |
| Osnovnoe Weapons | Avtopushka i rakety PVO |
| Requirements | Okeanicheskiy dok |

#### Sposobnosti
- «Radiopodavlenie»: otklyuchaet Weapons odnogo korablya na 5 sek; 28 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Ochen mobilnyy, silnaya podderzhka |
| Slabye storony | Slabaya Armor |
| Pryamye kontrmery | Krupnye korabli, beregovye batarei |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Manta na krylyakh. |
| Move | Podnimaemsya nad volnoy. |
| Attack | Ogon po bortu! |
| Ability | Glushu oruzheynyy kanal. |
| Damaged | Krylo zatsepilo! |
| Elite | Skorost — nashe bronirovanie. |
| Idle | Voda snizu. Nebo sverkhu. Udobno. |
| Death | Teryaem podyom… |

### 18. Esminets DDG-31 «Resolute» (`AL_ResoluteDestroyer`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_ResoluteDestroyer` |
| Kategoriya | Naval |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1900 |
| Vremya proizvodstva | 37 sek |
| Komandnyy limit | 7 |
| HP | 2100 |
| Tip broni | Morskaya |
| Skorost | 4.8 |
| Dalnost | 13 |
| Orientirovochnyy DPS | 66 |
| Prednaznachenie | Universalnyy korabl PVO i beregovogo ognya |
| Osnovnoe Weapons | Rakety i skorostrelnaya pushka |
| Requirements | Dok + razvedtsentr |

#### Sposobnosti
- «Sonarnyy impuls»: raskryvaet podlodki v oblasti; 30 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Universalen, nadyozhnoe PVO |
| Slabye storony | Ne prevoskhodit spetsializirovannye korabli |
| Pryamye kontrmery | Tyazhyolye kreysery, fokus podlodok |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Rezolyut v boevoy seti. |
| Move | Menyayu kurs. |
| Attack | Orudiya sinkhronizirovany. |
| Ability | Sonarnyy impuls. |
| Damaged | Proboina vyshe vaterlinii! |
| Elite | Naval derzhitsya na tekh, kto vidit vsyo. |
| Idle | More ne neytralno. Ono prosto zhdyot. |
| Death | Korabl pokidayut… |

### 19. Avianosets CVX-90 «Horizon» (`AL_HorizonCarrier`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_HorizonCarrier` |
| Kategoriya | Naval |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 3600 |
| Vremya proizvodstva | 65 sek |
| Komandnyy limit | 10 |
| HP | 3800 |
| Tip broni | Morskaya |
| Skorost | 2.5 |
| Dalnost | 20 |
| Orientirovochnyy DPS | 95 |
| Prednaznachenie | Distantsionnaya morskaya aviatsionnaya platforma |
| Osnovnoe Weapons | Roy udarnykh dronov |
| Requirements | Dok + laboratoriya |

#### Sposobnosti
- «Polnyy aviapaket»: zapuskaet 8 dronov po oblasti; 55 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Atakuet izdaleka, gibkie tseli |
| Slabye storony | without prikrytiya uyazvim for podlodok i aviatsii |
| Pryamye kontrmery | Podlodki, tyazhyolye rakety |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Gorizont razvorachivaet aviakrylo. |
| Move | Kurs avianosnoy gruppy prinyat. |
| Attack | Drony idut na tsel. |
| Ability | Polnyy aviapaket — zapusk. |
| Damaged | Polyotnaya paluba povrezhdena! |
| Elite | My prinosim nebo tuda, gde ego ne before. |
| Idle | Avianosets — eto Airfield, kotoryy umeet ukhodit. |
| Death | Paluba… zakryta… |

### 20. Agent Evelin Khart (`AL_Hero_Hart`)
| Parametr | Value |
| --- | --- |
| Stable ID | `AL_Hero_Hart` |
| Kategoriya | Hero |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 2500 |
| Vremya proizvodstva | 48 sek |
| Komandnyy limit | 8 |
| HP | 750 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 5.8 |
| Dalnost | 15 |
| Orientirovochnyy DPS | 82 |
| Prednaznachenie | Hero razvedki, diversiy i tochechnykh ubiystv |
| Osnovnoe Weapons | Snayperskaya sistema i boevye drony |
| Requirements | Laboratoriya |

#### Sposobnosti
- «Prizrachnyy protokol»: polnaya maskirovka na 10 sek; 45 sek
- «Vzlom»: vremenno otklyuchaet vrazheskoe zdanie or tekhniku; 50 sek
- Odin ekzemplyar

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Razvedka, ubiystvo klyuchevykh tseley, diversiya |
| Slabye storony | Khrupkaya, trebuet mikroupravleniya |
| Pryamye kontrmery | Skanery, oskolochnyy Damaged, massovyy ogon |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Khart na linii. Kakaya tsel vazhnee voyny? |
| Move | Uzhe v ikh slepoy zone. |
| Attack | Udalyayu problemu. |
| Ability | Protokol prizraka aktiven. |
| Damaged | Menya vsyo-taki zametili. |
| Elite | Razvedka zakonchilas. Teper ya znayu vsyo nuzhnoe. |
| Idle | Sekrety vsegda deshevle shturma. |
| Death | Dannye… otpravleny… |

# Fraktsiya: Vostochnaya Coalition
## Fraktsionnaya identichnost
Sbalansirovannaya tekhnologicheskaya armiya, postroennaya vokrug postroeniy, energeticheskikh svyazey, shchitov i kombinirovaniya roley. Slabosti: trebuet distsipliny, svyazannoy infrastruktury i plotnogo vzaimodeystviya yunitov.
## Fraktsionnyy resurs: Sinkhronizatsiya
Sinkhronizatsiya rasschityvaetsya iz chisla podklyuchyonnykh zdaniy, otryadov v postroenii i aktivnykh komandnykh uzlov. 25: +5% tochnost. 50: lyogkiy regen shchitov. 75: +10% skorost proizvodstva. 100: all svyazannye Units poluchayut +15% soprotivlenie uronu. Poterya uzlov mozhet bystro obrushit pokazatel.
## Buildings i Economy Factions
| Zdanie | Tsena | Vremya, s | Energiya | Naznachenie |
| --- | --- | --- | --- | --- |
| Mobilnyy uzel garmonii | 5000 | 60 | 0 | Razvorachivaetsya v komandnyy dvorets |
| Komandnyy dvorets | — | — | +105 | Yadro seti, +20 limita |
| Solnechnyy kollektor | 850 | 19 | +125 | Energiya usilivaetsya ryadom s drugimi kollektorami |
| Rudnyy sintezator | 2450 | 44 | -20 | Vklyuchaet platformu GRP-12 «Yuan» |
| Zal podgotovki | 720 | 18 | -15 | Infantry, +5 limita |
| Fabrika shagokhodov | 2250 | 41 | -40 | Nazemnaya Vehicles, +10 limita |
| Vozdushnaya pagoda | 1900 | 35 | -45 | Aviation i drony |
| Prilivnyy dok | 2050 | 40 | -45 | Naval |
| Bashnya koordinatsii | 1500 | 29 | -55 | Radar, set, T2 |
| Tsitadel issledovaniy | 3650 | 61 | -105 | T3 i shchity |
| Avtomaticheskaya turel «Igla» | 720 | 16 | -15 | Protiv pekhoty |
| Nebesnoe kopyo | 980 | 21 | -28 | PVO |
| Relsovaya bashnya «Nebesnyy sud» | 2050 | 36 | -78 | Protiv tyazhyoloy tekhniki |
| Uzel garmonicheskogo shchita | 1750 | 30 | -80 | Shchitovaya zona |
| Matritsa «Desyat tysyach shchitov» | 6100 | 90 | -190 | Globalno usilivaet shchity na 15 sekund |
| Seysmicheskiy rezonator | 7000 | 108 | -225 | Seriya udarnykh voln po oblasti; 8 min |

## EVA — kanonicheskie sistemnye repliki
| Sobytie | Replika |
| --- | --- |
| Start | Komandnyy dvorets soedinyon s setyu. Garmoniya ustanovlena. |
| Nizkaya energiya | Energeticheskie svyazi nestabilny. |
| Baza atakovana | Protivnik narushaet tselostnost nashey seti. |
| Yunit gotov | Novoe zveno vstupilo v stroy. |
| Superoruzhie vraga | Obnaruzheno strategicheskoe vozmushchenie. |
| Sinkhronizatsiya 100 | Set dostigla polnoy sinkhronizatsii. |
| Pobeda | Protivnik lishyon koordinatsii. Pole stabilizirovano. |
| Porazhenie | Svyaz between uzlami poteryana. Sistema raspalas. |

## Svodnaya tablitsa yunitov
| Yunit | ID | Klass | Tir | Tsena | Vremya | Limit | HP | Armor | Skorost | Dalnost | DPS | Rol |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Strelok Tip 21 «Tsyanvey» | CO_QianweiRifleman | Infantry | T1 | 200 | 7 | 1 | 125 | Lyogkaya Infantry | 4.8 | 6 | 10 | Bazovaya stroevaya Infantry |
| Protivotankovyy raschyot AT-8 «Vadzhra» | CO_VajraLancer | Infantry | T1 | 420 | 11 | 2 | 170 | Tyazhyolaya Infantry | 4.0 | 8 | 26 | PT i antimekhanicheskaya Infantry |
| Tekhnik seti Tip 06 «Tsze» | CO_JieTechnician | Infantry | T1 | 520 | 15 | 1 | 105 | Lyogkaya Infantry | 4.0 | 0 | 0 | Zakhvat, remont i usilenie seti |
| Fazovyy strelok QBS-19 «Shengun» | CO_ShengongMarksman | Infantry | T2 | 720 | 18 | 2 | 140 | Lyogkaya Infantry | 4.6 | 13 | 44 | Snayper protiv tyazhyoloy pekhoty i shchitov |
| Nanitnyy medik NM-7 «Sandzhivani» | CO_SanjivaniMedic | Infantry | T2 | 700 | 18 | 1 | 155 | Lyogkaya Infantry | 4.3 | 0 | 0 | Lechenie i vremennye shchity |
| Pochyotnyy strazh HG-33 «Raksha» | CO_RakshaGuard | Infantry | T3 | 1050 | 25 | 2 | 300 | Tyazhyolaya Infantry | 4.1 | 7 | 40 | Elitnyy zashchitnik komandirov i uzlov |
| Dobyvayushchaya platforma GRP-12 «Yuan» | CO_YuanCollector | Vehicles | T1 | 1425 | 27 | 4 | 1350 | Tyazhyolaya Vehicles | 3.5 | 0 | 0 | Dobycha i podderzhka energeticheskoy seti |
| Razvedshagokhod Tip 17 «Kamakiri» | CO_KamakiriWalker | Vehicles | T1 | 750 | 16 | 3 | 560 | Lyogkaya Vehicles | 7.0 | 7 | 21 | Razvedka, borba s pekhotoy i vertikalnyy relef |
| Osnovnoy Tank ZTZ-61 «Tsinlun» | CO_QinglongMBT | Vehicles | T2 | 1300 | 27 | 5 | 1500 | Tyazhyolaya Vehicles | 4.8 | 9 | 45 | Osnovnoy Tank s gruppovym shchitom |
| Artilleriya PHL-29 «Musson» | CO_MonsoonArtillery | Vehicles | T2 | 1650 | 35 | 5 | 900 | Osadnaya Vehicles | 3.4 | 17 | 60 | Mnogostupenchataya artilleriya po ploshchadi |
| Shchitovoy nositel Tip 42 «Seymon» | CO_SeimonShieldCarrier | Vehicles | T2 | 1700 | 36 | 6 | 1800 | Tyazhyolaya Vehicles | 3.8 | 0 | 0 | Podvizhnyy uzel shchita i Sinkhronizatsii |
| Shturmovoy shagokhod MBT-X «Ayravata» | CO_AiravataWalker | Vehicles | T3 | 2500 | 48 | 8 | 2700 | Tyazhyolaya Vehicles | 4.6 | 10 | 80 | Tyazhyolyy universalnyy shagokhod |
| Mobilnaya krepost ZTD-90 «Tyanmen» | CO_TianmenFortress | Vehicles | T3 | 3300 | 60 | 10 | 3900 | Tyazhyolaya Vehicles | 2.7 | 12 | 95 | Sverkhtyazhyolyy komandnyy uzel i artilleriya |
| Razveddron UAV-12 «Kavasemi» | CO_KawasemiDrone | Aviation | T1 | 700 | 14 | 3 | 360 | Vozdushnaya | 14.0 | 6 | 18 | Razvedka, obnaruzhenie i presledovanie |
| Shturmovik Z-28 «Leykhe» | CO_LeiheGunship | Aviation | T2 | 1550 | 32 | 5 | 1150 | Vozdushnaya | 7.8 | 8 | 62 | Shturm nazemnykh tseley i podderzhka postroeniy |
| Bombardirovshchik H-26 «Agnipaksha» | CO_AgnipakshaBomber | Aviation | T3 | 2550 | 50 | 7 | 1650 | Vozdushnaya | 8.8 | 14 | 105 | Ploshchadnoy bombardirovshchik i vozgoranie |
| Korvet Tip 32 «Kadzekiri» | CO_KazekiriCorvette | Naval | T1 | 900 | 18 | 4 | 780 | Morskaya | 7.5 | 8 | 28 | Bystryy perekhvat i torpednaya Attack |
| Relsovyy kreyser Tip 81 «Syuanu» | CO_XuanwuCruiser | Naval | T2 | 2200 | 42 | 8 | 2600 | Morskaya | 4.0 | 16 | 78 | Dalniy korabl protiv tyazhyolykh tseley |
| Podvodnyy avianosets SSGN-18 «Samudra» | CO_SamudraCarrier | Naval | T3 | 3700 | 68 | 10 | 4200 | Morskaya | 2.6 | 18 | 100 | Skrytaya baza morskikh dronov |
| Commander Mey Tszyan | CO_Hero_Mei | Hero | T3 | 2550 | 49 | 8 | 820 | Tyazhyolaya Infantry | 5.2 | 9 | 68 | Hero postroeniy, shchitov i komandnykh svyazey |

## Podrobnye kartochki yunitov
### 1. Strelok Tip 21 «Tsyanvey» (`CO_QianweiRifleman`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_QianweiRifleman` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 200 |
| Vremya proizvodstva | 7 sek |
| Komandnyy limit | 1 |
| HP | 125 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 4.8 |
| Dalnost | 6 |
| Orientirovochnyy DPS | 10 |
| Prednaznachenie | Bazovaya stroevaya Infantry |
| Osnovnoe Weapons | Impulsnaya vintovka |
| Requirements | Zal podgotovki |

#### Sposobnosti
- «Stroy»: ryadom s dvumya boytsami «Tsyanvey» poluchaet +15% zashchity

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Effektiven v gruppakh |
| Slabye storony | Slab with rasseivanii |
| Pryamye kontrmery | Oskolochnyy Damaged, snaypery |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Strelok Tip 21 «Tsyanvey» gotov zanyat mesto. |
| Move | Sokhranyaem interval. |
| Attack | Obshchiy ogon. |
| Ability | Stroy zamknut. |
| Damaged | Liniya narushena! |
| Elite | Odin shag, odin ritm, odna pobeda. |
| Idle | Dazhe Idle dolzhno byt organizovano. |
| Death | Moyo mesto… zaymyot sleduyushchiy… |

### 2. Protivotankovyy raschyot AT-8 «Vadzhra» (`CO_VajraLancer`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_VajraLancer` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 420 |
| Vremya proizvodstva | 11 sek |
| Komandnyy limit | 2 |
| HP | 170 |
| Tip broni | Tyazhyolaya Infantry |
| Skorost | 4.0 |
| Dalnost | 8 |
| Orientirovochnyy DPS | 26 |
| Prednaznachenie | PT i antimekhanicheskaya Infantry |
| Osnovnoe Weapons | Elektromagnitnoe kopyo |
| Requirements | Zal podgotovki |

#### Sposobnosti
- «Impulsnyy vypad»: korotkiy ryvok i otklyuchenie lyogkoy tekhniki na 2 sek; 24 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Khorosh protiv tekhniki v blizhney zone |
| Slabye storony | Uyazvim for strelkov i snayperov |
| Pryamye kontrmery | Strelkovaya Infantry, artilleriya |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Vadzhra zaryazhena. |
| Move | Sokrashchaem distantsiyu. |
| Attack | Probit serdtsevinu. |
| Ability | Impulsnyy vypad! |
| Damaged | Pole kopya nestabilno! |
| Elite | Stal tozhe znaet strakh. |
| Idle | Dlinnoe Weapons uchit derzhat distantsiyu. |
| Death | Kopyo… pogaslo… |

### 3. Tekhnik seti Tip 06 «Tsze» (`CO_JieTechnician`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_JieTechnician` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 520 |
| Vremya proizvodstva | 15 sek |
| Komandnyy limit | 1 |
| HP | 105 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 4.0 |
| Dalnost | 0 |
| Orientirovochnyy DPS | 0 |
| Prednaznachenie | Zakhvat, remont i usilenie seti |
| Osnovnoe Weapons | Servisnyy roy |
| Requirements | Zal podgotovki |

#### Sposobnosti
- «Svyazat uzel»: vremenno podklyuchaet izolirovannoe zdanie k seti
- «Remontnyy roy»: remont tekhniki

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Klyuch k Sinkhronizatsii |
| Slabye storony | Bezoruzhen |
| Pryamye kontrmery | Lyubaya boevaya edinitsa |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Tekhnik seti Tip 06 «Tsze» gotov. |
| Move | Prokladyvayu svyaz. |
| Attack | Moya zadacha — vosstanovlenie. |
| Ability | Uzel podklyuchyon. |
| Damaged | Roy teryaet apparaty! |
| Elite | Razryvov v seti bolshe no. |
| Idle | Provodov ne vidno. Oshibki — vidno. |
| Death | Svyaz… peredana… |

### 4. Fazovyy strelok QBS-19 «Shengun» (`CO_ShengongMarksman`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_ShengongMarksman` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 720 |
| Vremya proizvodstva | 18 sek |
| Komandnyy limit | 2 |
| HP | 140 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 4.6 |
| Dalnost | 13 |
| Orientirovochnyy DPS | 44 |
| Prednaznachenie | Snayper protiv tyazhyoloy pekhoty i shchitov |
| Osnovnoe Weapons | Fazovaya vintovka |
| Requirements | Zal + Bashnya koordinatsii |

#### Sposobnosti
- «Proboy shchita»: sleduyushchiy vystrel ignoriruet shchit; 28 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Tochnaya kontrelita |
| Slabye storony | Slab protiv tekhniki i mass pekhoty |
| Pryamye kontrmery | Razvedchiki, Vehicles |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Shengun sovmestil fazu. |
| Move | Ishchu chistuyu geometriyu. |
| Attack | Prokhozhu between sloyami. |
| Ability | Shchit ne schitaetsya pregradoy. |
| Damaged | Fazovyy kontur sbit! |
| Elite | Ya strelyayu tuda, gde zashchita eshchyo ne voznikla. |
| Idle | Tishina pomogaet uvidet sloi. |
| Death | Faza… poteryana… |

### 5. Nanitnyy medik NM-7 «Sandzhivani» (`CO_SanjivaniMedic`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_SanjivaniMedic` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 700 |
| Vremya proizvodstva | 18 sek |
| Komandnyy limit | 1 |
| HP | 155 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 4.3 |
| Dalnost | 0 |
| Orientirovochnyy DPS | 0 |
| Prednaznachenie | Lechenie i vremennye shchity |
| Osnovnoe Weapons | Nanitnyy roy |
| Requirements | Zal + Bashnya koordinatsii |

#### Sposobnosti
- «Zashchitnaya obolochka»: dayot 200 shchita na 10 sek; 32 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Lechit i zashchishchaet |
| Slabye storony | Bezoruzhen |
| Pryamye kontrmery | Snaypery, fokus |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Sandzhivani gotova k raspredeleniyu. |
| Move | Sleduyu za postroeniem. |
| Attack | U menya no boevogo rezhima. |
| Ability | Zashchitnaya obolochka sformirovana. |
| Damaged | Roy raskhoduetsya na menya! |
| Elite | Nanity uzhe znayut kazhduyu ranu. |
| Idle | Organizm — slozhnaya mashina. No chinitsya bystree. |
| Death | Roy… ishchet novogo nositelya… |

### 6. Pochyotnyy strazh HG-33 «Raksha» (`CO_RakshaGuard`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_RakshaGuard` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 1050 |
| Vremya proizvodstva | 25 sek |
| Komandnyy limit | 2 |
| HP | 300 |
| Tip broni | Tyazhyolaya Infantry |
| Skorost | 4.1 |
| Dalnost | 7 |
| Orientirovochnyy DPS | 40 |
| Prednaznachenie | Elitnyy zashchitnik komandirov i uzlov |
| Osnovnoe Weapons | Plazmennyy klinok i shchit |
| Requirements | Tsitadel issledovaniy |

#### Sposobnosti
- «Otrazhenie»: 4 sek otrazhaet 40% dalnego urona; 35 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Ochen zhivuch v blizhnem boyu |
| Slabye storony | Malaya dalnost, dorogoy |
| Pryamye kontrmery | Artilleriya, Aviation, krio |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Raksha prinimaet otvetstvennost. |
| Move | Ya zakroyu etot put. |
| Attack | Vstupayu v blizhniy boy. |
| Ability | Otrazit udar. |
| Damaged | Shchit vyderzhit. |
| Elite | Poka ya stoyu, liniya sushchestvuet. |
| Idle | Chest — eto distsiplina without svideteley. |
| Death | Liniya… ne dolzhna drognut… |

### 7. Dobyvayushchaya platforma GRP-12 «Yuan» (`CO_YuanCollector`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_YuanCollector` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 1425 |
| Vremya proizvodstva | 27 sek |
| Komandnyy limit | 4 |
| HP | 1350 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 3.5 |
| Dalnost | 0 |
| Orientirovochnyy DPS | 0 |
| Prednaznachenie | Dobycha i podderzhka energeticheskoy seti |
| Osnovnoe Weapons | without oruzhiya |
| Requirements | Rudnyy sintezator |

#### Sposobnosti
- «Energosvyaz»: ryadom so zdaniyami dayot +10 Sinkhronizatsii

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Polezen ne only ekonomicheski |
| Slabye storony | Srednyaya skorost, prioritetnaya tsel |
| Pryamye kontrmery | PT-zasady, Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Yuan podklyuchyon. |
| Move | Marshrut vkhodit v set. |
| Attack | Vooruzhenie ne predusmotreno. |
| Ability | Energosvyaz ustanovlena. |
| Damaged | Konteyner povrezhdyon! |
| Elite | Kazhdyy gruz ukreplyaet set. |
| Idle | Resurs tsenen only after dostavki. |
| Death | Potok… ostanovlen… |

### 8. Razvedshagokhod Tip 17 «Kamakiri» (`CO_KamakiriWalker`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_KamakiriWalker` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 750 |
| Vremya proizvodstva | 16 sek |
| Komandnyy limit | 3 |
| HP | 560 |
| Tip broni | Lyogkaya Vehicles |
| Skorost | 7.0 |
| Dalnost | 7 |
| Orientirovochnyy DPS | 21 |
| Prednaznachenie | Razvedka, borba s pekhotoy i vertikalnyy relef |
| Osnovnoe Weapons | Dve impulsnye pushki |
| Requirements | Fabrika shagokhodov |

#### Sposobnosti
- «Stennoy shag»: preodolevaet malye ustupy i barrikady

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Vysokaya prokhodimost |
| Slabye storony | Slabaya Armor |
| Pryamye kontrmery | PT-Infantry, tanki |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Kamakiri uderzhivaet ravnovesie. |
| Move | Traektoriya ne trebuet dorogi. |
| Attack | Perednie konechnosti stabilizirovany. Ogon. |
| Ability | Preodolevayu prepyatstvie. |
| Damaged | Odna opora povrezhdena! |
| Elite | Relef — lish rekomendatsiya. |
| Idle | Kolyosa pereotseneny. |
| Death | Balans… poteryan… |

### 9. Osnovnoy Tank ZTZ-61 «Tsinlun» (`CO_QinglongMBT`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_QinglongMBT` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1300 |
| Vremya proizvodstva | 27 sek |
| Komandnyy limit | 5 |
| HP | 1500 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 4.8 |
| Dalnost | 9 |
| Orientirovochnyy DPS | 45 |
| Prednaznachenie | Osnovnoy Tank s gruppovym shchitom |
| Osnovnoe Weapons | Plazmennaya pushka |
| Requirements | Fabrika + Bashnya koordinatsii |

#### Sposobnosti
- «Stseplenie shchitov»: soedinyaet shchity sosednikh tankov, raspredelyaya Damaged

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Ochen silyon v postroenii |
| Slabye storony | Slabee v odinochku |
| Pryamye kontrmery | Razdelenie, EMP, artilleriya |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Kontur Tsinluna gotov. |
| Move | Sokhranyaem stseplenie. |
| Attack | Plazma na obshchuyu tsel. |
| Ability | Shchity obedineny. |
| Damaged | Svyaz shchita oslabla! |
| Elite | Nas nelzya probit po odnomu, poka my vmeste. |
| Idle | Stroy — eto Armor, kotoroy ne vidno. |
| Death | Kontur… razorvan… |

### 10. Artilleriya PHL-29 «Musson» (`CO_MonsoonArtillery`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_MonsoonArtillery` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1650 |
| Vremya proizvodstva | 35 sek |
| Komandnyy limit | 5 |
| HP | 900 |
| Tip broni | Osadnaya Vehicles |
| Skorost | 3.4 |
| Dalnost | 17 |
| Orientirovochnyy DPS | 60 |
| Prednaznachenie | Mnogostupenchataya artilleriya po ploshchadi |
| Osnovnoe Weapons | Kassetnye plazmennye snaryady |
| Requirements | Fabrika + Bashnya koordinatsii |

#### Sposobnosti
- «Front mussona»: razvorachivaetsya, poluchaet +25% dalnosti i razdelyaet snaryad na tri podboepripasa

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Otlichno protiv grupp |
| Slabye storony | Nuzhdaetsya v razvyortyvanii i zashchite |
| Pryamye kontrmery | Razvedchiki, Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Musson svyornut. |
| Move | Ishchu mesto for raskrytiya. |
| Attack | Koordinaty prinyaty. |
| Ability | Musson razvorachivaetsya. |
| Damaged | Opornye lepestki povrezhdeny! |
| Elite | Kazhdyy lepestok nesyot otdelnuyu gibel. |
| Idle | Tsvetok zhdyot podkhodyashchey pochvy. |
| Death | Lepestki… slomany… |

### 11. Shchitovoy nositel Tip 42 «Seymon» (`CO_SeimonShieldCarrier`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_SeimonShieldCarrier` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1700 |
| Vremya proizvodstva | 36 sek |
| Komandnyy limit | 6 |
| HP | 1800 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 3.8 |
| Dalnost | 0 |
| Orientirovochnyy DPS | 0 |
| Prednaznachenie | Podvizhnyy uzel shchita i Sinkhronizatsii |
| Osnovnoe Weapons | Ne vooruzhyon |
| Requirements | Fabrika + Bashnya koordinatsii |

#### Sposobnosti
- «Kupol»: sozdayot krugovoy shchit 12 sek; 38 sek
- Passivno +10 Sinkhronizatsii ryadom s 4+ yunitami

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Klyuchevaya podderzhka armii |
| Slabye storony | Bezoruzhen, krupnaya tsel |
| Pryamye kontrmery | Flang, EMP, fokus |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Seymon prinimaet set. |
| Move | Perenoshu tsentr zashchity. |
| Attack | Oruzhiya no. Est shchit. |
| Ability | Kupol sformirovan. |
| Damaged | Polevye emittery peregruzheny! |
| Elite | Gde stoyu ya, tam stoit armiya. |
| Idle | Zashchita redko poluchaet blagodarnost. |
| Death | Kupol… raspalsya… |

### 12. Shturmovoy shagokhod MBT-X «Ayravata» (`CO_AiravataWalker`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_AiravataWalker` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 2500 |
| Vremya proizvodstva | 48 sek |
| Komandnyy limit | 8 |
| HP | 2700 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 4.6 |
| Dalnost | 10 |
| Orientirovochnyy DPS | 80 |
| Prednaznachenie | Tyazhyolyy universalnyy shagokhod |
| Osnovnoe Weapons | Relsovaya pushka i rakety PVO |
| Requirements | Tsitadel issledovaniy |

#### Sposobnosti
- «Nebesnyy pryzhok»: pereprygivaet liniyu fronta i nanosit udar with posadke; 42 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Prokhodimost, moshchnyy proryv |
| Slabye storony | Dorogoy, uyazvim vo vremya podgotovki pryzhka |
| Pryamye kontrmery | PVO, stazis, tyazhyolye tanki |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Ayravata gotova peresech liniyu. |
| Move | Shag vyshe prepyatstviy. |
| Attack | Relsovyy kanal otkryt. |
| Ability | Nebesnyy pryzhok. |
| Damaged | Karkas shaga povrezhdyon! |
| Elite | Vysota — eshchyo odno izmerenie stroya. |
| Idle | Tyazhyolaya mashina tozhe mozhet dvigatsya izyashchno. |
| Death | Opory… ne derzhat… |

### 13. Mobilnaya krepost ZTD-90 «Tyanmen» (`CO_TianmenFortress`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_TianmenFortress` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 3300 |
| Vremya proizvodstva | 60 sek |
| Komandnyy limit | 10 |
| HP | 3900 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 2.7 |
| Dalnost | 12 |
| Orientirovochnyy DPS | 95 |
| Prednaznachenie | Sverkhtyazhyolyy komandnyy uzel i artilleriya |
| Osnovnoe Weapons | Dve plazmennye pushki i drony |
| Requirements | Tsitadel + dva zavoda |

#### Sposobnosti
- «Komandnyy rezhim»: ostanavlivaetsya, +20 Sinkhronizatsii i remontiruet soyuznikov

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Moshchnyy tsentr armii |
| Slabye storony | Ochen medlennyy i dorogoy |
| Pryamye kontrmery | Osadnyy fokus, Aviation, EMP |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Tyanmen voshyol v set. |
| Move | Krepost menyaet osnovanie. |
| Attack | Otkryt vneshnie batarei. |
| Ability | Komandnyy rezhim ustanovlen. |
| Damaged | Vneshniy poyas razrushen! |
| Elite | Armiya bolshe ne nuzhdaetsya v stenakh. |
| Idle | Dvorets ne obyazan byt nepodvizhnym. |
| Death | Tsentralnyy zal… rushitsya… |

### 14. Razveddron UAV-12 «Kavasemi» (`CO_KawasemiDrone`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_KawasemiDrone` |
| Kategoriya | Aviation |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 700 |
| Vremya proizvodstva | 14 sek |
| Komandnyy limit | 3 |
| HP | 360 |
| Tip broni | Vozdushnaya |
| Skorost | 14.0 |
| Dalnost | 6 |
| Orientirovochnyy DPS | 18 |
| Prednaznachenie | Razvedka, obnaruzhenie i presledovanie |
| Osnovnoe Weapons | Lyogkiy impulsnyy luch |
| Requirements | Vozdushnaya pagoda |

#### Sposobnosti
- «Setevoy mayak»: povyshaet Sinkhronizatsiyu vidimykh soyuznikov

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Ochen bystryy i deshyovyy |
| Slabye storony | Khrupkiy, malyy Damaged |
| Pryamye kontrmery | Lyuboe PVO |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Kavasemi podklyuchyon. |
| Move | Marshrut svoboden. |
| Attack | Lyogkaya tsel podtverzhdena. |
| Ability | Setevoy mayak aktiven. |
| Damaged | Korpus drona narushen! |
| Elite | Ya vizhu set sverkhu. |
| Idle | Malyy razmer — bolshaya svoboda. |
| Death | Signal… ischez… |

### 15. Shturmovik Z-28 «Leykhe» (`CO_LeiheGunship`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_LeiheGunship` |
| Kategoriya | Aviation |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1550 |
| Vremya proizvodstva | 32 sek |
| Komandnyy limit | 5 |
| HP | 1150 |
| Tip broni | Vozdushnaya |
| Skorost | 7.8 |
| Dalnost | 8 |
| Orientirovochnyy DPS | 62 |
| Prednaznachenie | Shturm nazemnykh tseley i podderzhka postroeniy |
| Osnovnoe Weapons | Plazmennye rakety |
| Requirements | Vozdushnaya pagoda + Bashnya |

#### Sposobnosti
- «Zashchitnoe krylo»: dayot soyuznoy gruppe 150 shchita; 35 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Damaged plyus podderzhka |
| Slabye storony | Srednyaya skorost, zavisit ot PVO-prikrytiya |
| Pryamye kontrmery | Istrebiteli, PVO |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Leykhe gotov. |
| Move | Krylo sleduet za stroem. |
| Attack | Pikiruyu na tsel. |
| Ability | Zashchitnoe krylo raskryto. |
| Damaged | Plazmennaya tyaga nestabilna! |
| Elite | Grom prikhodit sverkhu, shchit — vmeste s nim. |
| Idle | Zhuravl spokoen before pervogo vystrela. |
| Death | Krylo… slomano… |

### 16. Bombardirovshchik H-26 «Agnipaksha» (`CO_AgnipakshaBomber`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_AgnipakshaBomber` |
| Kategoriya | Aviation |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 2550 |
| Vremya proizvodstva | 50 sek |
| Komandnyy limit | 7 |
| HP | 1650 |
| Tip broni | Vozdushnaya |
| Skorost | 8.8 |
| Dalnost | 14 |
| Orientirovochnyy DPS | 105 |
| Prednaznachenie | Ploshchadnoy bombardirovshchik i vozgoranie |
| Osnovnoe Weapons | Plazmennye zazhigatelnye bomby |
| Requirements | Tsitadel + pagoda |

#### Sposobnosti
- «Vozrozhdenie»: odin raz za zhizn with smertelnom urone vozvrashchaetsya na bazu s 25% HP

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Vysokiy ploshchadnoy Damaged, shans spastis |
| Slabye storony | Dorogoy, uyazvim na obratnom marshrute |
| Pryamye kontrmery | Istrebiteli, dalnee PVO |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Agnipaksha gotova k tsiklu. |
| Move | Vykhozhu na vysotu udara. |
| Attack | Ogon budet viden izdaleka. |
| Ability | Tsikl vozrozhdeniya aktiven. |
| Damaged | Plamya kosnulos kryla! |
| Elite | Ya vozvrashchayus ranshe, chem vrag prazdnuet. |
| Idle | Pepel — tozhe chast polyota. |
| Death | Na etot raz… tsikl zavershyon… |

### 17. Korvet Tip 32 «Kadzekiri» (`CO_KazekiriCorvette`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_KazekiriCorvette` |
| Kategoriya | Naval |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 900 |
| Vremya proizvodstva | 18 sek |
| Komandnyy limit | 4 |
| HP | 780 |
| Tip broni | Morskaya |
| Skorost | 7.5 |
| Dalnost | 8 |
| Orientirovochnyy DPS | 28 |
| Prednaznachenie | Bystryy perekhvat i torpednaya Attack |
| Osnovnoe Weapons | Lyogkie torpedy i pushka |
| Requirements | Prilivnyy dok |

#### Sposobnosti
- «Rezhushchiy manyovr»: ryvok vdol tseli, snizhaet eyo tochnost

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Skorost i okhota na podlodki |
| Slabye storony | Slab protiv kreyserov |
| Pryamye kontrmery | Tyazhyolye korabli |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Kadzekiri gotov. |
| Move | Rezhem techenie. |
| Attack | Torpedy po kursu. |
| Ability | Rezhushchiy manyovr. |
| Damaged | Kil povrezhdyon! |
| Elite | My razrezaem more i stroy vraga. |
| Idle | Techenie vsegda vydayot Move. |
| Death | Korpus… ukhodit vniz… |

### 18. Relsovyy kreyser Tip 81 «Syuanu» (`CO_XuanwuCruiser`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_XuanwuCruiser` |
| Kategoriya | Naval |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 2200 |
| Vremya proizvodstva | 42 sek |
| Komandnyy limit | 8 |
| HP | 2600 |
| Tip broni | Morskaya |
| Skorost | 4.0 |
| Dalnost | 16 |
| Orientirovochnyy DPS | 78 |
| Prednaznachenie | Dalniy korabl protiv tyazhyolykh tseley |
| Osnovnoe Weapons | Korabelnaya relsovaya pushka |
| Requirements | Dok + Bashnya |

#### Sposobnosti
- «Stabilizirovannyy vystrel»: probivaet neskolko tseley po linii; 38 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Vysokaya dalnost i probitie |
| Slabye storony | Medlennaya perezaryadka |
| Pryamye kontrmery | Podlodki, Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Syuanu stabilen. |
| Move | Kreyser menyaet liniyu ognya. |
| Attack | Relsovyy impuls. |
| Ability | Stabilizirovannyy proboy. |
| Damaged | Sektsii korpusa razgermetizirovany! |
| Elite | More ne skryvaet tsel ot pryamoy linii. |
| Idle | Bolshaya dalnost trebuet bolshogo terpeniya. |
| Death | Stabilizatsiya… poteryana… |

### 19. Podvodnyy avianosets SSGN-18 «Samudra» (`CO_SamudraCarrier`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_SamudraCarrier` |
| Kategoriya | Naval |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 3700 |
| Vremya proizvodstva | 68 sek |
| Komandnyy limit | 10 |
| HP | 4200 |
| Tip broni | Morskaya |
| Skorost | 2.6 |
| Dalnost | 18 |
| Orientirovochnyy DPS | 100 |
| Prednaznachenie | Skrytaya baza morskikh dronov |
| Osnovnoe Weapons | Udarnye podvodnye i vozdushnye drony |
| Requirements | Dok + Tsitadel |

#### Sposobnosti
- «Pogruzhyonnyy zapusk»: vypuskaet smeshannyy roy, ne raskryvayas 6 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Skrytnost i universalnost |
| Slabye storony | Ochen dorog, slab with obnaruzhenii vblizi |
| Pryamye kontrmery | Sonar, podlodki, massovaya Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Samudra skryta under prilivom. |
| Move | Glubina prinimaet nas. |
| Attack | Vypustit roy. |
| Ability | Pogruzhyonnyy zapusk. |
| Damaged | Vnutrenniy dok zatoplen! |
| Elite | My prinosim nebo iz glubiny. |
| Idle | Samaya silnaya krepost — ta, kotoruyu ne nashli. |
| Death | Dvorets… opuskaetsya… |

### 20. Commander Mey Tszyan (`CO_Hero_Mei`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CO_Hero_Mei` |
| Kategoriya | Hero |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 2550 |
| Vremya proizvodstva | 49 sek |
| Komandnyy limit | 8 |
| HP | 820 |
| Tip broni | Tyazhyolaya Infantry |
| Skorost | 5.2 |
| Dalnost | 9 |
| Orientirovochnyy DPS | 68 |
| Prednaznachenie | Hero postroeniy, shchitov i komandnykh svyazey |
| Osnovnoe Weapons | Plazmennaya vintovka i komandnye drony |
| Requirements | Tsitadel |

#### Sposobnosti
- «Sovershennyy stroy»: mgnovenno dayot gruppe maksimalnye bonusy postroeniya na 12 sek
- «Perenos shchita»: perenapravlyaet shchity soyuznikov k vybrannoy tseli; 40 sek
- Odin ekzemplyar

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Usilivaet armiyu i spasaet klyuchevye tseli |
| Slabye storony | Sama po sebe ne unichtozhaet tyazhyoluyu tekhniku |
| Pryamye kontrmery | Snaypery, artilleriya, izolyatsiya |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Commander Mey Tszyan prinimaet set. |
| Move | Ya zaymu tsentr postroeniya. |
| Attack | all zvenya — ogon po odnoy tseli. |
| Ability | Sovershennyy stroy. Dyshim v odnom ritme. |
| Damaged | Svyaz derzhitsya. Prodolzhayte. |
| Elite | Armiya stala edinym dvizheniem. |
| Idle | Poryadok ne zamedlyaet. On isklyuchaet lishnee. |
| Death | Sokhranite… stroy… |

# Fraktsiya: Khronolegion
## Fraktsionnaya identichnost
Dorogaya armiya kontrolya prostranstva i vremeni. Teleportatsii, zaderzhki, peremotka urona, vremennye kopii i izmenenie tempa boya. Slabosti: nizkaya bazovaya prochnost, vysokaya tsena oshibok i zavisimost ot temporalnoy stabilnosti.
## Fraktsionnyy resurs: Temporalnaya stabilnost
Stabilnost vosstanavlivaetsya po 1 ochku kazhdye 2 sekundy i uskoryaetsya u khronouzlov. Teleportatsii, peremotka i stazis raskhoduyut resurs. Nizhe 30: +25% perezaryadka sposobnostey i -10% skorost. with 0: aktivnye vremennye kopii ischezayut, a teleportatsiya blokiruetsya before vosstanovleniya 20 ochkov.
## Buildings i Economy Factions
| Zdanie | Tsena | Vremya, s | Energiya | Naznachenie |
| --- | --- | --- | --- | --- |
| Mobilnyy khronokovcheg | 5200 | 64 | 0 | Razvorachivaetsya v tsentr prichinnosti |
| Tsentr prichinnosti | — | — | +100 | Stroitelnaya oblast, +20 limita |
| Reaktor zamedlennogo raspada | 950 | 22 | +135 | Mnogo energii, nestabilen with unichtozhenii |
| Kvantovyy pererabotchik | 2600 | 46 | -25 | Vklyuchaet Harvester QH-4 «Veroyatnik» |
| Barracks ekha | 800 | 20 | -18 | Infantry, +5 limita |
| Fabrika kontinuuma | 2400 | 44 | -45 | Vehicles, +10 limita |
| Razlomnyy Airfield | 2000 | 37 | -55 | Aviation |
| Dok vremennogo priliva | 2200 | 44 | -50 | Naval |
| Nablyudatel veroyatnostey | 1650 | 32 | -65 | Radar, khronouzly, T2 |
| Arkhiv budushchego | 3900 | 65 | -120 | T3, peremotka i paradoksy |
| Turel ekha | 800 | 18 | -18 | Povtoryaet kazhdyy tretiy vystrel |
| Zenitnyy razryv | 1050 | 22 | -32 | PVO i zamedlenie |
| Proektor STS-5 «Pauza» | 2200 | 38 | -90 | Ostanavlivaet tseli na korotkoe vremya |
| Yakor prichinnosti | 1900 | 34 | -70 | Uskoryaet vosstanovlenie stabilnosti |
| Matritsa «Obratnyy otschyot» | 6500 | 94 | -200 | Perematyvaet druzhestvennye voyska v State 10 sekund nazad |
| Singulyarnyy kollapser | 7500 | 115 | -240 | Sozdayot vremennuyu singulyarnost; 9 min |

## EVA — kanonicheskie sistemnye repliki
| Sobytie | Replika |
| --- | --- |
| Start | Prichinnaya tsep zakreplena. Nastoyashchee dostupno for redaktirovaniya. |
| Nizkaya energiya | Energeticheskiy kontur otstayot ot vremennoy linii. |
| Baza atakovana | V tekushchem variante budushchego baza nakhoditsya under udarom. |
| Yunit gotov | Edinitsa sinkhronizirovana s nastoyashchim. |
| Superoruzhie vraga | Zafiksirovan iskhod s massovymi poteryami. Veroyatnost rastyot. |
| Stabilnost 30 | Temporalnaya stabilnost kriticheski snizhena. |
| Pobeda | Vrazheskaya liniya sobytiy zavershena. |
| Porazhenie | Etot variant istorii bolshe ne podderzhivaetsya. |

## Svodnaya tablitsa yunitov
| Yunit | ID | Klass | Tir | Tsena | Vremya | Limit | HP | Armor | Skorost | Dalnost | DPS | Rol |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Strelok ECHO-7 «Rezonans» | CH_ResonanceRifleman | Infantry | T1 | 260 | 8 | 1 | 105 | Lyogkaya Infantry | 5.1 | 6 | 12 | Bazovaya Infantry s povtorom vystrela |
| Kopeyshchik PHASE-L9 «Prokol» | CH_PunctureLancer | Infantry | T1 | 480 | 12 | 2 | 155 | Tyazhyolaya Infantry | 4.4 | 8 | 27 | PT-Infantry s korotkoy fazoy |
| Engineer CSE-2 «Redaktor» | CH_CausalityEngineer | Infantry | T1 | 600 | 16 | 1 | 95 | Lyogkaya Infantry | 4.1 | 0 | 0 | Zakhvat, remont i peremotka zdaniy |
| Operator RWD-3 «Revers» | CH_ReversalMedic | Infantry | T2 | 780 | 19 | 2 | 160 | Lyogkaya Infantry | 4.3 | 0 | 0 | Medik i vosstanovlenie nedavnego sostoyaniya |
| Snayper PDX-12 «Aporiya» | CH_AporiaSniper | Infantry | T2 | 850 | 20 | 2 | 120 | Lyogkaya Infantry | 4.7 | 15 | 55 | Snayper s zaderzhannym uronom |
| Operativnik NULL-12 «Tsenzor» | CH_CensorOperative | Infantry | T3 | 1150 | 26 | 2 | 230 | Tyazhyolaya Infantry | 5.5 | 8 | 42 | Diversant, otklyuchayushchiy sposobnosti i Production |
| Harvester QH-4 «Veroyatnik» | CH_ProbabilistHarvester | Vehicles | T1 | 1550 | 30 | 4 | 1100 | Lyogkaya Vehicles | 4.4 | 0 | 0 | Bystryy Harvester s avariynym vozvratom |
| Razvedchik BLK-8 «Parallaks» | CH_ParallaxScout | Vehicles | T1 | 800 | 16 | 3 | 400 | Lyogkaya Vehicles | 8.5 | 7 | 17 | Razvedka i korotkie teleporty |
| Tank CT-21 «Liniya» | CH_TimelineTank | Vehicles | T2 | 1450 | 29 | 5 | 1250 | Tyazhyolaya Vehicles | 5.0 | 9 | 48 | Osnovnoy Tank s nakopleniem vremennogo shchita |
| Artilleriya LAG-16 «Delta» | CH_DeltaDelayArtillery | Vehicles | T2 | 1800 | 37 | 5 | 800 | Osadnaya Vehicles | 3.3 | 19 | 64 | Snaryady s zaderzhannym vzryvom i kontrol zony |
| Proektor STS-5 «Pauza» | CH_PauseProjector | Vehicles | T2 | 1900 | 39 | 6 | 1400 | Tyazhyolaya Vehicles | 3.8 | 10 | 18 | Kontrol tyazhyolykh tseley |
| Tyazhyolyy Tank EPC-0 «Era» | CH_EraEngine | Vehicles | T3 | 3100 | 58 | 10 | 3200 | Tyazhyolaya Vehicles | 3.2 | 12 | 88 | Sverkhtyazhyolyy Tank, sozdayushchiy vremennye kopii |
| Perekhvatchik RFT-31 «Razryv» | CH_GapInterceptor | Aviation | T2 | 1200 | 24 | 4 | 580 | Vozdushnaya | 13.5 | 12 | 52 | Teleportiruyushchiysya perekhvatchik |
| Shturmovik AFG-6 «Shleyf» | CH_TrailGunship | Aviation | T2 | 1600 | 33 | 5 | 1000 | Vozdushnaya | 8.0 | 8 | 60 | Shturmovik s lozhnymi kopiyami |
| Bombardirovshchik CRV-9 «Kriticheskaya tochka» | CH_CriticalPointBomber | Aviation | T3 | 2800 | 54 | 7 | 1500 | Vozdushnaya | 8.7 | 15 | 115 | Strategicheskiy bombardirovshchik kontrolya |
| Fregat TMK-9 «Izobata» | CH_IsobathFrigate | Naval | T1 | 950 | 19 | 4 | 760 | Morskaya | 7.0 | 9 | 30 | Bystryy korabl kontrolya i razvedki |
| Podlodka ABY-14 «Batis» | CH_BathysSubmarine | Naval | T2 | 2000 | 39 | 7 | 2000 | Morskaya | 4.4 | 11 | 70 | Skrytaya podlodka s korotkim skachkom |
| Kovcheg SGA-1 «Attraktor» | CH_AttractorArk | Naval | T3 | 3900 | 70 | 10 | 4000 | Morskaya | 2.4 | 19 | 105 | Tyazhyolyy korabl kontrolya i teleportatsii |
| Arkhivist Selena Voss | CH_Hero_Voss | Hero | T3 | 2700 | 50 | 8 | 700 | Lyogkaya Infantry | 5.4 | 12 | 72 | Hero kontrolya vremeni i peremotki gruppy |

## Podrobnye kartochki yunitov
### 1. Strelok ECHO-7 «Rezonans» (`CH_ResonanceRifleman`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_ResonanceRifleman` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 260 |
| Vremya proizvodstva | 8 sek |
| Komandnyy limit | 1 |
| HP | 105 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 5.1 |
| Dalnost | 6 |
| Orientirovochnyy DPS | 12 |
| Prednaznachenie | Bazovaya Infantry s povtorom vystrela |
| Osnovnoe Weapons | Vintovka ekha |
| Requirements | Barracks ekha |

#### Sposobnosti
- Kazhdyy chetvyortyy zalp povtoryaetsya via 0.6 sek s 50% urona

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Khoroshiy dlitelnyy Damaged |
| Slabye storony | Khrupkiy, slab protiv oskolkov |
| Pryamye kontrmery | Oskolochnyy Damaged, Vehicles |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Rezonans otvechaet. |
| Move | My uzhe shli etim putyom. |
| Attack | Pervyy vystrel. Zatem povtor. |
| Ability | Ekho zakrepleno. |
| Damaged | Etot Damaged uzhe znakom. |
| Elite | Ya slyshu vystrel before nazhatiya spuska. |
| Idle | Tishina tozhe inogda povtoryaetsya. |
| Death | Ekho… zatikhaet… |

### 2. Kopeyshchik PHASE-L9 «Prokol» (`CH_PunctureLancer`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_PunctureLancer` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 480 |
| Vremya proizvodstva | 12 sek |
| Komandnyy limit | 2 |
| HP | 155 |
| Tip broni | Tyazhyolaya Infantry |
| Skorost | 4.4 |
| Dalnost | 8 |
| Orientirovochnyy DPS | 27 |
| Prednaznachenie | PT-Infantry s korotkoy fazoy |
| Osnovnoe Weapons | Temporalnoe kopyo |
| Requirements | Barracks |

#### Sposobnosti
- «Fazovyy shag»: stanovitsya neuyazvimym na 1 sek i prokhodit skvoz Units; 22 sek, 8 stabilnosti

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Khorosh v proryve k tekhnike |
| Slabye storony | Zavisim ot stabilnosti |
| Pryamye kontrmery | Strelki, kontrol, artilleriya |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Prokol nakhoditsya between mgnoveniyami. |
| Move | Shagnu via prepyatstvie. |
| Attack | Probivayu tekushchuyu versiyu broni. |
| Ability | Fazovyy shag. |
| Damaged | Vozvrat byl slishkom rannim! |
| Elite | Ya kasayus tseli ranshe eyo zashchity. |
| Idle | Stoyat — tozhe Move, esli vremya idyot. |
| Death | Ne uspel… vernutsya… |

### 3. Engineer CSE-2 «Redaktor» (`CH_CausalityEngineer`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_CausalityEngineer` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 600 |
| Vremya proizvodstva | 16 sek |
| Komandnyy limit | 1 |
| HP | 95 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 4.1 |
| Dalnost | 0 |
| Orientirovochnyy DPS | 0 |
| Prednaznachenie | Zakhvat, remont i peremotka zdaniy |
| Osnovnoe Weapons | Khronoinstrumenty |
| Requirements | Barracks |

#### Sposobnosti
- «Peremotka remonta»: vozvrashchaet zdaniyu State 6 sek nazad; 40 sek, 15 stabilnosti

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Mozhet otmenit nedavniy Damaged |
| Slabye storony | Ochen khrupkiy |
| Pryamye kontrmery | Lyubaya boevaya edinitsa |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Redaktor sveryaet prichinnuyu metku. |
| Move | Idu k povrezhdyonnomu sobytiyu. |
| Attack | U menya no oruzhiya v etoy linii. |
| Ability | Vozvrashchayu konstruktsiyu nazad. |
| Damaged | Moya metka sbilas! |
| Elite | Polomka — eto proshloe, kotoroe mozhno otmenit. |
| Idle | Instruktsiya ustarevaet ranshe, chem pechataetsya. |
| Death | Metka… poteryana… |

### 4. Operator RWD-3 «Revers» (`CH_ReversalMedic`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_ReversalMedic` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 780 |
| Vremya proizvodstva | 19 sek |
| Komandnyy limit | 2 |
| HP | 160 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 4.3 |
| Dalnost | 0 |
| Orientirovochnyy DPS | 0 |
| Prednaznachenie | Medik i vosstanovlenie nedavnego sostoyaniya |
| Osnovnoe Weapons | Meditsinskiy khronouzel |
| Requirements | Barracks + Nablyudatel |

#### Sposobnosti
- «Vozvrat sostoyaniya»: soyuznik vozvrashchaet HP, imevshiesya 5 sek nazad; 32 sek, 12 stabilnosti

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Ochen silnoe tochechnoe spasenie |
| Slabye storony | Ne lechit dlitelnyy staryy Damaged, bezoruzhen |
| Pryamye kontrmery | Fokus, snaypery |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Operator RWD-3 «Revers» gotov sravnit sostoyaniya. |
| Move | Idu k tochke raskhozhdeniya. |
| Attack | Boevaya funktsiya otsutstvuet. |
| Ability | Vozvrashchayu pyat sekund. |
| Damaged | Slishkom mnogo povrezhdeniy srazu! |
| Elite | Ya pomnyu vas tselymi. Etogo dostatochno. |
| Idle | Meditsina lechit telo. Ya lechu moment. |
| Death | State… ne vosstanovleno… |

### 5. Snayper PDX-12 «Aporiya» (`CH_AporiaSniper`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_AporiaSniper` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 850 |
| Vremya proizvodstva | 20 sek |
| Komandnyy limit | 2 |
| HP | 120 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 4.7 |
| Dalnost | 15 |
| Orientirovochnyy DPS | 55 |
| Prednaznachenie | Snayper s zaderzhannym uronom |
| Osnovnoe Weapons | Paradoksalnaya vintovka |
| Requirements | Barracks + Nablyudatel |

#### Sposobnosti
- «Otlozhennaya Death»: Damaged srabatyvaet via 4 sek i udvaivaetsya, esli tsel poluchaet vtoroy vystrel; 30 sek

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Ubivaet elitnuyu pekhotu i geroev |
| Slabye storony | Slozhen v upravlenii, slab protiv tekhniki |
| Pryamye kontrmery | Razvedchiki, Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Aporiya vychislena. |
| Move | Zaymu pozitsiyu before eyo poyavleniya. |
| Attack | Vystrel sdelan. Rezultat pozzhe. |
| Ability | Death otlozhena. |
| Damaged | Menya operedili. |
| Elite | Tsel uzhe mertva. Ona prosto eshchyo ne znaet. |
| Idle | Idle — chast vystrela. |
| Death | Takoy iskhod… ne predpolagalsya… |

### 6. Operativnik NULL-12 «Tsenzor» (`CH_CensorOperative`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_CensorOperative` |
| Kategoriya | Infantry |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 1150 |
| Vremya proizvodstva | 26 sek |
| Komandnyy limit | 2 |
| HP | 230 |
| Tip broni | Tyazhyolaya Infantry |
| Skorost | 5.5 |
| Dalnost | 8 |
| Orientirovochnyy DPS | 42 |
| Prednaznachenie | Diversant, otklyuchayushchiy sposobnosti i Production |
| Osnovnoe Weapons | Nulevoy izluchatel |
| Requirements | Arkhiv budushchego |

#### Sposobnosti
- «Obnulenie»: otklyuchaet aktivnye sposobnosti tseli na 8 sek; 38 sek, 18 stabilnosti
- Mozhet maskirovatsya vne boya

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Kontrit geroev i podderzhku |
| Slabye storony | Nevysokiy pryamoy Damaged |
| Pryamye kontrmery | Skanery, massovyy ogon |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Tsenzor aktiven. |
| Move | Udalyayu svoyo prisutstvie. |
| Attack | Tsel isklyuchaetsya. |
| Ability | Funktsii obnuleny. |
| Damaged | Nulevaya obolochka narushena! |
| Elite | Ya ne pobezhdayu vraga. Ya otmenyayu ego vozmozhnost pobedit. |
| Idle | Otsutstvie — tozhe forma kontrolya. |
| Death | Zapis… vosstanovlena vragom… |

### 7. Harvester QH-4 «Veroyatnik» (`CH_ProbabilistHarvester`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_ProbabilistHarvester` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 1550 |
| Vremya proizvodstva | 30 sek |
| Komandnyy limit | 4 |
| HP | 1100 |
| Tip broni | Lyogkaya Vehicles |
| Skorost | 4.4 |
| Dalnost | 0 |
| Orientirovochnyy DPS | 0 |
| Prednaznachenie | Bystryy Harvester s avariynym vozvratom |
| Osnovnoe Weapons | without oruzhiya |
| Requirements | Kvantovyy pererabotchik |

#### Sposobnosti
- «Kvantovyy vozvrat»: teleportiruetsya k pererabotchiku; 45 sek, 15 stabilnosti

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Trudno perekhvatit with vnimatelnom upravlenii |
| Slabye storony | Khrupkiy, dorogo stoit |
| Pryamye kontrmery | EMP, mgnovennyy fokus |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Marshrut Veroyatnika otkryt. |
| Move | Veroyatnost dostavki dostatochna. |
| Attack | Boevoy iskhod ne podderzhivaetsya. |
| Ability | Vozvrat k pererabotchiku. |
| Damaged | Veroyatnost poteri rastyot! |
| Elite | Ya dostavlyayu gruz iz tekh liniy, gde menya ne perekhvatili. |
| Idle | Marshrut sushchestvuet v neskolkikh variantakh. |
| Death | Eta veroyatnost… pobedila… |

### 8. Razvedchik BLK-8 «Parallaks» (`CH_ParallaxScout`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_ParallaxScout` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 800 |
| Vremya proizvodstva | 16 sek |
| Komandnyy limit | 3 |
| HP | 400 |
| Tip broni | Lyogkaya Vehicles |
| Skorost | 8.5 |
| Dalnost | 7 |
| Orientirovochnyy DPS | 17 |
| Prednaznachenie | Razvedka i korotkie teleporty |
| Osnovnoe Weapons | Impulsnyy izluchatel |
| Requirements | Fabrika kontinuuma |

#### Sposobnosti
- «Skachok»: teleport na korotkuyu distantsiyu; 12 sek, 6 stabilnosti

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Luchshiy razvedchik for obkhoda prepyatstviy |
| Slabye storony | Ochen khrupkiy |
| Pryamye kontrmery | Miny, zony kontrolya, PT |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Parallaks zafiksirovan. |
| Move | Sleduyushchaya tochka uzhe ryadom. |
| Attack | Poyavlyayus na linii ognya. |
| Ability | Skachok. |
| Damaged | Koordinaty drozhat! |
| Elite | Rasstoyanie — eto prosto plokhaya privychka. |
| Idle | between zdes i tam slishkom mnogo pustogo vremeni. |
| Death | Koordinata… zakryta… |

### 9. Tank CT-21 «Liniya» (`CH_TimelineTank`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_TimelineTank` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1450 |
| Vremya proizvodstva | 29 sek |
| Komandnyy limit | 5 |
| HP | 1250 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 5.0 |
| Dalnost | 9 |
| Orientirovochnyy DPS | 48 |
| Prednaznachenie | Osnovnoy Tank s nakopleniem vremennogo shchita |
| Osnovnoe Weapons | Kontinuumnaya pushka |
| Requirements | Fabrika + Nablyudatel |

#### Sposobnosti
- «Vremennoy pantsir»: 6 sek zapisyvaet Damaged, zatem vozvrashchaet 40% poteryannogo HP; 34 sek, 14 stabilnosti

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Khorosho perezhivaet fokus |
| Slabye storony | Slab after okonchaniya pantsirya |
| Pryamye kontrmery | Otlozhennyy Damaged, EMP, dlitelnyy ogon |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Liniya stabilna. |
| Move | Prodolzhayu liniyu dvizheniya. |
| Attack | Sobytie porazheniya tseli nachato. |
| Ability | Zapisyvayu povrezhdeniya. |
| Damaged | Pantsir zapominaet udar! |
| Elite | Moy Damaged vsegda vremennyy. Vash — no. |
| Idle | Nastoyashchee — samaya tonkaya Armor. |
| Death | Zapis… ne vosstanovilas… |

### 10. Artilleriya LAG-16 «Delta» (`CH_DeltaDelayArtillery`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_DeltaDelayArtillery` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1800 |
| Vremya proizvodstva | 37 sek |
| Komandnyy limit | 5 |
| HP | 800 |
| Tip broni | Osadnaya Vehicles |
| Skorost | 3.3 |
| Dalnost | 19 |
| Orientirovochnyy DPS | 64 |
| Prednaznachenie | Snaryady s zaderzhannym vzryvom i kontrol zony |
| Osnovnoe Weapons | Temporalnye miny-snaryady |
| Requirements | Fabrika + Nablyudatel |

#### Sposobnosti
- «Pole zaderzhki»: oblast zamedlyaet vragov na 50% 8 sek; 40 sek, 18 stabilnosti

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Otlichnyy kontrol i dalnost |
| Slabye storony | Slabaya Armor, Damaged ne mgnovennyy |
| Pryamye kontrmery | Razvedchiki, Aviation |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Delta nastroena. |
| Move | Sdvigayu tochku budushchego vzryva. |
| Attack | Snaryad pribyl. Vzryv pozzhe. |
| Ability | Pole zaderzhki created. |
| Damaged | Raschyot vremeni narushen! |
| Elite | Vrag uspevaet ponyat oshibku. Ispravit — no. |
| Idle | Speshka polezna only tseli. |
| Death | Taymer… ostanovlen… |

### 11. Proektor STS-5 «Pauza» (`CH_PauseProjector`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_PauseProjector` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1900 |
| Vremya proizvodstva | 39 sek |
| Komandnyy limit | 6 |
| HP | 1400 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 3.8 |
| Dalnost | 10 |
| Orientirovochnyy DPS | 18 |
| Prednaznachenie | Kontrol tyazhyolykh tseley |
| Osnovnoe Weapons | Stazis-luch |
| Requirements | Fabrika + Nablyudatel |

#### Sposobnosti
- «Polnyy stazis»: vyklyuchaet tsel iz boya na 5 sek; 42 sek, 22 stabilnosti

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Silneyshiy kontrol odnoy tseli |
| Slabye storony | Nizkiy Damaged, dorogoy |
| Pryamye kontrmery | Fokus, artilleriya, Null-effekty |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Pauza gotova ostanovit moment. |
| Move | Perenoshu zonu pokoya. |
| Attack | Zamedlyayu tsel before nulya. |
| Ability | Polnyy stazis. |
| Damaged | Kamera stazisa tresnula! |
| Elite | Vremya vraga zakanchivaetsya po moey komande. |
| Idle | Nepodvizhnost byvaet absolyutnoy. |
| Death | Moment… prodolzhilsya… |

### 12. Tyazhyolyy Tank EPC-0 «Era» (`CH_EraEngine`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_EraEngine` |
| Kategoriya | Vehicles |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 3100 |
| Vremya proizvodstva | 58 sek |
| Komandnyy limit | 10 |
| HP | 3200 |
| Tip broni | Tyazhyolaya Vehicles |
| Skorost | 3.2 |
| Dalnost | 12 |
| Orientirovochnyy DPS | 88 |
| Prednaznachenie | Sverkhtyazhyolyy Tank, sozdayushchiy vremennye kopii |
| Osnovnoe Weapons | Dvoynaya Chrono-pushka |
| Requirements | Arkhiv + dva zavoda |

#### Sposobnosti
- «Posleobraz»: sozdayot kopiyu s 45% kharakteristik na 15 sek; 55 sek, 30 stabilnosti

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Ogromnyy takticheskiy potentsial |
| Slabye storony | Ochen dorog i silno raskhoduet stabilnost |
| Pryamye kontrmery | EMP, fokus po originalu, Null-operativniki |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Tyazhyolyy Tank EPC-0 «Era» voshyol v nastoyashchee. |
| Move | Peremeshchayu epokhu vperyod. |
| Attack | Dva iskhoda skhodyatsya na tseli. |
| Ability | Sozdayu posleobraz. |
| Damaged | Prichinnyy korpus povrezhdyon! |
| Elite | Odnogo menya dostatochno. Dvukh — slishkom mnogo. |
| Idle | Istoriya lyubit povtoreniya. Ya — tozhe. |
| Death | Epokha… zavershena… |

### 13. Perekhvatchik RFT-31 «Razryv» (`CH_GapInterceptor`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_GapInterceptor` |
| Kategoriya | Aviation |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1200 |
| Vremya proizvodstva | 24 sek |
| Komandnyy limit | 4 |
| HP | 580 |
| Tip broni | Vozdushnaya |
| Skorost | 13.5 |
| Dalnost | 12 |
| Orientirovochnyy DPS | 52 |
| Prednaznachenie | Teleportiruyushchiysya perekhvatchik |
| Osnovnoe Weapons | Razlomnye rakety |
| Requirements | Razlomnyy Airfield |

#### Sposobnosti
- «Razryv kursa»: mgnovenno menyaet pozitsiyu za tselyu; 22 sek, 8 stabilnosti

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Ochen silyon v vozdushnoy dueli |
| Slabye storony | Khrupkiy, trebuet stabilnosti |
| Pryamye kontrmery | PVO, chislennost |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Razryv uderzhivaet kanal. |
| Move | Kurs mozhno sokratit. |
| Attack | Vykhozhu za khvost tseli. |
| Ability | Razryv kursa. |
| Damaged | Razlom nestabilen! |
| Elite | Vozdushnyy boy zakanchivaetsya before pervogo virazha. |
| Idle | Krylya nuzhny only for prilichiya. |
| Death | Razlom… zakrylsya… |

### 14. Shturmovik AFG-6 «Shleyf» (`CH_TrailGunship`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_TrailGunship` |
| Kategoriya | Aviation |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 1600 |
| Vremya proizvodstva | 33 sek |
| Komandnyy limit | 5 |
| HP | 1000 |
| Tip broni | Vozdushnaya |
| Skorost | 8.0 |
| Dalnost | 8 |
| Orientirovochnyy DPS | 60 |
| Prednaznachenie | Shturmovik s lozhnymi kopiyami |
| Osnovnoe Weapons | Temporalnye avtopushki |
| Requirements | Airfield + Nablyudatel |

#### Sposobnosti
- «Lozhnyy roy»: sozdayot 3 neatakuyushchikh kopii, sbivayushchikh navedenie; 34 sek, 12 stabilnosti

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Khorosho perezhivaet PVO |
| Slabye storony | Sredniy Damaged, kopii ne nanosyat Damaged |
| Pryamye kontrmery | Ploshchadnoe PVO, skanery |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Shleyf sinkhronizirovan. |
| Move | Kopii derzhat stroy. |
| Attack | Nastoyashchiy zalp sredi lozhnykh. |
| Ability | Lozhnyy roy sozdan. |
| Damaged | Popali v original! |
| Elite | Pust vyberut pravilnuyu tsel. Vremeni no. |
| Idle | Inogda ya sam ne uveren, kotoryy iz nas nastoyashchiy. |
| Death | Original… poteryan… |

### 15. Bombardirovshchik CRV-9 «Kriticheskaya tochka» (`CH_CriticalPointBomber`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_CriticalPointBomber` |
| Kategoriya | Aviation |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 2800 |
| Vremya proizvodstva | 54 sek |
| Komandnyy limit | 7 |
| HP | 1500 |
| Tip broni | Vozdushnaya |
| Skorost | 8.7 |
| Dalnost | 15 |
| Orientirovochnyy DPS | 115 |
| Prednaznachenie | Strategicheskiy bombardirovshchik kontrolya |
| Osnovnoe Weapons | Singulyarnye zaryady |
| Requirements | Arkhiv + Airfield |

#### Sposobnosti
- «Obratnaya volna»: after vzryva vragi prityagivayutsya k tsentru; 48 sek, 24 stabilnosti

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Silnyy Damaged i styagivanie |
| Slabye storony | Dorogoy, zametnyy after sbrosa |
| Pryamye kontrmery | Istrebiteli, dalnee PVO |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Kriticheskaya tochka sformirovana. |
| Move | Podkhozhu k tochke neobratimosti. |
| Attack | Sbrasyvayu singulyarnyy zaryad. |
| Ability | Obratnaya volna aktivna. |
| Damaged | Konteyner singulyarnosti povrezhdyon! |
| Elite | after moego udara puti nazad no bukvalno. |
| Idle | Ne smotrite v tsentr slishkom dolgo. |
| Death | Gorizont… poglotil nositel… |

### 16. Fregat TMK-9 «Izobata» (`CH_IsobathFrigate`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_IsobathFrigate` |
| Kategoriya | Naval |
| Tekhnologicheskiy uroven | T1 |
| Stoimost | 950 |
| Vremya proizvodstva | 19 sek |
| Komandnyy limit | 4 |
| HP | 760 |
| Tip broni | Morskaya |
| Skorost | 7.0 |
| Dalnost | 9 |
| Orientirovochnyy DPS | 30 |
| Prednaznachenie | Bystryy korabl kontrolya i razvedki |
| Osnovnoe Weapons | Impulsnye torpedy |
| Requirements | Dok vremennogo priliva |

#### Sposobnosti
- «Otmetka priliva»: pomechaet oblast; soyuznye korabli poluchayut +15% skorost

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Khoroshaya podderzhka flota |
| Slabye storony | Slab protiv tyazhyolykh korabley |
| Pryamye kontrmery | Kreysery, beregovaya artilleriya |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Izobata aktivna. |
| Move | Sdvigayu liniyu vody. |
| Attack | Torpedy vkhodyat v nastoyashchee. |
| Ability | Priliv otmechen. |
| Damaged | Vremennaya vaterliniya narushena! |
| Elite | More zapominaet nashi marshruty. |
| Idle | Priliv vsegda vozvrashchaetsya. |
| Death | Otmetka… smyta… |

### 17. Podlodka ABY-14 «Batis» (`CH_BathysSubmarine`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_BathysSubmarine` |
| Kategoriya | Naval |
| Tekhnologicheskiy uroven | T2 |
| Stoimost | 2000 |
| Vremya proizvodstva | 39 sek |
| Komandnyy limit | 7 |
| HP | 2000 |
| Tip broni | Morskaya |
| Skorost | 4.4 |
| Dalnost | 11 |
| Orientirovochnyy DPS | 70 |
| Prednaznachenie | Skrytaya podlodka s korotkim skachkom |
| Osnovnoe Weapons | Temporalnye torpedy |
| Requirements | Dok + Nablyudatel |

#### Sposobnosti
- «Glubinnyy skachok»: teleport under vodoy; 30 sek, 14 stabilnosti

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Mozhet obkhodit protivolodochnye linii |
| Slabye storony | Dorogaya, uyazvima with nizkoy stabilnosti |
| Pryamye kontrmery | Sonar, massovyy fokus |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Batis dvizhetsya under liniey. |
| Move | Glubina sokratit put. |
| Attack | Torpedy operezhayut volnu. |
| Ability | Glubinnyy skachok. |
| Damaged | Korpus iskazhyon! |
| Elite | Nas nelzya okruzhit v prostranstve. |
| Idle | V glubine vremya idyot inache. |
| Death | Batis… teryaet glubinu… |

### 18. Kovcheg SGA-1 «Attraktor» (`CH_AttractorArk`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_AttractorArk` |
| Kategoriya | Naval |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 3900 |
| Vremya proizvodstva | 70 sek |
| Komandnyy limit | 10 |
| HP | 4000 |
| Tip broni | Morskaya |
| Skorost | 2.4 |
| Dalnost | 19 |
| Orientirovochnyy DPS | 105 |
| Prednaznachenie | Tyazhyolyy korabl kontrolya i teleportatsii |
| Osnovnoe Weapons | Singulyarnaya artilleriya |
| Requirements | Dok + Arkhiv |

#### Sposobnosti
- «Morskoy portal»: teleportiruet before 6 soyuznykh korabley k sebe; 65 sek, 35 stabilnosti

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Menyaet rasstanovku Total flota |
| Slabye storony | Ochen dorog, ogromnaya tsel |
| Pryamye kontrmery | Podlodki, strategicheskie udary |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Attraktor uderzhivaet singulyarnost. |
| Move | Perenoshu tsentr prityazheniya. |
| Attack | Otkryt artilleriyskiy kollaps. |
| Ability | Morskoy portal sformirovan. |
| Damaged | Singulyarnost vykhodit iz tsentra! |
| Elite | Naval bolshe ne ogranichen morem between tochkami. |
| Idle | Kovcheg khranit ne lyudey. On khranit varianty. |
| Death | Singulyarnost… teryaet obolochku… |

### 19. Arkhivist Selena Voss (`CH_Hero_Voss`)
| Parametr | Value |
| --- | --- |
| Stable ID | `CH_Hero_Voss` |
| Kategoriya | Hero |
| Tekhnologicheskiy uroven | T3 |
| Stoimost | 2700 |
| Vremya proizvodstva | 50 sek |
| Komandnyy limit | 8 |
| HP | 700 |
| Tip broni | Lyogkaya Infantry |
| Skorost | 5.4 |
| Dalnost | 12 |
| Orientirovochnyy DPS | 72 |
| Prednaznachenie | Hero kontrolya vremeni i peremotki gruppy |
| Osnovnoe Weapons | Arkhivnyy izluchatel |
| Requirements | Arkhiv budushchego |

#### Sposobnosti
- «Arkhiv sostoyaniya»: zapisyvaet State gruppy i mozhet vernut ego v techenie 12 sek
- «Zapret sobytiya»: otmenyaet odno primenenie vrazheskoy sposobnosti; 60 sek
- Odin ekzemplyar

#### Balans i primenenie
| Aspekt | Description |
| --- | --- |
| Silnye storony | Unikalnoe spasenie armii i kontrsposobnosti |
| Slabye storony | Krayne khrupkaya i slozhnaya |
| Pryamye kontrmery | Snaypery, vnezapnyy fokus, Null-effekty |

#### Voiceover
| Uslovie | Kanonicheskaya replika |
| --- | --- |
| Selected | Arkhivist Voss. Ya pomnyu iskhody, kotorykh eshchyo no. |
| Move | Eta tochka prisutstvuet vo vsekh poleznykh variantakh. |
| Attack | Udalyayu nezhelatelnoe prodolzhenie. |
| Ability | State zapisano. Vozvrat razreshyon. |
| Damaged | Arkhiv poluchaet nesovmestimye dannye. |
| Elite | Budushchee bolshe ne udivlyaet menya. |
| Idle | Istoriya — plokhoy arkhiv. Slishkom mnogoe teryaet. |
| Death | Zapis… ne sokhranilas… |

# 5. Sistemnye pravila AI i avtopovedeniya
## 5.1. Prioritety tseley
| Tip yunita | Prioritet |
| --- | --- |
| Strelkovaya Infantry | Vrazheskaya Infantry → inzhenery → lyogkaya Vehicles |
| PT-Infantry | Lyogkaya Vehicles → tyazhyolaya Vehicles → Buildings |
| Osnovnoy Tank | Tyazhyolaya Vehicles → lyogkaya Vehicles → Buildings |
| Artilleriya | Skopleniya → oborona → Production |
| PVO | Bombardirovshchiki → shturmoviki → transport → istrebiteli |
| Istrebitel | Bombardirovshchiki → shturmoviki → istrebiteli |
| Hero/podderzhka | Klyuchevye tseli i primenenie sposobnostey po pravilam ugrozy |

## 5.2. Avtomaticheskiy otkhod
Po umolchaniyu Units ne otstupayut sami, chtoby ne lomat kontrol igroka. Dopuskaetsya optsionalnyy rezhim «Ostorozhnyy AI»: yunit with HP nizhe 20% otkhodit k blizhayshey remontnoy zone, esli ne nakhoditsya v rezhime Hold Position, Guard or Attack-Move s zapretom otkhoda.

## 5.3. Postroeniya
| Postroenie | Effekt | Nedostatok |
| --- | --- | --- |
| Liniya | Maksimum frontalnogo ognya | Uyazvima for artillerii |
| Kolonna | Bystroe peremeshchenie via uzkie mesta | Slabyy pervyy zalp |
| Klin | Bonus k proryvu i taranu | Otkrytye flangi |
| Krug | Zashchita podderzhki i geroev | Nizkaya skorost |
| Rasseivanie | Snizhenie urona ot ploshchadi | Khuzhe lokalnyy fokus |

# 6. Balans matchapov
| Fraktsiya | Silnee Total protiv | Slabee Total protiv | Uslovie pobedy |
| --- | --- | --- | --- |
| Soviet Union | Frontalnykh nazemnykh armiy, statichnoy oborony | Mobilnykh flangov, dalnego kontrolya, ekonomicheskogo kharassa | Dozhit before tyazhyoloy tekhniki i navyazat pryamoy boy |
| Alliance | Medlennykh armiy, aviatsionno slabykh baz | Massovoy deshyovoy pekhoty i zatyazhnogo obmena resursami | Poluchit razveddannye i vyigryvat tochechnymi udarami |
| Vostochnaya Coalition | Razroznennykh armiy i odinochnykh dorogikh tseley | Razdeleniya, EMP i udarov po uzlam seti | Sokhranit Sinkhronizatsiyu i vesti boy postroeniem |
| Khronolegion | Predskazuemykh atak i dorogikh klyuchevykh yunitov | Massovogo deshyovogo davleniya i postoyannogo fokusa | Vyigryvat temp za schyot kontrolya vremeni, ne obnulyaya stabilnost |

# 7. Rekomendovannye startovye sostavy
| Fraktsiya | Pervye 5 minut | Perekhod v T2 | T3-yadro armii |
| --- | --- | --- | --- |
| Soviet Union | 8–12 boytsov «Rubezh», 2 shturmovika «Zapal», odna «Rys» | «Granity» + raschyoty «Zaslon» + «Zarevo» | «Voevody» + «Gromoboi» + «Gromada»/«Svyatogor» |
| Alliance | «Sentinely», «Kestrel», ranniy «Longwatch» | «Bulvarki» + «Orakul» + «Uord» | «Tsitadeli» + «Naytveyly» + «Gorizont» |
| Vostochnaya Coalition | Boytsy «Tsyanvey» v stroyu, «Kamakiri», tekhnik «Tsze» | «Tsinluny» + «Seymon» + «Musson» | «Ayravata» + «Tyanmen» + «Samudra» |
| Khronolegion | «Rezonansy», «Parallaks», kopeyshchiki PHASE-L9 | Tanki «Liniya» + artilleriya «Delta» + «Revers» | «Era» + Voss + «Attraktor» |

# 8. Tekhnicheskoe predstavlenie v Unreal Engine 5
Kazhdyy yunit dolzhen opisyvatsya Primary Data Asset klassa `URA4UnitDefinition`. after naming reset Version 2.0 novye Stable ID iz razdela 0.2 yavlyayutsya kanonicheskimi. Starye znacheniya razresheny only v massive `LegacyAliases` i ne mogut ispolzovatsya kak imya novogo paketa, `VoiceId` or Gameplay Tag. Balansovye znacheniya ne khranyatsya v Blueprint. Obyazatelnye polya: UnitId, FactionTag, DomainTag, Tier, Cost, BuildTime, CommandCost, MaxHealth, ArmorType, MoveSpeed, TurnRate, VisionRange, WeaponDefinitions, AbilityDefinitions, Prerequisites, SoftClassReference na predstavlenie, VoiceId, UIIcon, Portrait, SelectionPriority i AIThreatValue. all sposobnosti poluchayut Gameplay Tags, no bazovye komandy Move/Attack/Stop ne dolzhny byt Gameplay Ability without neobkhodimosti.

## 8.1. Gameplay Tags
Primer obyazatelnoy ierarkhii:
```text
Faction.Soviet
Faction.Alliance
Faction.Coalition
Faction.Chrono
Unit.Domain.Infantry
Unit.Domain.Vehicle
Unit.Domain.Air
Unit.Domain.Naval
Unit.Role.Scout
Unit.Role.AntiArmor
Unit.Role.Artillery
Unit.Role.Support
Unit.Role.Hero
Damage.Ballistic
Damage.Fragmentation
Damage.ArmorPiercing
Damage.Siege
Damage.Electric
Damage.Plasma
Damage.Cryo
Damage.Temporal
State.Cloaked
State.Stunned
State.Stasis
State.Veteran
State.Elite
State.Heroic
```

## 8.2. Voice manifest
for kazhdogo yunita sozdayotsya `VoiceId`, sovpadayushchiy s ID v dokumente. Fayly: `VO_<Faction>_<UnitId>_<Event>_<Variant>.wav`. Kategorii sobytiy: Selected, Move, Attack, Ability, Damaged, CriticalDamage, Elite, Idle, Death, CannotComply, DestinationBlocked, EnemyDestroyed. Minimum 3 varianta Selected/Move/Attack i 2 varianta ostalnykh sobytiy.

# 9. Pravila posleduyushchego balansirovaniya
1. Nelzya menyat odnovremenno stoimost, HP i DPS odnogo yunita without otdelnogo testa.  
2. Lyuboy yunit s teleportatsiey obyazan imet vidimyy telegraf, stoimost stabilnosti or dlinnuyu perezaryadku.  
3. Lyubaya dalnoboynaya artilleriya dolzhna imet minimalnuyu distantsiyu or slabost v blizhnem boyu.  
4. Lyuboy sverkhtyazhyolyy yunit obyazan imet minimum dve dostupnye kontrmery u kazhdoy Factions.  
5. Hero ne dolzhen v odinochku unichtozhat polnotsennuyu armiyu; ego sila — v unikalnoy takticheskoy funktsii.  
6. Ni odna passivnaya Economy ne dolzhna okupatsya bystree chem za 4 minuty without riska zakhvata.  
7. V ranney igre odna poteryannaya edinitsa ne dolzhna avtomaticheski reshat match, krome gruboy oshibki s mobilnym shtabom.  
8. Pobeda dolzhna dostigatsya armiey i kontrolem karty, a ne only ozhidaniem superoruzhiya.  

# 10. Itogovyy production-cheklist
| Podsistema | Gotovnost opredelyaetsya tak |
| --- | --- |
| Economy | Dobycha, energiya, limit, otmena, prodazha i remont realizovany i pokryty testami |
| Factions | U kazhdoy est silnaya storona, slabost i rabochiy fraktsionnyy resurs |
| Units | all ID zavedeny v Data Assets, no vremennykh PlaceholderUnit |
| Sposobnosti | Stoimost, cooldown, telegraf, authority i kontrmery opredeleny |
| Voiceover | all sobytiya imeyut minimum trebuemoe chislo variantov, fayly v manifest |
| UI | Tsena, vremya, prerequisite, blokirovka i Ability otobrazhayutsya korrektno |
| AI | Umeet ispolzovat roli, postroeniya, podderzhku i superoruzhie |
| Set | Server avtoriteten po stoimosti, uronu, sposobnosti i peremeshcheniyu |
| Testy | Est unit, functional, multiplayer i nagruzochnye testy |