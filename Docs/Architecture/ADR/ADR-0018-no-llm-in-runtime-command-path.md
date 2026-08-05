# ADR-0018: No LLM In The Runtime Command Path

**Status:** Accepted (defect identified in `ra4-ui`, remediation required before that prototype ships)
**Date:** 2026-08-05
**Decider:** AI Engineer / Lead Architect
**Related:** ADR-0002 (simulation is the single source of truth), `CLAUDE.md` architectural invariants

## Context

The hierarchical AI blueprint states the rule directly:

> «В runtime должны работать быстрые, детерминированные и локальные алгоритмы.»
> Fable/LLM use is dev-time only — designing strategies, generating test scenarios,
> analysing lost matches, finding behaviour bugs, authoring AI profiles.

While wiring the AI subsystems (Packages 4–5) an existing violation of this rule was
found in the web UI prototype.

`ra4-ui/src/adminConsoleService.ts` issues a network call to
`https://openrouter.ai/api/v1/chat/completions`, passes the current game state plus
the player's text, and parses the reply into gameplay commands:

```json
{ "commands": [ { "action": "ATTACK", "entityIds": [42, 43], "targetEntityId": 12 } ] }
```

Two separate problems:

1. **Determinism.** The core invariant is "одинаковый seed и одинаковый поток команд
   должны приводить к одинаковому состоянию." A hosted LLM is not a function of the
   simulation state: it is non-deterministic across calls, versions, and time, and it
   can fail or time out. Placing it in the command path means two peers replaying the
   same seed and the same inputs can diverge. That breaks lockstep (desync) and breaks
   replay (a recorded match no longer reproduces). It cannot appear in a networked or
   replayable build in any form.

2. **Runtime dependency and latency.** A match tick would depend on an external
   service being reachable, authenticated, and fast. The simulation is headless and
   engine-free precisely so it has no such dependencies.

A third, unrelated but co-located defect: the same line hardcodes a live OpenRouter
API key. That is tracked separately as a security item — it is a committed secret on a
public remote and requires rotation, not just deletion. It is out of scope for this ADR
beyond noting that the key must be rotated regardless of what happens to the code.

## Decision

**No LLM, and no other non-deterministic external service, may participate in producing
or validating gameplay commands at runtime.**

Specifically:

- The command path is `input → Command → CommandBus validation → SimWorld`. Every
  producer in that path must be local, deterministic, and integer-only where it affects
  simulation state.
- `RA4AI` remains the only opponent brain in runtime: utility scoring, directors,
  opponent modelling, battle forecasting — all deterministic and engine-free, all
  emitting ordinary validated `Command` objects. It cannot cheat by construction
  because there is no path into simulation state except `ApplyCommand`.
- LLM assistance is permitted **only** outside the match loop, where its output is
  reviewed data rather than live state. Acceptable uses:
  - authoring AI profiles / doctrines that are then committed as content;
  - generating test scenarios and map layouts offline;
  - analysing telemetry, replays, and lost matches;
  - proposing balance or scoring-function changes for human review.
- If a natural-language command feature is wanted for players, it must be implemented
  as an **offline / out-of-band translator**: text is turned into a concrete `Command`
  list *before* it enters the tick, the result is validated by `CommandBus` like any
  other input, and the recorded command stream — never the prompt or the model reply —
  is what the replay contains. Any such feature must be disabled in lockstep multiplayer
  unless the translation is performed once by an authority and distributed as commands.

## Consequences

- `ra4-ui/src/adminConsoleService.ts` as written cannot ship in a build that supports
  multiplayer or replay. It is a prototype and must either be removed, or reduced to a
  dev-only tool that never runs during a match.
- The hardcoded key must be rotated at the provider. Removing the line does not
  un-expose a secret that was pushed to a public remote.
- Client-side LLM calls also expose whatever key they use to every user by design,
  which is an independent reason not to place them in shipped client code.
- No change is required in `RA4AI`: it already satisfies this rule, and the Package 4
  wiring was verified to keep the vertical-slice checksum unchanged
  (`3d34b67647f82b75`), confirming the AI additions introduced no non-determinism.

## Alternatives Considered

- **Allow the LLM but cache/seed its replies.** Rejected: a cache does not make the
  first call deterministic, and a cache miss mid-match still diverges peers.
- **Allow it in single-player only.** Rejected as a default: replay is a product
  requirement for all modes, and a mode-specific exception to the command rule is
  exactly the kind of parallel path `CLAUDE.md` forbids. It may be revisited only if
  replay is explicitly out of scope for that mode.
- **Server-side LLM with authoritative distribution.** Deferred, not rejected: it is
  the only shape that could be made lockstep-safe, since the authority would translate
  once and distribute concrete commands. It is out of scope until there is a product
  need.
