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
    int PostgresDB::createReservation(const ReservationRecord& reservation) {
    return guarded("createReservation", [&] {
        if (!validStationType(reservation.station_type))
            throw DatabaseException("createReservation: invalid station type");
        if (!validReservationStatus(reservation.status))
            throw DatabaseException("createReservation: invalid reservation status");
        if (reservation.deposit_cents < 0)
            throw DatabaseException("createReservation: negative deposit");

        pqxx::work tx(*conn_);
        const pqxx::row row = tx.exec_params1(
            "INSERT INTO reservations "
            "(user_id, branch_id, station_type, reserved_time, deposit_amount, status) "
            "VALUES ($1, $2, $3, $4::timestamp, ($5::bigint)::numeric / 100, $6) "
            "RETURNING id",
            reservation.user_id, reservation.branch_id, reservation.station_type,
            reservation.reserved_time, reservation.deposit_cents, reservation.status);
        tx.commit();
        return row[0].as<int>();
    });
}
std::optional<ReservationRecord> PostgresDB::getReservationById(int reservation_id) {
    return guarded("getReservationById", [&]() -> std::optional<ReservationRecord> {
        pqxx::nontransaction tx(*conn_);
        const pqxx::result res = tx.exec_params(
            "SELECT " + kReservationCols + " FROM reservations WHERE id = $1", reservation_id);
        if (res.empty()) return std::nullopt;
        return mapReservation(res[0]);
    });
}

std::vector<ReservationRecord> PostgresDB::listReservations(
    int branch_id, const std::optional<std::string>& status) {
    return guarded("listReservations", [&] {
        if (status && !validReservationStatus(*status))
            throw DatabaseException("listReservations: invalid status filter");

        pqxx::nontransaction tx(*conn_);
        const pqxx::result res = tx.exec_params(
            "SELECT " + kReservationCols + " FROM reservations "
            "WHERE branch_id = $1 AND ($2::text IS NULL OR status = $2::text) "
            "ORDER BY reserved_time, id",
            branch_id, status);

        std::vector<ReservationRecord> out;
        out.reserve(res.size());
        for (const auto& row : res) out.push_back(mapReservation(row));
        return out;
    });
}

bool PostgresDB::updateReservationStatus(int reservation_id, const std::string& status) {
    return guarded("updateReservationStatus", [&] {
        if (!validReservationStatus(status))
            throw DatabaseException("updateReservationStatus: invalid status");

        pqxx::work tx(*conn_);
        const pqxx::result res = tx.exec_params(
            "UPDATE reservations SET status = $2 WHERE id = $1", reservation_id, status);
        tx.commit();
        return res.affected_rows() > 0;
    });
}
    // ------------------------- SESSIONS [M6] --------------------------------
    std::vector<ActiveSessionView> PostgresDB::listActiveSessions() {
    return guarded("listActiveSessions", [&] {
        pqxx::nontransaction tx(*conn_);
        const pqxx::result res = tx.exec(
            "SELECT s.id AS session_id, s.station_id, st.type AS station_type, "
            "       s.user_id, u.name AS customer_name, "
            "       to_char(s.start_time, " + std::string(kTsFormat) + ") AS start_time, "
            "       ROUND(st.hourly_rate * 100)::bigint AS hourly_rate_cents, "
            "       GREATEST(0, FLOOR(EXTRACT(EPOCH FROM (LOCALTIMESTAMP - s.start_time)) / 60))::int "
            "         AS elapsed_minutes "
            "FROM sessions s "
            "JOIN stations st ON st.id = s.station_id "
            "JOIN users u ON u.id = s.user_id "
            "WHERE s.end_time IS NULL "
            "ORDER BY s.start_time, s.id");

        std::vector<ActiveSessionView> out;
        out.reserve(res.size());
        for (const auto& r : res) {
            ActiveSessionView v;
            v.session_id = r["session_id"].as<int>();
            v.station_id = r["station_id"].as<int>();
            v.station_type = r["station_type"].as<std::string>();
            v.user_id = r["user_id"].as<int>();
            v.customer_name = r["customer_name"].as<std::string>();
            v.start_time = r["start_time"].as<std::string>();
            v.hourly_rate_cents = r["hourly_rate_cents"].as<MoneyCents>();
            v.elapsed_minutes = r["elapsed_minutes"].as<int>();
            out.push_back(std::move(v));
        }
        return out;
    });
}

std::optional<SessionRecord> PostgresDB::getSessionById(int session_id) {
    return guarded("getSessionById", [&]() -> std::optional<SessionRecord> {
        pqxx::nontransaction tx(*conn_);
        const pqxx::result res = tx.exec_params(
            "SELECT " + kSessionCols + " FROM sessions WHERE id = $1", session_id);
        if (res.empty()) return std::nullopt;
        return mapSession(res[0]);
    });
}

// Returns new session id, or -1 if the station is missing / not Available.
int PostgresDB::startSession(const SessionRecord& session) {
    return guarded("startSession", [&] {
        pqxx::work tx(*conn_);

        // 1-2. verify station exists and is Available (row locked)
        const pqxx::result station = tx.exec_params(
            "SELECT status FROM stations WHERE id = $1 FOR UPDATE", session.station_id);
        if (station.empty()) return -1;
        if (station[0][0].as<std::string>() != "Available") return -1;

        // 3. insert active session (end_time / final_cost stay NULL);
        //    empty start_time => database clock
        const int session_id = tx.exec_params1(
            "INSERT INTO sessions (station_id, user_id, start_time) "
            "VALUES ($1, $2, COALESCE(NULLIF($3::text, '')::timestamp, LOCALTIMESTAMP)) "
            "RETURNING id",
            session.station_id, session.user_id, session.start_time)[0].as<int>();

        // 4. station -> InUse
        tx.exec_params("UPDATE stations SET status = 'InUse' WHERE id = $1", session.station_id);

        tx.commit();
        return session_id;
    });
}

// Returns false if the session does not exist or is already finished.
bool PostgresDB::finishSession(int session_id, const SessionRecord& finished_session) {
    return guarded("finishSession", [&] {
        if (!finished_session.final_cost_cents)
            throw DatabaseException("finishSession: final_cost_cents is required");
        if (*finished_session.final_cost_cents < 0)
            throw DatabaseException("finishSession: negative final cost");

        pqxx::work tx(*conn_);

        // 1. verify session is still active (row locked)
        const pqxx::result active = tx.exec_params(
            "SELECT station_id FROM sessions WHERE id = $1 AND end_time IS NULL FOR UPDATE",
            session_id);
        if (active.empty()) return false;
        const int station_id = active[0][0].as<int>();

        // 2. store end_time + final_cost (end must not precede start)
        const std::string end_text = finished_session.end_time.value_or("");
        const pqxx::result upd = tx.exec_params(
            "UPDATE sessions "
            "SET end_time = COALESCE(NULLIF($2::text, '')::timestamp, LOCALTIMESTAMP), "
            "    final_cost = ($3::bigint)::numeric / 100 "
            "WHERE id = $1 "
            "  AND COALESCE(NULLIF($2::text, '')::timestamp, LOCALTIMESTAMP) >= start_time",
            session_id, end_text, *finished_session.final_cost_cents);
        if (upd.affected_rows() == 0)
            throw DatabaseException("finishSession: end_time is earlier than start_time");

        // 3. release station
        tx.exec_params("UPDATE stations SET status = 'Available' WHERE id = $1", station_id);

        tx.commit();
        return true;
    });
}
