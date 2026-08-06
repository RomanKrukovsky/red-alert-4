# Production Risk Register (`RISK_REGISTER.md`)

**Audit Date**: August 4, 2026  
**Scope**: Comprehensive risk assessment for technical, legal, architectural, and production areas of the RA4 project.

---

## Risk Evaluation Matrix

Severity Ratings:
- **CRITICAL**: Immediate threat to project viability, legal compliance, or compilation.
- **HIGH**: Substantial technical debt, performance risk, or workflow blocker.
- **MEDIUM**: Manageable risk requiring scheduled refactoring or replacement.
- **LOW**: Minor operational overhead.

---

## 1. Legal & Intellectual Property (IP) Risks

| Risk ID | Risk Category | Risk Description | Probability | Impact | Severity Rating | Mitigation Strategy |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **RISK-01** | **Legal / IP** | **Command & Conquer / Red Alert Trademark Infringement**: Use of EA trademarked names (`RedAlert4`, `Soviet`, `Allied`, `Tiberium`, `EVA`). | High | Critical | **CRITICAL** | Perform complete identifier neutralization pass across project names, data assets, and audio. |
| **RISK-02** | **Legal / IP** | **Unlicensed Proprietary Font Embedding**: `Druk Cyr` font referenced in Noesis XAML assets without verifiable commercial license. | Medium | High | **HIGH** | Replace font in `Typography.xaml` with open-source SIL OFL fonts (`Oswald`, `Inter`, `Bebas Neue`). |
| **RISK-03** | **Legal / IP** | **Non-Commercial Third-Party 3D Assets**: Sketchfab / external 3D models with `CC-BY-NC` or `Editorial Use Only` licenses. | Medium | Medium | **MEDIUM** | Audit all imported 3D mesh assets in `Content/ThirdParty/` and replace non-commercial licenses with in-house or CC0 assets. |

---

## 2. Architecture & Engine Integration Risks

| Risk ID | Risk Category | Risk Description | Probability | Impact | Severity Rating | Mitigation Strategy |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **RISK-04** | **Engine / Build** | **Missing NoesisGUI Unreal Engine Plugin**: Noesis plugin absent from `Plugins/` prevents standard Unreal Build Tool compilation. | High | Critical | **CRITICAL** | Install NoesisGUI UE5 plugin into `Plugins/NoesisGUI` OR implement `#if WITH_NOESIS` preprocessor guards. |
| **RISK-05** | **Architecture** | **Presentation Polling Overhead**: `URA4PresentationSubsystem` polls all simulation entity positions every frame during high entity counts (>1000). | Low | High | **MEDIUM** | Implement a dirty entity queue or delta-event push mechanism from `SimWorld` to presentation subsystem. |
| **RISK-06** | **Determinism** | **Unreal Engine Float Non-Determinism in Presentation**: Visual actors mutating simulation state via physics overlap or float precision drift. | Low | Critical | **HIGH** | Maintain strict architectural boundary: Presentation layer only reads from simulation snapshots; never mutates `SimWorld`. |

---

## 3. UI Technology & Production Risks

| Risk ID | Risk Category | Risk Description | Probability | Impact | Severity Rating | Mitigation Strategy |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **RISK-07** | **UI / Tech Debt** | **Tri-Layer UI Fragmentation**: Concurrent existence of NoesisGUI, Slate/UMG, and `ra4-ui` Web prototype. | High | Medium | **HIGH** | Deprecate `ra4-ui` prototype and standardize on single production frontend framework (UMG or NoesisGUI). |
| **RISK-08** | **Input** | **Dormant Direct Control Possession Mode**: Unit possession (`F` key) exists in C++ but lacks PlayerController HUD integration. | Medium | Low | **LOW** | Connect possession toggle event to `RA4PlayerController` and present input prompt in HUD. |

---

## 4. Build, Packaging & Deployment Risks

| Risk ID | Risk Category | Risk Description | Probability | Impact | Severity Rating | Mitigation Strategy |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **RISK-09** | **Build Pipeline** | **Headless Test Relative Path Failure**: `RA4Tests` fails 18 tests if executed outside project root directory due to relative JSON pathing. | Medium | Low | **LOW** | Add root path detection logic to `BibleContentLoader.cpp` to resolve `Content/` directory regardless of launch CWD. |
| **RISK-10** | **Deployment** | **Automated Packaging Automation Gap**: Lack of CI/local UAT packaging script for shipping standalone executables. | Medium | Medium | **MEDIUM** | Create `Tools/Build/package.sh` calling Unreal Automation Tool (`RunUAT`) with shipping target parameters. |

---

## 5. Perception-Warfare Design & Player-Experience Risks

Added 2026-08-05 alongside ADR-0021..0026. These risks are qualitatively different from the
technical risks above: the systems can be *correct and still fail*, because they deliberately
withhold information from the player. Mitigations are therefore design contracts, not code fixes,
and each must be validated by playtest evidence before its system is declared done.

| Risk ID | Risk Category | Risk Description | Probability | Impact | Severity Rating | Mitigation Strategy |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **RISK-11** | **UX / Perception** | **Stale Intel Reads As A Bug**: Ghosted units at last-known positions, buildings shown after being sold, and approximate counts are indistinguishable from rendering or sync defects to a player who has not read a manual. Risk of "the game is broken" reviews rather than "I was outplayed". | High | High | **CRITICAL** | Mandatory UI language before implementation: desaturation ramp, explicit "last seen" timestamps, confidence percentage always visible on selection. Tutorial mission must teach the confidence readout before any competitive exposure. Ship with `Intel = Classic` available. |
| **RISK-12** | **UX / Fairness** | **Deception Feels Like Cheating**: A player who commits an army against a phantom column may conclude the game lied rather than that their scouting was thin. | High | High | **HIGH** | Honesty contract (GDD section 8): confidence is never falsified, source disagreement is surfaced, every deception is exposable by observation. Post-match report reveals what was real (deferred: idea 20). Deception tools capped at low confidence ceilings so a *careful* player is never fully fooled. |
| **RISK-13** | **UX / Controls** | **Order Delay Reads As Input Lag**: Command-network propagation is indistinguishable from netcode failure or a dropped click, attacking the product's core "zero input latency" claim. | Medium | Critical | **CRITICAL** | Healthy-path latency budget <= 4 ticks (200 ms) enforced as a CI performance gate; link status (`CONNECTED`/`DEGRADED`/`AUTONOMOUS`) visible *before* ordering; audible/visual order-acknowledged feedback distinct from order-issued; `CommandNetwork = Classic` default in ladder play; DirectControl exempt from delay. |
| **RISK-14** | **Design / Balance** | **Adaptive Opponent Feels Punitive**: An AI that hard-counters the player's preferred style can read as the game refusing to let them play, especially for lower-skill players. | Medium | High | **HIGH** | Clamp DoctrineBias influence ranges (ADR-0025); gate adaptation strength by difficulty; expose the full model to the player in a "what the enemy learned about you" screen with a reset button; adaptation off by default at the two lowest difficulties. |
| **RISK-15** | **Design / Campaign** | **Persistence Makes A Mission Unwinnable**: Accumulated `TheaterState` damage (destroyed bridges, contamination, wreck clutter) blocks a route a later mission requires. | Medium | Critical | **HIGH** | Declare mission-critical routes persistence-immune (ADR-0024); property-based scenario tests over randomized valid TheaterStates asserting critical-path traversability; corrupt or missing blob always falls back to the pristine authored map. |
| **RISK-16** | **Performance** | **Belief State Cost Scales By Player Count**: Per-player perceived worlds multiply state size, decay work and checksum cost by up to 4x (8x in larger custom matches), threatening the sim-tick budget in late game. | Medium | High | **HIGH** | Hard caps on tracks per player and amortized round-robin decay (ADR-0021/0026); budgets fixed in PERFORMANCE_BUDGETS.md section 4 with a combined 2.5 ms/tick ceiling; benchmark required in every implementation PR; kill switch proven bit-equal to the pre-intel baseline. |
| **RISK-17** | **Architecture / Scope** | **Wide Migration Underestimated**: ADR-0021 describes replacing every "is visible" query with "what do I believe" as a mechanical refactor; in practice it touches AI targeting, UI selection, command validation and fog-dependent abilities, and a partial migration silently reintroduces omniscience. | High | High | **HIGH** | Treat the migration as its own work package with a call-site inventory produced before implementation; instrumented leak detector in test builds failing any read of objective state for non-owned entities; migration is not "done" until the detector is green with zero exemptions. |
| **RISK-18** | **Process** | **Design Documents Drift From Implementation**: ADR-0021 (design) and ADR-0026 (implementation) already diverge in tick-rate and data-model naming; unreconciled ADR pairs will mislead future agents, and this repository has a documented history of fabricated status claims. | High | Medium | **MEDIUM** | ADR-0026 is the authority on implemented behaviour, ADR-0021 on intent; divergences must be recorded in ADR-0026's rejection log. Independent review required before either moves to Accepted. Duplicate ADR numbering across `Docs/ADRs/` and `Docs/Architecture/ADR/` must be consolidated (see NEXT_ACTIONS). |
| **RISK-19** | **Accessibility** | **Uncertainty Presentation Excludes Players**: Confidence conveyed primarily through desaturation and low-contrast ghosting is unreadable for colour-blind and low-vision players, and an information-dense uncertainty UI raises cognitive load for players with attention or processing differences. | Medium | High | **HIGH** | Confidence must be encoded redundantly (numeric value + icon fill state + text timestamp), never by colour or opacity alone; high-contrast intel mode; option to display numeric confidence permanently rather than on hover; UI_UX_BIBLE accessibility section required before implementation. |

---

## 6. Content Pipeline & Provenance Risks

Added 2026-08-06 from a repository audit. Every figure below came from running the command shown,
not from reading a document — this project has a documented history of status claims that did not
survive verification.

| Risk ID | Risk Category | Risk Description | Probability | Impact | Severity Rating | Mitigation Strategy |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **RISK-20** | **Version Control / Data Loss** | **11 GB of game content exists only on one developer's disk.** `.gitignore` excludes `*.uasset` and `*.umap` globally (lines 43-45), with narrow force-add exceptions: 4,579 asset files are on disk, 661 are tracked. `Content/ThirdParty/CityPark` (4.1 GB) and `FactoryEnvironment` (7.0 GB) are entirely absent from git — `git check-ignore` confirms they are silently ignored, so they never even appear as untracked. `RA4_Skirmish_Production` references CityPark flora and its water shader, so **the flagship map cannot be rebuilt from a fresh clone**. A disk failure or a clean checkout loses them. Verified: `git ls-files \| grep -cE '\.uasset$\|\.umap$'` = 661 vs `find Content -name '*.uasset' -o -name '*.umap' \| wc -l` = 4,579. | Medium | **Critical** | **CRITICAL** | Decide the binary-asset strategy and write it down as an ADR: Git LFS, a separate asset repo, or a documented re-acquisition procedure per pack. Until then, at minimum: an off-machine backup of `Content/` and a `Docs/Production/` note listing every pack a fresh clone must re-download to open each map. Note what is *not* broken: our own 590 untracked assets are the regenerable `Content/RA4/Audio/Generated/` SoundWaves, whose 1,412 source WAV/MP3 files **are** tracked — that part of the ignore rule is deliberate and documented. |
| **RISK-21** | **Legal / IP** | **Seven of eight third-party packs have no recorded provenance** (12.3 GB total): CityPark, FactoryEnvironment, QuantumCharacter, IndustryPropsPack6, plus Brushify/Quixel/EpicGames stubs. Only `ambientCG` appears in `LEGAL_AND_LICENSES.md`. Unknown licence terms cannot be cleared by counsel, so this blocks any commercial build regardless of what the terms turn out to be. QuantumCharacter supplies the infantry skeleton and animations; CityPark supplies flagship-map art — neither is cosmetic-only. | High | **Critical** | **CRITICAL** | Per pack: recover the store URL and purchase record, archive the licence text under `Docs/Legal/`, and record a verdict in `LEGAL_AND_LICENSES.md` §2. Combined with RISK-20 this is urgent in one direction: the two untracked packs have no git history to recover terms from. |
| **RISK-22** | **QA / CI** | **The full-package commandlet cannot be used as a CI gate.** `-run=ResavePackages` over the project exits 1 with `Failure - 208 error(s), 360 warning(s)`; all 208 are `LogBlueprint`/`LogScript` compile errors inside `Content/ThirdParty/FactoryEnvironment/`, none referenced by RA4 maps. Nothing in-game is broken, but a permanently red exit code means a real content regression would be indistinguishable from the existing noise. | High | Medium | **HIGH** | Tracked as NEXT_ACTIONS V-6: fix or delete the offending pack content (check RISK-21 licence status first — deleting may be simplest if the pack is unused), then adopt the commandlet exit code as a release gate in `RELEASE_CRITERIA.md`. |
| **RISK-23** | **Process** | **Partial-output verification produced a false green.** Closing V-4 initially relied on grepping a log file while the commandlet was still writing it (3.4 MB read of a file that finished at 15.8 MB), and an earlier attempt "passed" because it was wrapped in `timeout`, which does not exist on macOS — the command never ran and the grep returned 0 from an empty pipe. Both were caught and corrected, but the same shape of error is what produces this repository's fabricated status claims. | High | Medium | **HIGH** | Rule for every verification from here: capture the command's exit status, confirm the process has exited before reading its output, and quote the summary line (`Failure - N error(s)`) rather than a grep count. Recorded in NEXT_ACTIONS V-4 as a caveat for the next agent. |
