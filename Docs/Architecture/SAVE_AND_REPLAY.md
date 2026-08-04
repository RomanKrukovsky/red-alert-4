# Save System & Replay Architecture (`SAVE_AND_REPLAY.md`)

**Document Version**: 3.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. Replay Engine Architecture (`RA4Replay`)

Replays record the initial match seed and per-tick command frames. Because simulation execution is 100% deterministic, replaying the command stream recreates the exact match state.

### File Format (`.ra4replay`)
- **Magic Header**: `RA4R` (4 bytes).
- **Engine Version**: `uint32_t` engine build hash.
- **Match Config**: Random seed, player IDs, map ID, faction selections.
- **Snapshot Checkpoints**: `SimWorld` binary snapshot stored every 1,800 ticks (30 sec).
- **Command Stream**: Compressed per-tick command frame array.

---

## 2. Save Game Migration & Backward Compatibility

- **Save Header**: Contains `uint32_t SaveVersion`.
- **Migration Pipeline**: When an older save file is opened (e.g. Version 1 save opened on Version 2 build), `SaveMigrator::Upgrade()` executes sequential migration handlers (`V1_to_V2()`, `V2_to_V3()`) to patch entity data structures safely.
