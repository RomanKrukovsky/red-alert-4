# Network Authority Matrix

Security and client-server validation rules for multiplayer simulation authority.

## Server-Authoritative State
Clients never directly mutate simulation state. All user actions pass through `SimWorld::ApplyCommand` on the server:

| Action / Mutation | Client Permission | Server Validation | Security Checks |
| --- | --- | --- | --- |
| **Credit Balance** | Read-Only | Authoritative | Client RPCs requesting purchase are rejected if credits < cost. |
| **Command Cap** | Read-Only | Authoritative | Rejects unit train commands if max cap reached. |
| **Unit Production** | Request Only | Authoritative | Validates Prerequisites, Producer building existence, ownership. |
| **Building Placement** | Request Only | Authoritative | Validates build radius, footprint passability, construction status. |
| **Ability Casting** | Request Only | Authoritative | Validates cooldown, range, charges, player resource cost. |
| **Damage Calculation**| Read-Only | Authoritative | Damage applied strictly via server 9x9 matrix pipeline. |
| **Veterancy Rank** | Read-Only | Authoritative | Experience points & promotions awarded strictly on server kill events. |

## Rate Limiting & Anti-Cheat Protection
- `CommandId` deduplication buffer prevents replay/duplicate order exploits.
- Maximum 30 command RPCs per second per player connection.
- Quantized coordinates prevent floating-point desyncs between client and server.
