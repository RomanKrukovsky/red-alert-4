# Threat model

Scope: the authoritative dedicated server and the client-server protocol. Assume any
client is hostile and fully modified.

| Threat | Mitigation | State |
| --- | --- | --- |
| Client mutates its own state (credits, health, tech) | client sends commands only; all state lives on the server | implemented |
| Client orders units it does not own | ownership checked in `ApplyCommand` | implemented, tested |
| Client acts on destroyed or non-existent entities | slot + generation handles; stale handles fail to resolve | implemented, tested |
| Client produces what it cannot afford or has not teched to | cost and prerequisite checks server-side | implemented, tested |
| Client places buildings illegally (in water, off map, outside base) | placement validated server-side | implemented, tested |
| Command flooding to exhaust server CPU | per-player per-tick command budget, excess rejected | implemented, tested |
| Malformed or truncated packets crash the server | every read is bounds checked and sets an error flag; the whole packet is dropped | implemented, tested |
| Oversized replay or map payloads | tile count validated against `kMaxMapTiles` before allocation | implemented |
| Maphack / reading fog-hidden state | server must not send hidden enemy state to a client | **not implemented** - depends on fog of war |
| Desync, whether accidental or induced | per-tick state checksums exchanged on a fixed cadence | checksum implemented; exchange not implemented |
| Modified content used to gain an advantage | content hash compared in the lobby handshake | hash implemented; handshake not implemented |
| Replay or save tampering | format version and content hash validated on load | implemented, tested |

Not yet modelled: account and session security, matchmaking abuse, chat abuse,
crash-report data handling. Those arrive with the networking stage.
