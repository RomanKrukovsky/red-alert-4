# ADR 0012 - Multiplayer scope is a LAN lobby, not matchmaking

> **SUPERSEDED — NOT AUTHORITATIVE (marked 2026-08-05).**
> This file lives in `Docs/ADR/`, a legacy ADR directory. The single authoritative ADR
> location is **`Docs/Architecture/ADR/`** (per CLAUDE.md and ADR-0011). This document is
> retained for history and audit trail only; do not cite it as a current decision, and do not
> add new ADRs here. If the subject below is still live, the current record is in
> `Docs/Architecture/ADR/`.
>
> ADR-0011 decided in principle that these files be marked superseded, but the marking was never
> applied — this banner performs it.

Status: accepted, not implemented.

## Context

`RA4Network` has a lockstep session, a command frame with a stable wire format, a
per-tick checksum and desync detection. What it does not have is any way for two
people to actually find each other and start a match. That gap is not one task: it
is a family of tasks whose size depends entirely on a product decision nobody had
made, so the decision was blocking work rather than the code.

The two ends of the range differ by months, not by weeks:

* two players typing an IP address at each other -- handshake, seed, frame
  exchange, drop handling;
* ranked matchmaking -- accounts, a queue, a rating, relay servers to punch
  through NAT, anti-cheat, and a hosting bill that starts before the game ships.

The whole point of the vertical slice (one faction, ten units, one map, two
players, working AI) is to find out whether the game feels good before content is
built on top of it. Anything in the multiplayer stack that does not serve that
question is being built on the strength of an assumption that the game is worth
matchmaking for -- and that assumption is exactly what the slice exists to test.

There is a second reason to care, specific to this project: two `SimWorld`s in one
process agreeing is not evidence of cross-platform determinism. A real Mac-versus-PC
match is. Networking two machines together is currently the only honest test we
have, because CI has never run (see
`Docs/research/warzone2100/02_DETERMINISM_AND_DESYNC_LESSONS.md`).

## Decision

Scope for the vertical slice is **a LAN lobby**: host discovery by broadcast, plus
a lobby screen with slots, colours, faction, map and readiness, on top of direct
connection.

In scope:

* handshake that carries a **protocol version and a content hash**, and refuses the
  connection on mismatch. Clients on different builds must fail to connect with a
  clear message. WZ2100 shipped desyncs caused by mods and mismatched builds that
  players reported as game bugs; a refused connection is a support ticket avoided;
* a start packet fixing the RNG seed, map, slot assignment and content hash, so
  every peer begins from an identical `SimWorld`;
* frame exchange with the existing `CommandFrame`, and the existing per-tick
  checksum compared as it already is;
* host discovery by UDP broadcast on the local subnet, and a lobby with slots,
  colours, faction, map choice and per-player readiness;
* **`PlayerLeft` as a command inside a frame**, never as a transport event. Whatever
  happens at the socket -- timeout, pause, host decision -- lives outside the
  simulation and ends with somebody putting `PlayerLeft` into frame N. This is the
  single most under-rated desync class in WZ2100's history: at least five separate
  fixes there trace back to a network fact changing simulation state at a different
  moment on different peers;
* pause on peer loss, with an explicit host decision to drop or wait.

Out of scope, deliberately:

* accounts, persistent identity, rating, matchmaking queue;
* relay servers and NAT traversal. Over the internet, port forwarding is the
  answer for now;
* anti-cheat beyond the content hash;
* reconnect into a match in progress. Pause and drop is the behaviour; rejoining a
  lockstep match means shipping a full state snapshot, which is the save system's
  problem and should be solved once, there.

## Consequences

Matchmaking, when it comes, is built **on top of** this rather than instead of it.
Nothing in the list above is discarded by adding a queue later: a matchmaker
ultimately produces the same start packet and hands over the same two endpoints.
The lockstep core does not get rewritten. That is the property that makes this
staging safe rather than merely cheap.

The cost accepted: playing over the internet needs port forwarding, which will
filter out casual testers. That is acceptable for a slice whose audience is us and
a handful of deliberate testers, and unacceptable the moment we want strangers
playing each other -- which is the trigger for revisiting this ADR.

Residual desync is treated as a design assumption, not as something to be
eliminated by care. WZ2100 still ships host auto-kick on sustained desync after
25 years of fixes. RA4 needs defined behaviour for the case where checksums
diverge mid-match, and needs it before players see it, not after.
