// ============================================================================
// GAME&GO - USER HIERARCHY + FACTORY PATTERN (IMPLEMENTATION)
// OWNER: MEMBER 3
// ============================================================================
#include "Domain/User.h"

// ---------------------------------------------------------------------
// User (base)
// ---------------------------------------------------------------------

User::User(const UserRecord& record) : record_(record) {}

User::~User() = default;

int User::getId() const { return record_.id; }
const std::string& User::getName() const { return record_.name; }
const std::string& User::getPhone() const { return record_.phone; }
std::optional<int> User::getBranchId() const { return record_.branch_id; }
UserRole User::getRole() const { return parseUserRole(record_.role); }

// ---------------------------------------------------------------------
// AdminUser - full operational + management access.
// Not treated as a "customer" - canUseCustomerFeatures stays false so the
// GUI doesn't offer booking flows meant for paying customers.
// ---------------------------------------------------------------------

AdminUser::AdminUser(const UserRecord& record) : User(record) {}

bool AdminUser::canManageStations() const { return true; }
bool AdminUser::canManageUsers() const { return true; }
bool AdminUser::canManageReservations() const { return true; }
bool AdminUser::canViewReports() const { return true; }
bool AdminUser::canUseCustomerFeatures() const { return false; }

// ---------------------------------------------------------------------
// StaffUser - day-to-day lounge operations. Cannot manage user accounts.
// ---------------------------------------------------------------------

StaffUser::StaffUser(const UserRecord& record) : User(record) {}

bool StaffUser::canManageStations() const { return true; }
bool StaffUser::canManageUsers() const { return false; }
bool StaffUser::canManageReservations() const { return true; }
bool StaffUser::canViewReports() const { return true; }
bool StaffUser::canUseCustomerFeatures() const { return false; }

// ---------------------------------------------------------------------
// CustomerUser - least-privileged role: can only use customer-facing
// features (browsing stations, booking/canceling their own reservations
// through the customer flow) - not the staff/admin management screens.
// ---------------------------------------------------------------------

CustomerUser::CustomerUser(const UserRecord& record) : User(record) {}

bool CustomerUser::canManageStations() const { return false; }
bool CustomerUser::canManageUsers() const { return false; }
bool CustomerUser::canManageReservations() const { return false; }
bool CustomerUser::canViewReports() const { return false; }
bool CustomerUser::canUseCustomerFeatures() const { return true; }

// ---------------------------------------------------------------------
// UserFactory
// ---------------------------------------------------------------------

std::unique_ptr<User> UserFactory::create(const UserRecord& record) {
    switch (parseUserRole(record.role)) {
        case UserRole::Admin:
            return std::make_unique<AdminUser>(record);
        case UserRole::Staff:
            return std::make_unique<StaffUser>(record);
        case UserRole::Customer:
            return std::make_unique<CustomerUser>(record);
        case UserRole::Unknown:
        default:
            // Unsupported/unrecognized role text: reject rather than
            // guessing a privilege level. Callers (e.g. SessionManager's
            // loginByUserId) must treat nullptr as a failed login.
            return nullptr;
    }
}