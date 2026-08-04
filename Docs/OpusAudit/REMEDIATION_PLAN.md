# Opus Audit — Remediation Plan

**Auditor**: Claude Fable 5
**Date**: 2026-08-04
**Baseline**: `cae7b36`

---

## Tier 1 — CRITICAL (before any release)

| # | Issue | Effort | Status |
|---|-------|--------|--------|
| 1.1 | Rotate exposed API keys (.env has OPENROUTER_API_KEY) | 30 min | BLOCKED — needs user |
| 1.2 | Remove .env from git history | 2 hours | BLOCKED — needs user |
| 1.3 | Add LICENSE file | 1 hour | BLOCKED — needs user decision |
| 1.4 | Audit ThirdParty content licenses | 1 day | PENDING |
| 1.5 | Remove Brushify if no commercial license | 2 hours | PENDING |
| 1.6 | Rename "RedAlert4" to original IP | 1 day | PENDING |

## Tier 2 — HIGH (before Alpha)

| # | Issue | Effort | Status |
|---|-------|--------|--------|
| 2.1 | Create packaged Shipping build pipeline | 2 days | PENDING |
| 2.2 | Verify UE compilation (all 15 modules) | 1 day | PENDING |
| 2.3 | Visual verification of PIE | 2 days | BLOCKED — needs UE editor |
| 2.4 | Add 500/1000/2000 entity stress tests | 2 days | PENDING |
| 2.5 | Fix duplicate FogOfWar link in CMakeLists.txt | 15 min | PENDING |

## Tier 3 — MEDIUM (before Beta)

| # | Issue | Effort |
|---|-------|--------|
| 3.1 | Implement reconnection | 3 days |
| 3.2 | Implement LAN lobby UI | 3 days |
| 3.3 | Add packet loss/jitter tests | 2 days |
| 3.4 | Create campaign missions (3+) | 1 week |
| 3.5 | Add performance benchmarks | 2 days |
| 3.6 | Naval and air unit mechanics | 1 week |
| 3.7 | Tutorial system | 3 days |
| 3.8 | Localization pipeline | 3 days |

## Tier 4 — LOW (before RC)

| # | Issue | Effort |
|---|-------|--------|
| 4.1 | Superweapon abilities | 3 days |
| 4.2 | Save/load migration tests | 2 days |
| 4.3 | Crash reporting | 2 days |
| 4.4 | Telemetry | 3 days |
| 4.5 | Map editor | 2 weeks |
| 4.6 | Spectator mode | 3 days |

## External Dependencies

| Item | Required From |
|------|--------------|
| Production-quality faction art | 3D artists |
| Voice acting | Voice actors |
| Original music | Composer |
| Sound effects | Audio designer |
| Platform certification | Platform accounts |
| Mass playtesting | Human testers |
| Legal review of IP migration | Lawyer |
