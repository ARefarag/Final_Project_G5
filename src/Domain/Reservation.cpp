// ============================================================================
// GAME&GO - RESERVATION STATE PATTERN (IMPLEMENTATION)
// OWNER: MEMBER 3
//
// Transition table (matches the team's state diagram):
//   Pending   -> confirm -> Confirmed   (legal)
//   Pending   -> cancel  -> Canceled    (legal)
//   Confirmed -> confirm -> rejected    (already confirmed)
//   Confirmed -> cancel  -> Canceled    (legal)
//   Canceled  -> confirm -> rejected    (terminal state)
//   Canceled  -> cancel  -> rejected    (already canceled; idempotent no-op,
//                                        explicitly NOT treated as an error
//                                        by the caller - it simply returns
//                                        false so SessionManager can show a
//                                        "already canceled" message instead
//                                        of silently pretending it worked)
// ============================================================================
#include "Domain/Reservation.h"

#include <stdexcept>
#include <string>

IReservationState::~IReservationState() = default;

// ---------------------------------------------------------------------
// PendingState
// ---------------------------------------------------------------------

bool PendingState::confirm(Reservation& reservation) {
    reservation.transitionTo(std::make_unique<ConfirmedState>());
    return true;
}

bool PendingState::cancel(Reservation& reservation) {
    reservation.transitionTo(std::make_unique<CanceledState>());
    return true;
}

ReservationStatus PendingState::status() const { return ReservationStatus::Pending; }

// ---------------------------------------------------------------------
// ConfirmedState
// ---------------------------------------------------------------------

bool ConfirmedState::confirm(Reservation& /*reservation*/) {
    // Already confirmed: a second confirmation is not a valid transition.
    return false;
}

bool ConfirmedState::cancel(Reservation& reservation) {
    reservation.transitionTo(std::make_unique<CanceledState>());
    return true;
}

ReservationStatus ConfirmedState::status() const { return ReservationStatus::Confirmed; }

// ---------------------------------------------------------------------
// CanceledState - terminal
// ---------------------------------------------------------------------

bool CanceledState::confirm(Reservation& /*reservation*/) {
    return false;
}

bool CanceledState::cancel(Reservation& /*reservation*/) {
    // Documented behavior: canceling an already-canceled reservation is
    // rejected (returns false) rather than silently reporting success,
    // so the caller can distinguish "nothing changed" from "just canceled".
    return false;
}

ReservationStatus CanceledState::status() const { return ReservationStatus::Canceled; }

// ---------------------------------------------------------------------
// Reservation (context)
// ---------------------------------------------------------------------

Reservation::Reservation(const ReservationRecord& record) : record_(record) {
    switch (parseReservationStatus(record_.status)) {
        case ReservationStatus::Pending:
            state_ = std::make_unique<PendingState>();
            break;
        case ReservationStatus::Confirmed:
            state_ = std::make_unique<ConfirmedState>();
            break;
        case ReservationStatus::Canceled:
            state_ = std::make_unique<CanceledState>();
            break;
        case ReservationStatus::Unknown:
        default:
            // The SRS requires unknown/invalid database values to never be
            // silently turned into a valid state (e.g. defaulting to
            // Pending would let bad data masquerade as a real reservation).
            // Reject it outright instead.
            throw std::invalid_argument(
                "Reservation " + std::to_string(record_.id) +
                " has an unknown/invalid status: \"" + record_.status + "\"");
    }
    record_.status = toString(state_->status());
}

bool Reservation::confirm() { return state_->confirm(*this); }
bool Reservation::cancel() { return state_->cancel(*this); }

const ReservationRecord& Reservation::getRecord() const { return record_; }
ReservationStatus Reservation::getStatus() const { return state_->status(); }

void Reservation::transitionTo(std::unique_ptr<IReservationState> new_state) {
    state_ = std::move(new_state);
    // Keep the plain-data record in sync with the state object so
    // getRecord() (used for persistence via IDatabase) always reflects
    // the reservation's current, validated status.
    record_.status = toString(state_->status());
}