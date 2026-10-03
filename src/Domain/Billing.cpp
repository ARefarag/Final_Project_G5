// ============================================================================
// GAME&GO - BILLING STRATEGY PATTERN (IMPLEMENTATION)
// OWNER: MEMBER 3
// Agreed rounding rule: cost is rounded to the nearest cent (round-half-up).
// All math is done in integer cents to avoid floating-point rounding drift
// in money calculations.
// ============================================================================
#include "Domain/Billing.h"

#include <algorithm>

IBillingStrategy::~IBillingStrategy() = default;

// ---------------------------------------------------------------------
// StandardBillingStrategy
// cost_cents = round(hourly_rate_cents * elapsed_minutes / 60)
// ---------------------------------------------------------------------

MoneyCents StandardBillingStrategy::calculate(const BillingInput& input) const {
    if (input.hourly_rate_cents <= 0 || input.elapsed_minutes <= 0) {
        return 0;
    }

    const long long numerator =
        static_cast<long long>(input.hourly_rate_cents) * static_cast<long long>(input.elapsed_minutes);

    // "+30" is half of the 60-minute divisor, giving round-half-up rounding
    // to the nearest cent instead of always truncating down.
    return static_cast<MoneyCents>((numerator + 30) / 60);
}

std::string StandardBillingStrategy::name() const { return "Standard"; }

// ---------------------------------------------------------------------
// VipBillingStrategy
// Same hourly calculation as Standard, then the agreed VIP discount is
// applied and the result is rounded to the nearest cent.
// ---------------------------------------------------------------------

VipBillingStrategy::VipBillingStrategy(int discount_percent)
    // Clamp so a bad config value can never invert pricing (negative
    // discount) or produce a negative final cost (>100% discount).
    : discount_percent_(std::clamp(discount_percent, 0, 100)) {}

MoneyCents VipBillingStrategy::calculate(const BillingInput& input) const {
    const StandardBillingStrategy standard;
    const MoneyCents baseCost = standard.calculate(input);

    const long long discountNumerator = static_cast<long long>(baseCost) * discount_percent_;
    const MoneyCents discount = static_cast<MoneyCents>((discountNumerator + 50) / 100);

    return baseCost - discount;
}

std::string VipBillingStrategy::name() const {
    return "VIP (" + std::to_string(discount_percent_) + "% off)";
}
