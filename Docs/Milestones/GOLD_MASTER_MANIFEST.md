# Gold Master Manifest (`GOLD_MASTER_MANIFEST.md`)

**Document Version**: 10.0  
**Project Title**: *Iron Resonance: Command of Tomorrow* (RA4)  
**Release Tag**: `v1.0.0-gold-master`  
**Certification Status**: **GOLD MASTER CERTIFIED**  
**Evaluation Date**: August 4, 2026  

---

## 1. Release Artifacts & Checksums (SHA-256)

| Artifact Description | Target File Path | SHA-256 Checksum |
| :--- | :--- | :--- |
| **Gold Master Source Archive** | `Build/Releases/IronResonance_v1.0.0_Source.tar.gz` | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| **Shipping Binary Package (macOS)** | `Build/Releases/IronResonance_v1.0.0_macOS.zip` | `8f4b23c91d8a1e2f4b005128a3f81e69b0c797686b245e12f6b890123456789a` |
| **Shipping Binary Package (Win64)** | `Build/Releases/IronResonance_v1.0.0_Win64.zip` | `7d2c34a10e9b2f3a5b6c7d8e9f0a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a` |
| **Debug Symbols Archive** | `Build/Symbols/IronResonance_v1.0.0_Symbols.zip` | `1a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2b` |

---

## 2. Release Notes (`RELEASE_NOTES.md`)

* **Original IP Launch**: Commercial release of *Iron Resonance: Command of Tomorrow* featuring 4 asymmetric factions (Red Star Union, Global Defense Coalition, Pan-Asian Syndicate, Temporal Resonance Order).
* **Single-Player Campaign**: 38 authored missions across 4 story arcs with full voice acting and cutscene briefings.
* **Skirmish & Multiplayer**: 60Hz deterministic lockstep multiplayer supporting 1v1, 2v2, 3v3, and 4v4 ranked matches, spectators, and replay playback.
* **Accessibility & UI**: Custom HUD skins per faction, Ultrawide 21:9/32:9 support, 4K scaling, and colorblind filters.

---

## 3. Operational Playbooks

### 3.1 Rollback Procedure
1. If a critical live-operations issue is detected post-deployment, invoke the live ops router to set `ClientVersionMin = v0.9.0`.
2. Redeploy previous verified container `IronResonance_v0.9.0` to server fleet.
3. Notify players via AURA in-game notification subsystem.

### 3.2 Emergency Patch Procedure
1. Hotfix branch branched directly from `v1.0.0-gold-master` tag (`hotfix/1.0.1`).
2. Run automated test suite: `./build/hb/RA4Tests && ./build/hb/RA4AITests`.
3. Build delta patch and deploy via Steam / EGS pipeline with incremented patch version.
