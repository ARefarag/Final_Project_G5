// ============================================================================
// GAME&GO - POSTGRESQL DATABASE ADAPTER
// OWNER: MEMBER 4 + MEMBER 6
//
// MEMBER 4:
//   - DatabaseException and connection lifecycle.
//   - pqxx boundary and row-mapping helpers.
//   - User, Branch, and Station database operations.
// MEMBER 6:
//   - Reservation and Session database operations.
//   - Start/finish session transaction logic.
//
// IMPORTANT ARCHITECTURE RULE:
//   This header intentionally does NOT include pqxx. The PostgreSQL-specific
//   implementation belongs in PostgresDB.cpp so the rest of the application
//   depends only on IDatabase and the shared records.
// ============================================================================
#pragma once

#include "IDatabase.h"
#include <memory>
#include <stdexcept>
#include <string>

namespace pqxx {
class connection;
}

class DatabaseException : public std::runtime_error {
public:
    // Implement an application-level wrapper for DB failures.
    explicit DatabaseException(const std::string& message);
};

class PostgresDB final : public IDatabase {
private:
    std::unique_ptr<pqxx::connection> connection_;
    std::string connection_string_;

    // Add mapping helpers in the .cpp file for pqxx row -> records.
    // Member 6 will use these helpers for reservation/session rows.
    // Do not expose pqxx types outside this class.

public:
    // Open PostgreSQL connection and validate it.
    explicit PostgresDB(const std::string& connection_string);

    // Release connection cleanly. Define in PostgresDB.cpp.
    ~PostgresDB() override;

    // ---------------------- MEMBER 4: DATABASE CORE -------------------------
    // Implement User, Branch, and Station operations with
    // parameterized SQL and the shared-record mapping helpers.
    std::optional<UserRecord> getUserById(int user_id) override;
    std::vector<UserRecord> listUsers() override;
    std::vector<UserRecord> listUsersByRole(const std::string& role) override;
    std::vector<BranchRecord> listBranches() override;
    std::vector<StationRecord> listStations(
        int branch_id,
        const std::optional<std::string>& station_type) override;
    std::optional<StationRecord> getStationById(int station_id) override;
    bool updateStationStatus(int station_id,
                             const std::string& status) override;

    // ---------------------- MEMBER 6: APP DATA FLOWS ------------------------
    // Implement Reservation and Session operations with
    // parameterized SQL using Member 4's mapping helpers.
    int createReservation(const ReservationRecord& reservation) override;
    std::optional<ReservationRecord> getReservationById(
        int reservation_id) override;
    std::vector<ReservationRecord> listReservations(
        int branch_id,
        const std::optional<std::string>& status) override;
    bool updateReservationStatus(int reservation_id,
                                 const std::string& status) override;
    std::vector<ActiveSessionView> listActiveSessions() override;
    std::optional<SessionRecord> getSessionById(int session_id) override;

    // Implement transaction-safe session start/finish operations.
    // Both station and session changes must commit or roll back together.
    int startSession(const SessionRecord& session) override;
    bool finishSession(int session_id,
                       const SessionRecord& finished_session) override;
};
