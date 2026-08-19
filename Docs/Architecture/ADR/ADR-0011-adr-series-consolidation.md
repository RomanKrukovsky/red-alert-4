# ADR-0011: ADR Series Consolidation

> **STATUS UPDATE (2026-08-05)**: this ADR's decisions were recorded but only partially executed.
> Corrections and completion:
> - **Executed now**: all 23 files in `Docs/ADR/` and `Docs/ADRs/` carry a SUPERSEDED banner
>   (decision 3, previously unimplemented — no file had any marker).
> - **Erratum to decision 4**: it states ADR-007 (Faction Economy) is superseded by "ADR-0019 (Faction
>   Economy Extension Points)". Wrong number. That document was authored as ADR-0018 on
>   `remediation/foundation-fixes` only, where it collided with `main`'s ADR-0018 ("No LLM In The Runtime
>   Command Path"); it has been recovered and renumbered to **ADR-0027**. ADR-0019 is Chronolegion
>   Temporal Debt.
> - **Erratum to the Context section**: the directory counts are stale. `Docs/Architecture/ADR/` now
>   holds 27 files, not 10.
> - **Numbering collision — RESOLVED 2026-08-20**: this file and `ADR-0011-DirectControl.md` shared the
>   number ADR-0011 in the canonical directory — the exact defect this ADR set out to prevent. The
>   DirectControl ADR is now **ADR-0029** (`ADR-0029-direct-control.md`), with a redirect stub left at the
>   old path. P-3 and this erratum both prescribed ADR-0028, but ADR-0028 was taken by the telemetry ADRs
>   before the rename was carried out — a reminder that a reserved-but-unclaimed number is not reserved.
>   Inbound citations were updated in `RA4DirectControlSubsystem.h`, `RA4DirectControlProfile.h`,
>   `Command.h` (which pointed at `ADR-DirectControl.md`, a path that never existed), ADR-0022 and
>   ADR-0026. **ADR-0011 now means only this document.** The next free canonical number is **ADR-0030**.
> - **Second collision, still open**: `ADR-0028-unified-event-telemetry-architecture.md` and
>   `ADR-0028-telemetry-implementation-plan.md` both carry 0028. These are not competing decisions — the
>   second is the implementation plan for the first and says so in its title — so this is a filename
>   problem, not a split authority. Left as found; renaming the plan would break its inbound citations
>   for no decision-level gain.
> - **Executed now**: decision 5 — `Docs/integration/templates/ADR_INDEX.md` carries a warning banner.
>   It was not merely stale: it reserved ADR numbers 0012–0021 for unwritten template decisions, and
>   every one of those numbers is already occupied in the canonical series. Any agent following it would
>   have created ten collisions. The next free canonical number is **ADR-0028**.

## Context

Three parallel ADR directories exist in the repository:
- `Docs/ADR/` — 12 ADRs (0001–0012), descriptive filenames, "accepted" status
- `Docs/ADRs/` — 11 ADRs (ADR-001 to ADR-011), includes economy ADR-007 with pre-rename faction names
- `Docs/Architecture/ADR/` — 10 ADRs (ADR-0001 to ADR-0010), Context→Decision→Rationale→Status format

CLAUDE.md declares `Docs/Architecture/ADR/` as the mandatory ADR location. The `ADR_INDEX.md` template in `Docs/integration/templates/` tracks the `Docs/ADR/` series, creating a split authority.

## Decision

1. **Canonical location**: `Docs/Architecture/ADR/` is the single authoritative ADR directory.
2. **Numbering**: New ADRs continue from ADR-0011 (after existing ADR-0010).
3. **Existing ADRs in other directories**: Mark as "superseded" with a redirect note pointing to `Docs/Architecture/ADR/`. Do not delete — preserve history.
4. **ADR-007 (Faction Economy)**: Superseded by ADR-0019 (Faction Economy Extension Points) which uses current faction names and covers the complete faction economy design.
5. **ADR_INDEX.md**: Update to track `Docs/Architecture/ADR/` series.

## Rationale

- Eliminates split authority and conflicting numbering.
- CLAUDE.md already declares `Docs/Architecture/ADR/` as mandatory.
- New economy ADRs (0012–0019) need a single canonical location.
- Preserving old ADRs maintains git history and audit trail.

## Status

**ACCEPTED**. Economy ADRs 0012–0019 will be written to `Docs/Architecture/ADR/`.
