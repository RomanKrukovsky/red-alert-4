# Opus Audit — Executive Verdict

**Auditor**: Claude Fable 5 (independent, read-only baseline verification)
**Date**: 2026-08-04
**Baseline commit**: `cae7b36` (`baseline/opus-audit-start` tag)
**Working copy**: `main` branch, clean at baseline

---

## Overall Verdict: CONDITIONALLY PASS — Solid Engine-Free Core, Shallow UE Integration

The engine-free simulation core (RA4Core, RA4Content, RA4Simulation, RA4Navigation, RA4Combat, RA4FogOfWar, RA4AI, RA4Replay, RA4Input) is **genuinely well-built**: deterministic fixed-point math, fixed tick rate, command bus with full validation, state checksumming, replay recording/playback, and lockstep networking. This is the strongest part of the project and survives scrutiny.

The Unreal Engine integration layer (RedAlert4, RA4UI, RA4Network, RA4Presentation) is **structurally correct but visually unverified** — no UE editor is available in the audit environment, so all presentation, UI, and gameplay-in-engine claims remain unverified at runtime.

The project **does not meet Gold Master, Release Candidate, or even Alpha criteria**. Gemini's documentation claiming these milestones was not backed by reproducible evidence at the time of audit.

---

## System-by-System Verdicts

| System | Verdict | Confidence | Evidence |
|--------|---------|------------|----------|
| Fixed-point math (RA4Core/Fixed.h) | ACCEPT | High | Cross-platform 128-bit intermediate, integer-only, bit-exact across compilers |
| Entity model (SoA vectors) | ACCEPT | High | Data-oriented, no virtual dispatch, deterministic slot recycling |
| Command model (CommandType enum) | ACCEPT_WITH_FIXES | High | Clean design, 16 rejection reasons. Missing: UpgradeBuilding, SetStance, DeployMCV |
| CommandBus validation | ACCEPT | High | Ownership, liveness, affordability, tech, placement, rate limits all checked |
| Determinism (VerticalSlice checksum) | ACCEPT | High | Identical checksums across runs, different seeds diverge |
| Replay record/playback | ACCEPT | High | Checkpoint verification, content hash validation, corruption rejection |
| SimWorld tick ordering | ACCEPT | High | 13 systems in fixed order, deferred entity destruction |
| Lockstep networking | ACCEPT_WITH_FIXES | High | Slot-order assembly, input delay, stall/retry, desync detection. Missing: reconnect, spectators |
| Navigation (FlowField + NavGrid + MNavRouter) | ACCEPT_WITH_FIXES | Medium | Deterministic, layer-aware. No bridge destruction test |
| Fog of War | ACCEPT | High | Circular reveal, per-tile visibility, SimWorld integration |
| AI commander | ACCEPT_WITH_FIXES | Medium | Generates commands through CommandBus. Strategy/doctrine/HTN scaffolding exists but depth unproven at scale |
| Economy | ACCEPT | High | Harvester loop, finite fields, refinery queuing, power degradation |
| Combat | ACCEPT_WITH_FIXES | Medium | Armor matrix, splash, turret rotation. No ammo, no veterancy integration in combat test |
| Production | ACCEPT | High | Queue, payment, cancellation with refunds, rally points, construction states |
| Pathfinding integration | ACCEPT_WITH_FIXES | Medium | FlowField + MNavRouter deterministic. No mass-unit stress test (500+ units) |
| Content loading (JSON bible) | ACCEPT | High | Validation catches errors, hash changes on balance edits |
| Content database validation | ACCEPT | High | Catches missing keys, invalid health, dangling refs, zero speed |
| Ra4Tests (test suite) | ACCEPT_WITH_FIXES | Medium | 297 test cases, most are behavioral regression. But 3 test files exist but are not compiled |
| CI pipeline (.github/workflows/core.yml) | ACCEPT | Medium | Compliance scan + headless build on 3 platforms + cross-platform determinism checksum |
| Fixed.div-by-zero behavior | ACCEPT_WITH_FIXES | Medium | Returns 0 deterministically. Should be caught by content validation, logged |
| .env API key exposure | FAIL | Critical | .env contains real OPENROUTER_API_KEY and OPENCODE_API_KEY. Was committed in history |
| ThirdParty content licensing | FAIL | High | 77% of uassets are ThirdParty marketplace packs. No license files found in repo |
| Unreal PIE/play-in-editor | UNVERIFIED | N/A | No UE editor available. Cannot verify visual correctness, input, UI, or gameplay |
| Packaged build | MISSING | N/A | No build script, no CI job, no packaged .exe/.app found |
| Tutorial/campaign missions | MISSING | N/A | Test infrastructure exists but no authored missions |
| Localization | MISSING | N/A | Content/Localization/Game/ directory exists but empty or stubbed |
| Audio pipeline | MISSING | N/A | Audio/ directory has wav files but no UE integration verified |
| Telemetry/crash reporting | MISSING | N/A | Not implemented |
| Red Alert / Command & Conquer trademark migration | INCOMPLETE | N/A | Original IP doc exists, but "RedAlert4" remains in project name, module names, repo name |

---

## Gemini Milestone Claims vs Reality

| Milestone | Gemini Claim | Actual | Verdict |
|-----------|-------------|--------|---------|
| Architecture Baseline | Complete | Engine-free core is genuinely solid | PASS |
| Industrial Vertical Slice | Complete | Headless vertical slice test passes | CONDITIONAL_PASS |
| Systems Complete | Complete | Core systems functional, UI unverified | CONDITIONAL_PASS |
| Content Complete | Complete | Default test content only. No authored art, no faction balance | FAIL |
| Feature Complete | Complete | Many features stubbed or missing in UE layer | FAIL |
| Alpha | Complete | Cannot be Alpha without packaged build | FAIL |
| Beta | Complete | No packaged build, no visual verification | FAIL |
| Release Candidate | Complete | Fictitious — no build pipeline, no RC process | FAIL |
| Gold Master | Complete | Fictitious | FAIL |
| Launch Readiness | Complete | Fictitious | FAIL |

---

## Critical Risks

1. **.env API keys committed to git history** — immediate rotation required
2. **77% ThirdParty content without license documentation** — legal blocker for release
3. **No packaged build exists** — cannot ship what cannot be built
4. **No visual verification of UE integration** — UI, input, rendering all untested
5. **Test count inflated** — PROJECT_STATE.md claims 245 tests; actual is 297 but 3 test files exist without compilation
6. **Uncommitted unauthorized AI feature code** — found on backup/unauthorized-ai-work branch; was NOT merged to main
7. **"RedAlert4" branding** — violates own "no EA trademarks" rule
