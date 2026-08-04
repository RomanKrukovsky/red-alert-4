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
