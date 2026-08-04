# External Asset Technical Briefs & Acceptance Rules (`EXTERNAL_ASSET_BRIEFS.md`)

**Document Version**: 4.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. External Dependencies Honest Audit

Code generation cannot invent high-poly 3D meshes, human voice recordings, legal filings, or platform certification. All external assets are explicitly briefed, budgeted, and gated.

> [!IMPORTANT]
> **Production Rule**: No external asset is marked as "Existing" in the project inventory until the physical files have been received, inspected, and passed automated validation.

---

## 2. Technical Briefs & Acceptance Criteria

### A. Hero 3D Models & PBR Textures
- **Brief**: 78 unit meshes + 35 structure models across 4 factions.
- **Technical Specs**: FBX format, PBR texture maps (Albedo, Normal, RMA packed), polycount budgets (Infantry: 5k tris, Vehicles: 18k tris, Superunits: 50k tris).
- **Acceptance Criteria**: FBX passes automated import validation in UE5; zero non-manifold geometry; team color mask in Albedo alpha channel.

### B. Professional Voice Acting & EVA Announcers
- **Brief**: 624 audio lines across 4 faction commanders and EVA system (**AURA**).
- **Technical Specs**: 48kHz / 24-bit WAV format, uncompressed, noise-cleaned, loudness normalized to -23 LUFS.
- **Acceptance Criteria**: Lines match JSON voice manifest keys (`Select`, `Move`, `Attack`, `UnderFire`, `Promoted`).

### C. Music & Original Soundtrack (OST)
- **Brief**: 12 orchestral/industrial action tracks + 4 victory/defeat themes.
- **Technical Specs**: Seamless looping WAV audio stems, dynamic combat intensity layer hooks.
- **Acceptance Criteria**: Seamless looping in UE5 Audio Subsystem without click artifacts at loop points.

### D. Legal IP Opinion & Trademark Registration
- **Brief**: Formal trademark filing for *Iron Resonance: Command of Tomorrow* and legal clearance of all unit/faction names.
- **Acceptance Criteria**: Signed legal opinion document from accredited IP legal counsel.

### E. Platform Certification & Age Ratings
- **Brief**: Steam / Epic Games Store technical certification, ESRB (Teen) / PEGI (12) rating certificates.
- **Acceptance Criteria**: Approved rating certificate & greenlit store release package.
