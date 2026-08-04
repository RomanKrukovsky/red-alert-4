# ADR-0008: Utility HTN AI Commander for Deterministic Skirmish Bots

## Context
Non-deterministic Behavior Trees using random selectors or floating-point utility weights create desyncs in lockstep bot matches.

## Decision
Implement `AICommander` (`RA4AI`) using a deterministic Hierarchical Task Network (HTN) with integer fixed-point utility scoring.

## Rationale
- AI bots operate 100% deterministically over lockstep `CommandBus`.
- Verified by `AI.IsDeterministic` and `AI.FiveSkirmishScenariosFinishWithAWinner` (46/46 PASS).

## Status
**ACCEPTED / IMPLEMENTED**.
