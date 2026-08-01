Rabotay kak Principal UI/UX Designer AAA-strategiy, Senior React Engineer, Senior Babylon.js UI Integration Engineer i spetsialist po tochnoy rekonstruktsii interfeysov po vizualnym referensam. Tvoya zadacha — polnostyu zamenit tekushchiy interfeys RA4 Browser RTS i maksimalno tochno vosproizvesti dizayn, kompozitsiyu, vizualnuyu ierarkhiyu, plotnost, proportsii, raspolozhenie elementov, tsvetovye temy, fraktsionnye stili, sostoyaniya i igrovoy HUD iz prilozhennogo arkhiva `SCREENSHOTS.zip`.

Trebuetsya ne «vdokhnovitsya» referensami, ne sdelat pokhozhiy interfeys i ne interpretirovat dizayn po-svoemu, a vypolnit maksimalno tochnuyu vizualnuyu rekonstruktsiyu interfeysa v React. Tsel — dobitsya vizualnogo sovpadeniya s referensami na urovne professionalnogo pixel-perfect frontend-vosproizvedeniya s razumnoy adaptatsiey k raznym sootnosheniyam storon.

with etom zapreshcheno ispolzovat izobrazheniya referensov kak polnoekrannye fony, poverkh kotorykh razmeshchayutsya nevidimye knopki. Interfeys dolzhen byt nastoyashchim, strukturirovannym, interaktivnym i svyazannym s realnymi sistemami igry. all paneli, knopki, ramki, indikatory, spiski, vkladki, polosy progressa, kartochki, minikarty, ocheredi proizvodstva i tekstovye elementy dolzhny byt realizovany kak nastoyashchie React-komponenty, Canvas-sloi, SVG-grafika, CSS i podklyuchaemye igrovye dannye. Skrinshot mozhet ispolzovatsya only kak vizualnyy etalon for sravneniya.

Ne menyay deterministic sim-core, boevuyu logiku, ekonomiku, AI, server, replay i komandnyy protokol without dokazannoy neobkhodimosti. Rabotay preimushchestvenno v React UI Layer, UI ViewModels, Zustand-sostoyanii, Babylon Presentation Layer i sloe svyazi interfeysa s sushchestvuyushchimi komandami igry.

## Iskhodnye referensy

Raspakuy `SCREENSHOTS.zip` i naydi katalog `SCREENSHOTS`.

Glavnymi etalonami yavlyayutsya fayly:

```text
1.png
2.png
3.png
4.png
5.png
6.png
7.png
8.png
9.png
10.png
11.png
12.png
13.png
14.png
15.png
16.png
17.png
18.png
19.png
20.png
21.png
22.png
23.png
24.png
```

all eti izobrazheniya imeyut razreshenie priblizitelno 1672×941 i yavlyayutsya osnovnym vizualnym standartom interfeysa.

Katalog:

```text
SCREENSHOTS/Generated/
```

soderzhit starye or uproshchyonnye prototipy interfeysa. Ne kopiruy ikh vizualnyy stil. Ispolzuy ikh only for opredeleniya togo, kakie ekrany uzhe pytalis realizovat, kakie dannye uzhe sushchestvuyut i kakie tekushchie komponenty potrebuetsya zamenit.

with konflikte between izobrazheniyami `1.png–24.png` i faylami iz `Generated` vsegda ispolzuy `1.png–24.png` kak istochnik istiny.

## Pervyy obyazatelnyy shag: katalogizatsiya referensov

before napisaniya koda otkroy kazhdyy fayl `1.png–24.png` v polnom razreshenii. Ne analiziruy ikh only po miniatyuram. Sozday dokument:

```text
docs/ui/REFERENCE_SCREEN_CATALOG.md
```

for kazhdogo izobrazheniya zafiksiruy:

* imya fayla;
* predpolagaemyy ekran;
* fraktsiyu;
* osnovnoy rezhim;
* verkhnyuyu panel;
* levuyu panel;
* pravuyu panel;
* nizhnyuyu panel;
* tsentralnuyu kompozitsiyu;
* fon;
* tipografiku;
* dekorativnye elementy;
* interaktivnye elementy;
* igrovye dannye;
* sostoyaniya knopok;
* modalnye okna;
* osobennosti masshtabirovaniya;
* obshchie komponenty s drugimi ekranami;
* unikalnye komponenty.

Ispolzuy sleduyushchuyu predvaritelnuyu klassifikatsiyu, no utochni eyo after fakticheskogo prosmotra faylov:

```text
1.png  — startovyy ekran or zastavka RA4;
2.png  — glavnoe menyu;
3.png  — vybor kampanii;
4.png  — ekran kampanii Soviet Union;
5.png  — ekran kampanii Alyansa;
6.png  — ekran kampanii Vostochnoy koalitsii;
7.png  — ekran kampanii Khronolegiona;
8.png  — strategicheskaya karta kampanii;
9.png  — brifing or kartochka missii Soviet Union;
10.png — ekran videosvyazi, diplomaticheskogo or syuzhetnogo dialoga;
11.png — dopolnitelnyy ekran kampanii or komandovaniya Alyansa;
12.png — zagruzka missii;
13.png — osnovnoy nazemnyy HUD Soviet Union;
14.png — osnovnoy nazemnyy HUD Alyansa;
15.png — osnovnoy nazemnyy HUD Vostochnoy koalitsii;
16.png — osnovnoy nazemnyy HUD Khronolegiona;
17.png — nastroyka skhvatki or igrovoe lobbi;
18.png — dopolnitelnyy ekran Vostochnoy koalitsii;
19.png — ekran zagruzki karty or kampanii;
20.png — rasshirennyy boevoy HUD Soviet Union;
21.png — bazovyy HUD Soviet Union s vydelennym obektom;
22.png — morskoy HUD Alyansa;
23.png — boevoy morskoy or vozdushnyy HUD Alyansa;
24.png — boevoy HUD Khronolegiona.
```

Ne schitay etu klassifikatsiyu okonchatelnoy without proverki izobrazheniy.

## Tsel rekonstruktsii

Interfeys dolzhen vyglyadet kak dorogoy, mrachnyy, vysokotekhnologichnyy interfeys sovremennoy AAA RTS vo vselennoy alternativnoy kholodnoy voyny.

Obyazatelnye vizualnye svoystva:

* plotnyy interfeys without oshchushcheniya pustogo veb-sayta;
* tyomnye metallicheskie i steklyannye poverkhnosti;
* tonkie svetyashchiesya kontury;
* krasnye, sinie, zelyono-zolotye i fioletovye fraktsionnye temy;
* voennye skhemy, setki, linii, karty i telemetriya;
* skoshennye ugly;
* mnogosloynye ramki;
* glubokie teni;
* vnutrennee svechenie;
* upravlyaemyy bloom;
* teksturirovannye poverkhnosti;
* fraktsionnye znaki;
* vizualnaya glubina;
* State aktivnoy sistemy;
* oshchushchenie rabochego komandnogo terminala;
* otsutstvie standartnogo vida HTML-knopok;
* otsutstvie tipichnogo SaaS-interfeysa;
* otsutstvie chrezmerno okruglykh kartochek;
* otsutstvie mobilnogo Material Design;
* otsutstvie generic Tailwind-dashboard estetiki.

Interfeys ne dolzhen vyglyadet kak nabor obychnykh pryamougolnikov s `border-radius: 8px`. Formy dolzhny stroitsya via CSS `clip-path`, SVG, maski, psevdoelementy, mnogosloynye granitsy, dekorativnye ugly i fraktsionnye ornamenty.

## Zapret na vizualnuyu samodeyatelnost

Ne menyay kompozitsiyu referensov, chtoby ona stala «sovremennee» or «chishche». Ne sokrashchay kolichestvo elementov only potomu, chto interfeys kazhetsya plotnym. Ne perestavlyay minikartu, stroitelnuyu panel, kartochku vydelennogo obekta or komandnye knopki po sobstvennomu vkusu.

Esli na referense element raspolozhen sprava, on dolzhen ostavatsya sprava. Esli HUD zanimaet znachitelnuyu chast nizhney oblasti, ne zamenyay ego malenkoy plavayushchey panelyu. Esli menyu ispolzuet krupnuyu vertikalnuyu navigatsiyu sleva, ne prevrashchay eyo v gorizontalnyy navbar.

Ne ispolzuy shadcn-komponenty, standartnye Bootstrap-kartochki or gotovuyu dashboard-temu kak finalnyy vneshniy vid. Razresheno ispolzovat bazovye biblioteki for povedeniya, no finalnoe oformlenie dolzhno tochno sootvetstvovat referensam.

## Tekhnicheskiy protsess vizualnogo sravneniya

Sozday avtomatizirovannyy protsess snyatiya skrinshotov realizovannykh ekranov v tekh zhe razmerakh, chto i referensy.

Dobav komandy:

```bash
pnpm ui:references
pnpm ui:screenshots
pnpm ui:compare
pnpm ui:test
```

`ui:references` dolzhen proverit nalichie `1.png–24.png`, prochitat razmery i sformirovat katalog.

`ui:screenshots` dolzhen zapuskat prilozhenie via Playwright i snimat kazhdyy realizovannyy ekran with viewport:

```text
1672×941
```

Takzhe snimay dopolnitelnye razmery:

```text
1920×1080
2560×1440
1440×900
1366×768
```

`ui:compare` dolzhen sozdavat:

* etalonnyy skrinshot;
* skrinshot realizatsii;
* nalozhenie s prozrachnostyu;
* difference image;
* bazovuyu chislovuyu metriku razlichiya.

Ne pytaysya bessmyslenno dobitsya polnogo matematicheskogo sovpadeniya fonovogo 3D-rendera. Otsenivay otdelno geometriyu UI: polozhenie paneley, razmery, otstupy, shrifty, knopki, tsveta i kompozitsiyu.

Sozday katalog:

```text
artifacts/ui-comparison/
```

so strukturoy:

```text
screen-01/
  reference.png
  implementation.png
  overlay.png
  difference.png
  report.json
```

Povtori for kazhdogo ekrana.

## Architecture UI

Sozday edinuyu sistemu komponentov, no ne pytaysya sdelat odin universalnyy komponent, soderzhashchiy sotni uslovnykh vyrazheniy.

Primernaya struktura:

```text
apps/game-client/src/ui/
  app/
  screens/
    SplashScreen/
    MainMenuScreen/
    CampaignSelectScreen/
    FactionCampaignScreen/
    StrategicMapScreen/
    MissionBriefingScreen/
    LoadingScreen/
    SkirmishSetupScreen/
    GameplayScreen/
    PauseScreen/
    VictoryScreen/
    DefeatScreen/
  hud/
    common/
    soviet/
    allies/
    easternCoalition/
    chronolegion/
  components/
    Frame/
    BeveledPanel/
    FactionButton/
    MetallicButton/
    HolographicButton/
    ResourceCounter/
    PowerCounter/
    CommandCapCounter/
    MinimapFrame/
    ProductionGrid/
    ProductionCard/
    CommandGrid/
    SelectedEntityCard/
    QueueList/
    EVAPanel/
    Tooltip/
    Modal/
    Tabs/
    ProgressBar/
    PortraitPanel/
    VideoFeed/
  themes/
  tokens/
  assets/
  hooks/
  view-models/
```

Ne sozdavay fayly without fakticheskogo ispolzovaniya. after sozdaniya komponenta podklyuchay ego k realnomu ekranu.

## Sistema dizayn-tokenov

Izvleki iz referensov realnye znacheniya i sozday sistemu tokenov:

```text
ui/tokens/colors.css
ui/tokens/spacing.css
ui/tokens/typography.css
ui/tokens/effects.css
ui/tokens/layout.css
```

Sozday bazovye temy:

```text
theme-soviet
theme-allies
theme-eastern-coalition
theme-chronolegion
```

### Soviet Union

Osnovnye kharakteristiki:

* tyomno-krasnyy;
* bordovyy;
* chyornyy;
* grafit;
* tyoplyy metall;
* krasnoe svechenie;
* sovetskaya zvezda kak tsentralnyy motiv;
* massivnye paneli;
* agressivnye geometricheskie formy;
* krasnye preduprezhdeniya;
* tyazhyolyy promyshlennyy stil.

### Alliance

Osnovnye kharakteristiki:

* kholodnyy siniy;
* goluboe svechenie;
* stal;
* serebro;
* tyomno-siniy fon;
* tochnye tonkie linii;
* chistaya tekhnologicheskaya geometriya;
* golograficheskie elementy;
* lyogkaya prozrachnost;
* strogaya simmetriya.

### Vostochnaya Coalition

Osnovnye kharakteristiki:

* glubokiy zelyonyy;
* nefrit;
* tyomnoe zoloto;
* bronza;
* chyornyy;
* vostochnye geometricheskie motivy;
* sochetanie traditsionnykh uzorov i sovremennoy voennoy elektroniki;
* bolee organichnye linii;
* statusnye zolotye aktsenty.

### Khronolegion

Osnovnye kharakteristiki:

* fioletovyy;
* tyomnyy indigo;
* purpurnoe svechenie;
* prostranstvennye iskazheniya;
* kontsentricheskie elementy;
* khronograficheskie znaki;
* asimmetrichnye gologrammy;
* effekt energii i vremennykh razlomov.

Fraktsionnye temy dolzhny menyat ne only tsvet `accent`, no i:

* formu ramok;
* tip dekorativnykh uglov;
* silu svecheniya;
* fonovye uzory;
* vid progress bar;
* State hover;
* State active;
* vid selection;
* stili ikonok;
* oformlenie tooltip;
* ramku minikarty;
* stroitelnuyu setku;
* kartochku vydeleniya;
* EVA-panel.

## Tipografika

Opredeli vizualno blizkie svobodnye shrifty libo ispolzuy uzhe podklyuchyonnye v proekte, esli oni sootvetstvuyut stilyu. Nelzya ispolzovat sistemnyy Arial kak finalnyy osnovnoy shrift.

Nuzhny otdelnye roli:

* logotip i krupnye zagolovki;
* zagolovki ekranov;
* nazvaniya fraktsiy;
* knopki;
* chislovye pokazateli;
* tekhnicheskie podpisi;
* osnovnoy tekst;
* melkiy HUD-tekst.

Podderzhi kirillitsu. Ne vybiray shrift without kirillicheskikh glifov, esli osnovnoy interfeys russkiy.

for tsifrovykh pokazateley ispolzuy tablichnye tsifry. Chisla kreditov, energii, vremeni i ocheredey ne dolzhny menyat shirinu paneli with obnovlenii.

## Ekran 1: zastavka

Rekonstruiruy zastavku po `1.png`.

Trebuyutsya:

* polnoekrannyy kinematograficheskiy fon;
* tsentralnyy logotip;
* krasnaya zvezda;
* nazvanie igry;
* podzagolovok;
* nizhnyaya tekhnicheskaya stroka;
* myagkoe Move fonovykh chastits;
* atmosfernye oblaka or dym;
* dalyokie ogni;
* lyogkaya animatsiya poyavleniya logotipa;
* perekhod v glavnoe menyu.

Zastavka ne dolzhna byt obychnoy statichnoy kartinkoy. Razdeli fon, logotip, chastitsy i interfeysnye elementy na sloi. Dobav optsiyu propuska klikom or klavishey.

## Ekran 2: glavnoe menyu

Rekonstruiruy `2.png` kak osnovnoy etalon glavnogo menyu.

Sokhrani:

* bolshuyu vertikalnuyu panel navigatsii sleva;
* tsentralnuyu atmosfernuyu stsenu;
* logotip i nazvanie v verkhney oblasti;
* massivnye krasnye knopki;
* metallicheskie ramki;
* fon komandnogo tsentra;
* vybrannyy punkt;
* hover-sostoyaniya;
* nizhnie sluzhebnye elementy;
* vizualnye razdeliteli;
* zatemnenie fona under interfeysom.

Podklyuchi realnye deystviya:

* «Kampaniya»;
* «Skhvatka»;
* «Setevaya igra» kak disabled/deferred, esli rezhim poka zapreshchyon;
* «Nastroyki»;
* «Entsiklopediya» or sootvetstvuyushchiy punkt, esli on prisutstvuet;
* «Avtory»;
* «Vykhod» or vozvrat.

Ne delay nerabotayushchie knopki without oboznacheniya. Otlozhennye rezhimy dolzhny imet yavnoe State «v razrabotke».

## Ekran 3: vybor kampanii

Rekonstruiruy `3.png`.

Sozday chetyre krupnye fraktsionnye kartochki:

* Soviet Union;
* Alliance;
* Vostochnaya Coalition;
* Khronolegion.

Kartochki dolzhny imet:

* krupnyy portret or simvol;
* fraktsionnyy tsvet;
* sobstvennuyu ramku;
* nazvanie;
* hover;
* active;
* selected;
* locked, esli kampaniya nedostupna;
* kratkoe Description;
* perekhod k ekranu kampanii.

Sokhrani pravuyu informatsionnuyu panel i nizhnyuyu navigatsiyu, esli oni prisutstvuyut v referense.

## Ekrany 4–7: stranitsy kampaniy fraktsiy

Rekonstruiruy otdelnye stranitsy:

```text
4.png — Soviet Union
5.png — Alliance
6.png — Vostochnaya Coalition
7.png — Khronolegion
```

Ne delay odin i tot zhe ekran s zamenoy odnogo tsveta. Sokhrani unikalnost kazhdogo referensa.

Kazhdyy ekran dolzhen vklyuchat:

* krupnogo lidera or komanduyushchego;
* nazvanie kampanii;
* fraktsionnuyu simvoliku;
* navigatsionnuyu kolonku;
* Description;
* progress;
* spisok glav;
* statistiku;
* dostupnye missii;
* nizhnie knopki;
* otdelnyy vizualnyy yazyk Factions;
* animirovannyy fon;
* skaniruyushchie linii;
* golograficheskie elementy;
* lyogkoe Move interfeysa.

Sozday obshchiy `FactionCampaignScreen`, no razreshi via kompozitsiyu i theme configuration menyat strukturu i unikalnye dekorativnye elementy.

## Ekran 8: strategicheskaya karta kampanii

Rekonstruiruy `8.png`.

Trebuetsya polnotsennaya interaktivnaya karta:

* krupnaya tsentralnaya karta;
* soedinyonnye uzly missiy;
* aktivnaya missiya;
* zavershyonnye missii;
* zablokirovannye missii;
* bokovaya panel opisaniya;
* prevyu;
* nagrady;
* slozhnost;
* knopka zapuska;
* vozvrat;
* fraktsionnaya stilizatsiya;
* animatsiya liniy;
* hover po uzlam;
* vybor uzla;
* fokusirovka karty.

Ne zamenyay kartu obychnym vertikalnym spiskom missiy.

## Ekrany 9–11: brifingi, videosvyaz i komandovanie

Rekonstruiruy kompozitsii `9.png`, `10.png` i `11.png`.

Realizuy pereispolzuemye komponenty:

* `MissionBriefingPanel`;
* `CommanderPortrait`;
* `VideoCommunicationPanel`;
* `MissionObjectives`;
* `IntelFeed`;
* `FactionStatusPanel`;
* `ContinueButton`;
* `TransmissionOverlay`.

Podderzhi:

* bolshoy portret;
* razdelyonnyy ekran videosvyazi;
* voennye dannye;
* tseli;
* kartu;
* skaniruyushchie linii;
* pomekhi;
* subtitry;
* posledovatelnuyu vydachu teksta;
* perekhod k zagruzke missii.

Ne ispolzuy nastoyashchee video, esli ego no. Realizuy effekt zhivogo kanala via sloi, shum, scanline, lyogkoe Move portreta, waveform i interfeysnye animatsii.

## Ekrany 12 i 19: zagruzka

Rekonstruiruy `12.png` i `19.png`.

Nuzhny dva varianta zagruzki:

* fraktsionnaya or missionnaya zagruzka;
* zagruzka karty or boevogo stsenariya.

Ekran dolzhen pokazyvat realnyy progress:

* zagruzku karty;
* terrain;
* obyazatelnykh modeley;
* materialov;
* HUD;
* audio;
* sim initialization.

Ne ispolzuy iskusstvennyy progress bar, kotoryy prosto idyot ot 0 before 100 po taymeru.

Sokhrani:

* krupnyy fraktsionnyy simvol;
* kinematograficheskiy fon;
* nazvanie missii;
* Description;
* podskazku;
* indikator;
* protsenty;
* dekorativnuyu ramku;
* sluzhebnuyu informatsiyu;
* plavnyy perekhod v match.

## Ekran 17: nastroyka skhvatki

Rekonstruiruy `17.png`.

Ekran dolzhen byt svyazan s realnym zapuskom skirmish i pozvolyat:

* vybrat kartu;
* uvidet prevyu karty;
* vybrat storonu igroka;
* vybrat AI;
* vybrat fraktsiyu AI;
* vybrat tsvet;
* vybrat komandu;
* vybrat slozhnost;
* vybrat startovuyu pozitsiyu;
* nastroit resursy;
* nastroit skorost igry;
* nachat match;
* vernutsya nazad.

Sokhrani plotnuyu tablichnuyu strukturu, krasno-chyornuyu temu, dekorativnuyu levuyu navigatsiyu, tsentralnyy spisok uchastnikov, pravoe prevyu karty i nizhnyuyu knopku zapuska.

Ne prevrashchay ekran v poshagovyy veb-master s bolshimi belymi kartochkami.

## Ekrany 13, 20 i 21: HUD Soviet Union

Ispolzuy `13.png`, `20.png` i `21.png` kak sovmestnye etalony sovetskogo HUD.

Sozday funktsionalnyy `SovietGameplayHUD`, kotoryy vklyuchaet:

* verkhnyuyu tonkuyu panel sostoyaniya;
* soobshcheniya i tseli sleva sverkhu;
* minikartu sprava sverkhu;
* vertikalnuyu stroitelnuyu panel sprava;
* vkladki kategoriy;
* setku ikonok zdaniy i yunitov;
* stoimost;
* progress overlay;
* disabled state;
* ready state;
* ochered;
* nizhnyuyu kartochku vydelennogo obekta;
* portret or ikonku;
* zdorove;
* Status;
* sposobnosti;
* komandnuyu panel;
* gruppovye komandy;
* EVA-log;
* resursy;
* energiyu;
* komandnyy limit;
* knopki remonta, prodazhi i elektropitaniya;
* ramki v sovetskom stile.

HUD ne dolzhen blokirovat vazhnuyu chast karty bolshe, chem na referense. all razmery rasschityvay otnositelno etalonnogo viewport.

## Ekran 14: HUD Alyansa

Rekonstruiruy `14.png`.

Sozday `AlliesGameplayHUD` s toy zhe funktsionalnostyu, no drugim vizualnym yazykom:

* sinie paneli;
* kholodnaya stal;
* serebryanye orly;
* tonkaya tekhnologichnaya ramka;
* sinee svechenie;
* svetlye ikonki;
* bolee chistaya geometriya;
* otdelnyy vid minikarty;
* otdelnyy stil progress i selection.

Ne perekrashivay Soviet HUD via CSS `filter`. Ispolzuy obshchie komponenty, no unikalnye fraktsionnye obolochki.

## Ekran 15: HUD Vostochnoy koalitsii

Rekonstruiruy `15.png`.

Osobennosti:

* zelyono-zolotaya tema;
* nefritovye paneli;
* fraktsionnaya emblema;
* osobaya nizhnyaya kartochka;
* otdelnaya forma stroitelnoy paneli;
* zolotye aktivnye kontury;
* traditsionnye ornamentalnye motivy;
* chitaemaya tekhnologicheskaya setka;
* sobstvennye sostoyaniya knopok.

## Ekrany 16 i 24: HUD Khronolegiona

Rekonstruiruy `16.png` i `24.png`.

Osobennosti:

* fioletovoe i purpurnoe svechenie;
* effekt khronopolya;
* kontsentricheskie formy;
* golograficheskaya minikarta;
* nestabilnye energeticheskie linii;
* asimmetrichnye ramki;
* animirovannye vremennye indikatory;
* unikalnaya stroitelnaya setka;
* otdelnyy stil sostoyaniya ready;
* indikatory spetsialnykh sposobnostey;
* taymery.

Effekty ne dolzhny snizhat chitaemost or FPS. Uvazhay nastroyku reduced motion.

## Ekrany 22 i 23: morskoy or vozdushnyy HUD Alyansa

Rekonstruiruy `22.png` i `23.png`.

Sozday rezhimy:

* nazemnoe Production;
* morskoe Production;
* vozdushnoe Production.

Vizualno interfeys dolzhen sokhranyat obshchuyu temu Alyansa, no menyat:

* kategorii;
* ikonki;
* kartochku vybrannogo obekta;
* dostupnye komandy;
* tipy proizvodimykh edinits;
* kontekstnye paneli;
* State morskoy bazy;
* State aviatsionnykh podrazdeleniy.

## Svyaz s realnymi igrovymi dannymi

Nelzya ostavlyat HUD staticheskim maketom.

Podklyuchi:

* credits;
* power production;
* power consumption;
* low power state;
* command cap;
* selected entity;
* selected group;
* health;
* shield;
* armor;
* current command;
* production queue;
* build progress;
* unit abilities;
* match timer;
* objectives;
* notifications;
* minimap entities;
* fog of war;
* victory state;
* defeat state.

Ispolzuy ViewModels. Ne pozvolyay React chitat vnutrennie mutable-struktury sim-core napryamuyu.

Primer:

```ts
type GameplayHUDViewModel = {
  faction: FactionId;
  credits: number;
  powerProduced: number;
  powerConsumed: number;
  commandCapUsed: number;
  commandCapMaximum: number;
  selection: SelectionViewModel;
  production: ProductionViewModel;
  objectives: ObjectiveViewModel[];
  notifications: HUDNotification[];
  minimap: MinimapViewModel;
  matchTimeSeconds: number;
};
```

Ne peredavay v React pozitsii kazhdogo yunita tridtsat raz v sekundu. Minikarta mozhet poluchat agregirovannoe or canvas-orientirovannoe State s ogranichennoy chastotoy.

## Minikarta

Minikarta dolzhna vyglyadet kak na referensakh, a ne kak prostoy kvadratnyy canvas.

Realizuy:

* fraktsionnuyu ramku;
* terrain;
* vodu;
* obekty;
* resursy;
* soyuznikov;
* vragov v predelakh razvedki;
* tuman voyny;
* tekushchuyu oblast kamery;
* signaly;
* sobytiya napadeniya;
* kliki for peremeshcheniya kamery;
* drag;
* ping;
* pereklyuchenie rezhimov with nalichii.

Canvas minikarty dolzhen nakhoditsya vnutri vizualnoy ramki React/SVG, no ne sozdavay DOM-element na kazhdogo yunita.

## Stroitelnaya panel

Stroitelnaya panel dolzhna tochno povtoryat referensy po:

* shirine;
* vysote;
* raspolozheniyu;
* setke;
* razmeru ikonok;
* forme vkladok;
* tsvetam;
* sostoyaniyu ocheredi;
* tooltip;
* ready overlay;
* progress;
* stoimosti;
* blocked;
* locked;
* low power;
* insufficient funds.

Realizuy kategorii:

* Buildings;
* oborona;
* Infantry;
* Vehicles;
* Aviation;
* Naval;
* spetsialnye sposobnosti.

Pokazyvay only kategorii, dostupnye tekushchey Factions i vybrannomu proizvoditelyu.

## Kartochka vybrannogo obekta

Kartochka dolzhna vklyuchat:

* nazvanie;
* tip;
* fraktsiyu;
* portret or render;
* zdorove;
* shchit;
* opyt;
* bronyu;
* Status;
* tekushchiy prikaz;
* sposobnosti;
* kolichestvo vybrannykh obektov;
* ochered;
* knopki komand.

for gruppovogo vybora ispolzuy otdelnuyu setku ikonok, sootvetstvuyushchuyu referensu.

## Ikonki

Ne ispolzuy emoji, Unicode-simvoly or standartnye brauzernye ikonki v finalnom interfeyse.

Sozday edinyy pipeline ikonok:

* SVG for funktsionalnykh komand;
* rendery modeley for yunitov i zdaniy;
* fraktsionnye ramki;
* atlas libo organizovannyy nabor;
* sostoyaniya normal, hover, active, disabled, locked i ready.

Poka unikalnye ikonki otsutstvuyut, sozday vremennye ikonki v edinom stile, no yavno pomet ikh v asset manifest. Ne ispolzuy sluchaynuyu smes Lucide, Material Icons i rastrovykh izobrazheniy.

## Dekorativnye izobrazheniya i fon

Razreshaetsya ispolzovat otdelnye khudozhestvennye elementy iz prilozhennykh referensov only kak vremennyy orientir, no ne vyrezay tselye paneli iz skrinshotov for ispolzovaniya v igre.

Esli neobkhodimy novye fony, ramki, portrety or emblemy, sozday originalnye assety RA4 s tem zhe urovnem kachestva i kompozitsiey. Ne ostavlyay UI-zony pustymi iz-za otsutstviya finalnogo arta.

all dekorativnye rastrovye elementy dolzhny byt:

* optimizirovany;
* imet prozrachnost with neobkhodimosti;
* ne soderzhat lishnego pustogo prostranstva;
* podderzhivat vysokie DPI;
* ne rastyagivatsya s iskazheniem;
* ispolzovat nine-slice libo SVG for masshtabiruemykh ramok.

## Animatsii

Dobav sderzhannye interfeysnye animatsii:

* poyavlenie ekranov;
* hover;
* active press;
* glow pulse;
* scanning line;
* loading sweep;
* progress fill;
* video static;
* selection;
* notification;
* Warning;
* low power;
* ready state;
* victory;
* defeat.

Ne delay interfeys postoyanno dvizhushchimsya. Animatsii dolzhny podchyorkivat State, a ne meshat upravleniyu.

Podderzhi:

```css
@media (prefers-reduced-motion: reduce)
```

## Zvuki interfeysa

Podklyuchi via sushchestvuyushchiy Audio Manager:

* hover;
* click;
* back;
* confirm;
* error;
* tab switch;
* production ready;
* warning;
* mission selected;
* transmission;
* victory;
* defeat.

Ne vosproizvodi zvuk hover with bystrom dvizhenii po desyatkam knopok without ogranicheniya chastoty.

## Adaptivnost

Osnovnym etalonom yavlyaetsya 1672×941. Na etom razmere interfeys dolzhen maksimalno sovpadat s referensom.

Podderzhi:

* 16:9;
* 16:10;
* ultrawide;
* menshie noutbuki;
* device pixel ratio 1–2.

Ne perestraivay desktop HUD v mobilnyy interfeys. for maloy shiriny dopuskaetsya proportsionalnoe masshtabirovanie i minimalnoe uplotnenie, no struktura dolzhna sokhranyatsya.

Sozday obshchiy `--ui-scale`, zavisyashchiy ot viewport, s ogranichennym diapazonom. Izbegay situatsii, kogda tekst stanovitsya nechitaemym.

## Dostupnost i vzaimodeystvie

Nesmotrya na slozhnyy vizualnyy stil, obespech:

* klaviaturnuyu navigatsiyu po menyu;
* vidimyy focus;
* ARIA labels;
* dostatochnyy kontrast;
* tooltip;
* zapret poteri fokusa;
* Escape for vozvrata;
* Enter for podtverzhdeniya;
* rabotu s masshtabirovaniem brauzera.

Focus-State dolzhno byt stilizovano under fraktsiyu, a ne standartnoy siney ramkoy brauzera.

## Ocheryodnost realizatsii

Ne pytaysya odnovremenno realizovat all 24 ekrana.

### Phase 1 — obshchaya sistema

Snachala sozday:

* reference catalog;
* design tokens;
* bazovye ramki;
* knopki;
* typography;
* fraktsionnye temy;
* screenshot comparison pipeline;
* marshrutizatsiyu ekranov.

### Phase 2 — glavnyy polzovatelskiy put

Zatem realizuy:

1. `1.png` — zastavka;
2. `2.png` — glavnoe menyu;
3. `17.png` — nastroyka skhvatki;
4. `19.png` or `12.png` — zagruzka;
5. `13.png`, `20.png`, `21.png` — Soviet HUD;
6. pauza;
7. pobeda;
8. porazhenie.

Etot put dolzhen pozvolit otkryt igru i proyti nastoyashchuyu skhvatku.

### Phase 3 — fraktsionnye HUD

after polnogo sovetskogo HUD realizuy:

1. `14.png` — Alliance;
2. `15.png` — Vostochnaya Coalition;
3. `16.png` i `24.png` — Khronolegion;
4. `22.png` i `23.png` — morskoy/vozdushnyy variant Alyansa.

### Phase 4 — kampaniya

only zatem realizuy:

1. `3.png`;
2. `4.png`;
3. `5.png`;
4. `6.png`;
5. `7.png`;
6. `8.png`;
7. `9.png`;
8. `10.png`;
9. `11.png`;
10. `12.png`.

Esli razrabotka kampanii zapreshchena tekushchim vertical slice scope, sozday only vizualno zakonchennye storybook/demo routes without podklyucheniya novoy igrovoy logiki. Ne nachinay sozdavat kampaniyu v sim-core.

## Testirovanie

Dobav unit tests for:

* theme switching;
* HUD ViewModels;
* formatting;
* production states;
* disabled states;
* queue progress;
* resource counters;
* screen routing.

Dobav Playwright-testy:

* zagruzka zastavki;
* perekhod v menyu;
* perekhod v skhvatku;
* nastroyka matcha;
* zagruzka;
* poyavlenie HUD;
* otkrytie pauzy;
* vozvrat v igru;
* pobeda;
* porazhenie;
* pereklyuchenie razresheniya;
* otsutstvie oshibok konsoli.

Dobav vizualnye regression tests for klyuchevykh ekranov.

Ne obnovlyay baseline avtomaticheski, esli test ne sovpal. Snachala vyyasni prichinu.

## Proizvoditelnost

HUD ne dolzhen snizhat FPS igry.

Prover:

* React render count;
* stoimost backdrop-filter;
* kolichestvo SVG filters;
* kolichestvo odnovremenno animiruemykh box-shadow;
* canvas minimap;
* obnovlenie progress;
* spiski yunitov;
* tooltips;
* videoeffekty;
* blur.

Ne ispolzuy tyazhyolyy `backdrop-filter` na desyatkakh vlozhennykh elementov. Ne animiruy bolshie polnoekrannye blur-sloi kazhdyy kadr.

Tsel:

* stabilnye 60 FPS na normalnom kompyutere;
* otsutstvie pokadrovogo React rerender;
* HUD update ne chashche neobkhodimogo;
* otsutstvie layout thrashing;
* otsutstvie postoyannogo rosta pamyati.

## Obyazatelnye kriterii gotovnosti

Rabota schitaetsya zavershyonnoy only after vypolneniya vsekh trebovaniy:

1. Kazhdyy realizovannyy ekran imeet realnyy React route or igrovoy state.
2. Interfeys ne yavlyaetsya staticheskim izobrazheniem.
3. Glavnyy put ot zastavki before igrovogo HUD working.
4. Knopki vypolnyayut realnye deystviya.
5. HUD poluchaet realnye dannye igry.
6. Minikarta working.
7. Proizvodstvennaya panel working.
8. Kartochka vydeleniya working.
9. Pauza working.
10. Victory i Defeat rabotayut.
11. Na 1672×941 kompozitsiya maksimalno sovpadaet s referensami.
12. Sozdany comparison images.
13. no standartnogo generic dashboard vida.
14. no emoji i sluchaynykh ikonok.
15. no oshibok React.
16. no oshibok v konsoli.
17. `pnpm build` prokhodit.
18. UI-testy prokhodyat.
19. Determinirovannye testy sim-core prodolzhayut prokhodit.
20. Sozdan otdelnyy git-kommit.

V kontse vyday Report:

* kakie referensy izucheny;
* tablitsu sootvetstviya fayl → realizovannyy ekran;
* kakie komponenty sozdany;
* kakie starye komponenty zameneny;
* kakie realnye dannye podklyucheny;
* kakie sostoyaniya rabotayut;
* Results visual comparison;
* spisok otlichiy ot kazhdogo etalona;
* Results Playwright;
* Results sborki;
* pokazateli React render count;
* pokazateli proizvoditelnosti;
* izmenyonnye fayly;
* commit hash;
* ostavshiesya problemy.

Ne zayavlyay, chto interfeys «polnostyu skopirovan», poka ne predostavleny parnye izobrazheniya reference/implementation i poka glavnyy igrovoy put ne working. Prioritet — vizualnaya tochnost, no ne tsenoy fiktivnogo nerabotayushchego maketa. Rezultat dolzhen odnovremenno vyglyadet kak referens i byt nastoyashchim proizvodstvennym interfeysom igry.