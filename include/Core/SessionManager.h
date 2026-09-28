// ============================================================================
// GAME&GO - SESSION MANAGER / APPLICATION CORE
// OWNER: MEMBER 5
// ============================================================================
#pragma once

#include "../CoreData.h"
#include "../Database/IDatabase.h"
#include "../Domain/Billing.h"
#include "../Domain/User.h"
#include "../Domain/Reservation.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

class SessionManager {
private:
    std::unique_ptr<IDatabase> database_;
    std::unique_ptr<User> current_user_;
    std::unique_ptr<IBillingStrategy> billing_strategy_;
    std::optional<int> selected_branch_id_;

public:
    // TODO [M5]: Inject IDatabase. Do not construct PostgresDB deep inside.
    explicit SessionManager(std::unique_ptr<IDatabase> database);

    // TODO [M5]: Load UserRecord, use UserFactory, and reject invalid roles.
    bool loginByUserId(int user_id);

    // TODO [M5]: Return current user without transferring ownership.
    const User* getCurrentUser() const;

    // TODO [M5]: Store/read branch selected by the current UI session.
    void setSelectedBranch(int branch_id);
    std::optional<int> getSelectedBranch() const;

    // TODO [M5]: Load database-backed data through IDatabase for ImGui.
    std::vector<BranchRecord> loadBranches();
    std::vector<StationRecord> loadStations(
        int branch_id,
        const std::optional<std::string>& station_type);
    std::vector<ReservationRecord> loadReservations(
        int branch_id,
        const std::optional<std::string>& status);
    std::vector<ActiveSessionView> loadActiveSessions();

    // TODO [M5]: Validate request, build ReservationRecord, then insert.
    bool createReservation(int user_id,
                           int branch_id,
                           const std::string& station_type,
                           const std::string& reserved_time,
                           MoneyCents deposit_cents);

    // TODO [M5]: Load Reservation, invoke State Pattern, persist Confirmed.
    bool confirmReservation(int reservation_id);

    // TODO [M5]: Load Reservation, invoke State Pattern, persist Canceled.
    bool cancelReservation(int reservation_id);

    // TODO [M5]: Validate station/user rules, create SessionRecord, start session.
    bool startSession(int user_id, int station_id);

    // TODO [M5]: Read active session, choose billing Strategy, finish in DB.
    bool finishSession(int session_id);

    // TODO [M5]: Replace billing strategy without changing orchestration logic.
    void setBillingStrategy(std::unique_ptr<IBillingStrategy> strategy);

    // TODO [M5]: Calculate a read-only preview using the current strategy.
    MoneyCents previewCost(MoneyCents hourly_rate_cents,
                           int elapsed_minutes) const;
};
