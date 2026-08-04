# Opus Audit — Remediation Plan

**Auditor**: Claude Fable 5
**Date**: 2026-08-04
**Baseline**: `cae7b36`

---

## Priority Tiers

### TIER 1 — CRITICAL (must fix before any release)

| # | Issue | Effort | Impact |
|---|-------|--------|--------|
| 1.1 | Rotate exposed API keys (.env contains OPENROUTER_API_KEY, OPENCODE_API_KEY) | 30 min | Security |
| 1.2 | Remove .env from git history (git filter-branch or BFG) | 2 hours | Security |
| 1.3 | Add LICENSE file to repository | 1 hour | Legal |
| 1.4 | Audit ThirdParty content licenses (77.7% of assets) | 1 day | Legal |
| 1.5 | Remove Brushify if no commercial license | 2 hours | Legal |
| 1.6 | Rename "RedAlert4" to original IP throughout codebase | 1 day | Legal |

### TIER 2 — HIGH (must fix before Alpha)

| # | Issue | Effort | Impact |
|---|-------|--------|--------|
| 2.1 | Create packaged Shipping build pipeline | 2 days | Release infrastructure |
| 2.2 | Verify UE compilation (all 15 modules) | 1 day | Build integrity |
| 2.3 | Visual verification of PIE (main menu, skirmish setup, match) | 2 days | Gameplay |
| 2.4 | Add 500/1000/2000 entity stress tests | 2 days | Performance validation |
| 2.5 | Fix duplicate FogOfWar link in CMakeLists.txt | 15 min | Build hygiene |
| 2.6 | Remove 3 orphaned AI implementation files from backup branch | 30 min | Code hygiene |

### TIER 3 — MEDIUM (must fix before Beta)

| # | Issue | Effort | Impact |
|---|-------|--------|--------|
| 3.1 | Implement reconnection after disconnect | 3 days | Multiplayer |
| 3.2 | Implement LAN lobby UI | 3 days | Multiplayer |
| 3.3 | Add packet loss/jitter simulation to network tests | 2 days | Network robustness |
| 3.4 | Create actual campaign missions (at least 3) | 1 week | Content |
| 3.5 | Add performance benchmark tests (tick time, memory) | 2 days | Performance |
| 3.6 | Implement naval and air unit mechanics | 1 week | Gameplay |
| 3.7 | Add tutorial system | 3 days | Onboarding |
| 3.8 | Implement localization pipeline | 3 days | International |

### TIER 4 — LOW (must fix before RC)

| # | Issue | Effort | Impact |
|---|-------|--------|--------|
| 4.1 | Add superweapon abilities | 3 days | Gameplay |
| 4.2 | Implement save/load migration testing | 2 days | Data integrity |
| 4.3 | Add crash reporting | 2 days | Stability |
| 4.4 | Add telemetry | 3 days | Analytics |
| 4.5 | Create map editor | 2 weeks | Tools |
| 4.6 | Implement spectator mode | 3 days | Multiplayer |

---

## Recommended Execution Order

### Phase 1: Security & Legal (Days 1-2)
1. Rotate API keys
2. Clean git history
3. Add LICENSE file
4. Begin ThirdParty license audit

### Phase 2: Build Verification (Days 3-5)
1. Verify UE compilation of all modules
2. Fix CMakeLists.txt duplicate link
3. Create packaged build script
4. Visual verification of core gameplay loop

### Phase 3: Core Gameplay Polish (Days 6-10)
1. Add stress tests (500/1000/2000 entities)
2. Add performance benchmarks
3. Verify determinism under load
4. Fix any gameplay bugs found

### Phase 4: Network Hardening (Days 11-15)
1. Add packet loss/jitter simulation
2. Implement reconnection
3. Build LAN lobby UI
4. End-to-end multiplayer testing

### Phase 5: Content & IP (Days 16-25)
1. Execute IP migration (rename RedAlert4 → original name)
2. Remove/replace unlicensed ThirdParty content
3. Create placeholder art for missing faction units
4. Implement 3 campaign missions
5. Add tutorial

### Phase 6: Polish & Release Prep (Days 26-35)
1. Add superweapon abilities
2. Implement naval/air mechanics
3. Localization pipeline
4. Crash reporting
5. Final performance optimization

---

## Blocked Items (require external resources)

| Item | Dependency |
|------|-----------|
| Production-quality faction art | 3D artists |
| Voice acting for EVA lines | Voice actors |
| Original music score | Composer |
| Sound effects | Audio designer |
| Platform certification (Steam, etc.) | Platform accounts |
| Mass playtesting (Alpha/Beta) | Human testers |
| Legal review of IP migration | Lawyer |
| ESRB/PEGI rating | Rating board |
