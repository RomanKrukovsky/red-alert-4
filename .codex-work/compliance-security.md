# Compliance & Security Audit

Date: 2026-07-30
Repo: `/Users/romanmolodyko/Documents/red-alert-4`
Mode: read-only audit, except this report

## Scope analyzed

- Root repo instructions from session context. Physical `AGENTS.md` file in this repo root was not found.
- `Research/RA3_SAGE_Study/04_CLEAN_ROOM_POLICY.md`
- Git tracked files, working tree deltas, and untracked assets
- `Assets/`, `Content/`, `Docs/`, `Config/`, `Tools/`, `Binaries/`
- provenance and license signals
- current CI in `.github/workflows/core.yml`
- local compliance scanner in ignored `Build/Compliance/`

## Executive summary

Status: `DONE`

Main conclusion: the repo has a strong written clean-room policy, but the effective control boundary is weak. The current local scanner gives a false green result, is not versioned into git, is not called by CI, and does not cover the highest-risk paths now present in the workspace. The biggest compliance risk is not one single bug. It is the gap between policy, docs, provenance claims, and what the repo and working tree actually contain on 2026-07-30.

## What I validated

Normal path:

- Ran `python3 Build/Compliance/compliance_scan.py .`
- Result: `PASS`

Failure path:

- Read the scanner code and confirmed that the pass result is misleading.
- The scanner explicitly allowlists `Tools/ContentImport/RA3_XML_Source` and `ExternalResearch`-style paths, ignores `Build/`, and checks only a narrow string list and a few extensions.

Integration edge:

- Read `.github/workflows/core.yml`
- CI runs headless build, determinism checks, and sanitizers.
- There is no compliance job and no invocation of `Build/Compliance/run_scan.sh`.

Research analyzer spot-check:

- Read `Tools/RA3ResearchAnalyzer/main.cpp` and `CMakeLists.txt`
- Current implementation is a standalone offline CLI that only opens one file path, counts lines and `<...>` tags, prints metrics, and exits.
- It is not linked into game runtime and has no network or write path in the current code.
- It is therefore low risk as code execution surface today, but it is not a true isolation boundary because it accepts any filesystem path and does not enforce “external research only”.

## Confirmed findings

### 1. Clean-room forbidden EA/GPL corpus exists in the production workspace, and the tooling is designed to pull more of it in

Severity: High
Priority: P0

Evidence:

- Policy forbids EA XML/XSD and related production placement: `Research/RA3_SAGE_Study/04_CLEAN_ROOM_POLICY.md:8-15`, `:50-54`, `:83-90`
- ADR promises zero external code and automated compliance blocking: `Docs/ADRs/ADR-011-Clean-Room-Compliance.md:7-12`
- The fetcher downloads official EA Red Alert 3 content into `Tools/ContentImport/RA3_XML_Source`: `Tools/ContentImport/fetch_ra3_xmls.py:3-15`, `:32-38`, `:44-45`
- Local corpus present: `Tools/ContentImport/RA3_XML_Source` contains `123` files total, including `15` `.xml`, `107` `.xsd`, `1` `.h`
- EA/GPL provenance is explicit in bundled mod-support docs: `Tools/ContentImport/RA3_Mod_Files/README.md:1-15`, `Tools/ContentImport/RA3_Mod_Files/LICENSE.md:1-4`
- `Tools/ContentImport/RA3_Mod_Files` also contains a nested `.git/` directory in the workspace

Attack path:

- A teammate runs the fetcher or copies the local corpus during content import work.
- The files remain inside a normal production repo path under `Tools/`.
- Because the current scanner allowlists that path and CI has no compliance job, the corpus can survive local packaging, source archive export, or accidental commit review without being blocked.

Impact:

- Direct clean-room breach risk
- GPL and third-party license contamination risk
- High chance of policy drift becoming normalized in the toolchain

Exploitation prerequisites:

- Only ordinary repository access is needed
- No elevated privileges required

Smallest useful fix:

- Treat `Tools/ContentImport/RA3_XML_Source/**` and `Tools/ContentImport/RA3_Mod_Files/**` as deny-by-default in the blocking scanner
- Move any unavoidable local EA research corpus outside the repo to `ExternalResearch/` or another gitignored path that CI and release packaging never touch
- Block nested `.git/` directories anywhere under the project root

Expected risk reduction:

- Stops the clearest IP contamination route with one control boundary

### 2. The current compliance control is non-blocking in practice and produces false greens

Severity: High
Priority: P0

Evidence:

- CI has no compliance job: `.github/workflows/core.yml:1-94`
- The accepted ADR says CI must run `Build/Compliance/compliance_scan.py`: `Docs/ADRs/ADR-011-Clean-Room-Compliance.md:10-12`
- Checklist claims `Build/Compliance/compliance_scan.py` is implemented and `ExternalResearch/` was added to `.gitignore`: `Docs/Implementation/RA3_SAGE_IMPLEMENTATION_CHECKLIST.md:7-9`
- Gap analysis still says the target state is an automated blocking compliance scan: `Docs/Implementation/RA3_SAGE_IMPLEMENTATION_GAP.md:21-22`
- `.gitignore` ignores the whole `Build/` tree: `.gitignore:1-26`
- `git ls-files Build/Compliance` returns nothing, so the scanner is not versioned
- Local scanner allowlists `Tools/ContentImport/RA3_XML_Source` and `Binaries`: `Build/Compliance/compliance_scan.py:23-32`
- Local scanner passed even with the current repo/workspace contents

Attack path:

- A PR or local merge adds forbidden names, unprovenanced assets, or more imported research files.
- CI stays green because no blocking compliance stage exists.
- Reviewers assume the documented controls are real.

Impact:

- Control failure, not just coverage gap
- False sense of safety at the exact gate where blocking should happen

Exploitation prerequisites:

- Normal contributor workflow

Smallest useful fix:

- Move the scanner out of ignored `Build/` into a versioned path such as `Tools/Compliance/`
- Add a required `compliance-scan` CI job before build/test jobs
- Fail on missing manifests, nested repos, forbidden corpora, absolute local paths, and binary assets without approved provenance

Expected risk reduction:

- Converts policy from documentation to an enforceable gate

### 3. Provenance is incomplete and internally contradictory for tracked and untracked assets

Severity: High
Priority: P0

Evidence:

- Clean-room policy requires provenance fields per production asset: `Research/RA3_SAGE_Study/04_CLEAN_ROOM_POLICY.md:63-76`
- Asset plan says the registry is currently empty and nothing enters without a row: `AssetAcquisitionPlan.md:3-7`
- Integration template still says `Content/ThirdParty/` does not exist and the registry is empty: `Docs/integration/templates/CURRENT_PROJECT_AUDIT.md:177-183`
- Actual registry was updated on `2026-07-28T16:32:00Z` and contains two “Verified / Integrated” entries: `Content/AssetRegistry/ThirdPartyAssets.json:3-6`, `:8-25`, `:33-50`
- Both registry `localImportPath` targets do not exist on disk:
  - `TPA-KENNEY-RTS-KIT-01 -> Content/ThirdParty/KenneyRTSKit/`
  - `TPA-POLYHAVEN-TEXTURES-01 -> Content/ThirdParty/PolyHavenGround/`
- Kenney row uses SHA-256 `e3b0...b855`, the well-known empty-file hash: `Content/AssetRegistry/ThirdPartyAssets.json:23-25`
- Registry coverage is only `2` assets and does not cover:
  - `Content/RA4UI`
  - `Assets/RA4UI`
  - `Content/RA4/Audio`
  - `Config/Audio`
  - `Binaries/Mac`
- Tracked or present asset surfaces without provenance coverage include:
  - `206` tracked files in `Content/RA4/Audio/Generated`
  - `17` tracked files in `Assets/RA4UI/Generated`
  - `962` files present in `Content/RA4/Audio/EVA`
  - `40` files present in `Binaries/Mac`
  - third-party font files in `Content/RA4UI/Fonts/RA4_RobotoCondensed*.ttf`

Attack path:

- Assets are imported or generated into repo paths used by the project.
- Provenance registry does not cover them, or claims coverage for paths that do not exist.
- During release prep, nobody has a trustworthy allow/deny answer for what can legally ship.

Impact:

- Release-blocking provenance ambiguity
- High audit friction
- Increased chance of distributing unlicensed or weakly documented assets

Exploitation prerequisites:

- Ordinary asset import/generation workflow

Smallest useful fix:

- Make provenance coverage mandatory for every file under `Content/`, `Assets/`, `Config/Audio/`, and approved binary folders
- Fail if a registry row points to a missing path, has an empty-file checksum for a supposedly integrated asset, or claims “Integrated” without on-disk evidence
- Separate “planned”, “downloaded”, “integrated”, and “shipping-approved” states

Expected risk reduction:

- Restores trust in the registry as the source of truth

### 4. Generated audio and selection metadata leak local workstation paths and break the clean-room boundary

Severity: Medium
Priority: P1

Evidence:

- `40` tracked `selection.json` files contain absolute local paths into `GeneratedVO`
- Example: `Content/RA4/Audio/Generated/Anchors/Soviet/EVA_Soviet/selection.json:7-8`, `:19-20`, `:31-32`, `:42`
- Config ties runtime-like selection data back to `GeneratedVO/...` anchors: `Config/Audio/eva_voice_selection.json:4-16`, `:18-30`, `:32-44`, `:46-58`
- The working tree contains `951` status entries under `Content/RA4/Audio/EVA`
- The tracked tree already contains `206` generated audio files under `Content/RA4/Audio/Generated`

Attack path:

- Selection metadata or generated audio is committed, archived, mirrored, or handed to contractors.
- Local absolute paths reveal workstation structure and the exact upstream generation workspace.
- More importantly, the production tree stays coupled to an out-of-band generation corpus (`GeneratedVO`) instead of a sealed, provenance-reviewed import bundle.

Impact:

- Metadata leakage
- Weak separation between source generation workspace and production assets
- Higher chance of accidentally shipping drafts, anchors, auditions, or review-only artifacts

Exploitation prerequisites:

- Access to repo export, source archive, or shared workspace

Smallest useful fix:

- Ban absolute local paths in committed JSON
- Mark `GeneratedVO`, auditions, anchors, review HTML, and selection diagnostics as non-shipping by default
- Require a promotion step that copies only approved final WAV assets plus provenance

Expected risk reduction:

- Shrinks accidental disclosure and shipping-surface risk fast

### 5. Secret handling and importer robustness are weak around content tooling

Severity: Medium
Priority: P1

Evidence:

- Workspace `.env` contains set values for `OPENROUTER_API_KEY` and `OPENCODE_API_KEY` and several runtime config values
- Tracked Android file server settings expose a fixed token and network setting:
  - `Config/DefaultEngine.ini:19-24`
- XML import tool uses `xml.etree.ElementTree.parse()` directly on imported corpus and swallows all exceptions:
  - `Tools/ContentImport/ra3_xml_parser.py:15`
  - `Tools/ContentImport/ra3_xml_parser.py:124-139`
  - `Tools/ContentImport/ra3_xml_parser.py:247-252`
- `Tools/RA3ResearchAnalyzer` is simple and offline, but its current isolation is policy-by-comment, not enforced by path rules:
  - `Tools/RA3ResearchAnalyzer/main.cpp:2-4`
  - `Tools/RA3ResearchAnalyzer/main.cpp:13-24`
  - `Tools/RA3ResearchAnalyzer/CMakeLists.txt:1-7`

Attack path:

- Local secrets can leak through careless artifact bundling, logs, or repo copies if workspace scans are not enforced.
- The parser can silently skip malformed or unexpected files, which weakens auditability and makes it easier to hide forbidden content behind “successful” conversions.

Impact:

- Secret exposure risk in local/exported workspaces
- Low trust in content-ingestion outputs
- Missed parser failures can hide provenance or policy violations
- Research analyzer can read arbitrary local files if used carelessly, so it should not be treated as a strong compliance boundary by itself

Exploitation prerequisites:

- Access to local workspace or shared artifacts
- For parser issues, ability to introduce malformed or policy-breaking XML into the import corpus

Smallest useful fix:

- Add workspace-mode secret scanning for `.env`, export bundles, and CI artifacts
- Replace blanket `except Exception: pass` with explicit failure reporting and non-zero exit on parse errors
- Treat imported XML as untrusted input and log every parsed file deterministically
- Restrict `RA3ResearchAnalyzer` inputs to an approved external-research root and fail closed outside that root

Expected risk reduction:

- Better failure visibility and less accidental secret spill

## Minimal blocking compliance-scan architecture

Goal: smallest real gate that blocks the current failures without pretending to solve all legal questions.

### Recommended shape

1. Put the scanner in a versioned path

- Move from ignored `Build/Compliance/` to `Tools/Compliance/`
- Commit the scanner, config, and fixture tests

2. Add one blocking CI job before build/test

- Job name: `compliance-scan`
- Inputs:
  - tracked file inventory from `git ls-files`
  - optional changed-file inventory for PR annotation
- Output:
  - machine-readable JSON
  - human-readable markdown summary
- Hard fail on any deny hit or missing required provenance

3. Support two modes

- `repo` mode for CI: tracked files only
- `workspace` mode for local preflight: tracked + untracked + ignored-risk paths such as `Binaries/`, `GeneratedVO/`, `Build/Compliance/`, nested repos

4. Keep rules simple and explicit

- inventory phase
- deny phase
- allow-exception phase
- provenance coverage phase

### Minimum deny rules

Block on:

- any file under:
  - `Tools/ContentImport/RA3_XML_Source/**`
  - `Tools/ContentImport/RA3_Mod_Files/**`
  - `ExternalResearch/**` in repo mode
  - nested `.git/**` anywhere below project root
- any committed absolute local path matching:
  - `/Users/`
  - `C:\\`
  - `/home/`
- forbidden extensions outside explicit approved scopes:
  - `.xml`
  - `.xsd`
  - `.w3x`
  - `.tga`
  - `.big`
- known EA/C&C identifiers in shipping-sensitive paths:
  - `Content/`
  - `Assets/`
  - `Config/`
  - `Source/`
  - release-facing `Docs/`
- raw generated review artifacts:
  - auditions
  - anchor selection diagnostics
  - HTML review reports
  - intermediate pipeline state
- binary files in repo/workspace without explicit provenance approval

### Minimum allowlist schema

Use a versioned YAML or JSON file with one row per exception:

```yaml
- id: allow-0001
  path: Content/RA4UI/Fonts/RA4_RobotoCondensedRegular.ttf
  sha256: "<required>"
  reason: "Approved third-party font for UI prototype"
  source_type: "third_party"
  source_uri_or_contract: "<required>"
  license: "<required>"
  commercial_use_allowed: true
  contains_third_party_ip: true
  similarity_review_status: "reviewed"
  reviewer: "<required>"
  approved_on: "2026-07-30"
  expires_on: "2026-09-30"
  shipping_allowed: false
```

Rules:

- allow entries must be hash-pinned
- every exception must expire
- `shipping_allowed` defaults to `false`

### Minimum provenance schema

Start from the policy fields and add only what the gate needs:

```json
{
  "asset_id": "string",
  "path": "string",
  "sha256": "string",
  "artifact_kind": "source|derived|generated|binary|audio|image|font|doc",
  "creator": "string",
  "creation_date": "YYYY-MM-DD",
  "source_type": "original|third_party|generated_from_internal|research_reference",
  "source_uri_or_contract": "string",
  "license": "string",
  "commercial_use_allowed": true,
  "contains_third_party_ip": false,
  "similarity_review_status": "pending|reviewed|rejected",
  "reviewer": "string",
  "generated_by": "tool or workflow id",
  "release_allowed": false,
  "notes": "string"
}
```

Blocking rules:

- every file under approved asset roots must have a provenance row
- every row must point to an existing path
- `Integrated` or `release_allowed=true` cannot exist without a valid hash
- `research_reference` cannot live in shipping roots

## Suggested remediation order

1. Block `Tools/ContentImport/RA3_XML_Source/**`, `Tools/ContentImport/RA3_Mod_Files/**`, nested `.git/**`, and absolute local paths in the scanner.
2. Move scanner into a tracked path and add the blocking CI job.
3. Freeze promotion of new audio/UI/binary assets until provenance coverage is real.
4. Split review-only generation artifacts from shipping candidates.
5. Tighten parser error handling and workspace secret scanning.

## What still needs runtime or environment verification

- Unreal cook/stage output: confirm whether `Content/RA4/Audio/EVA/**`, `Content/RA4/Audio/Generated/**`, raw fonts, and review artifacts can be staged into packaged builds or release zips
- CI artifact policy outside the visible workflow: confirm no broader archive step ships the workspace or ignored directories
- Team workflow: confirm whether local EA corpora are ever copied into PR attachments, cloud drives, or contractor handoff bundles
- Binary provenance: confirm origin and license status of `Binaries/Mac/libtbb*`, `libmetalirconverter.dylib`, and project dylibs before any public distribution

## Residual risk

Even after the minimal scanner lands, static compliance review cannot prove non-derivative similarity of generated art, audio, or design phrasing. It can only block obvious forbidden inputs, missing provenance, and policy boundary breaks. Final commercial release still needs legal review and a packaging audit of the exact shipped artifact set.
