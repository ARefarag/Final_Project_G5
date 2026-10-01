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
    // Keep only digits, a single leading '-' and a single '.'; ignore
    // whitespace or stray characters that might slip in from user input.
    std::string cleaned;
    cleaned.reserve(numeric_value.size());
    bool sawDot = false;
    for (char ch : numeric_value) {
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            cleaned.push_back(ch);
        } else if (ch == '.' && !sawDot) {
            cleaned.push_back(ch);
            sawDot = true;
        } else if (ch == '-' && cleaned.empty()) {
            cleaned.push_back(ch);
        }
    }

    if (cleaned.empty() || cleaned == "-") {
        return 0;
    }

    try {
        size_t consumed = 0;
        const double value = std::stod(cleaned, &consumed);
        if (consumed != cleaned.size()) {
            return 0;
        }
        // Round to the nearest cent (round-half-away-from-zero).
        return static_cast<MoneyCents>(std::llround(value * 100.0));
    } catch (const std::exception&) {
        return 0;
    }
}