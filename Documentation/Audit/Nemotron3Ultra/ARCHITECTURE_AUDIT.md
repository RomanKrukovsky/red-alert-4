# RedAlert4 — Arkhitekturnyy Audit

**Date audita:** 2026-07-30  
**Oblast:** all C++ moduli, sistema sborki, zavisimosti moduley, garantii determinizma

---

## Graf zavisimostey moduley (Fakticheskiy vs Zaplanirovannyy)

### Zaplanirovannaya sloistaya Architecture (iz `RedAlert4.uproject` + `Build.cs`)
```
Unreal Engine (CoreUObject, Engine, etc.)
    │
    ├── RA4Editor (Editor-only)
    │
    ├── RA4UI (Runtime, zavisit ot CommonUI, MVVM, EnhancedInput)
    │
    ├── RA4Network (Runtime, zavisit ot CoreUObject)
    │
    ├── RA4Campaign (Runtime)
    │
    ├── RA4AI (Runtime)
    │
    ├── RA4Presentation (Runtime)
    │
    ├── RA4Input (Runtime, zavisit ot EnhancedInput)
    │
    ├── RA4FogOfWar (Runtime)
    │
    ├── RA4Navigation (Runtime)
    │
    ├── RA4Combat (Runtime)
    │
    ├── RA4Replay (Runtime)
    │
    ├── RA4Simulation (Runtime, **no UNREAL DEPS**)
    │
    ├── RA4Content (Runtime, **no UNREAL DEPS**)
    │
    └── RA4Core (Runtime, **no UNREAL DEPS**)
```

### Fakticheskie zavisimosti (iz `Build.cs` + include zagolovkov)

| Modul | Deklariruemye depy | Fakticheskie Unreal depy v zagolovkakh | Narusheniya |
|--------|-------------------|--------------------------------------|-----------|
| RA4Core | (no) | **no** ✅ | — |
| RA4Content | RA4Core | **no** ✅ | — |
| RA4Simulation | RA4Core, RA4Content, RA4Navigation | **no** ✅ | — |
| RA4Navigation | RA4Core | **no** ✅ | — |
| RA4Replay | RA4Core, RA4Simulation | **no** ✅ | — |
| RA4Combat | RA4Core | **no** ✅ | — |
| RA4FogOfWar | RA4Core | **no** ✅ | — |
| RA4AI | RA4Core, RA4Simulation, RA4Navigation | **no** ✅ | — |
| RA4Network | CoreUObject | only `WorldSubsystem` ✅ | — |
| RA4Input | EnhancedInput | `EnhancedInputComponent` ✅ | — |
| RA4Presentation | Engine, CoreUObject | `Niagara`, `AnimInstance` — **only v prezentatsii** ✅ | — |
| RA4UI | CommonUI, MVVM, EnhancedInput | `UUserWidget`, `Mvvm` — **only v UI** ✅ | — |
| RA4Editor | UnrealEd, RA4Simulation, RA4Content | `Commandlet` — **only editor** ✅ | — |
| RedAlert4 (main) | Engine, CoreUObject | GameMode, PlayerController, CameraPawn — **kley igry** ✅ | — |

**✅ Narusheniy sloistoy arkhitektury ne naydeno.** Yadro simulyatsii (RA4Core, RA4Content, RA4Simulation, RA4Navigation, RA4Replay, RA4Combat, RA4FogOfWar) imeet **nol Unreal Engine zagolovochnykh zavisimostey**. Eto nastoyashchee dostizhenie.

---

## Proverka na tsiklicheskie zavisimosti

**Ne obnaruzheno.** Graf zavisimostey moduley — chistyy DAG. verified:
- CMake `target_link_libraries` poryadok sobiraetsya korrektno
- Nikakie khedery ne tekut vverkh ot simulyatsii k prezentatsii
- `RA4Simulation` inklyudit `RA4Navigation` zagolovki, no ne naoborot

---

## Garantii determinizma — Uroven arkhitektury

### ✅ Garantirovano po dizaynu
| Mekhanizm | Mestopolozhenie | Verification |
|----------|----------------|-------------|
| Fiksirovannaya tochka (48.16) | `RA4Core/Fixed.h` | `__int128` widening + perenosimyy follbek |
| Generation-handled EntityIds | `RA4Core/Ids.h:13-31` | Pereispolzovanie slota ne mozhet perenatselit ustarevshie prikazy |
| Uporyadochennyy `std::map` for kadrov CommandBus | `CommandBus.h:45` | Determinirovannaya iteratsiya vs `unordered_map` |
| Fiksirovannyy poryadok sistem tika | `SimWorld.h:145-158` | 14 sistem, nikakoy dinamicheskoy registratsii |
| RNG na simulyatsiyu, s sidom | `SimWorld.h:137`, `Random.h` | `Xoshiro256++`, vosproizvodimyy |
| Kontrolnaya summa isklyuchaet keshi | `SimWorld.h:128` | Sobytiya, kesh flow field isklyucheny |
| Versionirovannyy format repleya | `Replay.h:22` | `kReplayFormatVersion=1`, magic `0x34414952` |

### ⚠️ Riski for determinizma
| Risk | Mestopolozhenie | Seryoznost | Mitigatsiya |
|------|----------------|-------------|-----------|
| `ContentDatabase` ispolzuet `unordered_map` for lookup indeksov | `ContentDatabase.h:58-62` | **VYSOKAYa** | `ComputeContentHash()` iteriruet `unordered_map` — poryadok otlichaetsya between libstdc++/libc++. **Khesh kontenta raskhoditsya krossplatformenno.** |
| `ToDoubleUnsafe()` sushchestvuet v `Fixed` | `Fixed.h:110` | SREDNYaYa | Nazvan "Unsafe", only for logirovaniya. Proaudirovat all call sites. |
| `NavigationGrid` polnaya perestroyka with kazhdoy postroyke | `SimWorld.cpp:399-414` | SREDNYaYa | O(WH) na postroyku. Ispolzovat dirty-rect inkrementalnoe obnovlenie. |
| `FlowFieldCache` LRU eviction ispolzuet `AccessSerial` | `SimWorld.cpp:526-536` | NIZKAYa | Determinirovano esli schyotchik na tik. verified. |
| `std::sort` v `RefreshPlayerTech` | `SimWorld.cpp:612` | NIZKAYa | Sortiruet `ContentId` (uint32) — stabilno krossplatformenno. |

### ❌ Nedeterminizm khesha kontenta — **Kriticheskiy bloker for krossplatformennogo lokstepa**
```cpp
// ContentDatabase.h:58-62
std::unordered_map<uint32_t, size_t> EntityIndex;
std::unordered_map<uint32_t, size_t> WeaponIndex;
// ...
uint64_t ComputeContentHash() const {
    // Iteriruet unordered_map → PORYaDOK NEOPREDELYoN → KhESh RASKhODITSYa
}
```
**Neobkhodimo ispravlenie:** Zamenit na `std::map` or sortirovat klyuchi pered kheshirovaniem. Eto lomaet krossplatformennyy multipleer i verifikatsiyu repleev na neidentichnykh standartnykh bibliotekakh.

---

## Inventar tekhnicheskogo dolga

### Kriticheskiy (Blokiruet shipping or vyzyvaet skrytye bagi korrektnosti)

| ID | Mestopolozhenie | Problema | Usiliya |
|----|----------------|----------|--------|
| ARCH-001 | `ContentDatabase.h:58-62` | `unordered_map` poryadok iteratsii nedeterminirovan → khesh kontenta raskhoditsya | S (1 den) |
| ARCH-002 | `DefaultContent.cpp` | only 2/4 Factions, 10/78 yunitov, matritsa urona 7/64 zapisi | L (nedeli — kontent) |
| ARCH-003 | `BibleContentLoader.cpp` | Ozhidaet `RA4_Bible_Normalized.json` — **fayl otsutstvuet v repozitorii** | M (1 nedelya payplayn) |
| ARCH-004 | `SimWorld.cpp:399` | Polnaya perestroyka navigatsionnogo grida with kazhdoy postroyke | M (2 dnya) |

### Mazhornyy (Degradiruet podderzhivaemost/proizvoditelnost)

| ID | Mestopolozhenie | Problema | Usiliya |
|----|----------------|----------|--------|
| ARCH-005 | `SimWorld.cpp:551-549` | Kesh flow field ogranichen 64 zapisyami, LRU vytesnenie | S (tyuning) |
| ARCH-006 | `CommandBus.h:45` | `std::map<TickIndex, CommandFrame>` — O(log N) na kadr, dopustimo no moglo byt ring buffer | S |
| ARCH-007 | `SimWorld.h:236` | `kMaxCommandsPerPlayerPerTick = 64` zakhardkozheno — dolzhno byt konfigom | S |
| ARCH-008 | `SimConfig.h` | `kMaxEntities` nevidim — byudzhet entiti neprozrachen | S |
| ARCH-009 | `DefaultContent.cpp:45` | Struktura FactionSetup dubliruet dannye for Sovetskogo/Alyansa — ne data-driven | M (refaktor v Data Assets) |

### Minornyy (Kachestvo koda)

| ID | Mestopolozhenie | Problema |
|----|----------------|----------|
| ARCH-010 | `Fixed.h:142-143` | `FxSin`/`FxCos` obyavleny `RA4CORE_API` no `.cpp` ne viden — proverit linkovku |
| ARCH-011 | `Ids.h:59-68` | `HashName` FNV-1a — khorosho, no `constexpr` working only for literalov |
| ARCH-012 | `Command.h:71-87` | `Serialize`/`Deserialize` ruchnye — rassmotret generatsiyu serializatsii |

---

## Otsenka sistemy sborki

### CMake konfiguratsiya (`build/CMakeCache.txt`)
- **Generator:** Ninja
- **Tipy sborki:** Debug, Development, Shipping (Conclusion)
- **Sanitayzery:** ASan sborki sushchestvuyut (`build/asan/`, `build/hb-asan/`)
- **Testy:** CTest vklyuchyon (`build/Testing/`), `RA4Tests` ispolnyaemyy zaregistrirovan

### Integratsiya s Unreal Build Tool
- Moduli kompiliruyutsya kak **staticheskie biblioteki** (`libRA4Core.a` i t.d.) via CMake
- **UBT ne ispolzuetsya** — eto kastomnaya CMake sborka, imitiruyushchaya strukturu UBT moduley
- **Risk:** Unreal plaginy (GameplayAbilities, CommonUI, MVVM, EnhancedInput) obyavleny v `.uproject` no **ne linkuyutsya v CMake**. Editor sborka upadyot without nikh.

### Otsutstvuyushchaya Verification sborki
| Proverka | Status |
|----------|--------|
| Editor sborki (`RedAlert4Editor`) | ❌ Ne testirovalos |
| Shipping sborka (optimizatsii, without assertov) | ❌ Ne testirovalos |
| Linkovka plaginov (GameplayAbilities, CommonUI, MVVM) | ❌ no v CMake |
| Cooking / pakovka | ❌ Ne testirovalos |
| iOS / Android / Linux targety | ❌ Ne nastroeny |

---

## Bezopasnost i Supply Chain

| Proverka | Rezultat |
|----------|-----------|
| Khardkodnye sekrety v kode | ✅ Ne naydeno |
| Storonnie zavisimosti | Minimalny: only Unreal Engine + STL |
| Istochnik sida `Random.h` | Determinirovannyy (yavnyy sid) — **ne kriptograficheskiy** |
| Proverka granits serializatsii | `ByteReader::HasError()` proveryaetsya v `CommandFrame::Deserialize` ✅ |
| Rate limiting komand | 64 komandy/igrok/tik prinuditelno ✅ |

---

## Rekomendatsii (Architecture)

1. **NEMEDLENNO:** Ispravit khesh kontenta `ContentDatabase` — zamenit `unordered_map` na `std::map` or otsortirovannyy vektor before lyubogo krossplatformennogo testirovaniya.
2. **NEMEDLENNO:** Dobavit `RA4_Bible_Normalized.json` v repozitoriy or zadokumentirovat payplayn ego generatsii iz markdauna.
3. **KRATKOSROChNO:** Migrirovat `DefaultContent.cpp` → Data Assets (Primary Data Assets na yunit/zdanie). Pattern `FactionSetup` dokazyvaet, chto data-driven dizayn zalozhen.
4. **KRATKOSROChNO:** Inkrementalnye obnovleniya navigatsionnogo grida (dirty rects) — 500+ entiti zastanet na polnoy perestroyke.
5. **SREDNESROChNO:** Pereyti na UBT for Unreal moduley, ostavit CMake only for headless simulyatsionnykh lib. Gibridnaya sborka khrupka.
6. **SREDNESROChNO:** Dobavit vyzov `ContentDatabase::Validate()` v `SimWorld::Initialize()` — lovit avtorskie oshibki na starte matcha.
7. **DOLGOSROChNO:** Migratsiya na ECS arkhetYPES (tekushchiy SoA fiksirovannoy skhemy). Tekushchiy dizayn podderzhivaet ~20 komponentov; dobavlenie fraktsionno-unikalnykh komponentov potrebuet rosta massivov.

---

*Konets arkhitekturnogo audita*