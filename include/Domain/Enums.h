// ============================================================================
// GAME&GO - ENUMERATIONS
// OWNER: MEMBER 3
// ============================================================================
#pragma once
#include <string>

enum class UserRole {
    Admin,
    Staff,
    Customer,
    Unknown
};

enum class StationType {
    PC,
    PS4,
    PS5,
    Unknown
};

enum class StationStatus {
    Available,
    InUse,
    Maintenance,
    Unknown
};

enum class ReservationStatus {
    Pending,
    Confirmed,
    Canceled,
    Unknown
};

// TODO [M3]: Implement human-readable enum-to-string conversion.
std::string toString(UserRole role);
std::string toString(StationType type);
std::string toString(StationStatus status);
std::string toString(ReservationStatus status);

// TODO [M3]: Parse exact PostgreSQL text values into enums.
//             Unknown input must remain Unknown and never imply privilege.
UserRole parseUserRole(const std::string& value);
StationType parseStationType(const std::string& value);
StationStatus parseStationStatus(const std::string& value);
ReservationStatus parseReservationStatus(const std::string& value);
