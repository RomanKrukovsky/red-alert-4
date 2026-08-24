# SCARLET HORIZON — Промпты для генерации арта, 3D-моделей и иконок

Промт-шаблоны для генеративных нейросетей: концепт-арты, 3D-модели, здания, супероружие, доктринные скины, национальные маркировки, портреты и UI-иконки. Все юниты и здания Пакта и Альянса.

Источник канона: `SCARLET_HORION_Factions_Modern_World.md` (разделы 6–7).
Палитра: `Assets/RA4UI/Generated/ScarletHorizonRemaster/README.md:9` — Пакт = фиолетовый (НЕ красный), Альянс = холодная синяя сталь.

---

## Как использовать

1. Скопируй блок `STYLE` + нужный список юнитов целиком в генератор.
2. Для единообразия держи `STYLE` и `PALETTE` неизменными во всех запросах; меняй только список юнитов.
3. Для разных выходов (2D-концепт / 3D-модель / иконка / портрет) выбери соответствующий `STYLE`-блок.
4. Stable ID в подписи изображения — для прямой интеграции с движком без переименования.

---

## Общий блок STYLE (2D-концепты)

```
PROJECT: SCARLET HORIZON — RTS unit concept art (two factions)
STYLE: high-end military concept art, realistic near-future 2035–2045,
clean studio turnaround on neutral grey backdrop, subtle ground shadow,
RTS unit readability at small icon size, clear silhouette, faction color
coding on armor panels/insignia only (not on whole body), weathering
light, no text, no watermarks, no UI. Photoreal materials: steel,
composite armor, brushed metal, carbon, canvas, glass optics. Cinematic
rim light. 8K, sharp focus, concept-art quality.
```

---

## BLOCK 1 — Юниты (концепт-арт, 2D)

```
============================================================
FACTION 1 — EURASIAN PACT (Россия)
============================================================
VISUAL LANGUAGE: heavy mechanization, layered reactive armor, low wide
silhouettes, slab-sided brutalist geometry, rail/industrial detailing,
cold-weather kit, ERA blocks, snow/dust weathering.
PALETTE: deep violet and plum as faction accent (NEVER red), cold steel
grey, gunmetal, graphite, rust-orange warning stripes, muted olive
secondary. Violet glow only on energy/EMP weapons and optics.
INSIGNIA: geometric violet rhombus/star marking, subtle.

Render EACH of the following as a separate image, full unit, 3/4 hero
view, same camera angle & lighting for lineup consistency:

INFANTRY (human figure + gear, ~6-8 heads tall):
1. "Рубеж" MС-12 — rifleman: winter parka, violet armband, AK-style
   rifle, light gear, helmet with slit visor, entrenching tool.
2. "Запал" ОШ-4 — grenadier: heavy padded suit, automatic grenade
   launcher, thermobaric backpack tank with warning stripes.
3. "Заслон" ПЗК-9 — AA team: two-man team, man-portable flak cannon on
   shoulder, rangefinder, ghillie-tinged winter camo.
4. "Мастер" ИС-3 — engineer: tool harness, mine detector, welding kit,
   unarmed, satchel charges, violet hardhat lamp.
5. "Разряд" ЭШ-8 — shock trooper: insulated heavy suit, backpack
   capacitor, hand-arc emitter with violet lightning crackle, rubber
   gloves, face shield.
6. "Вектор" КС-6 — officer: long coat, submachine gun, command radio
   mast with violet beacon, binoculars, officer cap.

VEHICLES (ground):
7. "Богатырь" ГРМ-8 — ore harvester: huge 8-wheel armored truck, ore
   hopper, emergency armor plates, industrial crane, no weapon.
8. "Рысь" БРМ-27 — scout: 6-wheel fast IFV, twin MG turret, sensor mast,
   aggressive low profile.
9. "Гранит" ОБТ-92 — MBT: 125mm gun, slab ERA turret, violet optics
   glow, caterpillar tracks, reactive brick armor.
10. "Зарево" ТРС-18 — MLRS: thermobaric rocket launcher box on tracked
    chassis, warning stripes, elevated launcher array.
11. "Громобой" ЭМП-7 — EMP ram vehicle: directed EMP dish array,
    capacitors, violet arcing energy, heavy ram plate.
12. "Воевода" ТТП-11 — superheavy tank: twin heavy cannons, AA missile
    pods, enormous dual-tracked hull, siege-mode stabilizer legs.

AIRCRAFT:
13. "Кречет" И-47 — interceptor jet: delta wing, AAM load, violet
    afterburner, sleek.
14. "Коршун" ШВ-38 — gunship helicopter: nose cannon, rocket pods, side
    troop bay, tandem cockpit, winter camo.
15. "Громада" ТДА-8 — strategic airship: huge drone-carrying bomber
    airship, guided munitions bays, slow ominous silhouette.

NAVAL:
16. "Буран" БК-27 — patrol boat: fast gunboat, autocannon, light AAM,
    electric mine rack.
17. "Морок" УПЛ-90 — attack submarine: heavy torpedo tubes, stealthy
    black hull, conning tower sensors.
18. "Святогор" РКР-44 — missile cruiser: large hull, cruise missile
    VLS arrays, radar masts, long-range bombardment.

HERO:
19. Майор Елена Морозова — female commander, heavy winter coat with
    violet trim, experimental tesla rifle with violet coils, officer
    cap, commanding pose, single figure.

============================================================
FACTION 2 — ATLANTIC ALLIANCE (США/UK/FR/DE + общий пул)
============================================================
VISUAL LANGUAGE: high-tech precision, modular clean armor, angular
composites, networked optics, active-protection panels, drone pods,
Western/NATO national markings.
PALETTE: cold blue steel as faction accent, white/grey panels, brushed
aluminum, subtle national sub-accent (US=navy blue+white, UK=green+red,
FR=navy+tan, DE=grey+red). Blue glow on energy/rail/stealth optics.
INSIGNIA: blue shield/star, NATO-style stencil number.

Render EACH as separate image, same 3/4 hero view & lighting:

COMMON ATLANTIC POOL:
20. "Sentinel" M6 — rifleman: modular assault rifle, light blue-trim
    plates, flashbang ability, clean helmet with HUD visor.
21. Field Engineer E-4 — technician, repair drone swarm orbiting,
    capture kit, blue hardshell.
22. "Lifeline" M-12 medic — medical drone swarm, sterile white panels,
    med-packs, unarmed.
23. "Ward" M46 — active-defense carrier: 6-wheel chassis, interceptor
    drone racks, directional shield emitter, no cannon.
24. "Manta" PHM-22 — hydrofoil patrol craft: fast foil boat, autocannon
    + SAM, radar mast, radio-jammer array.

USA:
25. "Lancer" FGM-31 — missile team: guided launcher, laser designator,
    two-man, US markings.
26. "Frostline" C-7 — cryo specialist: cryo aerosol backpack, frost
    nozzle, cold-blue vent glow, heavy suit.
27. "Pioneer" M88 — harvester: 6-wheel deployable truck, ore bin,
    outpost-deploy mode, US tan+navy.
28. "Oracle" XM190 — railgun artillery: long rail barrel on wheeled
    chassis, precision targeting mast, blue capacitors.
29. "Shrike" F/A-48 — interceptor jet: air-superiority fighter, AAM
    load, blue afterburner, US stars.
30. "Vector" AV-27 — VTOL: tilt-rotor/VTOL assault jet, guided missiles,
    hovering pose.
31. "Nightveil" B-39 — stealth bomber: flying-wing, dark radar-absorbent
    skin, bomb bay, low-observable geometry.
32. "Horizon" CVX-90 — carrier: large flattop, drone launch swarm, US
    navy markings, supercarrier silhouette.

UK:
33. "Longwatch" R-9 — sniper: ghillie, high-precision rifle, scope
    glint, spotter gear, UK flag patch.
34. "Refraction" XM27 — mirage tank: thermal-beam turret, adaptive
    camo skin (half-visible shimmer), low hull.
35. "Resolute" DDG-31 — destroyer: guided-missile destroyer, VLS,
    sonar dome, naval grey, RN markings.
36. Agent Evelyn Hart — female stealth hero, sniper+drone rig, cloak
    shimmer, light armor, UK markings.

FRANCE:
37. "Kestrel" LAV-41 — scout: 8x8 wheeled IFV, autocannon, active-scan
    radar mast, fast sleek, French tricolor accent.

GERMANY:
38. "Bulwark" M14 — MBT: 120mm composite gun, fast mobile hull, target-
    designator optic, Bundeswehr grey+red.
39. "Citadel" M70 — heavy tank: thick sloped armor, twin upgrade
    turret, siege stabilizers, German markings.

============================================================
OUTPUT RULES
============================================================
- One unit per image, transparent-style neutral grey studio backdrop.
- Maintain scale readability: infantry ~1.8m, light vehicle ~4m, tank
  ~7m, superheavy ~12m, aircraft appropriate, naval shown at waterline.
- Faction accent (violet for Pact, blue for Alliance) used sparingly —
  on insignia, optics glow, energy weapons, trim panels only.
- No faction logos from real world; markings are original geometric.
- Each image captioned with unit's Stable ID (e.g. RU_GranitMBT).
- 8K, 16:9 or square, concept turnaround sheet acceptable per unit.
```

---

## BLOCK 2 — 3D game-ready models

Заменить блок `STYLE` из BLOCK 1 на этот, список юнитов 1–39 оставить тем же.

```
PROJECT: SCARLET HORIZON — RTS game-ready 3D models
STYLE: production 3D asset, game-ready mesh, PBR material standards,
clean topology, quads-only, UVs unwrapped, no N-gons, no floating
vertices, scale in meters (infantry ~1.8m, light vehicle ~4m, MBT ~7m,
superheavy ~12m). 2048–4096 textures: albedo, normal, ORM (packed
occlusion/roughness/metallic), emissive for energy/optics. Unreal
Engine 5 compatible, LOD0 + LOD1 + LOD2, collision proxy mesh. Neutral
grey studio render for preview.
WORKFLOW: concept turnaround (front/side/rear/3-4 + top for vehicles),
then T-pose or neutral pose for infantry, then baked preview render.
NO textures baked from photos; original hand-painted + procedural PBR.
Faction color only on trim/insignia/emissive, not full body.
```

---

## BLOCK 3 — Buildings & defenses

```
PROJECT: SCARLET HORIZON — RTS building concept art, two factions
STYLE: high-end military concept art, realistic near-future 2035–2045,
clean studio turnaround on neutral grey backdrop, subtle ground shadow,
architectural concept, isometric-friendly low angle 3/4 view, clear
footprint readability, construction-stage progression hint
(foundation → frame → clad → operational), faction flag/insignia on
rooftop. 8K, sharp focus.

============================================================
EURASIAN PACT — buildings (violet accent, cold steel, brutalist slab)
============================================================
40. Полевой командный пункт — deployable mobile command module (MCV):
    6-wheel truck unfolding into HQ, armored, violet beacon mast.
41. Штаб — HQ: bunker complex with radar mast, construction yard,
    violet comms dish, snow-dusted.
42. Электростанция — power plant: smokestack cooling towers, reactor
    block, high-voltage lines, warning stripes.
43. Перерабатывающий комплекс — ore refinery: industrial hopper +
    conveyor, smelting stacks, Bogatyr dock bay.
44. Казарма — barracks: concrete block bunker, slit windows, violet
    flag, sandbags.
45. Завод бронетехники — war factory: heavy crane, tank assembly bay,
    roller doors, tracks leading out.
46. Аэродром — airfield: 3 pads, control tower, hardened shelters.
47. Военно-морская база — naval yard: drydock cradle, gantry crane,
    submarine/destroyer slips.
48. Радарный узел — radar: rotating array dish, comms tower, bunker
    base.
49. Центр перспективных разработок — tech lab: Tesla coil arrays,
    violet glow, reinforced dome.
50. Пулемётный ДОТ — MG pillbox: concrete bunker, firing slit, MG.
51. Зенитный комплекс — AA tower: Shilka-style multi-barrel on
    rotating base.
52. Комплекс ЭМИ — EMP coil: Perun Tesla coil, violet arcing, base.
53. Бункер — infantry bunker: sandbagged concrete, 5 firing slits.
54. Комплекс активной защиты — Iron Dome analog: interceptor launcher
    battery, radar, armored.
55. Ракетная шахта — missile silo: vertical VLS hatch, command bunker,
    launch plume hint.

============================================================
ATLANTIC ALLIANCE — buildings (blue accent, modular clean panels)
============================================================
56. Мобильный узел — mobile deploy node: wheeled command cab unfolding
    into network HQ, blue comms mast.
57. Сетевой штаб — network HQ: modular hub with antenna lattice, blue
    glow, rooftop flag.
58. Компактный реактор — compact reactor: clean cylindrical reactor,
    small footprint, blue core glow.
59. Переработчик — refinery: automated ore processor, drone bay,
    sleeker than Pact version.
60. Тактическая казарма — tactical barracks: modular container,
    blue flag, drone-pad.
61. Модульный завод — modular war factory: panel-build hangar, crane,
    vehicle exit ramp.
62. Авиабаза — airbase: 4 pads, tower, hardened shelters, blue.
63. Океанический док — oceanic dock: floating modular dock, crane,
    drone-boat bay.
64. Разведцентр — recon center: radar dome + sigint antennas.
65. Центр перспективных систем — advanced systems lab: cryo/mask
    dome, blue energy.
66. Автопушка — autocannon turret: "Страж"-style multi-gun, sandbags.
67. Ракетная ПВО — SAM "Купол": launcher + radar, blue.
68. Электромагнитная батарея — EM battery: prism-style emitter,
    blue capacitors.
69. Комплекс активной защиты — shield projector: directional shield
    dome emitter.
70. Сеть воздушной эвакуации — recall network: large beacon pylon,
    blue portal hint.
71. Гиперзвуковой ударный комплекс — hypersonic strike complex: VLS
    silo, kinetic-rod markings, blue.

OUTPUT RULES: one building per image, same 3/4 isometric-ish angle
for lineup, footprint-readable silhouette, faction color only on
trim/insignia/energy. Construction-stage variant optional (show
foundation vs operational).
```

---

## BLOCK 4 — Superweapon keyframes

```
PROJECT: SCARLET HORIZON — superweapon ability concept keys
STYLE: cinematic in-game action keyframe, atmospheric, faction-tinted
lighting, scale established by ground troops/vehicles for reference.
8K, dramatic.

PACT:
72. "Огненный квадрат" — Зарево MLRS thermobaric barrage: grid of fire
    blossoming across a fortified sector, violet-tinted smoke.
73. "Разряд по земле" — Громобой EMP cone: violet lightning cone
    arcing into enemy formation, vehicles stalled.
74. "Железный купол" — active defense dome: shimmering violet shield
    intercepting incoming missiles over a base.
75. "Каратель" — missile silo launch: ICBM rising from silo, violet
    exhaust, dawn sky.
76. "Заградительный залп" — Святогор cruise-missile volley: missiles
    arcing from cruiser toward coastline.

ALLIANCE:
77. "Синхронный залп" — Oracle railgun shot: hypervelocity slug
    streaking, blue ionization trail.
78. "Режим тени" — Nightveil stealth bomber run: dark wing gliding
    over lit base, bomb doors opening.
79. "Полный авиапакет" — Horizon carrier drone swarm: 8 drones
    launching from flattop in blue trails.
80. "Протокол призрака" — Evelyn Hart cloaked infiltration: shimmering
    silhouette behind enemy lines.
81. "Зенит" — hypersonic kinetic strike: rods-from-god streaking down
    in blue trails onto target zone.
82. "Хроноэвакуация"-style — "Сеть воздушной эвакуации": blue portal
    pulling friendly units back to base.
```

---

## BLOCK 5 — Doctrine / veteran upgrade skins

```
PROJECT: SCARLET HORIZON — doctrine & veteran upgrade skins
STYLE: same base unit as the standard concept, but with upgrade
visuals: veteran rank = extra armor panels + faction trim glow; elite
rank = unique decal + weathered battle damage + enhanced weapon; heroic
rank = distinctive helmet crest/fin + full emissive faction glow +
unique passive-effect aura particle (violet sparks for Pact, blue
data motes for Alliance). Show the SAME unit at all 4 ranks in a
4-panel horizontal progression sheet.

PACT examples to render as 4-rank sheets:
83. RU_RubezhRifleman — Recruit → Veteran (sandbags+ERA vest) → Elite
    (thermobaric satchel) → Heroic (violet aura, flag bearer).
84. RU_GranitMBT — stock → reactive-brick added → upgraded gun + kill
    markings → heroic (violet energy rails, command antenna).
85. RU_VoevodaHeavyTank — stock → siege-mode deployed → elite twin-gun
    refit → heroic (violet arc capacitors, ground tremor aura).

ALLIANCE examples:
86. ATL_SentinelRifleman — stock → veteran (shield drone) → elite
    (laser designator) → heroic (blue holographic tactical aura).
87. US_OracleArtillery — stock → veteran (extra rail caps) → elite
    (targeting laser) → heroic (blue ionization trail on barrel).
88. DE_CitadelTank — stock → veteran (APS panels) → elite (twin EM
    gun) → heroic (blue shield bubble, command mast).

OUTPUT: 4-panel horizontal sheet per unit, same pose, rank increases
left→right, rank label visible small.
```

---

## BLOCK 6 — National sub-accent detail sheets

```
PROJECT: SCARLET HORIZON — national marking detail callouts
STYLE: macro close-up detail render, 3/4 of a single panel/insignia/
helmet side, studio lit, shows national sub-accent within the faction
palette. Use as supplementary reference sheets next to the main unit
concept.

ALLIANCE national sub-accents (all sit on the blue/steel Alliance
palette but carry a small national marking):
89. USA — navy blue + white star stencil, "US" tactical number, olive
    drab secondary panels.
90. UK — green + small red-on-red roundel, "RN"/"ARMY" stencil, desert
    pink or temperate green secondary.
91. France — navy + tan, small tricolor tab, "FRA" stencil.
92. Germany — field grey + red trim, Bundeswehr cross (original
    geometric variant, NOT real-world), "DEU" stencil.
93. Atlantic common pool — blue-only, geometric shield/star, no
    national sub-mark.

PACT national sub-accents:
94. Russia — violet + cold steel, geometric rhombus/star marking,
    "RU" stencil, winter white secondary.
95. Belarus (planned) — violet + grey, optics/repair theme, "BY"
    stencil.
96. Kazakhstan (planned) — violet + tan/steppe, logistics/scout
    theme, "KZ" stencil.

OUTPUT: one detail sheet per nation, 3 markings shown: shoulder patch,
helmet side, vehicle door panel — same sheet, 3 callouts.
```

---

## BLOCK 7 — Commander portraits

```
PROJECT: SCARLET HORIZON — character portrait heads
STYLE: cinematic painted portrait, half-body, 3/4 angle, dramatic
faction rim light (violet for Pact, blue for Alliance), neutral dark
background, readable expression, concept-art realism, no text.
Resolution 1024×1280 vertical.

PACT:
97. Майор Елена Морозова — female, late 30s, short dark hair, winter
    coat with violet trim, tesla rifle over shoulder, resolute stare,
    frost breath.
98. EVA_RU — female tactical AI voice avatar: composed, modern,
    violet-lit comm headset, slight warmth, sharp Russian features.
99. "Вектор" КС-6 officer — male, peaked cap, greatcoat, radio mast
    with violet beacon, weathered face.

ALLIANCE:
100. Agent Evelyn Hart — female, late 20s, light recon suit, sniper
     rifle, drone swarm orbiting, blue stealth shimmer, composed.
101. EVA_AL_Astra — female tactical AI, clean intelligent, blue-lit
     headset, precise modern features, slightly warm.
102. US commander (generic) — male, flight suit or field uniform, US
     navy markings, blue tactical visor.
103. UK sniper corporal — male, ghillie hood down, Longwatch rifle,
     Royal Marines patch (original variant).

OUTPUT: one portrait per request, square or vertical, dark
background, single light source in faction color.
```

---

## BLOCK 8 — UI icons

```
PROJECT: SCARLET HORIZON — RTS UI production icons
STYLE: clean icon, 256×256, transparent or dark-neutral background,
top-down 3/4 silhouette of the unit/building, faction rim light
(violet Pact / blue Alliance), bold readable shape, no fine detail,
works at 64×64 downsize. One icon per image.

Render icons for all 39 units + 36 buildings + 11 superweapon keys
listed above = ~86 icons total. Same camera angle across the set so
infantry icons read as a family and vehicles read as a family.

OUTPUT: filename = Stable ID (e.g. RU_GranitMBT.png,
ATL_SentinelRifleman.png, RU_HQ.png, ATL_WarFactory.png).
```

---

## Рекомендации по разбивке и структуре файлов

- Батчи по фракции, затем по домену (пехота / техника / авиация / флот / здания) — сохраняет палитру.
- Для группового ростер-постера: отдельный запрос "faction unit group shot, all 19 Pact units lined up at scale".
- Блок `STYLE` + `PALETTE` держи дословным во всех подзапросах — меняется только список юнитов.
- Для иконок UI добавь финальную строку:
  `OUTPUT: clean 256×256 silhouette icon on transparent background, top-down 3/4, faction-colored rim light`.

### Целевая структура папок (совпадает с репо)

```
ArtSource/Concepts/
  Pact/        RU_RubezhRifleman.png, RU_GranitMBT.png, ...
  Alliance/    ATL_SentinelRifleman.png, US_OracleArtillery.png, ...
ArtSource/Models/        (3D game-ready)
ArtSource/Icons/         (UI 256×256)
ArtSource/Portraits/     (commander/EVA)
ArtSource/Doctrines/     (4-rank sheets)
ArtSource/DetailSheets/  (national markings)
```

Имена файлов = Stable IDs из библии, движок подхватывает ассеты без переименования.

---

## Источники канона

- `SCARLET_HORION_Factions_Modern_World.md` — раздел 6 (Евразийский пакт — Россия), раздел 7 (Атлантический альянс).
- `Assets/RA4UI/Generated/ScarletHorizonRemaster/README.md:9` — палитра фракций (Пакт = фиолетовый, не красный).
- `CLAUDE.md` — инварианты проекта, запрет на нелицензированные ассеты и копирование реальных мировых логотипов/дизайна.