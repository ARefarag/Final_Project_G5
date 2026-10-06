// ============================================================================
// GAME&GO - POSTGRESQL DATABASE IMPLEMENTATION
// SHARED IMPLEMENTATION FILE: MEMBER 4 + MEMBER 6
//
// MEMBER 4 owns the infrastructure section and the User/Branch/Station methods.
// MEMBER 6 owns the Reservation/Session methods and session transactions.
// Keep each member's work inside the labeled sections below to reduce Git
// conflicts. Both members may use the pqxx types in THIS .cpp file only.
// ============================================================================
#include "Database/PostgresDB.h"
   #include <pqxx/pqxx>
   #include <initializer_list>
   #include <optional>
   #include <string>
   #include <vector>
// -------------------------- MEMBER 4: INFRASTRUCTURE ------------------------
// TODO [M4]: Include <pqxx/pqxx> in THIS .cpp file only.
// TODO [M4]: Implement DatabaseException, PostgresDB constructor/destructor,
//             and low-level pqxx row -> shared-record mapping helpers.
// TODO [M4]: Implement User, Branch, and Station database operations here.
// TODO [M4]: Use parameterized queries and preserve NULL/empty-result behavior
//             required by IDatabase and the SRS.
// TODO [M4]: Do not move pqxx types into any UI/core header.

// -------------------------- MEMBER 6: DATA FLOWS ----------------------------
namespace m6 {

constexpr const char* kTsFormat = "'YYYY-MM-DD HH24:MI:SS'";

template <class F>
auto guarded(const char* operation, F&& fn) -> decltype(fn()) {
    try {
        return fn();
    } catch (const DatabaseException&) {
        throw;
    } catch (const std::exception& e) {
        throw DatabaseException(std::string(operation) + " failed: " + e.what());
    }
}

inline bool oneOf(const std::string& v, std::initializer_list<const char*> allowed) {
    for (const char* a : allowed)
        if (v == a) return true;
    return false;
}

inline bool validStationType(const std::string& v) { return oneOf(v, {"PC", "PS4", "PS5"}); }
inline bool validReservationStatus(const std::string& v) {
    return oneOf(v, {"Pending", "Confirmed", "Canceled"});
}

const std::string kReservationCols =
    "id, user_id, branch_id, station_type, "
    "to_char(reserved_time, " + std::string(kTsFormat) + ") AS reserved_time, "
    "ROUND(deposit_amount * 100)::bigint AS deposit_cents, status";

inline ReservationRecord mapReservation(const pqxx::row& r) {
    ReservationRecord rec;
    rec.id = r["id"].as<int>();
    rec.user_id = r["user_id"].as<int>();
    rec.branch_id = r["branch_id"].as<int>();
    rec.station_type = r["station_type"].as<std::string>();
    rec.reserved_time = r["reserved_time"].as<std::string>();
    rec.deposit_cents = r["deposit_cents"].as<MoneyCents>();
    rec.status = r["status"].as<std::string>();
    return rec;
}

const std::string kSessionCols =
    "id, station_id, user_id, "
    "to_char(start_time, " + std::string(kTsFormat) + ") AS start_time, "
    "to_char(end_time, " + std::string(kTsFormat) + ") AS end_time, "
    "ROUND(final_cost * 100)::bigint AS final_cost_cents";

inline SessionRecord mapSession(const pqxx::row& r) {
    SessionRecord rec;
    rec.id = r["id"].as<int>();
    rec.station_id = r["station_id"].as<int>();
    rec.user_id = r["user_id"].as<int>();
    rec.start_time = r["start_time"].as<std::string>();
    if (!r["end_time"].is_null()) rec.end_time = r["end_time"].as<std::string>();
    if (!r["final_cost_cents"].is_null())
        rec.final_cost_cents = r["final_cost_cents"].as<MoneyCents>();
    return rec;
}

}  // namespace m6

// ------------------------------- Reservations -------------------------------

int PostgresDB::createReservation(const ReservationRecord& reservation) {
    return m6::guarded("createReservation", [&]() -> int {
        if (!m6::validStationType(reservation.station_type))
            throw DatabaseException("createReservation: invalid station type");
        if (!m6::validReservationStatus(reservation.status))
            throw DatabaseException("createReservation: invalid reservation status");
        if (reservation.deposit_cents < 0)
            throw DatabaseException("createReservation: negative deposit");

        pqxx::work tx(*connection_);
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
    return m6::guarded("getReservationById", [&]() -> std::optional<ReservationRecord> {
        pqxx::nontransaction tx(*connection_);
        const pqxx::result res = tx.exec_params(
            "SELECT " + m6::kReservationCols + " FROM reservations WHERE id = $1",
            reservation_id);
        if (res.empty()) return std::nullopt;
        return m6::mapReservation(res[0]);
    });
}

std::vector<ReservationRecord> PostgresDB::listReservations(
    int branch_id, const std::optional<std::string>& status) {
    return m6::guarded("listReservations", [&]() -> std::vector<ReservationRecord> {
        if (status && !m6::validReservationStatus(*status))
            throw DatabaseException("listReservations: invalid status filter");

        pqxx::nontransaction tx(*connection_);
        const pqxx::result res = tx.exec_params(
            "SELECT " + m6::kReservationCols + " FROM reservations "
            "WHERE branch_id = $1 AND ($2::text IS NULL OR status = $2::text) "
            "ORDER BY reserved_time, id",
            branch_id, status);

        std::vector<ReservationRecord> out;
        out.reserve(res.size());
        for (const auto& row : res) out.push_back(m6::mapReservation(row));
        return out;
    });
}

bool PostgresDB::updateReservationStatus(int reservation_id, const std::string& status) {
    return m6::guarded("updateReservationStatus", [&]() -> bool {
        if (!m6::validReservationStatus(status))
            throw DatabaseException("updateReservationStatus: invalid status");

        pqxx::work tx(*connection_);
        const pqxx::result res = tx.exec_params(
            "UPDATE reservations SET status = $2 WHERE id = $1", reservation_id, status);
        tx.commit();
        return res.affected_rows() > 0;
    });
}

// -------------------------------- Sessions ----------------------------------

std::vector<ActiveSessionView> PostgresDB::listActiveSessions() {
    return m6::guarded("listActiveSessions", [&]() -> std::vector<ActiveSessionView> {
        pqxx::nontransaction tx(*connection_);
        const pqxx::result res = tx.exec(
            "SELECT s.id AS session_id, s.station_id, st.type AS station_type, "
            "       s.user_id, u.name AS customer_name, "
            "       to_char(s.start_time, " + std::string(m6::kTsFormat) + ") AS start_time, "
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
    return m6::guarded("getSessionById", [&]() -> std::optional<SessionRecord> {
        pqxx::nontransaction tx(*connection_);
        const pqxx::result res = tx.exec_params(
            "SELECT " + m6::kSessionCols + " FROM sessions WHERE id = $1", session_id);
        if (res.empty()) return std::nullopt;
        return m6::mapSession(res[0]);
    });
}

// Returns new session id, or -1 if the station is missing / not Available.
int PostgresDB::startSession(const SessionRecord& session) {
    return m6::guarded("startSession", [&]() -> int {
        pqxx::work tx(*connection_);

        // verify station exists + is Available (row locked)
        const pqxx::result station = tx.exec_params(
            "SELECT status FROM stations WHERE id = $1 FOR UPDATE", session.station_id);
        if (station.empty()) return -1;
        if (station[0][0].as<std::string>() != "Available") return -1;

        // insert active session; empty start_time => database clock
        const int session_id = tx.exec_params1(
            "INSERT INTO sessions (station_id, user_id, start_time) "
            "VALUES ($1, $2, COALESCE(NULLIF($3::text, '')::timestamp, LOCALTIMESTAMP)) "
            "RETURNING id",
            session.station_id, session.user_id, session.start_time)[0].as<int>();

        // station -> InUse
        tx.exec_params("UPDATE stations SET status = 'InUse' WHERE id = $1",
                       session.station_id);

        tx.commit();
        return session_id;
    });
}

// Returns false if the session does not exist or is already finished.
bool PostgresDB::finishSession(int session_id, const SessionRecord& finished_session) {
    return m6::guarded("finishSession", [&]() -> bool {
        if (!finished_session.final_cost_cents)
            throw DatabaseException("finishSession: final_cost_cents is required");
        if (*finished_session.final_cost_cents < 0)
            throw DatabaseException("finishSession: negative final cost");

        pqxx::work tx(*connection_);

        // verify session is still active (row locked)
        const pqxx::result active = tx.exec_params(
            "SELECT station_id FROM sessions WHERE id = $1 AND end_time IS NULL FOR UPDATE",
            session_id);
        if (active.empty()) return false;
        const int station_id = active[0][0].as<int>();

        // store end_time + final_cost (end must not precede start)
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

        // release station
        tx.exec_params("UPDATE stations SET status = 'Available' WHERE id = $1", station_id);

        tx.commit();
        return true;
    });
}
