# Project State Master Reference

**Last Updated**: 2026-08-05
**Current Phase**: Audit & Foundation Remediation
**Branch**: `feat/soviet-asset-integration`
**HEAD**: `1fe9f58`

---

## 1. Project Overview

RA4 is a deterministic real-time strategy game engine built on a pure C++ simulation kernel with an Unreal Engine 5 presentation layer.

### Verified Metrics

| Metric | Value | Evidence |
|--------|-------|----------|
| C++ Tests | 308 pass, 0 fail | `ctest` on HEAD `1fe9f58` |
| Test Suites | 4 (core, input, presentation, ai) | CMakeLists.txt |
| C++ Modules | 15 (excluding RA4Tests) | .Build.cs files |
| ADRs | 23 | Docs/Architecture/ADR/ + Docs/ADRs/ |
| Content Files | 5934 total, 4531 uassets | `find Content/ -type f` |
| Maps | 8 RA4 maps + 9 ThirdParty | `find Content/ -name "*.umap"` |
| Campaign Missions | 38 defined in data | TestMissionRuntime.cpp |
| Factions | 4 (Soviet, Alliance, Coalition, Chronolegion) | Content database |
| Units | 78 unique types | ra4_content.normalized.json |

---

## 2. Component Health Matrix

| Subsystem | Health | Status | Notes |
|-----------|--------|--------|-------|
| Fixed-point math | ✅ | ACCEPT | 48.16, 128-bit intermediate, no float in sim |
| Entity model (SoA) | ✅ | ACCEPT | Slot+generation, deterministic recycling |
| CommandBus | ✅ | ACCEPT | 16 command types, 14 rejection reasons, rate limiting |
| LockstepSession | ✅ | ACCEPT_WITH_FIXES | 13 tests, no reconnect/spectators tested |
| SimWorld | ✅ | ACCEPT | 13-system tick ordering, state checksum |
| Replay | ✅ | ACCEPT | Record/checksum/verify, corruption detection |
| Navigation | ✅ | ACCEPT_WITH_FIXES | FlowField+NavGrid+MNavRouter+ReservationGrid |
| Fog of War | ✅ | ACCEPT | Per-player, combat respects visibility |
| AI Commander | ✅ | ACCEPT | 40+ tests, AI-vs-AI acceptance |
| AIDirectors | ✅ | ACCEPT | 15 tests, economy/scouting/defence/offence/production |
| OpponentModel | ⚠️ | REWORK | Header-only, composition tracking stubbed |
| Economy | ✅ | ACCEPT | Harvester loop, finite fields, power degradation |
| Combat | ✅ | ACCEPT | Armor matrix, splash, turret tracking |
| Production | ✅ | ACCEPT | Pay→build→place, queue+spawn+rally, cancel refunds |
| Content database | ✅ | ACCEPT | JSON bible, validation, hash sensitivity |
| Save system | ✅ | ACCEPT | Mid-match save/restore preserves checksum |
| Campaign framework | ⚠️ | ACCEPT_WITH_FIXES | 21 tests, no authored missions |
| HUD/Sidebar | ✅ | ACCEPT | 22 HudSnapshot tests |
| UE integration | ❓ | UNVERIFIED | Cannot build/run UE in this environment |
| Packaged build | ❌ | MISSING | No Shipping configuration |
| Localization | ⚠️ | EXTERNAL_DEPENDENCY | en/ru dirs exist, generated content |
| Audio pipeline | ⚠️ | EXTERNAL_DEPENDENCY | WAV files exist, no voice actor recordings |
| CI/CD | ⚠️ | ACCEPT_WITH_FIXES | core.yml covers headless only |
| ThirdParty licensing | ❌ | EXTERNAL_DEPENDENCY | 77% marketplace assets, no license files |
| IP migration | ❌ | MISSING | "Red Alert 4" name throughout |

---

## 3. Audit Reports

| Document | Status |
|----------|--------|
| `Docs/OpusAudit/EXECUTIVE_VERDICT.md` | ✅ Updated |
| `Docs/OpusAudit/BUILD_AUDIT.md` | ✅ Updated |
| `Docs/OpusAudit/CLAIMS_VS_REALITY.md` | ✅ Updated |
| `Docs/OpusAudit/TEST_QUALITY_AUDIT.md` | ✅ Updated |
| `Docs/OpusAudit/ARCHITECTURE_AUDIT.md` | Exists |
| `Docs/OpusAudit/GAMEPLAY_AUDIT.md` | Exists |
| `Docs/OpusAudit/CONTENT_AUDIT.md` | Exists |
| `Docs/OpusAudit/MULTIPLAYER_AUDIT.md` | Exists |
| `Docs/OpusAudit/PERFORMANCE_AUDIT.md` | Exists |
| `Docs/OpusAudit/LICENSE_AUDIT.md` | Exists |
| `Docs/OpusAudit/REMEDIATION_PLAN.md` | Exists |

---

## 4. What Is Genuinely Good

1. **Industrial-grade simulation core**: Fixed-point, deterministic, SoA entities, 13-system tick, command bus with validation.
2. **308 behavioral regression tests**: Stress tests to 2000 entities. AI-vs-AI acceptance.
3. **23 ADRs**: Every major design decision documented with rationale.
4. **Clean headless build**: Compiles with -Werror, passes all tests in ~12s.

## 5. What Must Be Fixed

1. **UE integration verification** — Run in editor, confirm simulation drives visuals
2. **Packaged Shipping build** — No build script exists
3. **OpponentModel completion** — Header exists, .cpp stubbed
4. **ThirdParty licensing** — Legal blocker
5. **IP migration** — "Red Alert 4" name must change
6. **CI pipeline for UE** — Extend beyond headless
7. **Campaign content** — Framework exists, no authored missions
8. **Localization, audio, art** — External dependencies
