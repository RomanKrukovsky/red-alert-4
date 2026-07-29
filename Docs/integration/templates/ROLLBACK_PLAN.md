# ROLLBACK_PLAN

## Baseline artefacts

| Artefact | Location | Covers |
| --- | --- | --- |
| Git tag | `baseline-pre-template-integration` → commit `7f9f9e9` | committed state only |
| Filesystem archive | `~/Documents/ra4-backups/ra4-pre-template-integration-20260728-212529.tar.gz` (207 MB, 574 files) | **entire working tree, including the 83 uncommitted files the tag does not capture** |

Excluded from the archive by design: `build/`, `.venv/`, `Binaries/`, `Intermediate/`,
`DerivedDataCache/`, `Saved/`, `.git/` — all regenerable.

## Rollback of committed work

```bash
git checkout baseline-pre-template-integration
```

Restores tracked files to `7f9f9e9`. **Does not restore the 83 uncommitted files** —
use the archive for those.

## Rollback including uncommitted work

```bash
# from an empty directory, NOT over a live tree
tar -xzf ~/Documents/ra4-backups/ra4-pre-template-integration-20260728-212529.tar.gz
```

Extract to a scratch directory and copy back selectively. Never extract over a working
tree another session is editing.

## Rollback of engine build artefacts

Deleting `Binaries/`, `Intermediate/` and `DerivedDataCache/` and rebuilding is always
safe and is the first step when UBT, UHT or the Blueprint compiler behaves
inconsistently after a rollback.

```bash
rm -rf Binaries Intermediate
"/Users/Shared/Epic Games/UE_5.6/Engine/Build/BatchFiles/Mac/Build.sh" \
  RedAlert4Editor Mac Development -project="$PWD/RedAlert4.uproject"
```

## Verification after any rollback

Run all four; a rollback is not complete until each passes.

```bash
cmake -S Tools/HeadlessBuild -B build/hb && cmake --build build/hb -j8   # 1. core compiles
./build/hb/RA4Tests                                                       # 2. tests
Build.sh RedAlert4Editor Mac Development -project=.../RedAlert4.uproject  # 3. UE compiles
git status --short                                                        # 4. tree as expected
```

Record the outcome in `INTEGRATION_LOG.md`. Note that check 2 currently reports
**98 passed / 5 failed** at baseline (see `KNOWN_ISSUES.md` K-1) — that is the
expected baseline result, not a rollback failure.

## Per-stage checkpoints (once integration can start)

Each integration branch gets its own tag before the first asset lands:
`checkpoint/mass-gas-pre`, `checkpoint/rts25-pre`, `checkpoint/scifi-rts-pre`, and so
on. Asset migration additionally requires a pre-migration Asset Report, because
redirectors and broken soft references are not visible in `git status`.
