// ============================================================================
// GAME&GO - POSTGRESQL DATABASE ADAPTER
// OWNER: MEMBER 4
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
    // TODO [M4]: Implement an application-level wrapper for DB failures.
    explicit DatabaseException(const std::string& message);
};

class PostgresDB final : public IDatabase {
private:
    std::unique_ptr<pqxx::connection> connection_;
    std::string connection_string_;

    // TODO [M4]: Add mapping helpers in the .cpp file for pqxx row -> records.
    //             Do not expose pqxx types outside this class.

public:
    // TODO [M4]: Open PostgreSQL connection and validate it.
    explicit PostgresDB(const std::string& connection_string);

    // TODO [M4]: Release connection cleanly. Define in PostgresDB.cpp.
    ~PostgresDB() override;

    // TODO [M4]: Implement all IDatabase methods with parameterized SQL.
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
    int startSession(const SessionRecord& session) override;
    bool finishSession(int session_id,
                       const SessionRecord& finished_session) override;
};
