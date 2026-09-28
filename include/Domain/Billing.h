// ============================================================================
// GAME&GO - BILLING STRATEGY PATTERN
// OWNER: MEMBER 3
// ============================================================================
#pragma once

#include "../CoreData.h"
#include <string>

struct BillingInput {
    MoneyCents hourly_rate_cents{};
    int elapsed_minutes{};
};

class IBillingStrategy {
public:
    virtual ~IBillingStrategy();

    // TODO [M3]: Define the pricing contract. No database access is allowed.
    virtual MoneyCents calculate(const BillingInput& input) const = 0;

    // TODO [M3]: Return the strategy name shown by GUI/reporting code.
    virtual std::string name() const = 0;
};

class StandardBillingStrategy final : public IBillingStrategy {
public:
    // TODO [M3]: Implement standard hourly billing and the agreed rounding rule.
    MoneyCents calculate(const BillingInput& input) const override;

    // TODO [M3]: Return "Standard" (or the team-agreed display label).
    std::string name() const override;
};

class VipBillingStrategy final : public IBillingStrategy {
private:
    int discount_percent_{};

public:
    // TODO [M3]: Store the team-approved VIP discount percentage.
    explicit VipBillingStrategy(int discount_percent);

    // TODO [M3]: Calculate the base amount then apply the VIP discount rule.
    MoneyCents calculate(const BillingInput& input) const override;

    // TODO [M3]: Return a clear GUI/reporting label.
    std::string name() const override;
};
