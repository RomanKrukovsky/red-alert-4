# ADR-0009: Data-Driven JSON Schema for Gameplay Content Loading

## Context
Hardcoding unit stats, weapon damage, and build times in C++ source requires full binary re-compilation for minor balance tweaks.

## Decision
Incorporate runtime JSON content loading (`ra4_content.normalized.json`) ingested via `BibleContentLoader` into `ContentDatabase`.

## Rationale
- Allows designers and modders to adjust unit stats and armor matrices without C++ re-compilation.
- Verified by `BibleImport.*` unit tests.

## Status
**ACCEPTED / IMPLEMENTED**.
