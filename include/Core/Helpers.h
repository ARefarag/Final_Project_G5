// ============================================================================
// GAME&GO - CORE HELPERS
// OWNER: MEMBER 3
// ============================================================================
#pragma once

#include "../CoreData.h"
#include <optional>
#include <string>

// TODO [M3]: Return current time in the format agreed for PostgreSQL TIMESTAMP.
std::string nowUtcTimestamp();

// TODO [M3]: Calculate whole elapsed minutes. When end_time is empty, use now.
int calculateElapsedMinutes(const std::string& start_time,
                            const std::optional<std::string>& end_time);

// TODO [M3]: Format MoneyCents for GUI/reporting, e.g. "EGP 150.75".
std::string formatMoney(MoneyCents cents);

// TODO [M3]: Convert a NUMERIC(10,2) text value to MoneyCents safely.
MoneyCents parseMoneyCents(const std::string& numeric_value);
