# Security & Threat Model (`THREAT_MODEL.md`)

**Document Version**: 3.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. Trust Boundaries & Security Architecture

```
[ UNTRUSTED CLIENT ENVIRONMENT ]
  - User Memory (Cheat Engine, Hooking)
  - Network Interface (Packet Modification)
             |
             | UDP/TCP Lockstep Command Frames
             v
+-------------------------------------------------------------+
|                     TRUST BOUNDARY                          |
+-------------------------------------------------------------+
             |
             v
[ TRUSTED AUTHORITATIVE SERVER ENVIRONMENT ]
  - Command Frame Sanity & Rate Limit Verification
  - 64-Bit State Checksum Validation
  - Match Results Verification & MMR Calculation
```

---

## 2. Network Attack Vectors & Mitigations

### A. Malformed Packet Injection
- **Threat**: Attacker sends buffer overflow payloads or corrupted command frames.
- **Mitigation**: All incoming network buffers are deserialized using explicit bounds-checked readers (`PacketReader::ReadBytes`). Invalid packets are immediately dropped, and the connection is closed.

### B. Command Flooding / Rate Limiting
- **Threat**: Attacker floods server with 10,000 commands per second to stall simulation ticks.
- **Mitigation**: `RA4Network` enforces a strict rate limit of 10 commands per player per tick frame. Excess commands are rejected with zero simulation effect.

### C. Map Editor / Modding Arbitrary Code Execution
- **Threat**: Malicious custom map loading executing shell commands.
- **Mitigation**: Map files contain purely declarative data (heightmap, entity placements, ECA triggers). No executable C++ or DLL scripts are loaded from map files.

---

## 3. Supply-Chain Security & Dependency Scanning

- **Dependency Scanning**: `cargo audit` / `npm audit` / GitHub Dependabot automatically scan build dependencies for known CVE vulnerabilities.
- **Secret Isolation**: Game client binaries contain zero API keys or backend database credentials. All authentication uses OAuth2 JWT tokens issued by secure backend services.
