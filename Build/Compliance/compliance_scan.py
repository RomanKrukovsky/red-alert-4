#!/usr/bin/env python3
"""Blocking compliance scanner.

Referenced by .github/workflows/core.yml since the workflow was written, but the
script itself was never committed -- only a two-file test corpus. Every CI run
therefore failed this job on a clean checkout with "No such file or directory",
which is the same class of defect the project keeps hitting: a green-looking
process that never actually ran.

Two checks, both chosen because they catch failures that have really happened in
this repository rather than hypothetical ones:

1. Asset provenance. Third-party binary assets must be declared in
   LEGAL_AND_LICENSES.md. Seven of eight packs (12.3 GB) were found undocumented
   during the 2026-08-06 audit (RISK-21), and an unverified licence cannot be
   cleared by counsel, so this blocks a commercial build regardless of what the
   terms turn out to be.

2. Commit scope. A commit that DELETES code outside its own stated subject is
   the mechanism behind five separate losses of finished work in one day: a
   "fix(build): cook to disk" commit removed 522 lines of gameplay code across
   seven files, a "fix(ui): sidebar width" commit removed 165 lines of QA tests,
   and a "perf(sim)" commit dropped an accessor three tests depended on. None of
   that is visible from the commit message, which is exactly why it needs a
   machine check.

Exit code 0 means clean, 1 means a blocking violation. Usage:

    python3 Build/Compliance/compliance_scan.py --scope both
    python3 Build/Compliance/compliance_scan.py --scope provenance --root <dir>
    python3 Build/Compliance/compliance_scan.py --scope commit-scope --base <ref>
"""
import argparse
import os
import re
import subprocess
import sys

# Binary extensions that carry licence obligations. Source files are excluded:
# provenance for code is handled by review, not by this scan.
ASSET_EXTENSIONS = {
    ".uasset", ".umap", ".fbx", ".obj", ".blend",
    ".png", ".jpg", ".jpeg", ".tga", ".exr", ".hdr",
    ".wav", ".mp3", ".ogg", ".flac",
    ".ttf", ".otf", ".woff", ".woff2",
}

LEGAL_DOC = os.path.join("Docs", "Production", "LEGAL_AND_LICENSES.md")

# Directories whose contents are first-party by definition, so they need no
# third-party provenance entry.
FIRST_PARTY_PREFIXES = (
    os.path.join("Content", "RA4"),
    os.path.join("Content", "Maps"),
    os.path.join("Content", "Movies"),
)


# Directories that are copies of the repository or build output, not content to
# audit. Without this the scan walks .claude/worktrees/ and reports the same pack
# once per worktree -- three worktrees turned two real findings into nine lines.
# Duplicate noise is how a scanner gets ignored, so it is filtered at the source
# rather than deduplicated afterwards: the point is not to look at copies at all.
SKIP_DIR_NAMES = {
    ".git", ".claude", "node_modules", "__pycache__",
    "Intermediate", "Binaries", "DerivedDataCache", "Saved",
}


def find_third_party_roots(root):
    """Every immediate subdirectory of a ThirdParty/ folder is a pack to account for."""
    packs = []
    for dirpath, dirnames, _filenames in os.walk(root):
        # Prune before descending, so a worktree's whole tree is skipped rather
        # than walked and discarded.
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIR_NAMES]
        if os.path.basename(dirpath) != "ThirdParty":
            continue
        for name in sorted(dirnames):
            packs.append(os.path.relpath(os.path.join(dirpath, name), root))
        # Do not descend further: nested ThirdParty inside a pack is the pack's problem.
        dirnames[:] = []
    return packs


def pack_has_assets(root, pack_rel):
    for dirpath, dirnames, filenames in os.walk(os.path.join(root, pack_rel)):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIR_NAMES]
        for f in filenames:
            if os.path.splitext(f)[1].lower() in ASSET_EXTENSIONS:
                return True
    return False


def scan_provenance(root):
    """Third-party packs containing assets must be named in the legal inventory.

    Blind spot worth stating: this walks the FILESYSTEM, so it only sees packs that
    are actually present. CityPark (4.1 GB) is gitignored, so a CI checkout has no
    copy of it and this scan cannot report it -- yet it is one of the packs with no
    recorded provenance, and RA4_Skirmish_Production depends on its art. Packs
    outside version control therefore need the manual audit in
    Docs/Production/THIRD_PARTY_PACK_AUDIT.md; this check cannot substitute for it
    (RISK-20 and RISK-21 together).
    """
    violations = []
    legal_path = os.path.join(root, LEGAL_DOC)
    if not os.path.isfile(legal_path):
        # A missing inventory is itself the violation: without it nothing can be
        # cleared. Reported rather than skipped, because skipping is how this
        # check would silently pass forever.
        return ["%s is missing -- no asset provenance can be verified" % LEGAL_DOC]

    with open(legal_path, "r", encoding="utf-8", errors="replace") as fh:
        legal_text = fh.read().lower()

    for pack in find_third_party_roots(root):
        pack_name = os.path.basename(pack)
        if not pack_has_assets(root, pack):
            # An empty or stub directory carries no licence obligation yet.
            continue
        if pack_name.lower() not in legal_text:
            violations.append(
                "third-party pack '%s' contains assets but is not documented in %s "
                "(record source URL, licence and commercial-use verdict)" % (pack, LEGAL_DOC)
            )
    return violations


# A commit's subject declares its area. Deleting code in an unrelated area is the
# pattern this catches. Keys are conventional-commit scopes/types seen in this
# repo; values are the path prefixes such a commit may legitimately delete from.
SCOPE_ALLOWED_DELETIONS = {
    "build": ("Scripts/", "Build/", "Tools/", ".github/", "CMakeLists"),
    "ci": (".github/", "Build/", "Scripts/"),
    "docs": ("Docs/", "README"),
    "ui": ("Source/RA4UI/", "Source/RedAlert4/", "Content/RA4UI"),
    "loc": ("Content/", "Config/", "Docs/"),
    "content": ("Content/", "Docs/"),
    "art": ("Content/", "Source/RA4Presentation/", "Docs/"),
    "map": ("Content/Maps", "Tools/", "Docs/"),
}

# Deleting from these paths is always significant enough to justify naming it in
# the subject, whatever the scope claims.
PROTECTED_PREFIXES = (
    "Source/RA4Simulation/",
    "Source/RA4Core/",
    "Source/RA4Tests/",
    "Source/RA4Recon/",
    "Source/RA4AI/",
)

DELETION_THRESHOLD = 40   # lines removed from one protected file before we object


def git_output(args, cwd):
    return subprocess.run(["git"] + args, cwd=cwd, capture_output=True, text=True).stdout


def scan_commit_scope(root, base_ref):
    """Flag commits that delete substantial code outside their declared scope."""
    violations = []
    rev_range = "%s..HEAD" % base_ref
    shas = [s for s in git_output(["rev-list", rev_range], root).split() if s]
    if not shas:
        return violations

    for sha in shas:
        subject = git_output(["log", "-1", "--format=%s", sha], root).strip()
        # A revert or an explicit restore is allowed to delete anything: undoing
        # is its whole purpose.
        if re.match(r"^(revert|Revert)", subject):
            continue

        match = re.match(r"^(\w+)\(([^)]*)\)", subject)
        declared = []
        if match:
            declared = [p.strip() for p in match.group(2).split(",") if p.strip()]
            declared.append(match.group(1).strip())
        elif re.match(r"^(\w+):", subject):
            declared = [re.match(r"^(\w+):", subject).group(1)]

        allowed = set()
        for scope in declared:
            allowed.update(SCOPE_ALLOWED_DELETIONS.get(scope, ()))
        if not allowed:
            # An unrecognised scope cannot be judged; review covers it.
            continue

        numstat = git_output(["show", "--numstat", "--format=", sha], root)
        for line in numstat.splitlines():
            parts = line.split("\t")
            if len(parts) != 3:
                continue
            added, removed, path = parts
            if not removed.isdigit():
                continue
            removed_n = int(removed)
            if removed_n < DELETION_THRESHOLD:
                continue
            if not path.startswith(PROTECTED_PREFIXES):
                continue
            if any(path.startswith(prefix) for prefix in allowed):
                continue
            violations.append(
                "%s '%s' deletes %d lines from %s, which is outside its declared scope "
                "(%s). If the deletion is intended, say so in the subject or split the "
                "commit; five pieces of finished work were lost this way on 2026-08-06."
                % (sha[:9], subject, removed_n, path, ", ".join(declared) or "none")
            )
    return violations


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--scope", default="both",
                        choices=["both", "provenance", "commit-scope"])
    parser.add_argument("--root", default=".", help="repository root to scan")
    parser.add_argument("--base", default="origin/main",
                        help="ref to compare against for commit-scope")
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    violations = []

    if args.scope in ("both", "provenance"):
        violations += scan_provenance(root)

    if args.scope in ("both", "commit-scope"):
        # Missing base ref (shallow clone, first push) is not a violation: there
        # is nothing to compare against, and inventing a failure here would train
        # people to ignore this scanner.
        if git_output(["rev-parse", "--verify", args.base], root).strip():
            violations += scan_commit_scope(root, args.base)
        else:
            print("compliance: base ref '%s' not found, skipping commit-scope" % args.base)

    for v in violations:
        print("COMPLIANCE VIOLATION: %s" % v)

    if violations:
        print("\ncompliance scan FAILED with %d violation(s)" % len(violations))
        return 1
    print("compliance scan PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
