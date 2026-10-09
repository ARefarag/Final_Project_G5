// ============================================================================
// GAME&GO - SHARED DATA CONTRACT
// OWNER: SHARED - TEAM AGREEMENT REQUIRED BEFORE CHANGES
//
// These plain structs are the common language between PostgreSQL, the core,
// and Dear ImGui. They intentionally contain data only. No SQL, pqxx, or GUI
// rendering code belongs here.
// ============================================================================
#pragma once

#include <cstdint>
#include <optional>
#include <string>

using MoneyCents = std::int64_t;

struct BranchRecord {
    int id{};
    std::string branch_name;
    std::string street_address;
    std::string district;
    std::string location_city;
    std::string location_country;
};

struct UserRecord {
    int id{};
    std::string name;
    std::string role;             // Admin / Staff / Customer
    std::string phone;
    std::optional<int> branch_id; // Database allows NULL here.
    std::string username;         // ADDED: login name; password hash never leaves database.
};

struct StationRecord {
    int id{};
    int branch_id{};
    std::string type;             // PC / PS4 / PS5
    MoneyCents hourly_rate_cents{};
    std::string status;           // Available / InUse / Maintenance
};

struct ReservationRecord {
    int id{};
    int user_id{};
    int branch_id{};
    std::string station_type;     // PC / PS4 / PS5
    std::string reserved_time;    // PostgreSQL TIMESTAMP text.
    MoneyCents deposit_cents{};
    std::string status;           // Pending / Confirmed / Canceled
};

struct SessionRecord {
    int id{};
    int station_id{};
    int user_id{};
    std::string start_time;
    std::optional<std::string> end_time;
    std::optional<MoneyCents> final_cost_cents;
};

struct ActiveSessionView {
    int session_id{};
    int station_id{};
    std::string station_type;
    int user_id{};
    std::string customer_name;
    std::string start_time;
    MoneyCents hourly_rate_cents{};
    int elapsed_minutes{};
};
