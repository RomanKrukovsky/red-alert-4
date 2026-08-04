# Scheduled Patch & Compatibility Policy (`PATCH_POLICY.md`)

**Document Version**: 11.0  
**Status**: **ACTIVE PATCH RELEASE GUIDELINES**  

---

## 1. Release Cadence & Versioning

* **Patch Release Frequency**: Scheduled minor patches released bi-weekly (e.g. `v1.1.0`, `v1.2.0`).
* **Semantic Versioning**: `MAJOR.MINOR.PATCH` (Major: breaking engine changes; Minor: content/balance updates; Patch: bug fixes).

---

## 2. Backward Compatibility Guarantees

* **Save Compatibility**: Mid-match save snapshots from minor version `v1.X` remain 100% loadable across `v1.X.Y` patch updates via `SaveMigration` data transformers.
* **Replay Compatibility**: Replays record full binary command streams. A major simulation logic change increments `ReplayFormatVersion`, isolating legacy replays for playback under legacy simulation binaries.
* **Mod Compatibility**: Modding schemas defined in `MODDING_AND_EDITOR_DESIGN.md` maintain backward compatibility across minor patch releases.
