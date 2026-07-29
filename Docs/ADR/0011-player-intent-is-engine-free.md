# ADR 0011 - Camera, selection and order resolution live outside Unreal

Status: accepted, implemented (`RA4Input`).

## Context

Selection rules and context-sensitive right-click are where an RTS is won or lost as
a piece of software. They are also, by default, the least testable code in the
project: a `PlayerController` that hit-tests actors, reads modifier keys and calls
into gameplay can only be exercised by launching PIE and clicking.

The behaviours involved are not incidental. A marquee that catches the player's four
tanks, their barracks and an enemy scout has exactly one correct outcome. A right
click on an enemy with a mixed selection of tanks and harvesters has to issue two
different orders. A cursor that shows a crosshair over a target the selection cannot
actually attack is a lie the player will act on.

## Decision

`RA4Input` holds the decision logic and takes no Unreal dependency:

* `CameraController` -- pan, edge scroll, zoom, bounds, middle-drag. Uses `float`,
  because the camera is presentation and never feeds back into simulation state.
* `SelectionModel` -- click disambiguation, marquee priority, double-click
  same-type, control groups, pruning of dead handles.
* `OrderResolver` -- turns a click plus modifiers plus hover target into the command
  list, and exposes the matching cursor hint through the *same* function family so
  the cursor cannot promise something the click will not do.

The `PlayerController` becomes an adapter: it does the projection and hit-testing
that genuinely needs the engine, hands the results to these classes, and forwards
the resulting commands.

## Consequences

The rules are covered by 30 headless tests that run in milliseconds, including the
ones that are tedious to reproduce by hand: alt-tabbing with the cursor at a screen
edge must not slide the camera, a control group must forget units that died, a
selection of pure harvesters must not show an attack cursor.

One test asserts the property that matters most at the boundary: every command the
resolver emits for a legal gesture is accepted by `SimWorld::ApplyCommand`. The
client cannot generate an order the server will reject, which is what keeps
"my click did nothing" from becoming a class of bug.

The cost is a hit-test seam: the caller supplies candidate entities rather than the
resolver querying the world for them. That is the correct split anyway, since
projection depends on the viewport and the fog of war filter.

Selection is deliberately capped (`kMaxSelectedEntities`) below the server's
per-tick command budget, so a large group order cannot be silently truncated by rate
limiting after the fact.
