// Copyright (c) Red Alert 4 project. Coarse observation categories.
//
// The confusion matrix (ADR-0026 §4.3.3) is authored over these six coarse
// categories -- fine-grained per-unit matrices would be an authoring burden
// nobody sustains across four factions (owner decision 2026-08-06: six is
// enough). Lives in its own header because both the data types and the
// designer config need it and neither may include the other.
#pragma once

#include <cstdint>

namespace RA4
{
namespace Recon
{

enum class ObservedCategory : uint8_t
{
    Infantry = 0,
    LightVehicle,
    HeavyVehicle,
    Aircraft,
    Ship,
    Structure,
    Count,
};

constexpr int32_t kObservedCategoryCount = int32_t(ObservedCategory::Count);

} // namespace Recon
} // namespace RA4
