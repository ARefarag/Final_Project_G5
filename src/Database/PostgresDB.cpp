// ============================================================================
// GAME&GO - POSTGRESQL DATABASE IMPLEMENTATION
// SHARED IMPLEMENTATION FILE: MEMBER 4 + MEMBER 6
//
// MEMBER 4 owns the infrastructure section and the User/Branch/Station methods.
// MEMBER 6 owns the Reservation/Session methods and session transactions.
// Keep each member's work inside the labeled sections below to reduce Git
// conflicts. Both members may use the pqxx types in THIS .cpp file only.
// ============================================================================
#include "Database/PostgresDB.h"

// -------------------------- MEMBER 4: INFRASTRUCTURE ------------------------
// TODO [M4]: Include <pqxx/pqxx> in THIS .cpp file only.
// TODO [M4]: Implement DatabaseException, PostgresDB constructor/destructor,
//             and low-level pqxx row -> shared-record mapping helpers.
// TODO [M4]: Implement User, Branch, and Station database operations here.
// TODO [M4]: Use parameterized queries and preserve NULL/empty-result behavior
//             required by IDatabase and the SRS.
// TODO [M4]: Do not move pqxx types into any UI/core header.

// -------------------------- MEMBER 6: DATA FLOWS ----------------------------
// TODO [M6]: Implement Reservation operations here using parameterized queries
//             and the shared mapping helpers prepared by Member 4.
// TODO [M6]: Implement ActiveSessionView / SessionRecord queries here.
// TODO [M6]: Implement startSession() as one atomic transaction:
//             verify station -> insert session -> set station InUse -> commit.
// TODO [M6]: Implement finishSession() as one atomic transaction:
//             verify active session -> update session -> set station Available -> commit.
// TODO [M6]: Roll back the transaction on any failure and translate DB failures
//             to the application-facing error behavior defined by IDatabase/SRS.
