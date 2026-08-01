# 04. Clean-room policy for a commercial project
> This is an engineering risk mitigation policy, not a legal opinion. Review by an intellectual property lawyer is required before commercial release.
## 4.1. Prohibited actions
The following cannot be placed in the production repository:
- EA C++ source files or their almost literal translation;
- RA3 XML as runtime data;- EA XSD, shaders, W3X, TGA, 3ds Max files;
- original names of units, factions, abilities, characters and missions;
- original numerical balance tables;
- EA audio, music, voice lines, UI sprites, logos;
- generated assets, built as a derivative copy of a specific EA resource;
- the word `Red Alert` in the commercial name without a license.
## 4.2. Permitted research result
You can save documents you write yourself that contain:
- general architectural boundaries;
- lists of requirements for subsystems;
- abstract flow diagrams;
- independent interfaces;
- test invariants;
- own taxonomy of game roles;
- comparison tables without copying significant arrays of values;
- links and provenance metadata.
## 4.3. Separation of roles
Recommended process:
```text
Researcher
reads EA materials
  → writes a neutral specification of behavior and restrictions
Clean-room implementer
receives only the specification
  → writes Unreal code independently
Reviewer
checks for the absence of literal similarity, EA identifiers and prohibited files```

On a small team, physically separating people may not be possible. Then separate branches, contexts and a mandatory review of the origin log are applied.
## 4.4. Directory structure
```text
Research/RA3_SAGE_Study/ self-notes only
ExternalResearch/ gitignored, local copies of sources
Source/ only original project code
Content/ only original/licensed assets
Build/Compliance/scanners and allow/deny lists```

`ExternalResearch/` should not end up in Git, CI artifacts, packaged builds or public backup archives.
## 4.5. Provenance for each production asset
Minimum fields:
```text
asset_id
path
creator
creation_date
source_type
source_uri_or_contract
license
commercial_use_allowed
contains_third_party_ip
similarity_review_status
reviewer
notes
```

## 4.6. Automated CI checks
Add job `compliance-scan`:
- prohibited extensions and directories;- known EA filenames;
- C&C faction/unit/person names;
- EA copyright headers;
- suspicious XML namespaces and path prefixes;- shader signatures/filenames;
- asset manifest without provenance;
- binary files without allowlist.

The result is blocking, not advisory.
## 4.7. Rule for AI code generation
Prompts for code agents must explicitly say:
- do not copy or translate the EA code;
- do not generate APIs based on specific EA class names;
- work according to independent specification RA4;
- indicate the origin of any external fragments;
- mark questionable places for legal/compliance review.