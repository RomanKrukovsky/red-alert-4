#pragma once

#include "CoreMinimal.h"
#include "TelemetryEvent.h"
#include "EconomyTickRecord.h"   // ADR-0020

/**
 * ADR-0028 Telemetry Hub
 * Central, deterministic collector registry.
 * All collectors are registered at simulation start.
 * Production mode is gated by bEnabled + PrivacyMode.
 */

namespace RA4
{

enum class ETelemetryPrivacyMode : uint8_t
{
    Off = 0,
    ProdAnonymized,
    DevFull
};

class ITelemetryCollector
{
public:
    virtual ~ITelemetryCollector() = default;
    virtual void RecordEvent(const FTelemetryEventHeader& Header, const void* Payload) = 0;
};

class FTelemetryHub
{
public:
    static FTelemetryHub& Get();

    void Initialize(uint64_t InMatchId, ETelemetryPrivacyMode InMode);
    void Shutdown(); // flushes buffers, writes files

    bool IsEnabled() const { return bEnabled; }
    ETelemetryPrivacyMode GetPrivacyMode() const { return PrivacyMode; }

    // Called by collectors (CommandBus, SimWorld, Combat, etc.)
    void Emit(const FTelemetryEventHeader& Header, const void* Payload, uint16_t PayloadSize);

    // Economy tick (re-uses ADR-0020 record)
    void RecordEconomyTick(const EconomyTickRecord& Record, uint8_t PlayerId);

    void RegisterCollector(ITelemetryCollector* Collector);
    void UnregisterCollector(ITelemetryCollector* Collector);

private:
    FTelemetryHub() = default;

    uint64_t MatchId = 0;
    ETelemetryPrivacyMode PrivacyMode = ETelemetryPrivacyMode::Off;
    bool bEnabled = false;
    TArray<ITelemetryCollector*> Collectors;
    // Pre-allocated ring buffer + file writer (implementation in .cpp)
};

} // namespace RA4