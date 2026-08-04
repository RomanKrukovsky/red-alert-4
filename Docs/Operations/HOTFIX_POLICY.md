# Hotfix & Emergency Patch Policy (`HOTFIX_POLICY.md`)

**Document Version**: 11.0  
**Status**: **ACTIVE HOTFIX GOVERNANCE**  

---

## 1. Hotfix Qualification Criteria

An emergency hotfix is authorized **ONLY** when a production incident reaches Severity **S1 (Critical)** or **S2 (Major)**. Minor visual bugs, balance tweaks, or cosmetic requests are deferred to regular scheduled patches.

---

## 2. Hotfix Workflow Specification

1. **Branch Isolation**: Create branch `hotfix/1.0.X` directly from the current production release tag (`v1.0.0-gold-master`).
2. **Regression Test Mandate**: Every hotfix branch must include a new automated C++ unit test in `Source/RA4Tests/` that fails before the fix and passes after.
3. **Targeted Review**: Requires dual code review by Lead Architect and QA Lead.
4. **CI/CD Gate**: Full GitHub Actions pipeline (`.github/workflows/core.yml`) must pass 100% (395/395 unit tests).
5. **Tagging & Staging**: Tag `v1.0.X` applied, deployed first to 5% canary servers, then rolled out globally upon 1 hour of clean telemetry.
