# ADR 0001 - The simulation uses fixed-point arithmetic and no engine types

> **SUPERSEDED — NOT AUTHORITATIVE (marked 2026-08-05).**
> This file lives in `Docs/ADR/`, a legacy ADR directory. The single authoritative ADR
> location is **`Docs/Architecture/ADR/`** (per CLAUDE.md and ADR-0011). This document is
> retained for history and audit trail only; do not cite it as a current decision, and do not
> add new ADRs here. If the subject below is still live, the current record is in
> `Docs/Architecture/ADR/`.
>
> ADR-0011 decided in principle that these files be marked superseded, but the marking was never
> applied — this banner performs it.

Status: accepted, implemented.

## Context

Lockstep networking, replays and desync detection all require that two machines
produce identical state from identical inputs. IEEE floating point does not give
that guarantee across compilers and architectures: FMA contraction, x87 excess
precision and per-platform `libm` implementations of `sin`/`sqrt` all differ.

## Decision

The simulation uses a 48.16 fixed-point type. Trigonometry is integer CORDIC and
square root is an integer restoring algorithm, both implemented in the project.
No simulation code calls `libm` or stores a `float`.

The simulation also takes no dependency on Unreal, so it can be stepped in a unit
test and on a headless server.

## Consequences

Positive: replays are a command stream rather than a state dump; desync detection is
a 64-bit comparison; the whole gameplay suite runs in 26 ms without an editor.

Negative: arithmetic is more verbose, and range must be managed explicitly. This is
not theoretical -- the first implementation overflowed `LengthSquared()` at a
separation of ~460 m, which was caught by a unit test on a full-map distance and
fixed by widening the multiply intermediate to 128 bits.

Presentation code is unaffected and uses `float` freely.
