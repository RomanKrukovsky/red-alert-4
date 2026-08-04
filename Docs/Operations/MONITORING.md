# Live Operations Monitoring & Observability (`MONITORING.md`)

**Document Version**: 11.0  
**Status**: **ACTIVE MONITORING SPECIFICATION**  
**Privacy Compliance**: **ZERO-PII COMPLIANT (GDPR / CCPA)**  

---

## 1. Key Performance Indicators (KPIs) & Alert Thresholds

| Metric Name | Description | Target SLA | Alert Trigger Threshold | Escalation Severity |
| :--- | :--- | :---: | :---: | :---: |
| **`crash_rate_session`** | Percentage of sessions experiencing a process crash | < 0.1% | > 0.5% over 5 mins | **S1 (Critical)** |
| **`desync_rate_match`** | Percentage of multiplayer matches detecting state divergence | < 0.01% | > 0.05% over 10 mins | **S1 (Critical)** |
| **`matchmaking_queue_time`** | Average time spent in 1v1 / 2v2 matchmaking queue | < 30 sec | > 120 sec | **S2 (Major)** |
| **`server_tick_rate`** | Lockstep server simulation tick frequency | 60.0 Hz | < 58.0 Hz avg | **S2 (Major)** |
| **`client_disconnect_rate`** | Unexpected socket disconnect percentage | < 1.0% | > 3.0% | **S3 (Moderate)** |
| **`save_restore_failure_rate`** | Failure rate when loading mid-match save snapshots | 0.0% | > 0.1% | **S2 (Major)** |

---

## 2. Privacy & Telemetry Principles

* **Zero PII Policy**: Telemetry payloads collect no player names, IP addresses, email addresses, or hardware MAC addresses.
* **Match Telemetry Payload**: Collects match duration, winning faction, unit production counts, command frame latencies, and FNV-1a checksums.
* **Crash Ingestion**: Crash dumps strip local file paths and environment variables prior to transmission.
