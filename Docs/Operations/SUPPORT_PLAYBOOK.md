# Player Support Playbook (`SUPPORT_PLAYBOOK.md`)

**Document Version**: 11.0  
**Status**: **PLAYER SUPPORT & TROUBLESHOOTING GUIDE**  

---

## 1. Ticket Triage & Resolution Workflows

| Ticket Category | Typical User Query | Triage Action | Escalation Path |
| :--- | :--- | :--- | :--- |
| **Graphics / Performance** | Low frame rate, ultrawide black bars | Direct user to graphics settings; verify resolution scale and driver version | Support Level 1 |
| **Keybindings / Input** | Custom hotkey binding issue | Provide steps to reset keybindings to shipped defaults in Options menu | Support Level 1 |
| **Crash on Launch** | Game exits immediately upon boot | Request `Saved/Logs/RedAlert4.log` and verify GPU DirectX 12 / Metal support | Support Level 2 |
| **Multiplayer Desync** | Match terminated with desync popup | Request Match ID and export binary replay file (`.ra4rep`) for QA inspection | QA / Network Team |

---

## 2. Moderation & Community Guidelines

* **In-Game Chat Moderation**: Automated filter masks abusive language in public skirmish lobbies.
* **Cheating & Exploits**: Network validation engine enforces authoritative server rule check. Cheating attempts trigger automated player account suspension.
