// ============================================================================
// GAME&GO - RESERVATION STATE PATTERN
// OWNER: MEMBER 3
// ============================================================================
#pragma once

#include "../CoreData.h"
#include "Enums.h"
#include <memory>

class Reservation;

class IReservationState {
public:
    virtual ~IReservationState();

    // TODO [M3]: Return the status represented by the current state object.
    virtual ReservationStatus status() const = 0;

    // TODO [M3]: Implement legal/illegal transition behavior.
    virtual bool confirm(Reservation& reservation) = 0;
    virtual bool cancel(Reservation& reservation) = 0;
};

class PendingState final : public IReservationState {
public:
    // TODO [M3]: Pending -> Confirmed is legal.
    bool confirm(Reservation& reservation) override;

    // TODO [M3]: Pending -> Canceled is legal.
    bool cancel(Reservation& reservation) override;

    // TODO [M3]: Return ReservationStatus::Pending.
    ReservationStatus status() const override;
};

class ConfirmedState final : public IReservationState {
public:
    // TODO [M3]: A second confirmation must be rejected.
    bool confirm(Reservation& reservation) override;

    // TODO [M3]: Confirmed -> Canceled is legal.
    bool cancel(Reservation& reservation) override;

    // TODO [M3]: Return ReservationStatus::Confirmed.
    ReservationStatus status() const override;
};

class CanceledState final : public IReservationState {
public:
    // TODO [M3]: Canceled is terminal; confirmation must be rejected.
    bool confirm(Reservation& reservation) override;

    // TODO [M3]: Re-cancel behavior must be explicitly documented.
    bool cancel(Reservation& reservation) override;

    // TODO [M3]: Return ReservationStatus::Canceled.
    ReservationStatus status() const override;
};

class Reservation {
private:
    ReservationRecord record_;
    std::unique_ptr<IReservationState> state_;

public:
    // TODO [M3]: Build the initial concrete state from record_.status.
    explicit Reservation(const ReservationRecord& record);

    // TODO [M3]: Delegate state changes to state_, not raw status if/else logic.
    bool confirm();
    bool cancel();

    // TODO [M3]: Return read-only record/state information.
    const ReservationRecord& getRecord() const;
    ReservationStatus getStatus() const;

    // TODO [M3]: Replace the active state after a valid transition.
    void transitionTo(std::unique_ptr<IReservationState> new_state);
};
