// Copyright (c) Red Alert 4 project. Loads normalized JSON into ContentDatabase.
#pragma once

#include "RA4Content/ContentDatabase.h"

#include <string>
#include <vector>

namespace RA4
{

// Loads the normalized JSON produced by Tools/ContentImport/parse_bible.py
// into the ContentDatabase. Idempotent: calling twice with the same file
// produces the same state. Does not duplicate entries.
//
// Returns true on success. On failure, OutErrors contains human-readable
// messages (missing fields, bad numbers, etc.).
RA4CONTENT_API bool LoadBibleContent(ContentDatabase& Db, const std::string& JsonPath,
                                       std::vector<std::string>& OutErrors);

// Validates that the loaded content matches the bible's expected counts:
// 4 factions, 78 unique units, 64 buildings, damage matrix complete, etc.
RA4CONTENT_API bool ValidateBibleContent(const ContentDatabase& Db,
                                           std::vector<std::string>& OutErrors);

} // namespace RA4