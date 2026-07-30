# ADR-007: Multi-Tier Economy & Faction Resources

## Status
Accepted

## Context
Each faction in Red Alert 4 features distinct economic mechanics alongside global credits and power.

## Decision
1. **Shared Economy**: Credits, Power, and Command Limit are tracked per player state in `RA4Simulation`.
2. **Unique Faction Resources**:
   - `Soviet`: Mobilization (accrues from damage taken).
   - `Alliance`: Intelligence (gained via radar surveillance and recon).
   - `Eastern Coalition`: Synchronization (gained via formation combat).
   - `Chrono Legion`: Temporal Stability (regenerates passively over time).
