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
| **RISK-02** | **Legal / IP** | **Native UI font provenance**: CommonUI/UMG/Slate loads the bundled `RA4_RobotoCondensedRegular` and `RA4_RobotoCondensedSemiBold` font assets, but their commercial-use provenance is not recorded in the repository. | Medium | High | **HIGH** | Obtain and record commercial-use clearance for the font files under `Content/RA4UI/Fonts/` before release, or replace them with fonts whose license is documented in the project license inventory. |
| **RISK-03** | **Legal / IP** | **Non-Commercial Third-Party 3D Assets**: Sketchfab / external 3D models with `CC-BY-NC` or `Editorial Use Only` licenses. | Medium | Medium | **MEDIUM** | Audit all imported 3D mesh assets in `Content/ThirdParty/` and replace non-commercial licenses with in-house or CC0 assets. |

---

## 2. Architecture & Engine Integration Risks

| Risk ID | Risk Category | Risk Description | Probability | Impact | Severity Rating | Mitigation Strategy |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **RISK-04** | **Engine / Build** | **Native UI build integration**: CommonUI/UMG/Slate assets and code must remain covered by the standard Unreal build. | Medium | High | **HIGH** | Keep the native CommonUI + UMG + Slate stack in the Unreal targets and verify it with the UI validation/build gates. |
| **RISK-05** | **Architecture** | **Presentation Polling Overhead**: `URA4PresentationSubsystem` polls all simulation entity positions every frame during high entity counts (>1000). | Low | High | **MEDIUM** | Implement a dirty entity queue or delta-event push mechanism from `SimWorld` to presentation subsystem. |
| **RISK-06** | **Determinism** | **Unreal Engine Float Non-Determinism in Presentation**: Visual actors mutating simulation state via physics overlap or float precision drift. | Low | Critical | **HIGH** | Maintain strict architectural boundary: Presentation layer only reads from simulation snapshots; never mutates `SimWorld`. |

---

## 3. UI Technology & Production Risks

| Risk ID | Risk Category | Risk Description | Probability | Impact | Severity Rating | Mitigation Strategy |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **RISK-07** | **UI / Tech Debt** | **Legacy web UI prototype**: The abandoned `ra4-ui` React/Vite prototype could be mistaken for a supported production client. | Low | Low | **LOW** | **Resolved 2026-08-25**: removed `ra4-ui/` and its obsolete implementation brief; CommonUI + UMG + Slate is the only supported production UI stack. |
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
