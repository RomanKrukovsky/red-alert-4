# Podrobnyy akt soderzhatelnogo audita stsenariya ozvuchki Soviet Union (Detailed Narrative & Editorial QA Audit)

**Project**: Red Alert 4  
**Fraktsiya**: Soviet Union  
**Yazyk**: Russkiy (`ru-RU`)  
**Date audita**: 30 iyulya 2026 g.  

---

## 1. Metodologiya i etapy audita

V sootvetstvii s trebovaniyami proizvodstvennoy biblii `RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md` byl provedyon gluboky soderzhatelnyy redaktorskiy Audit **vsekh 152 replik** for 19 yunitov Soviet Union:

1. **Postrochnoe sopostavlenie (String-by-String Diff)**:
   Kazhdaya iz 152 replik v `soviet_voice_script_ru.json`, `.csv` i `.md` sveryalas s tekstom Biblii v2.0. Podtverzhdeno **100% doslovnoe sootvetstvie** (0 raskhozhdeniy). all 152 repliki imeyut Status `canonical`.
2. **Semanticheskiy i narrativnyy analiz**:
   * **Golos tekhniki**: verified, chto vsyu tekhniku ozvuchivaet odin postoyannyy operator/Commander ekipazha, vosprinimayushchiy mashinu kak rabochuyu sistemu (without abstraktnogo «my all pogibnem» or pereklyucheniya roley).
   * **Sootvetstvie sposobnostey**: verified, chto sobytie `Ability` for kazhdogo yunita strogo otrazhaet ego realnuyu Ability iz Biblii (napr., «V zemlyu!» for `SU_RubezhRifleman` — *Okopatsya*, «Termobaricheskiy — vnutr!» for `SU_ZapalGrenadier` — *Termobaricheskiy zaryad*, «Na taran!» for `SU_GranitMBT` — *Taran*, «Perekhodim v osadnyy rezhim» for `SU_VoevodaHeavyTank` — *Osadnyy rezhim*).
   * **Otsutstvie povtorov**: Vypolnen skriptovyy poisk dublikatov teksta po vsemu korpusu Factions — obnaruzheno **0 sovpadeniy**.
   * **Razlichie sobytiy**: verified smyslovoe i energeticheskoe razdelenie sobytiy (`Selected` — gotovnost/identifikatsiya, `Move` — peredvizhenie, `Attack` — boevoy prikaz, `Ability` — aktivatsiya navyka, `Damaged` — reaktsiya na povrezhdenie, `Elite` — boevaya gordost, `Idle` — professionalnyy yumor, `Death` — gibel).
   * **Analiz rechi Mayor Eleny Morozovoy (`SU_Hero_Morozova`)**: Repliki geroya otrazhayut vyderzhku, avtoritet i operativnuyu otvetstvennost without flirta, krikov or shtampov.

---

## 2. Itogovaya matritsa audita po yunitam (19 yunitov × 8 sobytiy)

### 1. `SU_RubezhRifleman` (Motostrelok MS-12 «Rubezh»)
- **Selected**: «Motostrelok MS-12 «Rubezh» na svyazi.» (`canonical` | PASS)
- **Move**: «Bezhim, poka doroga svobodna.» (`canonical` | PASS)
- **Attack**: «Tsel vizhu. Otkryvayu ogon.» (`canonical` | PASS)
- **Ability**: «V zemlyu! Derzhim pozitsiyu!» [Ability: *Okopatsya*] (`canonical` | PASS)
- **Damaged**: «Nas prizhali!» (`canonical` | PASS)
- **Elite**: «Teper my ne prosto popolnenie.» (`canonical` | PASS)
- **Idle**: «Obeshchali formu poteplee.» (`canonical` | PASS)
- **Death**: «Peredayte… pozitsiyu derzhali.» (`canonical` | PASS)

### 2. `SU_ZapalGrenadier` (Shturmovik OSh-4 «Zapal»)
- **Selected**: «Granaty snaryazheny.» (`canonical` | PASS)
- **Move**: «Podoydyom na brosok.» (`canonical` | PASS)
- **Attack**: «Nakryvayu sektor!» (`canonical` | PASS)
- **Ability**: «Termobaricheskiy — vnutr!» [Ability: *Termobaricheskiy zaryad*] (`canonical` | PASS)
- **Damaged**: «Oskolkami zadelo!» (`canonical` | PASS)
- **Elite**: «Teper popadayu s pervogo broska.» (`canonical` | PASS)
- **Idle**: «Glavnoe — ne pereputat sumki.» (`canonical` | PASS)
- **Death**: «Cheka… uzhe vydernuta…» (`canonical` | PASS)

### 3. `SU_ZaslonAATeam` (Zenitnyy raschyot PZK-9 «Zaslon»)
- **Selected**: «Nebo under kontrolem.» (`canonical` | PASS)
- **Move**: «Ishchu chistyy sektor.» (`canonical` | PASS)
- **Attack**: «Vysota podtverzhdena. Ogon!» (`canonical` | PASS)
- **Ability**: «Zatailis. Pust podletyat.» [Ability: *Vozdushnaya zasada*] (`canonical` | PASS)
- **Damaged**: «Raschyot under obstrelom!» (`canonical` | PASS)
- **Elite**: «Ni odin bort ne uydyot.» (`canonical` | PASS)
- **Idle**: «Letyat krasivo. Padayut luchshe.» (`canonical` | PASS)
- **Death**: «Nebo… vashe…» (`canonical` | PASS)

### 4. `SU_MasterEngineer` (Engineer-sapyor IS-3 «Master»)
- **Selected**: «Instrument est. Plan by eshchyo.» (`canonical` | PASS)
- **Move**: «Doberus i pochinyu.» (`canonical` | PASS)
- **Attack**: «Ya ne strelok. Pokazhite obekt.» (`canonical` | PASS)
- **Ability**: «Seychas zavedyom etu razvalinu.» [Ability: *Polevoy remont / Zakhvat*] (`canonical` | PASS)
- **Damaged**: «Inzhenera prikroyte!» (`canonical` | PASS)
- **Elite**: «Ya pochinyu dazhe to, chego eshchyo ne postroili.» (`canonical` | PASS)
- **Idle**: «Po instruktsii eto dolzhno before rabotat.» (`canonical` | PASS)
- **Death**: «Skhema… byla vernoy…» (`canonical` | PASS)

### 5. `SU_RazryadTrooper` (Elektroshturmovik ESh-8 «Razryad»)
- **Selected**: «Kontur zaryazhen.» (`canonical` | PASS)
- **Move**: «Tok poydyot za nami.» (`canonical` | PASS)
- **Attack**: «Razryad na tsel!» (`canonical` | PASS)
- **Ability**: «Peregruzka seti!» [Ability: *Peregruzka*] (`canonical` | PASS)
- **Damaged**: «Izolyatsiya probita!» (`canonical` | PASS)
- **Elite**: «Molniya slushaetsya menya.» (`canonical` | PASS)
- **Idle**: «Ne trogayte kabel. Poslednee Warning.» (`canonical` | PASS)
- **Death**: «Zazemlenie… ne srabotalo…» (`canonical` | PASS)

### 6. `SU_VektorOfficer` (Ofitser svyazi KS-6 «Vektor»)
- **Selected**: «Svyaz s frontom ustanovlena.» (`canonical` | PASS)
- **Move**: «Peredayu novyy rubezh.» (`canonical` | PASS)
- **Attack**: «Prikaz utverzhdyon. Unichtozhit.» (`canonical` | PASS)
- **Ability**: «Pervyy prikaz: ni shaga nazad!» [Ability: *Prikaz №1*] (`canonical` | PASS)
- **Damaged**: «Kanal under ognyom!» (`canonical` | PASS)
- **Elite**: «Teper armiya slyshit menya without pomekh.» (`canonical` | PASS)
- **Idle**: «Molchanie v efire podozritelnee strelby.» (`canonical` | PASS)
- **Death**: «Komandovanie… prodolzhayte without menya.» (`canonical` | PASS)

### 7. `SU_BogatyrOreCarrier` (Gornorudnaya mashina GRM-8 «Bogatyr»)
- **Selected**: «Bogatyr gotov k reysu.» (`canonical` | PASS)
- **Move**: «Tyazhyolyy gruz idyot.» (`canonical` | PASS)
- **Attack**: «Oruzhiya no. Mogu pereekhat.» (`canonical` | PASS)
- **Ability**: «Zakryvayu bronevye shtorki.» [Ability: *Avariynaya Armor*] (`canonical` | PASS)
- **Damaged**: «Obshivka derzhit!» (`canonical` | PASS)
- **Elite**: «Marshrut znayu luchshe generalov.» (`canonical` | PASS)
- **Idle**: «Ruda sama sebya ne privezyot.» (`canonical` | PASS)
- **Death**: «Gruz… ne dostavlen…» (`canonical` | PASS)

### 8. `SU_RysScout` (Boevaya razvedmashina BRM-27 «Rys»)
- **Selected**: «Rys vyshla na marshrut.» (`canonical` | PASS)
- **Move**: «Uzhe tam.» (`canonical` | PASS)
- **Attack**: «Srezaem khvost kolonne!» (`canonical` | PASS)
- **Ability**: «Pereprygivaem!» [Ability: *Pryzhok via prepyatstvie*] (`canonical` | PASS)
- **Damaged**: «Armor tonkaya, ne stoyte!» (`canonical` | PASS)
- **Elite**: «Ya vizhu flang ranshe radara.» (`canonical` | PASS)
- **Idle**: «Glavnoe — ne dognat sobstvennyy sled.» (`canonical` | PASS)
- **Death**: «Skorost… ne spasla…» (`canonical` | PASS)

### 9. `SU_GranitMBT` (Osnovnoy Tank OBT-92 «Granit»)
- **Selected**: «Granit gotov.» (`canonical` | PASS)
- **Move**: «Gusenitsy — vperyod.» (`canonical` | PASS)
- **Attack**: «Razdrobit tsel.» (`canonical` | PASS)
- **Ability**: «Na taran!» [Ability: *Taran*] (`canonical` | PASS)
- **Damaged**: «Lob derzhit, bort podstavili!» (`canonical` | PASS)
- **Elite**: «Stal nauchilas pobezhdat.» (`canonical` | PASS)
- **Idle**: «Tishe edesh — dolshe strelyaesh.» (`canonical` | PASS)
- **Death**: «Bashnya… zaklinila…» (`canonical` | PASS)

### 10. `SU_ZarevoMLRS` (Termobaricheskaya RSZO TRS-18 «Zarevo»)
- **Selected**: «Paket raket zaryazhen.» (`canonical` | PASS)
- **Move**: «Derzhim distantsiyu.» (`canonical` | PASS)
- **Attack**: «Raschyotnyy kvadrat podtverzhdyon.» (`canonical` | PASS)
- **Ability**: «Podzhigaem ves sektor.» [Ability: *Ognennyy kvadrat*] (`canonical` | PASS)
- **Damaged**: «Puskovaya under ognyom!» (`canonical` | PASS)
- **Elite**: «Odin zalp — odin novyy gorizont.» (`canonical` | PASS)
- **Idle**: «Krasivo gorit only chuzhoe.» (`canonical` | PASS)
- **Death**: «Boekomplekt… seychas rvanyot…» (`canonical` | PASS)

### 11. `SU_GromoboyRam` (Elektrotaran ETM-7 «Gromoboy»)
- **Selected**: «Gromoboy zhdyot komandy.» (`canonical` | PASS)
- **Move**: «K kontaktu.» (`canonical` | PASS)
- **Attack**: «Zamykayu tsep!» (`canonical` | PASS)
- **Ability**: «Razryad v grunt!» [Ability: *Razryad po zemle*] (`canonical` | PASS)
- **Damaged**: «Katushki peregrevayutsya!» (`canonical` | PASS)
- **Elite**: «Groza teper idyot po zemle.» (`canonical` | PASS)
- **Idle**: «Sukhaya pogoda — vremennaya problema.» (`canonical` | PASS)
- **Death**: «Kontur… razomknut…» (`canonical` | PASS)

### 12. `SU_VoevodaHeavyTank` (Tyazhyolyy Tank proryva TTP-11 «Voevoda»)
- **Selected**: «Voevoda vstupaet v boy.» (`canonical` | PASS)
- **Move**: «Zemlya vyderzhit.» (`canonical` | PASS)
- **Attack**: «Steret koordinaty.» (`canonical` | PASS)
- **Ability**: «Perekhodim v osadnyy rezhim.» [Ability: *Osadnyy rezhim*] (`canonical` | PASS)
- **Damaged**: «Povrezhdenie prinyato. Prodolzhaem.» (`canonical` | PASS)
- **Elite**: «Teper eto ne Tank. Eto napravlenie fronta.» (`canonical` | PASS)
- **Idle**: «My ne opazdyvaem. Nas zhdut.» (`canonical` | PASS)
- **Death**: «Voevoda… ostavlyaet rubezh…» (`canonical` | PASS)

### 13. `SU_KrechetInterceptor` (Istrebitel I-47 «Krechet»)
- **Selected**: «Krechet na polose.» (`canonical` | PASS)
- **Move**: «Kurs prinyat.» (`canonical` | PASS)
- **Attack**: «Rakety soshli.» (`canonical` | PASS)
- **Ability**: «Forsazh!» [Ability: *Forsazh*] (`canonical` | PASS)
- **Damaged**: «Poterya davleniya!» (`canonical` | PASS)
- **Elite**: «Nebo after tesnym.» (`canonical` | PASS)
- **Idle**: «Toplivo lyubit reshitelnykh.» (`canonical` | PASS)
- **Death**: «Katapulta… otkaz…» (`canonical` | PASS)

### 14. `SU_KorshunGunship` (Shturmovoy vertolyot ShV-38 «Korshun»)
- **Selected**: «Korshun gotov k vyletu.» (`canonical` | PASS)
- **Move**: «Idyom na maloy vysote.» (`canonical` | PASS)
- **Attack**: «Rabotaem po zemle!» (`canonical` | PASS)
- **Ability**: «Zakhodim na krug!» [Ability: *Krug ognya*] (`canonical` | PASS)
- **Damaged**: «Khvostovoy sektor povrezhdyon!» (`canonical` | PASS)
- **Elite**: «Infantry zovyot — my otvechaem.» (`canonical` | PASS)
- **Idle**: «V kabine pakhnet toplivom i pobedoy.» (`canonical` | PASS)
- **Death**: «Bort padaet…» (`canonical` | PASS)

### 15. `SU_GromadaAirship` (Tyazhyolyy dirizhabl TDA-8 «Gromada»)
- **Selected**: «Gromada v vozdukhe.» (`canonical` | PASS)
- **Move**: «Medlenno. Neotvratimo.» (`canonical` | PASS)
- **Attack**: «Otkryt bombolyuki.» (`canonical` | PASS)
- **Ability**: «Polnyy gaz. Dvigateli na predel.» [Ability: *Polnyy gaz*] (`canonical` | PASS)
- **Damaged**: «Obshivka gorit, kurs derzhim.» (`canonical` | PASS)
- **Elite**: «Goroda uznayut nas po teni.» (`canonical` | PASS)
- **Idle**: «Vysota khoroshaya. Mir kazhetsya tishe.» (`canonical` | PASS)
- **Death**: «Ballast… uzhe ne pomozhet…» (`canonical` | PASS)

### 16. `SU_BuranPatrolBoat` (Boevoy kater BK-27 «Buran»)
- **Selected**: «Buran na vode.» (`canonical` | PASS)
- **Move**: «Rezhem volnu.» (`canonical` | PASS)
- **Attack**: «Tsel po pravomu bortu!» (`canonical` | PASS)
- **Ability**: «Set v vodu!» [Ability: *Elektroset*] (`canonical` | PASS)
- **Damaged**: «Korpus prinimaet vodu!» (`canonical` | PASS)
- **Elite**: «More zapomnilo nash sled.» (`canonical` | PASS)
- **Idle**: «Shtil — eto prosto pauza.» (`canonical` | PASS)
- **Death**: «Otsek zatoplen…» (`canonical` | PASS)

### 17. `SU_MorokSubmarine` (Udarnaya podlodka UPL-90 «Morok»)
- **Selected**: «Morok slushaet glubinu.» (`canonical` | PASS)
- **Move**: «Pogruzhaemsya.» (`canonical` | PASS)
- **Attack**: «Torpednyy rastvor otkryt.» (`canonical` | PASS)
- **Ability**: «Bezzvuchnyy khod.» [Ability: *Bezzvuchnyy khod*] (`canonical` | PASS)
- **Damaged**: «Prochnyy korpus deformirovan!» (`canonical` | PASS)
- **Elite**: «V more nas zamechayut slishkom pozdno.» (`canonical` | PASS)
- **Idle**: «Naverkhu shumyat. Zdes dumayut.» (`canonical` | PASS)
- **Death**: «Glubina… prinimaet…» (`canonical` | PASS)

### 18. `SU_SvyatogorCruiser` (Raketnyy kreyser RKR-44 «Svyatogor»)
- **Selected**: «Svyatogor zhdyot koordinaty.» (`canonical` | PASS)
- **Move**: «Kreyser menyaet pozitsiyu.» (`canonical` | PASS)
- **Attack**: «Raketnyy zalp.» (`canonical` | PASS)
- **Ability**: «Zagraditelnyy ogon po sektoru.» [Ability: *Zagraditelnyy zalp*] (`canonical` | PASS)
- **Damaged**: «Paluba probita!» (`canonical` | PASS)
- **Elite**: «Bereg zakanchivaetsya tam, gde nachinayutsya nashi rakety.» (`canonical` | PASS)
- **Idle**: «More bolshoe. Dalnost bolshe.» (`canonical` | PASS)
- **Death**: «Pogreba… detoniruyut…» (`canonical` | PASS)

### 19. `SU_Hero_Morozova` (Mayor Elena Morozova)
- **Selected**: «Mayor Morozova. Dokladyvayte.» (`canonical` | PASS)
- **Move**: «Ya budu na peredovoy.» (`canonical` | PASS)
- **Attack**: «Etot uchastok fronta zakryvaem seychas.» (`canonical` | PASS)
- **Ability**: «Podavit ikh svyaz i Move.» [Ability: *Pole podavleniya*] (`canonical` | PASS)
- **Damaged**: «Tsarapina. Prikaz ne menyaetsya.» (`canonical` | PASS)
- **Elite**: «Segodnya front dvizhetsya vmeste so mnoy.» (`canonical` | PASS)
- **Idle**: «Generaly lyubyat karty. Ya predpochitayu mestnost.» (`canonical` | PASS)
- **Death**: «Prodolzhayte… nastuplenie…» (`canonical` | PASS)

---

## 3. Redaktorskiy reestr ispravleniy i kvalifikatsiya replik

* **Izmenyonnykh po kachestvu replik (`edited_for_quality`)**: `0` (Kazhdaya iz 152 kanonicheskikh replik Biblii v2.0 priznana stilisticheski, ritmicheski i funktsionalno bezuprechnoy).
* **Sgenerirovannykh otsutstvuyushchikh replik (`generated_missing_line`)**: `0` (Ni odna replika ne uteryana v iskhodnoy biblii).
* **Kanonicheskikh originalnykh replik (`canonical`)**: `152` (100% strogaya preemstvennost).

---

## 4. Svodnaya statistika audita

* **Total yunitov**: 19
* **Total replik**: 152
* **Dublikatov replik**: 0
* **Unikalnost rekomenduemykh imyon `.wav`**: 152 / 152 (100%)
* **Status validnosti JSON/CSV/MD**: Valid & Synchronized
* **Itogovoe reshenie auditora**: **`PASS`** (Okonchatelnoe utverzhdenie stsenariya)