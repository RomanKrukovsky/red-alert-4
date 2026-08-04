# Data Flow Architecture (`DATA_FLOW.md`)

**Document Version**: 3.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. Input-to-Render Data Flow

```
[ User Input (Mouse / WASD) ]
             |
             v
[ RA4InputRouter (Input Isolation & Validation) ]
             |
             v
[ Command Struct Created (e.g. CommandType::Move) ]
             |
             v
[ LockstepSession::SubmitLocalCommand (Tick N + Delay) ]
             |
             v (Network Packet Broadcast)
[ LockstepSession::ReceivePlayerFrame (All Peers) ]
             |
             v
[ CommandBus::DispatchTick (Fixed 60Hz Execution) ]
             |
             v
[ SimWorld Update (Transforms, Combat, Economy) ]
             |
             +---> [ State Hash (64-bit Checksum Validation) ]
             |
             v
[ SimWorld Event Queue (SimEventType::BuildingPlaced, etc.) ]
             |
             v
[ URA4PresentationSubsystem (Visual Actor Interpolation) ]
             |
             v
[ Render Frame & UMG HUD Snapshot Update ]
```

---

## 2. Content Data Flow Pipeline

```
[ RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md ]
             |
             v (Python ContentImport Script)
[ Content/RA4/Data/Generated/ra4_content.normalized.json ]
             |
             v
[ BibleContentLoader::LoadBibleContent ]
             |
             v
[ ContentDatabase (EntityDef, DamageMatrixDef, VoiceSetDef) ]
             |
             v
[ SimWorld & Presentation Subsystem Initialization ]
```
