// ============================================================================
// GAME&GO - POSTGRESQL DATABASE IMPLEMENTATION
// SHARED IMPLEMENTATION FILE: MEMBER 4 + MEMBER 6
// ============================================================================

#include "Database/PostgresDB.h"
#include "Domain/Enums.h"
#include <pqxx/pqxx>
#include <optional>
#include <string>
#include <vector>
#include <exception>
#include <memory>
#include <utility>
#include "Core/Helpers.h"

// -------------------------- MEMBER 4: INFRASTRUCTURE ------------------------

IDatabase::~IDatabase() = default;

DatabaseException::DatabaseException(const std::string &message)
    : std::runtime_error(message)
{
}

PostgresDB::PostgresDB(const std::string &connection_string)
    : connection_string_(connection_string)
{
    try
    {
        connection_ =
            std::make_unique<pqxx::connection>(connection_string_);
    }
    catch (const pqxx::broken_connection &e)
    {
        throw DatabaseException(
            std::string("Unable to connect to PostgreSQL: ") + e.what());
    }
    catch (const std::exception &e)
    {
        throw DatabaseException(
            std::string("PostgreSQL connection setup failed: ") + e.what());
    }

    // ADDED: reject a local database that has not been migrated for login.
    try
    {
        pqxx::nontransaction check(*connection_);
        check.exec("SELECT username, password_hash FROM users LIMIT 0");
        check.exec("SELECT crypt('GameAndGo schema check', gen_salt('bf'))");
    }
    catch (const std::exception &e)
    {
        throw DatabaseException(std::string("Local database is not ready for username/password login. Run game_and_go_database.sql on a fresh local database or migrations/001_auth_management.sql on an existing one. Details: ") + e.what());
    }
}

PostgresDB::~PostgresDB() = default;

namespace
{

    template <class F>
    auto guardedM4(const char *operation, F &&fn) -> decltype(fn())
    {
        try
        {
            return fn();
        }
        catch (const DatabaseException &)
        {
            throw;
        }
        catch (const std::exception &e)
        {
            throw DatabaseException(
                std::string(operation) + " failed: " + e.what());
        }
    }
    const std::string kUserCols =
        "id, name, role, phone, branch_id, username";

    const std::string kBranchCols =
        "id, branch_name, street_address, district, "
        "location_city, location_country";

    const std::string kStationCols =
        "id, branch_id, type, "
        "(hourly_rate * 100)::bigint AS hourly_rate_cents, status";

    UserRecord mapUser(pqxx::row_ref row)
    {
        UserRecord record{};

        record.id = row["id"].as<int>();
        record.name = row["name"].as<std::string>();
        record.role = row["role"].as<std::string>();

        record.phone = row["phone"].is_null()
                           ? std::string{}
                           : row["phone"].as<std::string>();

        if (!row["branch_id"].is_null())
        {
            record.branch_id = row["branch_id"].as<int>();
        }
        record.username = row["username"].is_null()
                              ? std::string{}
                              : row["username"].as<std::string>();

        return record;
    }

    BranchRecord mapBranch(pqxx::row_ref row)
    {
        BranchRecord record{};

        record.id = row["id"].as<int>();
        record.branch_name = row["branch_name"].as<std::string>();
        record.street_address = row["street_address"].as<std::string>();
        record.district = row["district"].as<std::string>();
        record.location_city = row["location_city"].as<std::string>();
        record.location_country = row["location_country"].as<std::string>();

        return record;
    }

    StationRecord mapStation(pqxx::row_ref row)
    {
        StationRecord record{};

        record.id = row["id"].as<int>();
        record.branch_id = row["branch_id"].as<int>();
        record.type = row["type"].as<std::string>();
        record.hourly_rate_cents =
            row["hourly_rate_cents"].as<MoneyCents>();
        record.status = row["status"].as<std::string>();

        return record;
    }

}

// -------------------------- MEMBER 4: USERS --------------------------------

std::optional<UserRecord> PostgresDB::getUserById(int user_id)
{
    return guardedM4("getUserById",
                     [&]() -> std::optional<UserRecord>
                     {
                         pqxx::nontransaction tx(*connection_);

                         const pqxx::result result = tx.exec_params(
                             "SELECT " + kUserCols +
                                 " FROM users WHERE id = $1",
                             user_id);

                         if (result.empty())
                         {
                             return std::nullopt;
                         }

                         return mapUser(result[0]);
                     });
}

std::vector<UserRecord> PostgresDB::listUsers()
{
    return guardedM4("listUsers", [&]
                     {
        pqxx::nontransaction tx(*connection_);

        const pqxx::result result = tx.exec(
            "SELECT " + kUserCols +
            " FROM users ORDER BY id");

        std::vector<UserRecord> users;
        users.reserve(result.size());

        for (const auto& row : result) {
            users.push_back(mapUser(row));
        }

        return users; });
}

std::vector<UserRecord> PostgresDB::listUsersByRole(
    const std::string &role)
{
    return guardedM4("listUsersByRole", [&]
                     {
        pqxx::nontransaction tx(*connection_);

        const pqxx::result result = tx.exec_params(
            "SELECT " + kUserCols +
            " FROM users WHERE role = $1 ORDER BY id",
            role);

        std::vector<UserRecord> users;
        users.reserve(result.size());

        for (const auto& row : result) {
            users.push_back(mapUser(row));
        }

        return users; });
}
// ADDED: verify a password against the hash generated by pgcrypto.
std::optional<UserRecord> PostgresDB::authenticateUser(
    const std::string& username, const std::string& password)
{
    return guardedM4("authenticateUser", [&]() -> std::optional<UserRecord>
    {
        pqxx::nontransaction tx(*connection_);
        const pqxx::result result = tx.exec_params(
            "SELECT " + kUserCols + " FROM users "
            "WHERE lower(username) = lower($1) "
            "AND password_hash = crypt($2, password_hash) LIMIT 1",
            username, password);
        if (result.empty()) return std::nullopt;
        return mapUser(result[0]);
    });
}

// ADDED: save a password hash, never the submitted password itself.
int PostgresDB::createUser(const UserRecord& user, const std::string& password)
{
    return guardedM4("createUser", [&]() -> int
    {
        const int branch_id = user.branch_id.value_or(0);
        pqxx::work tx(*connection_);
        const pqxx::row inserted = tx.exec_params1(
            "INSERT INTO users (name, role, phone, branch_id, username, password_hash) "
            "VALUES ($1, $2, NULLIF($3, ''), NULLIF($4, 0), lower($5), crypt($6, gen_salt('bf'))) "
            "RETURNING id",
            user.name, user.role, user.phone, branch_id, user.username, password);
        const int new_id = inserted[0].as<int>();
        tx.commit();
        return new_id;
    });
}

// -------------------------- MEMBER 4: BRANCHES -----------------------------

std::vector<BranchRecord> PostgresDB::listBranches()
{
    return guardedM4("listBranches", [&]
                     {
        pqxx::nontransaction tx(*connection_);

        const pqxx::result result = tx.exec(
            "SELECT " + kBranchCols +
            " FROM branches ORDER BY id");

        std::vector<BranchRecord> branches;
        branches.reserve(result.size());

        for (const auto& row : result) {
            branches.push_back(mapBranch(row));
        }

        return branches; });
}

// ADDED: insert branches from the management UI.
int PostgresDB::createBranch(const BranchRecord& branch)
{
    return guardedM4("createBranch", [&]() -> int
    {
        pqxx::work tx(*connection_);
        const pqxx::row inserted = tx.exec_params1(
            "INSERT INTO branches (branch_name, street_address, district, location_city, location_country) "
            "VALUES ($1, $2, $3, $4, $5) RETURNING id",
            branch.branch_name, branch.street_address, branch.district,
            branch.location_city, branch.location_country);
        const int new_id = inserted[0].as<int>();
        tx.commit();
        return new_id;
    });
}

// -------------------------- MEMBER 4: STATIONS -----------------------------

std::vector<StationRecord> PostgresDB::listStations(
    int branch_id,
    const std::optional<std::string> &station_type)
{
    return guardedM4("listStations", [&]
                     {
        pqxx::nontransaction tx(*connection_);
        pqxx::result result;

        if (station_type.has_value()) {
            result = tx.exec_params(
                "SELECT " + kStationCols +
                " FROM stations"
                " WHERE branch_id = $1 AND type = $2"
                " ORDER BY id",
                branch_id, *station_type);
        }
        else {
            result = tx.exec_params(
                "SELECT " + kStationCols +
                " FROM stations WHERE branch_id = $1 ORDER BY id",
                branch_id);
        }

        std::vector<StationRecord> stations;
        stations.reserve(result.size());

        for (const auto& row : result) {
            stations.push_back(mapStation(row));
        }

        return stations; });
}

std::optional<StationRecord> PostgresDB::getStationById(int station_id)
{
    return guardedM4("getStationById",
                     [&]() -> std::optional<StationRecord>
                     {
                         pqxx::nontransaction tx(*connection_);

                         const pqxx::result result = tx.exec_params(
                             "SELECT " + kStationCols +
                                 " FROM stations WHERE id = $1",
                             station_id);

                         if (result.empty())
                         {
                             return std::nullopt;
                         }

                         return mapStation(result[0]);
                     });
}

bool PostgresDB::updateStationStatus(
    int station_id,
    const std::string &status)
{
    return guardedM4("updateStationStatus", [&]
                     {
        if (status != "Available" &&
            status != "InUse" &&
            status != "Maintenance") {
            throw DatabaseException(
                "updateStationStatus: invalid station status");
        }

        pqxx::work tx(*connection_);

        const pqxx::result result = tx.exec_params(
            "UPDATE stations SET status = $2 WHERE id = $1",
            station_id, status);

        const bool updated = result.affected_rows() > 0;
        tx.commit();

        return updated; });
}

// ADDED: insert stations from the management UI.
int PostgresDB::createStation(const StationRecord& station)
{
    return guardedM4("createStation", [&]() -> int
    {
        const std::string status = station.status.empty() ? "Available" : station.status;
        pqxx::work tx(*connection_);
        const pqxx::row inserted = tx.exec_params1(
            "INSERT INTO stations (branch_id, type, hourly_rate, status) "
            "VALUES ($1, $2, ($3::bigint)::numeric / 100, $4) RETURNING id",
            station.branch_id, station.type, station.hourly_rate_cents, status);
        const int new_id = inserted[0].as<int>();
        tx.commit();
        return new_id;
    });
}

// -------------------------- MEMBER 6: DATA FLOWS ----------------------------

namespace
{

    constexpr const char *kSessionTsFormat = "'YYYY-MM-DD HH24:MI:SS'";

    template <class F>
    auto guarded(const char *operation, F &&fn) -> decltype(fn())
    {
        try
        {
            return fn();
        }
        catch (const DatabaseException &)
        {
            throw;
        }
        catch (const std::exception &e)
        {
            throw DatabaseException(std::string(operation) + " failed: " + e.what());
        }
    }

    std::optional<std::string> optText(pqxx::field_ref f)
    {
        if (f.is_null())
            return std::nullopt;
        return f.as<std::string>();
    }

    const std::string kReservationCols =
        "id, user_id, branch_id, station_type, "
        "to_char(reserved_time, " +
        std::string(kSessionTsFormat) + ") AS reserved_time, "
                                        "(deposit_amount * 100)::bigint AS deposit_cents, status";

    ReservationRecord mapReservation(pqxx::row_ref r)
    {
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
        "to_char(start_time, " +
        std::string(kSessionTsFormat) + ") AS start_time, "
                                        "to_char(end_time, " +
        std::string(kSessionTsFormat) + ") AS end_time, "
                                        "(final_cost * 100)::bigint AS final_cost_cents";

    SessionRecord mapSession(pqxx::row_ref r)
    {
        SessionRecord rec;
        rec.id = r["id"].as<int>();
        rec.station_id = r["station_id"].as<int>();
        rec.user_id = r["user_id"].as<int>();
        rec.start_time = r["start_time"].as<std::string>();
        rec.end_time = optText(r["end_time"]);
        if (!r["final_cost_cents"].is_null())
            rec.final_cost_cents = r["final_cost_cents"].as<MoneyCents>();
        return rec;
    }

} // namespace

int PostgresDB::createReservation(const ReservationRecord &record)
{
    return guarded("createReservation", [&]
                   {
        if (parseStationType(record.station_type) == StationType::Unknown)
            throw DatabaseException("createReservation: unknown station type");
        if (parseReservationStatus(record.status) == ReservationStatus::Unknown)
            throw DatabaseException("createReservation: unknown reservation status");

        pqxx::work tx(*connection_);
        const pqxx::row row = tx.exec_params1(
            "INSERT INTO reservations "
            "(user_id, branch_id, station_type, reserved_time, deposit_amount, status) "
            "VALUES ($1, $2, $3, $4::timestamp, ($5::bigint)::numeric / 100, $6) "
            "RETURNING id",
            record.user_id, record.branch_id, record.station_type,
            record.reserved_time, record.deposit_cents, record.status);
        tx.commit();
        return row[0].as<int>(); });
}

std::optional<ReservationRecord> PostgresDB::getReservationById(int reservation_id)
{
    return guarded("getReservationById", [&]() -> std::optional<ReservationRecord>
                   {
        pqxx::nontransaction tx(*connection_);
        const pqxx::result res = tx.exec_params(
            "SELECT " + kReservationCols + " FROM reservations WHERE id = $1", reservation_id);
        if (res.empty()) return std::nullopt;
        return mapReservation(res[0]); });
}

std::vector<ReservationRecord> PostgresDB::listReservations(
    int branch_id,
    const std::optional<std::string> &status)
{

    return guarded("listReservations", [&]
                   {
        std::optional<std::string> status_text;
        if (status) {
            if (parseReservationStatus(*status) == ReservationStatus::Unknown)
                throw DatabaseException("listReservations: unknown reservation status filter");
            status_text = *status;
        }

        pqxx::nontransaction tx(*connection_);
        const pqxx::result res = tx.exec_params(
            "SELECT " + kReservationCols + " FROM reservations "
            "WHERE branch_id = $1 AND ($2::text IS NULL OR status = $2::text) "
            "ORDER BY reserved_time, id",
            branch_id, status_text);

        std::vector<ReservationRecord> out;
        out.reserve(res.size());
        for (const auto& row : res) out.push_back(mapReservation(row));
        return out; });
}

bool PostgresDB::updateReservationStatus(
    int reservation_id,
    const std::string &status)
{

    return guarded("updateReservationStatus", [&]
                   {
        if (parseReservationStatus(status) == ReservationStatus::Unknown)
            throw DatabaseException("updateReservationStatus: refusing to persist Unknown status");

        pqxx::work tx(*connection_);
        const pqxx::result res = tx.exec_params(
            "UPDATE reservations SET status = $2 WHERE id = $1", reservation_id, status);
        tx.commit();
        return res.affected_rows() > 0; });
}

std::vector<ActiveSessionView> PostgresDB::listActiveSessions()
{
    return guarded("listActiveSessions", [&]
                   {
        pqxx::nontransaction tx(*connection_);
        const pqxx::result res = tx.exec(
            "SELECT s.id AS session_id, s.station_id, st.type AS station_type, "
            "       s.user_id, u.name AS customer_name, "
            "       to_char(s.start_time, " + std::string(kSessionTsFormat) + ") AS start_time, "
            "       (st.hourly_rate * 100)::bigint AS hourly_rate_cents "
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
            v.elapsed_minutes = calculateElapsedMinutes(v.start_time, std::nullopt);

            out.push_back(std::move(v));
        }
        return out; });
}

std::optional<SessionRecord> PostgresDB::getSessionById(int session_id)
{
    return guarded("getSessionById", [&]() -> std::optional<SessionRecord>
                   {
        pqxx::nontransaction tx(*connection_);
        const pqxx::result res = tx.exec_params(
            "SELECT " + kSessionCols + " FROM sessions WHERE id = $1", session_id);
        if (res.empty()) return std::nullopt;
        return mapSession(res[0]); });
}

int PostgresDB::startSession(const SessionRecord &record)
{
    return guarded("startSession", [&]
                   {
        pqxx::work tx(*connection_);

        const pqxx::result station = tx.exec_params(
            "SELECT status FROM stations WHERE id = $1 FOR UPDATE", record.station_id);
        if (station.empty()) return -1;
        if (parseStationStatus(station[0][0].as<std::string>()) != StationStatus::Available) return -1;

        const std::string start_text =
            record.start_time.empty() ? nowUtcTimestamp() : record.start_time;

        const int session_id = tx.exec_params1(
            "INSERT INTO sessions (station_id, user_id, start_time) "
            "VALUES ($1, $2, $3::timestamp) "
            "RETURNING id",
            record.station_id, record.user_id, start_text)[0].as<int>();

        tx.exec_params("UPDATE stations SET status = $2 WHERE id = $1",
                       record.station_id, toString(StationStatus::InUse));

        tx.commit();
        return session_id; });
}

bool PostgresDB::finishSession(int session_id, const SessionRecord &record)
{
    return guarded("finishSession", [&]
                   {
        if (!record.final_cost_cents)
            throw DatabaseException("finishSession: final_cost_cents is required");

        pqxx::work tx(*connection_);

        const pqxx::result active = tx.exec_params(
            "SELECT station_id FROM sessions WHERE id = $1 AND end_time IS NULL FOR UPDATE",
            session_id);
        if (active.empty()) return false;
        const int station_id = active[0][0].as<int>();

        const std::string end_text =
            !record.end_time || record.end_time->empty()
                ? nowUtcTimestamp()
                : *record.end_time;

        const pqxx::result upd = tx.exec_params(
            "UPDATE sessions "
            "SET end_time = $2::timestamp, "
            "    final_cost = ($3::bigint)::numeric / 100 "
            "WHERE id = $1 "
            "  AND $2::timestamp >= start_time",
            session_id, end_text, *record.final_cost_cents);
        if (upd.affected_rows() == 0)
            throw DatabaseException("finishSession: end_time is earlier than start_time");

        tx.exec_params("UPDATE stations SET status = $2 WHERE id = $1",
                       station_id, toString(StationStatus::Available));

        tx.commit();
        return true; });
}