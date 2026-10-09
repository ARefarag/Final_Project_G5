#pragma once

#include "IDatabase.h"
#include "../Core/Helpers.h"
#include <vector>
#include <algorithm>
#include <iostream>

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

public:
    StandaloneDB()
    {
        std::cout << "[STANDALONE DB] Initializing in-memory fallback database...\n";

        // Seed Branches
        branches_ = {
            {1, "dev-Main Branch", "111 Sidi-gaber St", "Smouha", "Alexandria", "Egypt"},
            {2, "dev-Uptown Branch", "456 El-eqbal St", "Victoria", "Alexandria", "Egypt"},
            {3, "dev-Maadi Branch", "789 Maadi St", "Maadi", "Cairo", "Egypt"}};

        // Seed Users
        users_ = {
            {1, "dev-Alice Admin", "Admin", "123-456-7890", std::nullopt},
            {4, "dev-Bob Staff", "Staff", "234-567-8901", 1},
            {6, "dev-Eve Staff", "Staff", "567-890-1234", 2},
            {10, "dev-Charlie Customer", "Customer", "345-678-9012", 1},
            {11, "dev-Adam Customer", "Customer", "012-759-880", 1}};

        // Seed Stations
        stations_ = {
            {1, 1, "PC", 1000, "Available"},
            {2, 1, "PC", 1000, "InUse"},
            {3, 1, "PS4", 1200, "Available"},
            {4, 1, "PS4", 1200, "Available"},
            {5, 1, "PS5", 1500, "Maintenance"},
            {6, 1, "PS5", 1500, "Available"}};

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

    // --- BRANCHES ---
    std::vector<BranchRecord> listBranches() override { return branches_; }

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