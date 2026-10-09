#include "Core/SessionManager.h"
#include "Core/Helpers.h"
#include <utility>

SessionManager::SessionManager(std::unique_ptr<IDatabase> database)
    : database_(std::move(database)),
      current_user_(nullptr),
      billing_strategy_(std::make_unique<StandardBillingStrategy>()),
      selected_branch_id_(std::nullopt) {}

bool SessionManager::loginByUserId(int user_id)
{
    if (!database_)
        return false;

    auto record_opt = database_->getUserById(user_id);
    if (!record_opt.has_value())
    {
        return false;
    }

    auto user = UserFactory::create(record_opt.value());
    if (!user || user->getRole() == UserRole::Unknown)
    {
        return false;
    }

    current_user_ = std::move(user);

    selected_branch_id_ = std::nullopt;

    if (current_user_->getBranchId().has_value())
    {
        selected_branch_id_ = current_user_->getBranchId();
    }
    return true;
}

// ADDED: credential login routes through the database abstraction and the existing UserFactory.
bool SessionManager::loginByCredentials(const std::string& username,
                                        const std::string& password)
{
    if (!database_ || username.empty() || password.empty())
        return false;

    auto record = database_->authenticateUser(username, password);
    if (!record.has_value())
        return false;

    auto user = UserFactory::create(*record);
    if (!user || user->getRole() == UserRole::Unknown)
        return false;

    current_user_ = std::move(user);
    selected_branch_id_ = current_user_->getBranchId();
    return true;
}

const User *SessionManager::getCurrentUser() const
{
    return current_user_.get();
}

void SessionManager::setSelectedBranch(int branch_id)
{
    selected_branch_id_ = branch_id;
}

std::optional<int> SessionManager::getSelectedBranch() const
{
    return selected_branch_id_;
}

std::vector<BranchRecord> SessionManager::loadBranches()
{
    if (!database_)
        return {};
    return database_->listBranches();
}

// ADDED: load live customer/member records for station dropdowns.
std::vector<UserRecord> SessionManager::loadUsersByRole(const std::string& role)
{
    if (!database_) return {};
    return database_->listUsersByRole(role);
}

// ADDED: enforce who can create what in core logic, not only through hidden UI buttons.
bool SessionManager::createUserAccount(const UserRecord& user, const std::string& password)
{
    if (!database_ || !current_user_ || user.name.empty() || user.username.empty() ||
        password.empty())
        return false;

    const UserRole actor = current_user_->getRole();
    const bool target_is_customer = user.role == "Customer";
    const bool target_is_staff = user.role == "Staff";
    const bool allowed = (actor == UserRole::Admin && (target_is_staff || target_is_customer)) ||
                         (actor == UserRole::Staff && target_is_customer);
    if (!allowed)
        return false;

    // Staff accounts must belong to a branch; customer branch assignment is optional.
    if (target_is_staff && !user.branch_id.has_value())
        return false;
    return database_->createUser(user, password) > 0;
}

bool SessionManager::createBranch(const BranchRecord& branch)
{
    if (!database_ || !current_user_ || current_user_->getRole() != UserRole::Admin)
        return false;
    if (branch.branch_name.empty() || branch.street_address.empty() ||
        branch.district.empty() || branch.location_city.empty() ||
        branch.location_country.empty())
        return false;
    return database_->createBranch(branch) > 0;
}

bool SessionManager::createStation(const StationRecord& station)
{
    if (!database_ || !current_user_ || current_user_->getRole() != UserRole::Admin)
        return false;
    if (station.branch_id <= 0 || station.hourly_rate_cents < 0 ||
        (station.type != "PC" && station.type != "PS4" && station.type != "PS5"))
        return false;
    return database_->createStation(station) > 0;
}

std::vector<StationRecord> SessionManager::loadStations(
    int branch_id,
    const std::optional<std::string> &station_type)
{
    if (!database_)
        return {};
    return database_->listStations(branch_id, station_type);
}

std::vector<ReservationRecord> SessionManager::loadReservations(
    int branch_id,
    const std::optional<std::string> &status)
{
    if (!database_)
        return {};
    return database_->listReservations(branch_id, status);
}

std::vector<ActiveSessionView> SessionManager::loadActiveSessions()
{
    if (!database_)
        return {};
    return database_->listActiveSessions();
}

bool SessionManager::createReservation(int user_id,
                                       int branch_id,
                                       const std::string &station_type,
                                       const std::string &reserved_time,
                                       MoneyCents deposit_cents)
{
    if (!database_)
        return false;

    ReservationRecord record;
    record.user_id = user_id;
    record.branch_id = branch_id;
    record.station_type = station_type;
    record.reserved_time = reserved_time;
    record.deposit_cents = deposit_cents;
    record.status = toString(ReservationStatus::Pending);

    int new_id = database_->createReservation(record);
    return new_id > 0;
}

bool SessionManager::confirmReservation(int reservation_id)
{
    if (!database_)
        return false;

    auto record_opt = database_->getReservationById(reservation_id);
    if (!record_opt.has_value())
        return false;

    Reservation reservation(record_opt.value());
    if (!reservation.confirm())
    {
        return false;
    }

    return database_->updateReservationStatus(
        reservation_id,
        toString(reservation.getStatus()));
}

bool SessionManager::cancelReservation(int reservation_id)
{
    if (!database_)
        return false;

    auto record_opt = database_->getReservationById(reservation_id);
    if (!record_opt.has_value())
        return false;

    Reservation reservation(record_opt.value());
    if (!reservation.cancel())
    {
        return false;
    }

    return database_->updateReservationStatus(
        reservation_id,
        toString(reservation.getStatus()));
}

bool SessionManager::startSession(int user_id, int station_id)
{
    if (!database_)
        return false;

    auto station = database_->getStationById(station_id);
    if (!station.has_value() || station->status != toString(StationStatus::Available))
    {
        return false;
    }

    SessionRecord session;
    session.station_id = station_id;
    session.user_id = user_id;
    session.start_time = nowUtcTimestamp();
    session.end_time = std::nullopt;
    session.final_cost_cents = std::nullopt;

    int session_id = database_->startSession(session);
    return session_id > 0;
}

bool SessionManager::finishSession(int session_id)
{
    if (!database_)
        return false;

    auto session_opt = database_->getSessionById(session_id);
    if (!session_opt.has_value())
        return false;

    auto session = session_opt.value();
    auto station_opt = database_->getStationById(session.station_id);
    if (!station_opt.has_value())
        return false;

    std::string finish_time = nowUtcTimestamp();
    int elapsed = calculateElapsedMinutes(session.start_time, finish_time);

    MoneyCents cost = previewCost(station_opt->hourly_rate_cents, elapsed);

    session.end_time = finish_time;
    session.final_cost_cents = cost;

    return database_->finishSession(session_id, session);
}

void SessionManager::setBillingStrategy(std::unique_ptr<IBillingStrategy> strategy)
{
    if (strategy)
    {
        billing_strategy_ = std::move(strategy);
    }
}

MoneyCents SessionManager::previewCost(MoneyCents hourly_rate_cents,
                                       int elapsed_minutes) const
{
    if (!billing_strategy_)
    {
        StandardBillingStrategy default_strategy;
        return default_strategy.calculate({hourly_rate_cents, elapsed_minutes});
    }
    return billing_strategy_->calculate({hourly_rate_cents, elapsed_minutes});
}