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
    // TODO [M4]: SELECT by primary key and map one row to UserRecord.
    virtual std::optional<UserRecord> getUserById(int user_id) = 0;

    // TODO [M4]: SELECT all users for management/reporting.
    virtual std::vector<UserRecord> listUsers() = 0;

    // TODO [M4]: SELECT users filtered by role.
    virtual std::vector<UserRecord> listUsersByRole(const std::string& role) = 0;

    // ------------------------- BRANCHES [M4] --------------------------------
    // TODO [M4]: SELECT all branches in a predictable order.
    virtual std::vector<BranchRecord> listBranches() = 0;

    // ------------------------- STATIONS [M4] --------------------------------
    // TODO [M4]: SELECT stations for a branch, optionally filtered by type.
    virtual std::vector<StationRecord> listStations(
        int branch_id,
        const std::optional<std::string>& station_type) = 0;

    // TODO [M4]: SELECT one station by primary key.
    virtual std::optional<StationRecord> getStationById(int station_id) = 0;

    // TODO [M4]: UPDATE station status and return whether a row changed.
    virtual bool updateStationStatus(int station_id,
                                     const std::string& status) = 0;

    // ------------------------- RESERVATIONS [M6] ----------------------------
    // TODO [M6]: INSERT reservation and return generated SERIAL id.
    virtual int createReservation(const ReservationRecord& reservation) = 0;

    // TODO [M6]: SELECT one reservation by primary key.
    virtual std::optional<ReservationRecord> getReservationById(
        int reservation_id) = 0;

    // TODO [M6]: SELECT branch reservations, optionally filtered by status.
    virtual std::vector<ReservationRecord> listReservations(
        int branch_id,
        const std::optional<std::string>& status) = 0;

    // TODO [M6]: UPDATE reservation status after core validation.
    virtual bool updateReservationStatus(int reservation_id,
                                         const std::string& status) = 0;

    // ------------------------- SESSIONS [M6] --------------------------------
    // TODO [M6]: JOIN sessions/stations/users to build active-session rows.
    virtual std::vector<ActiveSessionView> listActiveSessions() = 0;

    // TODO [M6]: SELECT one session by primary key.
    virtual std::optional<SessionRecord> getSessionById(int session_id) = 0;

    // TODO [M6]: INSERT a session and change station status atomically.
    //             This transaction belongs to Member 6's session-persistence section.
    virtual int startSession(const SessionRecord& session) = 0;

    // TODO [M6]: Update session and release station atomically.
    //             This transaction belongs to Member 6's session-persistence section.
    virtual bool finishSession(int session_id,
                               const SessionRecord& finished_session) = 0;
};
