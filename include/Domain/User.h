// ============================================================================
// GAME&GO - USER HIERARCHY + FACTORY PATTERN
// OWNER: MEMBER 3
// ============================================================================
#pragma once

#include "../CoreData.h"
#include "Enums.h"
#include <memory>
#include <optional>
#include <string>

class User {
private:
    UserRecord record_;

public:
    // TODO [M3]: Construct the base user from a UserRecord.
    explicit User(const UserRecord& record);
    virtual ~User();

    // TODO [M3]: Implement shared getters while keeping record_ private.
    int getId() const;
    const std::string& getName() const;
    const std::string& getPhone() const;
    std::optional<int> getBranchId() const;
    UserRole getRole() const;

    // TODO [M3]: Define capabilities polymorphically.
    virtual bool canManageStations() const = 0;
    virtual bool canManageUsers() const = 0;
    virtual bool canManageReservations() const = 0;
    virtual bool canViewReports() const = 0;
    virtual bool canUseCustomerFeatures() const = 0;
};

class AdminUser final : public User {
public:
    // TODO [M3]: Construct AdminUser and implement admin permissions.
    explicit AdminUser(const UserRecord& record);

    bool canManageStations() const override;
    bool canManageUsers() const override;
    bool canManageReservations() const override;
    bool canViewReports() const override;
    bool canUseCustomerFeatures() const override;
};

class StaffUser final : public User {
public:
    // TODO [M3]: Construct StaffUser and implement operational permissions.
    explicit StaffUser(const UserRecord& record);

    bool canManageStations() const override;
    bool canManageUsers() const override;
    bool canManageReservations() const override;
    bool canViewReports() const override;
    bool canUseCustomerFeatures() const override;
};

class CustomerUser final : public User {
public:
    // TODO [M3]: Construct CustomerUser and restrict management permissions.
    explicit CustomerUser(const UserRecord& record);

    bool canManageStations() const override;
    bool canManageUsers() const override;
    bool canManageReservations() const override;
    bool canViewReports() const override;
    bool canUseCustomerFeatures() const override;
};

class UserFactory {
public:
    // TODO [M3]: Convert record.role into the matching concrete User object.
    //             Reject or safely handle unsupported roles.
    static std::unique_ptr<User> create(const UserRecord& record);
};
