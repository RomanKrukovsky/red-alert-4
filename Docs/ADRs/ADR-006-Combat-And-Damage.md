# ADR-006: Combat System, Damage Matrix & Veterancy

## Status
Accepted

## Context
RTS balance relies on rock-paper-scissors combat mechanics expressed through warhead classes against armor classes.

## Decision
1. **Damage Matrix**: 2D lookup table (`WarheadClass` $\times$ `ArmorClass`) stored as fixed-point per-mille multipliers.
2. **Veterancy Ranks**: Four ranks (`Recruit`, `Veteran`, `Elite`, `Heroic`) scaling damage output, max HP, and passive abilities.
