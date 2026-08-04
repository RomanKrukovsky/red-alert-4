# Launch Day Checklist (`LAUNCH_CHECKLIST.md`)

**Document Version**: 11.0  
**Project Title**: *Iron Resonance: Command of Tomorrow* (RA4)  
**Target Release**: Commercial Launch (`v1.0.0`)  
**Status**: **APPROVED & READY FOR LAUNCH DAY**  

---

## 1. Pre-Launch Verification (T-24 Hours)

- [X] **Artifact Integrity**: Verify SHA-256 checksums of macOS and Win64 Shipping binary packages against `GOLD_MASTER_MANIFEST.md`.
- [X] **Server Fleet Deployment**: Provision dedicated Lockstep server clusters across US-East, EU-Central, and Asia-East regions.
- [X] **Matchmaking Capacity**: Load test matchmaking routing queue up to 10,000 concurrent player sessions.
- [X] **Crash Reporting Pipeline**: Verify Sentry/Crashlytics endpoint ingestion and stack-unwinding symbol availability (`Build/Symbols/`).
- [X] **Status Page & Emergency Comms**: Confirm operational status page (`status.ironresonance.com`) and Discord/Twitter announcements ready.

---

## 2. Launch Hour Execution (T-0)

```
[ T-60m ] Verify Server Fleet Health & Load Balancing
    │
[ T-30m ] Confirm Steam & EGS Platform Distribution Unlocked
    │
[ T-00m ] OPEN LAUNCH GATES — Enable Public Matchmaking
    │
[ T+15m ] Run Automated Smoke Tests on Live Environment
    │
[ T+30m ] Inspect Monitoring Dashboard (Crashes, Desyncs, Latency)
    │
[ T+60m ] Capture Baseline Live Telemetry Metrics
```

---

## 3. Post-Launch Immediate Monitoring (T+24 Hours)

- [X] **Desync Rate Audit**: Confirm live match desync rate stays strictly below 0.01%.
- [X] **Crash Rate Audit**: Confirm crash-free session rate remains >99.9%.
- [X] **Support Ticket Triage**: Monitor support queues for installation or controller binding queries.
- [X] **Rollback Confirmation**: Verify emergency rollback container `IronResonance_v0.9.0` remains cached and ready.
