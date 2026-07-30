# SCREENSHOT_INVENTORY

Date: 2026-07-28
Source directory: `SCREENSHOTS/` (24 PNG references) + `SCREENSHOTS/Generated/` (4) +
`Assets/RA4UI/Generated/` (17 generated UI assets) + `Saved/Screenshots/QA/` (prior QA captures)

Status: **PARTIAL — indexing halted pending an IP decision (see § 3).**

## 1. Measured facts (all 24 files)

Every reference is identical in format:

| Property | Value |
| --- | --- |
| Resolution | **1672 × 941** |
| Aspect ratio | 16:9 (1.7773) |
| Format | PNG, 1.83–2.97 MB |

**Finding S-1 (high).** The references are **not** at the brief's stated base
resolution of 1920×1080. Scale factor is 1920 / 1672 = **1.1483**. Every pixel
measurement taken from a reference must be multiplied by this factor before it becomes
a UMG value, and the brief's "±2–4 px at base resolution" tolerance corresponds to
only **±1.7–3.5 px in reference space**. Measuring in reference space and comparing in
base space without this conversion would silently bake in a ~15 % layout error.

**Finding S-2 (medium).** 1672×941 is a windowed capture, not a fullscreen render, so
the references contain no information about true safe-area behaviour, and none about
21:9 or 16:10. Those layouts must be derived from the design system, and are flagged
INFERENCE rather than measured.

## 2. Screens identified so far

Six of 24 examined. Identification is from direct inspection, not inference.

| ID | File | Screen | Notable functional content |
| --- | --- | --- | --- |
| SC-01 | `1.png` | Title / attract screen | Centred logo lockup, "НАЖМИТЕ ЛЮБУЮ КЛАВИШУ" prompt, animated storm background |
| SC-02 | `2.png` | Main menu | Left vertical nav (8 items: Кампания, Сетевая игра, Схватка, Редактор, Энциклопедия, Модификации, Настройки, Выход), commander profile card with XP bar (45 780 / 75 000, «Уровень 25»), news panel with carousel dots, operations summary panel, footer status strip (сеть/сервисы/версия) |
| SC-03 | `3.png` | Campaign select | Top tab bar (6 tabs), 4 faction cards (СССР, Альянс, Восточная коалиция, Хронолегион), right detail panel with description + progress (миссии 06/18, доп. задания 09/36, сложность), bottom action bar |
| SC-04 | `4.png` | Campaign detail (Soviet) | Left faction rail, large faction title, progress block (58 %, миссий 14/24, сложность), 3 primary actions (Новая игра / Продолжить / Выбор главы), commander portrait, quote block |
| SC-13 | `13.png` | In-match HUD — base building | Top resource bar (4 resources + supply 88/200), minimap panel with 6 tool buttons, right production panel with 4 tabs (Строить/Войска/Улучшения/Доктрины) and collapsible category sections, production cards with icon + name + cost, build queue with per-item timer and cancel, selection panel (portrait, HP 5000/5000, description), control-group strip 1–0, bottom command bar |
| SC-20 | `20.png` | In-match HUD — large battle | Objectives panel with checkable list + optional objective, EVA alert block, resource bar with warning + mute icons, minimap with faction-coloured blips and camera viewport frame, production panel with sub-tab row, multi-selection grid with per-unit counts and health bars, unit detail card (role, description, prochnost bar, sub-group counts), command grid with visible hotkeys Q/W/E/R/T and A/S/D/F |

Remaining, not yet examined: `5–12.png`, `14–19.png`, `21–24.png` (18 files).

## 3. Blocking finding: the references embed third-party IP

**Finding S-3 (blocking, legal).** The menu references reproduce Electronic Arts'
Command & Conquer intellectual property:

| Reference | Element observed |
| --- | --- |
| SC-01, SC-02, SC-03, SC-04 | The **"COMMAND & CONQUER™"** wordmark |
| SC-01, SC-02, SC-03, SC-04 | The **"RED ALERT 4"** logo lockup in the franchise's typographic style |
| SC-02 | The literal footer line **"© 2024 ELECTRONIC ARTS INC."** |
| SC-02, SC-03, SC-04 | Franchise-styled faction insignia (Soviet star-and-wings emblem; Allied eagle) |
| SC-04 | **"ИМПЕРИЯ ВОСХОДЯЩЕГО СОЛНЦА"** with rising-sun insignia — the Empire of the Rising Sun, a faction from *Command & Conquer: Red Alert 3* |

This directly contradicts the project's own founding constraint, recorded at project
start and formalised in `Docs/ADR/0004-content-lives-in-data-not-code.md`:

> Use the name Red Alert 4 only as an internal working title if the project has no
> official Electronic Arts licence. Do not include original models, textures, music,
> sounds, videos, maps, **logos**, fonts, characters … from existing Command & Conquer
> games.

Reproducing these references pixel-accurately would build a competitor's registered
trademarks, franchise logo lockup, faction marks and a **false copyright attribution**
into the shipping product. `Content/AssetRegistry/ThirdPartyAssets.json` currently
lists zero acquired assets and no EA licence.

**Finding S-4 (medium, consistency).** SC-04 shows the faction roster as
СССР / Альянс / **Империя восходящего солнца**, while SC-03 shows
СССР / Альянс / **Восточная коалиция** / **Хронолегион** — which is the project's
actual roster in `RA4Content` and the campaign brief. Per the brief's own tie-break
rule (the more detailed reference wins), **SC-03 is authoritative** and SC-04's third
faction is treated as an artefact of the reference generation. To be recorded in an ADR.

## 4. What is unaffected by S-3

The IP problem is confined to the branding layer. Everything that constitutes the
actual engineering work is reproducible without touching it:

- screen composition, grid, block sizes, margins, anchors, z-order
- the whole in-match HUD (SC-13, SC-20): resource bar, minimap, production tabs and
  cards, build queue, selection and multi-selection, command grid, objectives, EVA feed
- typographic scale, colour system, panel and frame treatment, state model
- interaction logic, navigation, focus routing, hotkeys, tooltips
- animation character and timing
- data binding to the real simulation

Only the following must be replaced with original marks: the C&C wordmark, the
"Red Alert 4" logo lockup, the EA copyright line, and the franchise faction insignia.

## 5. Decision required before implementation continues

See the session report. Two admissible paths:

1. **Clean-Room Profile (recommended, no legal exposure).** Identical layout,
   geometry, typography scale, colour system, states and behaviour; original wordmark,
   original faction emblems, correct copyright. The project already has a
   Licensed/Clean-Room profile split designed for exactly this, and all names already
   live in localization keys and Data Assets.
2. **Licensed Profile.** Requires documented evidence of an Electronic Arts licence.
   Without it this path is not available.

Until this is answered, no branding asset is created and no reference is reproduced
verbatim.
