# ADR-003: Command Protocol & Issuer Verification

## Status
Accepted

## Context
In lockstep multiplayer RTS games, client inputs must be packaged into deterministic commands, validated by ownership and sequence counters, and dispatched simultaneously across all connected simulation instances.

## Decision
1. **Command Bus**: Inputs are converted into `Command` structs containing `ExecutionFrame`, `SequenceIndex`, `PlayerIndex`, `Type`, and target coordinates / handles.
2. **Rejection & Validation**: `SimWorld` validates ownership (`Command.PlayerIndex == Entity.OwnerPlayer`), prerequisite tech tree state, and handle generation validity before applying orders.
