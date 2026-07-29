# LICENSE_AND_AI_USAGE_REPORT

Date: 2026-07-28
Status: **BLOCKED — no package is present on this machine.**

## Verification performed

| Check | Command | Result |
| --- | --- | --- |
| Project plugin directory | `ls Plugins` | does not exist |
| Third-party content | `ls Content` | only `AssetRegistry`, `Collections`, `Developers`, `RA4UI` |
| Epic vault cache | `ls ~/Library/Application Support/Epic/*Vault*` | no match |
| Staging projects | `ls ~/Documents/Unreal Projects` | `Countryballs`, `Level`, `Test` — none is an RTS template |
| Filesystem sweep | `find ~ -maxdepth 4 -iname "*RTS*Template*" -o -iname "*SciFi*RTS*" -o -iname "*Unit*Template*"` | **no results** |

None of the three named packages exists on this machine:

- RTS/RPG Unit Template — MassAI, Multiplayer, GAS
- RTS TEMPLATE 25
- Multiplayer Sci-fi RTS Template

## Why this is a hard stop, not a delay

The brief requires, before any content is processed, that each package's licence and
its **Allows usage with AI** flag be checked, and that a package which forbids
machine analysis must not be indexed or passed to a model. That check is impossible
to perform on files that do not exist, and the flag cannot be read from a Fab
listing without the purchasing account.

Proceeding without it would mean one of two failures, both unacceptable:

1. Writing inventories, class mappings and conflict matrices for packages never
   seen — fabricated documents that read as authoritative.
2. Downloading packages from somewhere other than the buyer's own Fab library —
   which is exactly the unlicensed-source path the project's asset policy forbids.

## What is required from the account owner

These steps need the Fab/Epic account and cannot be delegated:

1. In the Epic Games Launcher, **Fab Library → each of the three packages → Add to
   Project / Download**, targeting Unreal Engine **5.6** (the project's version).
   If a package does not offer a 5.6 build, record which versions it does offer —
   that is itself an integration risk that changes the plan.
2. For each package, open its Fab listing and record verbatim:
   - licence name and version (Fab Standard / Fab Professional / other),
   - the **Allows usage with AI** setting,
   - any redistribution or source-availability restriction,
   - seller, package version, and date of purchase.
3. Place each downloaded package in its **own staging project** on UE 5.6, not in
   this project — per the brief, and because a template that overwrites
   `DefaultEngine.ini` or the default GameMode must be allowed to do so somewhere
   harmless first.
4. Report the three "Allows usage with AI" values back here.

## Decision rule once the flags are known

| Flag | What I may do |
| --- | --- |
| AI usage **allowed** | Read the package's source, assets and configs; produce full inventory, class mapping, conflict matrix and migration manifest. |
| AI usage **forbidden** | I will **not** read or index the package. I will instead design the RA4 integration interfaces from public documentation only, and write an exact step-by-step manual procedure for you to execute in the Unreal Editor, with verification checkpoints I can then check the *results* of. |
| Flag unclear | Treated as forbidden until the seller clarifies in writing. |

## Documents deliberately not written in this pass

Writing these now would require inventing content about packages that have never
been observed:

`TEMPLATE_INVENTORY_MASSAI_GAS.md`, `TEMPLATE_INVENTORY_RTS25.md`,
`TEMPLATE_INVENTORY_SCIFI_RTS.md`, `CLASS_MAPPING.md`, `CONFLICT_MATRIX.md`,
`ASSET_MIGRATION_MANIFEST.csv`, `GAMEPLAY_TAG_MIGRATION.md`,
`PERFORMANCE_BASELINE.md` (no Insights capture has been taken),
`FINAL_ACCEPTANCE_REPORT.md`.

They are listed in `ADR_INDEX.md` as pending with their unblocking condition.
