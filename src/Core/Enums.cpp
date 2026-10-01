// ============================================================================
// GAME&GO - ENUMERATIONS (IMPLEMENTATION)
// OWNER: MEMBER 3
//
// Parsing is intentionally an EXACT, case-sensitive match against the text
// values PostgreSQL will actually contain (see CoreData.h comments: "Admin /
// Staff / Customer", "PC / PS4 / PS5", "Available / InUse / Maintenance",
// "Pending / Confirmed / Canceled"). Any value that doesn't match one of
// those exact strings maps to Unknown - it never silently maps to a
// privileged or otherwise "guessed" role/state.
// ============================================================================
#include "Domain/Enums.h"

// ---------------------------------------------------------------------
// UserRole
// ---------------------------------------------------------------------

std::string toString(UserRole role) {
    switch (role) {
        case UserRole::Admin:    return "Admin";
        case UserRole::Staff:    return "Staff";
        case UserRole::Customer: return "Customer";
        case UserRole::Unknown:  return "Unknown";
    }
    return "Unknown";
}

UserRole parseUserRole(const std::string& value) {
    if (value == "Admin")    return UserRole::Admin;
    if (value == "Staff")    return UserRole::Staff;
    if (value == "Customer") return UserRole::Customer;
    return UserRole::Unknown;
}

// ---------------------------------------------------------------------
// StationType
// ---------------------------------------------------------------------

std::string toString(StationType type) {
    switch (type) {
        case StationType::PC:      return "PC";
        case StationType::PS4:     return "PS4";
        case StationType::PS5:     return "PS5";
        case StationType::Unknown: return "Unknown";
    }
    return "Unknown";
}

StationType parseStationType(const std::string& value) {
    if (value == "PC")  return StationType::PC;
    if (value == "PS4") return StationType::PS4;
    if (value == "PS5") return StationType::PS5;
    return StationType::Unknown;
}

// ---------------------------------------------------------------------
// StationStatus
// ---------------------------------------------------------------------

std::string toString(StationStatus status) {
    switch (status) {
        case StationStatus::Available:   return "Available";
        case StationStatus::InUse:       return "InUse";
        case StationStatus::Maintenance: return "Maintenance";
        case StationStatus::Unknown:     return "Unknown";
    }
    return "Unknown";
}

StationStatus parseStationStatus(const std::string& value) {
    if (value == "Available")   return StationStatus::Available;
    if (value == "InUse")       return StationStatus::InUse;
    if (value == "Maintenance") return StationStatus::Maintenance;
    return StationStatus::Unknown;
}

// ---------------------------------------------------------------------
// ReservationStatus
// ---------------------------------------------------------------------

std::string toString(ReservationStatus status) {
    switch (status) {
        case ReservationStatus::Pending:   return "Pending";
        case ReservationStatus::Confirmed: return "Confirmed";
        case ReservationStatus::Canceled:  return "Canceled";
        case ReservationStatus::Unknown:   return "Unknown";
    }
    return "Unknown";
}

ReservationStatus parseReservationStatus(const std::string& value) {
    if (value == "Pending")   return ReservationStatus::Pending;
    if (value == "Confirmed") return ReservationStatus::Confirmed;
    if (value == "Canceled")  return ReservationStatus::Canceled;
    return ReservationStatus::Unknown;
}