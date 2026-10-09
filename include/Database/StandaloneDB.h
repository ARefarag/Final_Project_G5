#pragma once

#include "IDatabase.h"
#include "../Core/Helpers.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <string>
#include <functional>
#include <cctype>

class StandaloneDB final : public IDatabase
{
private:
    std::vector<BranchRecord> branches_;
    std::vector<UserRecord> users_;
    std::vector<StationRecord> stations_;
    std::vector<ReservationRecord> reservations_;
    std::vector<SessionRecord> sessions_;
    int next_res_id_{4};
    int next_session_id_{7};
    int next_user_id_{15};
    int next_branch_id_{4};
    int next_station_id_{19};
    // ADDED: ephemeral password digests used only by the in-memory fallback.
    // The real local PostgreSQL backend uses pgcrypto bcrypt hashes.
    std::unordered_map<std::string, std::string> password_digests_;

    static std::string normalizeUsername(std::string value)
    {
        for (char& ch : value)
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        return value;
    }

    static std::string demoPasswordDigest(const std::string& password)
    {
        // Development-only in-memory backend: std::hash is not a production password hash.
        return std::to_string(std::hash<std::string>{}(password));
    }

public:
    StandaloneDB()
    {
        std::cout << "[STANDALONE DB] Initializing in-memory fallback database...\n";

        // Seed Branches
        branches_ = {
            {1, "dev-Main Branch", "111 Sidi-gaber St", "Smouha", "Alexandria", "Egypt"},
            {2, "dev-Uptown Branch", "456 El-eqbal St", "Victoria", "Alexandria", "Egypt"},
            {3, "dev-Maadi Branch", "789 Maadi St", "Maadi", "Cairo", "Egypt"}};

        // Seed Users (same public development accounts as the local SQL script).
        users_ = {
            {1, "dev-Alice Admin", "Admin", "123-456-7890", std::nullopt, "alice.admin"},
            {2, "dev-John Admin", "Admin", "987-654-3210", std::nullopt, "john.admin"},
            {3, "dev-Jane Admin", "Admin", "555-555-5555", std::nullopt, "jane.admin"},
            {4, "dev-Bob Staff", "Staff", "234-567-8901", 1, "bob.staff"},
            {5, "dev-Malak Staff", "Staff", "342-578-9012", 1, "malak.staff"},
            {6, "dev-Eve Staff", "Staff", "567-890-1234", 2, "eve.staff"},
            {7, "dev-Frank Staff", "Staff", "678-901-2345", 2, "frank.staff"},
            {8, "dev-Grace Staff", "Staff", "789-012-3456", 3, "grace.staff"},
            {9, "dev-Heidi Staff", "Staff", "890-123-4567", 3, "heidi.staff"},
            {10, "dev-Charlie Customer", "Customer", "345-678-9012", 1, "charlie.customer"},
            {11, "dev-Adam Customer", "Customer", "012-759-880", 1, "adam.customer"},
            {12, "dev-Sandy Customer", "Customer", "015-888-333", 2, "sandy.customer"},
            {13, "dev-Oscar Customer", "Customer", "019-777-444", 3, "oscar.customer"},
            {14, "dev-Diana Customer", "Customer", "456-789-0123", 3, "diana.customer"}};

        for (const auto& user : users_)
            password_digests_[normalizeUsername(user.username)] = demoPasswordDigest("Welcome123!");

        // Seed Stations (18 stations, across all three demo branches).
        stations_ = {
            {1, 1, "PC", 1000, "Available"}, {2, 1, "PC", 1000, "InUse"},
            {3, 1, "PS4", 1200, "Available"}, {4, 1, "PS4", 1200, "Available"},
            {5, 1, "PS5", 1500, "Maintenance"}, {6, 1, "PS5", 1500, "Available"},
            {7, 2, "PC", 1000, "Available"}, {8, 2, "PC", 1000, "Available"},
            {9, 2, "PS4", 1200, "Available"}, {10, 2, "PS4", 1200, "InUse"},
            {11, 2, "PS5", 1500, "InUse"}, {12, 2, "PS5", 1500, "Maintenance"},
            {13, 3, "PC", 1000, "Available"}, {14, 3, "PC", 1000, "Available"},
            {15, 3, "PS4", 1200, "Available"}, {16, 3, "PS4", 1200, "Available"},
            {17, 3, "PS5", 1500, "InUse"}, {18, 3, "PS5", 1500, "Available"}};

        // Seed Reservations
        reservations_ = {
            {1, 10, 1, "PC", "2026-06-01 10:00:00", 500, "Pending"},
            {2, 11, 2, "PS4", "2026-06-02 14:00:00", 600, "Confirmed"}};

        // Seed Active Sessions
        sessions_ = {
            {2, 2, 11, "2026-06-02 14:00:00", std::nullopt, std::nullopt}};
    }

    // --- USERS ---
    std::optional<UserRecord> getUserById(int user_id) override
    {
        for (const auto &u : users_)
            if (u.id == user_id)
                return u;
        return std::nullopt;
    }
    std::vector<UserRecord> listUsers() override { return users_; }
    std::vector<UserRecord> listUsersByRole(const std::string &role) override
    {
        std::vector<UserRecord> out;
        for (const auto &u : users_)
            if (u.role == role)
                out.push_back(u);
        return out;
    }

    // ADDED: username/password authentication for no-server fallback mode.
    std::optional<UserRecord> authenticateUser(const std::string& username,
                                                const std::string& password) override
    {
        const std::string key = normalizeUsername(username);
        auto digest = password_digests_.find(key);
        if (digest == password_digests_.end() || digest->second != demoPasswordDigest(password))
            return std::nullopt;
        for (const auto& user : users_)
            if (normalizeUsername(user.username) == key)
                return user;
        return std::nullopt;
    }

    int createUser(const UserRecord& user, const std::string& password) override
    {
        const std::string key = normalizeUsername(user.username);
        if (key.empty() || password.empty() ||
            std::any_of(users_.begin(), users_.end(), [&](const UserRecord& existing) {
                return normalizeUsername(existing.username) == key;
            }))
            return 0;

        UserRecord inserted = user;
        inserted.id = next_user_id_++;
        inserted.username = key;
        users_.push_back(inserted);
        password_digests_[key] = demoPasswordDigest(password);
        return inserted.id;
    }

    // --- BRANCHES ---
    std::vector<BranchRecord> listBranches() override { return branches_; }
    int createBranch(const BranchRecord& branch) override
    {
        if (branch.branch_name.empty()) return 0;
        BranchRecord inserted = branch;
        inserted.id = next_branch_id_++;
        branches_.push_back(inserted);
        return inserted.id;
    }

    // --- STATIONS ---
    std::vector<StationRecord> listStations(int branch_id, const std::optional<std::string> &station_type) override
    {
        std::vector<StationRecord> out;
        for (const auto &s : stations_)
        {
            if (s.branch_id == branch_id)
            {
                if (!station_type || s.type == *station_type)
                    out.push_back(s);
            }
        }
        return out;
    }
    std::optional<StationRecord> getStationById(int station_id) override
    {
        for (const auto &s : stations_)
            if (s.id == station_id)
                return s;
        return std::nullopt;
    }
    bool updateStationStatus(int station_id, const std::string &status) override
    {
        for (auto &s : stations_)
        {
            if (s.id == station_id)
            {
                s.status = status;
                return true;
            }
        }
        return false;
    }

    // ADDED: create station in the in-memory fallback backend.
    int createStation(const StationRecord& station) override
    {
        const bool branch_exists = std::any_of(branches_.begin(), branches_.end(),
            [&](const BranchRecord& branch) { return branch.id == station.branch_id; });
        if (!branch_exists || station.hourly_rate_cents < 0 ||
            (station.type != "PC" && station.type != "PS4" && station.type != "PS5"))
            return 0;
        StationRecord inserted = station;
        inserted.id = next_station_id_++;
        if (inserted.status.empty()) inserted.status = "Available";
        stations_.push_back(inserted);
        return inserted.id;
    }

    // --- RESERVATIONS ---
    int createReservation(const ReservationRecord &record) override
    {
        ReservationRecord rec = record;
        rec.id = next_res_id_++;
        reservations_.push_back(rec);
        return rec.id;
    }
    std::optional<ReservationRecord> getReservationById(int id) override
    {
        for (const auto &r : reservations_)
            if (r.id == id)
                return r;
        return std::nullopt;
    }
    std::vector<ReservationRecord> listReservations(int branch_id, const std::optional<std::string> &status) override
    {
        std::vector<ReservationRecord> out;
        for (const auto &r : reservations_)
        {
            if (r.branch_id == branch_id)
            {
                if (!status || r.status == *status)
                    out.push_back(r);
            }
        }
        return out;
    }
    bool updateReservationStatus(int id, const std::string &status) override
    {
        for (auto &r : reservations_)
        {
            if (r.id == id)
            {
                r.status = status;
                return true;
            }
        }
        return false;
    }

    // --- SESSIONS ---
    std::vector<ActiveSessionView> listActiveSessions() override
    {
        std::vector<ActiveSessionView> out;
        for (const auto &s : sessions_)
        {
            if (!s.end_time.has_value())
            {
                ActiveSessionView v;
                v.session_id = s.id;
                v.station_id = s.station_id;
                v.user_id = s.user_id;
                v.customer_name = "Player " + std::to_string(s.user_id);
                for (const auto &u : users_)
                    if (u.id == s.user_id)
                        v.customer_name = u.name;
                for (const auto &st : stations_)
                    if (st.id == s.station_id)
                    {
                        v.station_type = st.type;
                        v.hourly_rate_cents = st.hourly_rate_cents;
                    }
                v.start_time = s.start_time;
                v.elapsed_minutes = calculateElapsedMinutes(s.start_time, std::nullopt);
                out.push_back(v);
            }
        }
        return out;
    }
    std::optional<SessionRecord> getSessionById(int id) override
    {
        for (const auto &s : sessions_)
            if (s.id == id)
                return s;
        return std::nullopt;
    }
    int startSession(const SessionRecord &session) override
    {
        SessionRecord s = session;
        s.id = next_session_id_++;
        sessions_.push_back(s);
        updateStationStatus(s.station_id, "InUse");
        return s.id;
    }
    bool finishSession(int session_id, const SessionRecord &finished_session) override
    {
        for (auto &s : sessions_)
        {
            if (s.id == session_id)
            {
                s.end_time = finished_session.end_time;
                s.final_cost_cents = finished_session.final_cost_cents;
                updateStationStatus(s.station_id, "Available");
                return true;
            }
        }
        return false;
    }
};