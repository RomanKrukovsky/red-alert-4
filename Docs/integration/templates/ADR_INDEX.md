# ADR_INDEX

> **STALE AND DANGEROUS — DO NOT USE FOR NUMBERING (marked 2026-08-05).**
>
> This index tracks the **legacy** `Docs/ADR/` series (now superseded — see ADR-0011). The
> authoritative ADR directory is **`Docs/Architecture/ADR/`**.
>
> Critically, the "Required before template integration" table below reserves ADR numbers
> **0012–0021, every one of which is already taken** in the canonical series by an unrelated
> decision:
>
> | Reserved here | Actually occupied on `main` by |
> | --- | --- |
> | 0012 | Flow payment state machine |
> | 0013 | Energy system deficit effects |
> | 0014 | Soft command limit penalty |
> | 0015 | Harvester state machine / fleet |
> | 0016 | Resource depletion & regeneration |
> | 0017 | Expansion investment payback |
> | 0018 | No LLM in the runtime command path |
> | 0019 | Chronolegion temporal debt |
> | 0020 | Economic telemetry & balance metrics |
> | 0021 | Knowledge Map / intel decay |
>
> An agent that allocated a number from the table below would create a collision. The next free
> number in the canonical series is **ADR-0028** (0027 is Faction Economy Extension Points).
>
> The *questions* in that table are still legitimate open architecture decisions — they simply need
> fresh numbers when written. Treat the table as a to-do list of topics, never as an allocation.

## Accepted

| ADR | Title | Status |
| --- | --- | --- |
| 0001 | Deterministic fixed-point core | accepted, implemented |
| 0002 | Commands are the only mutation path | accepted, implemented |
| 0003 | Hybrid entity representation | accepted, simulation side implemented |
| 0004 | Content lives in data, not code | accepted, struct model implemented |
| 0005 | Authoritative server, network protocol, resync | accepted, not implemented |
| 0006 | Hierarchical navigation, topology versioning | accepted, implemented |
| 0007 | Save system, authoritative state serialization | accepted, not implemented |
| 0008 | Damage matrix and content hot-reload validation | accepted |
| 0009 | Simulation/presentation lifecycle, world origin | accepted |
| 0010 | Production build pipeline and stripping | accepted |
| 0011 | Player intent is engine-free | accepted, implemented |

## Open architecture questions (topics only — numbers below are INVALID, see banner)

| ADR | Decision needed | Blocked by |
| --- | --- | --- |
| 0012 | Single canonical `ARA4GameMode`; fate of `ARA4UIShowcaseGameMode` | nothing — can be written now |
| 0013 | Single canonical HUD: `ARA4HUD` (CommonUI) vs `ARA4RtsHud` (debug draw) | nothing |
| 0014 | EnhancedInput adoption or removal of the dead dependency | nothing |
| 0015 | Introduce `ARA4GameState`, `ARA4PlayerState`, `URA4GameInstance`, `URA4LocalPlayerSubsystem` | nothing |
| 0016 | Extract economy/production/construction/combat from `RA4Simulation` into their own modules | nothing |
| 0017 | Mass vs own data-oriented storage for massed units | package inventory |
| 0018 | GAS adoption scope — abilities only, never economy or basic attacks | package inventory |
| 0019 | Selection/command ownership: incumbent `RA4Input` vs Sci-fi RTS template | package inventory |
| 0020 | Economy/production/UI ownership: incumbent vs RTS Template 25 | package inventory |
| 0021 | Networking model: incumbent command pipeline vs template replication | package inventory |

The first five topics above are unblocked and address conflicts that **already exist in the tree**.
They should be settled before any template arrives, so that template systems land in slots with a
single, documented incumbent. When writing them, allocate numbers from ADR-0028 upward in
`Docs/Architecture/ADR/` — the 0012–0021 numbers in the table are already in use and must not be
reused.
