# Pre-Import Baseline Metrics (Phase 0)

This document captures the state of the `RA4_Skirmish` map before applying any Phase 0 visual modifications (exposure fixes, procedural ground, distinct building primitives, UI scaling).

## Map Information
* **Name:** `Content/Maps/RA4_Skirmish.umap`
* **Resolution:** 1920x1080 (Target baseline)

## Initial State Metrics
* **Lighting:** Global default Directional Light and Skylight. Unclamped Auto-Exposure leads to overblown whites and loss of detail in shadow geometry.
* **Ground Material:** `M_RA4Ground` / `M_RA4Ground_Lit`. Simple flat gray surface, lacking any macro variation, paths, or visual anchor points.
* **Building Models (Blockout):** 140 basic primitives imported in `Content/RA4/Art/Blockout/`. Currently, almost all use the single placeholder material `M_RA4EntityPlaceholder`, making buildings indistinguishable from one another by faction or type.
* **Selection & Feedback:** Visual selection ring relies on UI bounds (2D Canvas rectangle).
* **HUD:** The UI layout is strictly hardcoded to a 1920x1080 canvas size (e.g. `kSidebarWidth = 232.0f`) without DPI scaling or safe zone margins, breaking layout on 4K.

## Performance Snapshot (Before Visual Rescue)
* **CPU / GPU Frame Time:** ~10-15ms (Editor PIE)
* **Draw Calls:** Minimal (pure blockout rendering).

*(See `Saved/VisualAudit/00_original_blockout.png` for visual proof of this baseline state.)*
