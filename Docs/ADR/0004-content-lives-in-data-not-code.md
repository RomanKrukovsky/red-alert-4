# ADR 0004 - Names, balance and faction identity live in data

Status: accepted; struct model implemented, Data Asset pipeline not started.

## Context

The project has two content profiles: a Licensed profile, valid only with an actual
Electronic Arts licence, and a Clean-Room profile for an independent commercial
successor. A rebrand must not be a code change.

## Decision

Every definition is a plain struct in `RA4Content`, and every player-facing string is
a localization key -- `ContentDatabase::Validate` fails authoring that hardcodes
display text. Content ids are hashes of stable string names, not array indices, so
a mod adding units cannot renumber existing content and invalidate replays.

`ComputeContentHash()` covers every value that can change simulation outcomes, and is
compared during the lobby handshake and before replay playback.

## Consequences

A full rebrand is a data swap. Balance changes are detectable: a replay recorded
before a patch is refused rather than replayed incorrectly, which is verified by a
test that changes one damage multiplier.
