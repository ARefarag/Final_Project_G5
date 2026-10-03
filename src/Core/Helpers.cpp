// GAME&GO - CORE HELPERS (IMPLEMENTATION)
// OWNER: MEMBER 3
//
// Team-agreed timestamp format: "YYYY-MM-DD HH:MM:SS", UTC, with no timezone
// suffix - this matches a PostgreSQL TIMESTAMP (without time zone) column
// and is exactly what ReservationRecord::reserved_time / SessionRecord
// start_time/end_time are expected to hold.
// ============================================================================
#include "Core/Helpers.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace {

constexpr const char* kTimestampFormat = "%Y-%m-%d %H:%M:%S";

// Parses a "YYYY-MM-DD HH:MM:SS" UTC string into a time_t.
// Returns false (leaving 'out' untouched) if the string is malformed.
bool ParseUtcTimestamp(const std::string& text, std::time_t& out) {
    std::tm tmValue{};
    std::istringstream iss(text);
    iss >> std::get_time(&tmValue, kTimestampFormat);
    if (iss.fail()) {
        return false;
    }

#if defined(_WIN32)
    out = _mkgmtime(&tmValue);
#else
    out = timegm(&tmValue);
#endif
    return out != static_cast<std::time_t>(-1);
}

} // namespace

std::string nowUtcTimestamp() {
    const std::time_t now = std::time(nullptr);

    std::tm utcTm{};
#if defined(_WIN32)
    gmtime_s(&utcTm, &now);
#else
    gmtime_r(&now, &utcTm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&utcTm, kTimestampFormat);
    return oss.str();
}

int calculateElapsedMinutes(const std::string& start_time,
                             const std::optional<std::string>& end_time) {
    std::time_t startSeconds{};
    if (!ParseUtcTimestamp(start_time, startSeconds)) {
        // Malformed start time: we cannot know elapsed time, so report 0
        // rather than guessing or crashing.
        return 0;
    }

    std::time_t endSeconds{};
    if (end_time.has_value()) {
        if (!ParseUtcTimestamp(*end_time, endSeconds)) {
            return 0;
        }
    } else {
        // Session still running: elapsed time is measured against "now".
        endSeconds = std::time(nullptr);
    }

    if (endSeconds <= startSeconds) {
        return 0;
    }

    const double diffSeconds = std::difftime(endSeconds, startSeconds);
    return static_cast<int>(diffSeconds / 60.0);
}

std::string formatMoney(MoneyCents cents) {
    const bool negative = cents < 0;
    const MoneyCents absCents = negative ? -cents : cents;

    const long long whole = static_cast<long long>(absCents / 100);
    const long long fraction = static_cast<long long>(absCents % 100);

    std::ostringstream oss;
    oss << "EGP " << (negative ? "-" : "") << whole << "." << std::setfill('0') << std::setw(2)
        << fraction;
    return oss.str();
}

MoneyCents parseMoneyCents(const std::string& numeric_value) {
    // Strict format only: an optional leading '-', one or more digits, and
    // an optional '.' followed by exactly 1 or 2 digits. Anything else
    // (stray letters, multiple dots, trailing garbage like "12abc34") is
    // rejected as invalid input rather than silently stripped out.
    //
    // The decimal string is converted directly to integer cents - it never
    // passes through a floating-point intermediate (no std::stod/double),
    // so no binary rounding error can enter a money value.
    const size_t n = numeric_value.size();
    size_t i = 0;

    if (n == 0) {
        return 0;
    }

    bool negative = false;
    if (numeric_value[i] == '-') {
        negative = true;
        ++i;
    }

    const size_t integerStart = i;
    while (i < n && std::isdigit(static_cast<unsigned char>(numeric_value[i]))) {
        ++i;
    }
    const size_t integerLen = i - integerStart;
    if (integerLen == 0) {
        // No digits before an optional decimal point - e.g. "-", ".", "-.5".
        return 0;
    }
    const std::string integerPart = numeric_value.substr(integerStart, integerLen);

    std::string fractionalPart;
    if (i < n && numeric_value[i] == '.') {
        ++i;
        const size_t fracStart = i;
        while (i < n && std::isdigit(static_cast<unsigned char>(numeric_value[i]))) {
            ++i;
        }
        const size_t fracLen = i - fracStart;
        // NUMERIC(10,2) means at most 2 decimal digits; require at least 1
        // digit after a decimal point is actually present.
        if (fracLen == 0 || fracLen > 2) {
            return 0;
        }
        fractionalPart = numeric_value.substr(fracStart, fracLen);
    }

    if (i != n) {
        // Leftover characters after the number (e.g. "12abc34", "12.5x")
        // mean the whole string is not a valid numeric value.
        return 0;
    }

    // Pad a single fractional digit to two ("12.5" -> "50" cents, not 5).
    if (fractionalPart.empty()) {
        fractionalPart = "00";
    } else if (fractionalPart.size() == 1) {
        fractionalPart.push_back('0');
    }

    try {
        const long long integerCents = std::stoll(integerPart) * 100;
        const long long fractionCents = std::stoll(fractionalPart);
        const long long totalCents = integerCents + fractionCents;
        return static_cast<MoneyCents>(negative ? -totalCents : totalCents);
    } catch (const std::exception&) {
        // Overflow on an unrealistically long digit string.
        return 0;
    }
}