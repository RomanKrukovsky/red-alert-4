# ADR-0011: ADR Series Consolidation

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
