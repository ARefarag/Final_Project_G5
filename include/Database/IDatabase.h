// ============================================================================
// GAME&GO - DATABASE ABSTRACTION
// OWNER: SHARED CONTRACT / DATABASE IMPLEMENTATION: MEMBER 4 + MEMBER 6
//
// MEMBER 4 owns the database infrastructure-facing operations:
//   - User, Branch, and Station reads/updates.
// MEMBER 6 owns reservation and session persistence operations:
//   - Reservation CRUD/lifecycle and Session queries/transactions.
//
// Treat this file as a shared contract. Any signature/type change requires
// agreement because SessionManager and both PostgresDB implementation sections
// depend on it.
// ============================================================================
#pragma once

#include "../CoreData.h"
#include <optional>
#include <string>
#include <vector>

class DatabaseException;

class IDatabase {
public:
    virtual ~IDatabase();

    // ------------------------- USERS [M4] -----------------------------------
    virtual std::optional<UserRecord> getUserById(int user_id) = 0;

    virtual std::vector<UserRecord> listUsers() = 0;

    virtual std::vector<UserRecord> listUsersByRole(const std::string& role) = 0;

    // ------------------------- BRANCHES [M4] --------------------------------
    virtual std::vector<BranchRecord> listBranches() = 0;

    // ------------------------- STATIONS [M4] --------------------------------
    virtual std::vector<StationRecord> listStations(
        int branch_id,
        const std::optional<std::string>& station_type) = 0;

    virtual std::optional<StationRecord> getStationById(int station_id) = 0;

    virtual bool updateStationStatus(int station_id,
                                     const std::string& status) = 0;

    // ------------------------- RESERVATIONS [M6] ----------------------------
    virtual int createReservation(const ReservationRecord& reservation) = 0;

    virtual std::optional<ReservationRecord> getReservationById(
        int reservation_id) = 0;

    virtual std::vector<ReservationRecord> listReservations(
        int branch_id,
        const std::optional<std::string>& status) = 0;

    virtual bool updateReservationStatus(int reservation_id,
                                         const std::string& status) = 0;

    // ------------------------- SESSIONS [M6] --------------------------------
    virtual std::vector<ActiveSessionView> listActiveSessions() = 0;

    virtual std::optional<SessionRecord> getSessionById(int session_id) = 0;

    virtual int startSession(const SessionRecord& session) = 0;

    virtual bool finishSession(int session_id,
                               const SessionRecord& finished_session) = 0;
};
