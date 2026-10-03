DROP TABLE IF EXISTS sessions, reservations, stations, users, branches CASCADE;
-- ============================================================================
-- GAME&GO - GAMING LOUNGE TIME & TAB MANAGER
-- POSTGRESQL SCHEMA AND QUERY CONTRACT -- IMPLEMENTED BY MEMBER 1
--
-- HOW TO USE THIS FILE
--   1. Create an empty database in pgAdmin (see section 0 for the dev name).
--   2. Open this file in the pgAdmin Query Tool against that database.
--   3. Run the whole file (F5). It is idempotent: it drops the five tables
--      first, recreates them with all constraints and indexes, then loads the
--      development seed data. Running it twice is safe.
--   4. The read-only demo queries in section 8 execute on that run. The write
--      examples are commented on purpose and are run one statement at a time;
--      section 8 explains why and how.
--
-- FILE OWNER MAP
--   Member 1: THIS ENTIRE FILE. Member 1 owns all PostgreSQL/database work
--             represented here, including schema, constraints, seed data,
--             SQL queries, indexes, and database-side testing.
--
-- DATABASE CONTRACT
--   The names/types below MUST match main.cpp and the SRS.
--   If the team changes a field, update the C++ shared record and SRS together.
--   Column names and types were verified against include/CoreData.h; see the
--   section 14 checklist for the item-by-item result.
-- ============================================================================

-- ============================================================================
-- 0. DATABASE SETUP
-- ============================================================================
-- DONE [M1]: development database created in pgAdmin and used for all testing.
-- DONE [M1]: database name recorded below.
-- NOTE: Never hard-code a real password into C++ or Git. The final connection
--       configuration is handled by the application's database adapter.
-- Dev database name: game_and_go_dev

-- ============================================================================
-- 1. TABLE: branches
-- ============================================================================
-- PURPOSE:
--   Stores each physical Game&Go lounge branch.
--
-- REQUIRED COLUMNS:
--   id                SERIAL PRIMARY KEY
--   branch_name       VARCHAR(100) NOT NULL
--   street_address    VARCHAR(150) NOT NULL
--   district          VARCHAR(100) NOT NULL
--   location_city     VARCHAR(100) NOT NULL
--   location_country  VARCHAR(100) NOT NULL
--
-- DONE [M1]: CREATE TABLE written using exactly these columns.
-- DECISION [M1] branch uniqueness: NO uniqueness rule added. The SRS does not
--   state that a branch is identified by its name or address, and BranchRecord
--   carries no natural key, so inventing UNIQUE(branch_name) could reject a
--   legitimate branch. Identity is the SERIAL id only.
-- Decision shape agreed by M1:
-- Final:
--
 CREATE TABLE branches (
     id SERIAL PRIMARY KEY,
     branch_name VARCHAR(100) NOT NULL,
     street_address VARCHAR(150) NOT NULL,
     district VARCHAR(100) NOT NULL,
     location_city VARCHAR(100) NOT NULL,
     location_country VARCHAR(100) NOT NULL
 );

-- ============================================================================
-- 2. TABLE: users
-- ============================================================================
-- PURPOSE:
--   Stores administrators, staff members, and customers.
--
-- REQUIRED COLUMNS:
--   id          SERIAL PRIMARY KEY
--   name        VARCHAR(100) NOT NULL
--   role        VARCHAR(50) NOT NULL
--   phone       VARCHAR(20) NULLABLE
--   branch_id   INT NULLABLE -> branches(id)
--
-- FOREIGN KEY:
--   branch_id references branches(id) ON DELETE SET NULL
--
-- REQUIRED ROLE VALUES:
--   Admin | Staff | Customer
--
-- DONE [M1]: CREATE TABLE written.
-- DONE [M1]: CHECK constraint added on role; verified an INSERT with
--   role 'Hacker' is rejected.
-- DECISION [M1] phone uniqueness: NOT required. The SRS does not state that a
--   phone number identifies a user, and customers may share a number or leave
--   it NULL. Adding UNIQUE would also reject two NULLs-in-practice bookings in
--   a way the application never needs. Left unconstrained on purpose.
--
--
 CREATE TABLE users (
     id SERIAL PRIMARY KEY,
     name VARCHAR(100) NOT NULL,
     role VARCHAR(50) NOT NULL,
     phone VARCHAR(20),
     branch_id INT REFERENCES branches(id) ON DELETE SET NULL,
        CHECK (role IN ('Admin', 'Staff', 'Customer'))
 );

-- ============================================================================
-- 3. TABLE: stations
-- ============================================================================
-- PURPOSE:
--   Stores the physical gaming stations in every branch.
--
-- REQUIRED COLUMNS:
--   id           SERIAL PRIMARY KEY
--   branch_id    INT NOT NULL -> branches(id)
--   type         VARCHAR(50) NOT NULL
--   hourly_rate  NUMERIC(10,2) NOT NULL
--   status       VARCHAR(50) NOT NULL
--
-- FOREIGN KEY:
--   branch_id references branches(id) ON DELETE CASCADE
--
-- REQUIRED TYPE VALUES:
--   PC | PS4 | PS5
--
-- REQUIRED STATUS VALUES:
--   Available | InUse | Maintenance
--
-- DONE [M1]: CREATE TABLE written.
-- DONE [M1]: CHECK constraints added on type, status and hourly_rate >= 0;
--   verified INSERTs with type 'Xbox', status 'Broken' and a negative rate are
--   all rejected.
-- DECISION [M1] station naming/numbering: SERIAL id is sufficient. The SRS
--   identifies a station by its id and shows the same station in StationRecord
--   with no label column, so no extra name or numbering column was added. A
--   display label can be derived as branch name + id at the presentation layer
--   without changing the schema or the C++ contract.
--
--
 CREATE TABLE stations (
     id SERIAL PRIMARY KEY,
     branch_id INT NOT NULL REFERENCES branches(id) ON DELETE CASCADE,
     type VARCHAR(50) NOT NULL,
     hourly_rate NUMERIC(10,2) NOT NULL,
     status VARCHAR(50) NOT NULL,
        CHECK (type IN ('PC', 'PS4', 'PS5')),
        CHECK (status IN ('Available', 'InUse', 'Maintenance')),
        CHECK (hourly_rate >= 0)
 );

-- ============================================================================
-- 4. TABLE: reservations
-- ============================================================================
-- PURPOSE:
--   Stores customer requests for a station type at a branch and time.
--
-- REQUIRED COLUMNS:
--   id              SERIAL PRIMARY KEY
--   user_id         INT NOT NULL -> users(id)
--   branch_id       INT NOT NULL -> branches(id)
--   station_type    VARCHAR(50) NOT NULL
--   reserved_time   TIMESTAMP NOT NULL
--   deposit_amount  NUMERIC(10,2) NOT NULL
--   status          VARCHAR(50) NOT NULL
--
-- FOREIGN KEYS:
--   user_id references users(id) ON DELETE CASCADE
--   branch_id references branches(id) ON DELETE CASCADE
--
-- REQUIRED STATION TYPE VALUES:
--   PC | PS4 | PS5
--
-- REQUIRED STATUS VALUES:
--   Pending | Confirmed | Canceled
--
-- DONE [M1]: CREATE TABLE written.
-- DONE [M1]: CHECK constraints added on station_type, status and
--   deposit_amount >= 0; verified invalid values are rejected.
-- DECISION [M1] overlapping reservations: enforced at APPLICATION level, not by
--   a database rule. Reasoning: a reservation here is a request for a station
--   TYPE at a branch and time, not a hold on a specific station row, so the
--   database cannot tell whether two rows for the same type and time genuinely
--   conflict without knowing the real station count and capacity per branch.
--   A CHECK or EXCLUDE constraint here would be inventing a rule the SRS does
--   not state, so none was added.
--   The integration layer must therefore reject a duplicate request inside the
--   same transaction, treating Canceled rows as not occupying capacity:
--     SELECT COUNT(*) FROM reservations
--      WHERE branch_id = ? AND station_type = ?
--        AND reserved_time = ? AND status IN ('Pending', 'Confirmed');
--   If that count is at or above the number of stations of that type in the
--   branch, the request must be refused. This rule is stated here so the C++
--   owner implements the same one.
-- NOTE: Application-level reservation validation must use the same status and
--       station-type values documented here.
--
--
 CREATE TABLE reservations (
     id SERIAL PRIMARY KEY,
     user_id INT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
     branch_id INT NOT NULL REFERENCES branches(id) ON DELETE CASCADE,
     station_type VARCHAR(50) NOT NULL,
     reserved_time TIMESTAMP NOT NULL,
     deposit_amount NUMERIC(10,2) NOT NULL,
     status VARCHAR(50) NOT NULL,
        CHECK (station_type IN ('PC', 'PS4', 'PS5')),
        CHECK (status IN ('Pending', 'Confirmed', 'Canceled')),
        CHECK (deposit_amount >= 0)
     );

-- ============================================================================
-- 5. TABLE: sessions
-- ============================================================================
-- PURPOSE:
--   Stores actual gaming sessions / tabs.
--
-- REQUIRED COLUMNS:
--   id          SERIAL PRIMARY KEY
--   station_id  INT NOT NULL -> stations(id)
--   user_id     INT NOT NULL -> users(id)
--   start_time  TIMESTAMP NOT NULL
--   end_time    TIMESTAMP NULLABLE
--   final_cost  NUMERIC(10,2) NULLABLE
--
-- FOREIGN KEYS:
--   station_id references stations(id) ON DELETE CASCADE
--   user_id references users(id) ON DELETE CASCADE
--
-- DONE [M1]: CREATE TABLE written.
-- DONE [M1]: CHECK added so final_cost is never negative when supplied. A NULL
--   final_cost passes, which is required because an open session has no cost yet.
--   Verified an INSERT with final_cost -10 is rejected.
-- DECISION [M1] one active session per station: ENFORCED IN THE DATABASE as a
--   partial unique index created below, because a station running two open tabs
--   at once is a billing and data-integrity bug, not just a UI problem. It also
--   guards the start-session transaction in section 12: if two clients race to
--   start the same station, the second INSERT fails instead of silently
--   double-booking. This is safe with the dev seed, where every InUse station
--   has exactly one row with end_time IS NULL.
--   IMPACT ON THE C++ SIDE: start-session must handle a unique-violation on
--   that index as "station already occupied", not as an unexpected crash. The
--   status check in section 12 still runs first, so this is the backstop only.
-- NOTE: The C++ application treats end_time = NULL as an open session and
--       the final integration must preserve that meaning.
--
 CREATE TABLE sessions (
     id SERIAL PRIMARY KEY,
     station_id INT NOT NULL REFERENCES stations(id) ON DELETE CASCADE,
     user_id INT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
     start_time TIMESTAMP NOT NULL,
     end_time TIMESTAMP,
     final_cost NUMERIC(10,2),
        CHECK (final_cost >= 0),
        CHECK (end_time IS NULL OR end_time >= start_time)
    );

-- Enforces the one-active-session-per-station decision documented above. This
-- must come after the CREATE TABLE, and it is checked against the seed data
-- below, which satisfies it by design.
CREATE UNIQUE INDEX idx_sessions_one_active_per_station
    ON sessions(station_id) WHERE end_time IS NULL;

-- ============================================================================
-- 6. RELATIONSHIP MAP
-- ============================================================================
-- DONE [M1]: all six relationships above exist as real foreign keys with the
-- ON DELETE rules listed below. Verified against pg_indexes and the
-- information_schema in the game_and_go_dev database.
--
-- branches 1 ---- * users
-- branches 1 ---- * stations
-- users    1 ---- * reservations
-- branches 1 ---- * reservations
-- stations 1 ---- * sessions
-- users    1 ---- * sessions
--
-- ON DELETE rules required by the original project specification:
--   users.branch_id     -> branches.id  ON DELETE SET NULL
--   stations.branch_id  -> branches.id  ON DELETE CASCADE
--   reservations.user_id   -> users.id ON DELETE CASCADE
--   reservations.branch_id -> branches.id ON DELETE CASCADE
--   sessions.station_id -> stations.id ON DELETE CASCADE
--   sessions.user_id    -> users.id   ON DELETE CASCADE

-- ============================================================================
-- 7. CONSTRAINT CHECKLIST
-- ============================================================================
-- DONE [M1]: all implemented and tested in pgAdmin on game_and_go_dev.
--   [x] Primary key on every table
--   [x] Foreign key on every relationship
--   [x] role is Admin / Staff / Customer
--   [x] station type is PC / PS4 / PS5
--   [x] station status is Available / InUse / Maintenance
--   [x] reservation station_type is PC / PS4 / PS5
--   [x] reservation status is Pending / Confirmed / Canceled
--   [x] station hourly_rate is non-negative
--   [x] reservation deposit_amount is non-negative
--   [x] session final_cost is non-negative when supplied
--   [x] Required text fields are NOT NULL
--
-- ADDITIONAL CONSTRAINTS [M1] beyond the original list. These are M1 decisions
-- and must be added to the SRS by whoever owns it before integration:
--   [x] sessions: end_time IS NULL OR end_time >= start_time
--         Prevents a negative-duration tab, which would produce a negative bill.
--   [x] sessions: partial unique index, one active row per station
--         See the DECISION note above section 5.
--   [ ] users: phone uniqueness -- decided NO, documented above in section 2.
--   [ ] reservations: no overlap rule in SQL -- decided APPLICATION level,
--       documented above in section 4.
--   [ ] reservations: overlap validation MUST be implemented by the C++ owner
--       using the stated count query, otherwise double-booking is possible.

-- ============================================================================
-- 8. CRUD / SELECT WORK REQUIRED BY MEMBER 1
-- ============================================================================
-- The training requirements require the team to demonstrate database design,
-- INSERT, UPDATE, DELETE, SELECT, JOINs, keys/relationships and PostgreSQL
-- integration with C++.
--
-- DONE [M1]: working examples for every required operation are below and were
-- each executed in pgAdmin. The INSERT/UPDATE/DELETE examples are stored
-- commented because they reference Test rows that only exist after the seed has
-- run; the SELECT/JOIN/aggregate examples run on every execute of this file.
--
-- HOW TO RUN THE WRITE EXAMPLES BELOW
--   These statements are commented out on purpose. Section 10 seed data runs
--   first on a full-file execute, so the Test rows below only get the ids they
--   reference (branch 4, customer 17, stations 19/20/21) after that seed runs.
--   To demo a write, uncomment only the statement you need, select just that
--   line in pgAdmin, and execute that single selection.
--
--   Test id map after a fresh load of this file:
--     dev branches     = 1, 2, 3
--     dev users        = 1..14   (customers are 10..14)
--     dev stations     = 1..18
--     Test branch      = 4
--     Test users       = 15 admin, 16 staff, 17 customer
--     Test stations    = 19 PC, 20 PS4, 21 PS5
--     Test reservation = 4
--     Test session     = 7
--
-- INSERT examples:
--   1. Insert a branch.
--   2. Insert an Admin, Staff and Customer.
--   3. Insert one PC, one PS4 and one PS5 station.
--   4. Insert a reservation.
--   5. Insert a session for testing.
--
-- INSERT INTO branches (branch_name, street_address, district, location_city, location_country) VALUES
-- ('Test Branch', '123 Test St', 'Test District', 'Test City', 'Test Country');
--
-- INSERT INTO users (name, role, phone, branch_id) VALUES
-- ('Test Admin', 'Admin', '123-456-7890', NULL),
-- ('Test Staff', 'Staff', '234-567-8901', 4),
-- ('Test Customer', 'Customer', '345-678-9012', 4);
--
-- INSERT INTO stations (branch_id, type, hourly_rate, status) VALUES
-- (4, 'PC', 10.00, 'Available'),
-- (4, 'PS4', 12.00, 'Available'),
-- (4, 'PS5', 15.00, 'Available');
--
-- INSERT INTO reservations (user_id, branch_id, station_type, reserved_time, deposit_amount, status) VALUES
-- (17, 4, 'PC', '2026-06-01 10:00:00', 5.00, 'Pending');
--
-- INSERT INTO sessions (station_id, user_id, start_time, end_time, final_cost) VALUES
-- (19, 17, '2026-06-01 10:00:00', NULL, NULL);
--
-- UPDATE examples:
--   1. Change a station status.
--   2. Change a reservation status.
--   3. Update a customer's phone.
--
-- UPDATE stations SET status = 'InUse' WHERE id = 19; -- test station id
-- UPDATE reservations SET status = 'Confirmed' WHERE id = 4; -- test reservation id
-- UPDATE users SET phone = '999-999-9999' WHERE id = 17; -- test customer id
--
-- DELETE examples:
--   1. Delete a development/test station and observe FK behavior.
--   2. Delete a branch in a controlled test and observe ON DELETE rules.
--   Run the whole block as ONE selection. stations.branch_id, reservations and
--   sessions disappear with the branch by CASCADE, while Test users survive with
--   branch_id NULL by SET NULL. ROLLBACK then restores everything.
--
-- BEGIN;
-- DELETE FROM stations WHERE id = 20; -- test station id
-- SELECT * FROM stations WHERE id = 20; -- expect no rows (deleted)
-- DELETE FROM branches WHERE id = 4; -- test branch id
-- SELECT * FROM branches WHERE id = 4; -- expect no rows (deleted)
-- SELECT * FROM stations WHERE branch_id = 4; -- expect no rows (ON DELETE CASCADE)
-- SELECT * FROM users WHERE branch_id = 4; -- expect no rows (ON DELETE SET NULL)
-- SELECT id, name, branch_id FROM users WHERE name LIKE 'Test%'; -- users still exist, branch_id NULL
-- ROLLBACK; -- undo the test deletes to preserve test data
--
-- Verify after ROLLBACK that the seed is untouched:
-- SELECT id FROM branches WHERE id = 4; -- expect one row back
-- SELECT id FROM stations WHERE branch_id = 4; -- expect three rows back
--
-- SELECT examples:
--   1. List all branches.
--   2. List all users.
--   3. List stations for one branch.
--   4. List pending reservations.
--   5. List active sessions (end_time IS NULL).

SELECT * FROM branches;
SELECT * FROM users;
SELECT * FROM stations WHERE branch_id = 4; -- test branch id
SELECT * FROM reservations WHERE status = 'Pending';
SELECT * FROM sessions WHERE end_time IS NULL;
--
-- JOIN examples:
--   1. sessions JOIN users JOIN stations to produce active tab information.
--   2. reservations JOIN users JOIN branches for reservation management.
--   3. stations JOIN branches for the branch station screen.

--      Column list matches ActiveSessionView in include/CoreData.h, in order.
--      OPEN CONTRACT QUESTION FOR THE TEAM:
--      SRS says the C++ application owns timestamp parsing and elapsed-minute
--      calculation, so either this query returns elapsed_minutes (as below) or
--      the column is dropped and the C++ computes it from start_time.
--      Aliases match the C++ struct names exactly; hourly_rate stays
--      NUMERIC(10,2) here and becomes MoneyCents on the C++ side.
SELECT s.id AS session_id,
       st.id AS station_id,
       st.type AS station_type,
       u.id AS user_id,
       u.name AS customer_name,
       s.start_time,
       st.hourly_rate AS hourly_rate_cents,
       EXTRACT(EPOCH FROM NOW() - s.start_time) / 60 AS elapsed_minutes
FROM sessions s
JOIN users u ON s.user_id = u.id
JOIN stations st ON s.station_id = st.id
WHERE s.end_time IS NULL;

SELECT r.id, u.name, b.branch_name, r.station_type, r.reserved_time, r.status
FROM reservations r
JOIN users u ON r.user_id = u.id
JOIN branches b ON r.branch_id = b.id; 

SELECT st.id, st.type, st.status, b.branch_name
FROM stations st
JOIN branches b ON st.branch_id = b.id;

--
-- AGGREGATE examples:
--   1. COUNT stations by status.
--   2. COUNT reservations by status.
--   3. SUM final_cost for completed sessions.

SELECT status, COUNT(*) FROM stations GROUP BY status;
SELECT status, COUNT(*) FROM reservations GROUP BY status;
SELECT SUM(final_cost) FROM sessions WHERE end_time IS NOT NULL;
--
-- ============================================================================
-- 9. INDEX WORK
-- ============================================================================
-- DONE [M1]: six indexes added, each with a comment above it naming the filter
--   it serves. Verified all six exist via pg_indexes, and confirmed each maps
--   to a real query: users(branch_id) and stations(branch_id, status) for the
--   branch screens, reservations(branch_id, reserved_time) and
--   reservations(user_id, status) for reservation management, and
--   sessions(station_id, end_time) with sessions(user_id, end_time) for the
--   active-tab lookups. A seventh partial unique index, documented in section
--   5, enforces one active session per station.
-- Suggested query targets to evaluate:
--   users(branch_id)
--   stations(branch_id, status)
--   reservations(branch_id, reserved_time)
--   reservations(user_id, status)
--   sessions(station_id, end_time)
--   sessions(user_id, end_time)

--speed up queries filterred by branch
CREATE INDEX idx_users_branch ON users(branch_id); 
--speed up queries filtered by branch and status
CREATE INDEX idx_stations_branch_status ON stations(branch_id, status);
--speed up queries filtered by branch and reserved_time
CREATE INDEX idx_reservations_branch_time ON reservations(branch_id, reserved_time);
--speed up queries filtered by user and status
CREATE INDEX idx_reservations_user_status ON reservations(user_id, status);
--speed up queries filtered by station and end_time
CREATE INDEX idx_sessions_station_end ON sessions(station_id, end_time);
--speed up queries filtered by user and end_time
CREATE INDEX idx_sessions_user_end ON sessions(user_id, end_time);
--
-- Do not add indexes blindly. Member 1 should be able to explain why each
-- index exists and which query uses it.

-- ============================================================================
-- 10. DEVELOPMENT SEED DATA
-- ============================================================================
-- DONE [M1]: seed data below meets every item above:
--   * 3 branches (requirement was 2)
--   * 3 Admins, 6 Staff, 5 Customers (customers were the short one, now 5)
--   * 18 stations: 6 PC, 6 PS4, 6 PS5 across the 3 branches
--   * statuses mixed: Available, InUse and Maintenance all present
--   * reservations covering Pending, Confirmed and Canceled
--   * 2 completed sessions (end_time and final_cost filled) and 4 active
--     sessions (end_time and final_cost NULL), one on every InUse station so
--     the active-tab JOIN returns meaningful demo rows
-- DONE [M1]: every branch and user name carries a 'dev-' prefix so the seed is
--   obviously development data and safe to delete. Stations, reservations and
--   sessions inherit that identity through their foreign keys.
INSERT INTO branches (branch_name, street_address, district, location_city, location_country) VALUES
('dev-Main Branch', '111 Sidi-gaber St', 'Smouha', 'Alexandria', 'Egypt'),
('dev-Uptown Branch', '456 El-eqbal St', 'Victoria', 'Alexandria', 'Egypt'),
('dev-Maadi Branch', '789 Maadi St', 'Maadi', 'Cairo', 'Egypt');

INSERT INTO users (name, role, phone, branch_id) VALUES
('dev-Alice Admin', 'Admin', '123-456-7890', NULL),
('dev-John Admin', 'Admin', '987-654-3210', NULL),
('dev-Jane Admin', 'Admin', '555-555-5555', NULL),
('dev-Bob Staff', 'Staff', '234-567-8901', 1),
('dev-Malak Staff','Staff','342-578-9012',1),
('dev-Eve Staff', 'Staff', '567-890-1234', 2),
('dev-Frank Staff', 'Staff', '678-901-2345', 2),
('dev-Grace Staff', 'Staff', '789-012-3456', 3),
('dev-Heidi Staff', 'Staff', '890-123-4567', 3),
('dev-Charlie Customer', 'Customer', '345-678-9012', 1),
('dev-Adam Customer', 'Customer', '012-759-880', 1),
('dev-Sandy Customer', 'Customer', '015-888-333', 2),
('dev-Oscar Customer', 'Customer', '019-777-444', 3),
('dev-Diana Customer', 'Customer', '456-789-0123', 3);

INSERT INTO stations (branch_id, type, hourly_rate, status) VALUES
(1, 'PC', 10.00, 'Available'),
(1, 'PC', 10.00, 'InUse'),
(1, 'PS4', 12.00, 'Available'),
(1, 'PS4', 12.00, 'Available'),
(1, 'PS5', 15.00, 'Maintenance'),
(1, 'PS5', 15.00, 'Available'),
(2, 'PC', 10.00, 'Available'),
(2, 'PC', 10.00, 'Available'),
(2, 'PS4', 12.00, 'Available'),
(2, 'PS4', 12.00, 'InUse'),
(2, 'PS5', 15.00, 'InUse'),
(2, 'PS5', 15.00, 'Maintenance'),
(3, 'PC', 10.00, 'Available'),
(3, 'PC', 10.00, 'Available'),
(3, 'PS4', 12.00, 'Available'),
(3, 'PS4', 12.00, 'Available'),
(3, 'PS5', 15.00, 'InUse'),
(3, 'PS5', 15.00, 'Available');

INSERT INTO reservations (user_id, branch_id, station_type, reserved_time, deposit_amount, status) VALUES
(10, 1, 'PC', '2026-06-01 10:00:00', 5.00, 'Pending'),
(11, 2, 'PS4', '2026-06-02 14:00:00', 6.00, 'Confirmed'),
(10, 3, 'PS5', '2026-06-03 16:00:00', 7.50, 'Canceled');

INSERT INTO sessions (station_id, user_id, start_time, end_time, final_cost) VALUES
(1, 10, '2026-06-01 10:00:00', '2026-06-01 12:00:00', 20.00),
(2, 11, '2026-06-02 14:00:00', NULL, NULL),
(3, 10, '2026-06-03 16:00:00', '2026-06-03 18:30:00', 37.50),
(10, 10, '2026-10-02 09:00:00', NULL, NULL),
(11, 11, '2026-10-02 09:30:00', NULL, NULL),
(17, 10, '2026-10-02 10:00:00', NULL, NULL);

-- ============================================================================
-- 11. DATABASE-TO-APPLICATION QUERY CONTRACT
-- OWNER: MEMBER 1 (DATABASE CONTRACT ONLY)
-- ============================================================================
-- NOTE: The C++ database integration must map its operations to these database
--       objects and column names exactly. The C++ implementation belongs in
--       the separate PostgresDB files; this SQL file remains M1-owned.
--
-- Every query method should document:
--   * SQL statement purpose
--   * parameters
--   * expected row shape
--   * conversion into the corresponding shared C++ record
--   * empty-result behavior
--   * exception/error behavior
--
-- NOTE FOR M1:
--   Keep the final SQL contract parameter-friendly for the C++ integration.
--   The actual libpqxx implementation is documented in the SRS and is not
--   part of this SQL file.
--
-- Required mapping targets:
--   branches       -> BranchRecord
--   users          -> UserRecord
--   stations       -> StationRecord
--   reservations   -> ReservationRecord
--   sessions       -> SessionRecord
--   active JOIN    -> ActiveSessionView

-- ============================================================================
-- 12. DATABASE TRANSACTION REQUIREMENTS
-- OWNER: MEMBER 1 (DATABASE BEHAVIOR CONTRACT)
-- ============================================================================
-- NOTE: The application's start-session operation must preserve the following
--       transaction requirements in the final database design/integration:
--   1. Verify the station exists.
--   2. Lock/read the station row as needed.
--   3. Verify the station is Available.
--   4. Insert the session with end_time NULL and final_cost NULL.
--   5. Update station status to InUse.
--   6. Commit all changes together.
--   7. Roll back everything if any step fails.
--
-- NOTE: The application's finish-session operation must preserve the following
--       transaction requirements in the final database design/integration:
--   1. Verify the session exists and is active.
--   2. Update end_time and final_cost.
--   3. Update the station status back to Available.
--   4. Commit both updates together.
--   5. Roll back everything on failure.

-- ============================================================================
-- 13. C++ / SQL VALUE CONTRACT
-- ============================================================================
-- DONE [M1]: these exact textual values are kept in the CHECK constraints above,
--   with the capitalisation shown here, and in the seed data.
-- NOTE: The C++ application must map these values exactly.
--
-- users.role:
--   Admin | Staff | Customer
--
-- stations.type:
--   PC | PS4 | PS5
--
-- stations.status:
--   Available | InUse | Maintenance
--
-- reservations.station_type:
--   PC | PS4 | PS5
--
-- reservations.status:
--   Pending | Confirmed | Canceled
--
-- MONEY:
--   PostgreSQL NUMERIC(10,2)
--   C++ MoneyCents (int64_t)
--
-- TIME:
--   PostgreSQL TIMESTAMP
--   C++ stores timestamp text inside shared records; the application layer is
--   responsible for time parsing and elapsed-minute calculations.

-- ============================================================================
-- 14. FINAL DATABASE ACCEPTANCE CHECKLIST
-- OWNER: MEMBER 1
-- ============================================================================
-- STATUS [M1] verified in pgAdmin against database game_and_go_dev.
--   [x] All five required tables exist.
--   [x] All columns match the SRS exactly. Compared column by column against
--       include/CoreData.h: branches/users/stations/reservations/sessions map to
--       BranchRecord/UserRecord/StationRecord/ReservationRecord/SessionRecord,
--       money is NUMERIC(10,2) against MoneyCents, timestamps are TIMESTAMP
--       against C++ timestamp text, and phone/branch_id/end_time/final_cost are
--       nullable exactly where the C++ structs use std::optional.
--   [x] PK/FK relationships work.
--   [x] ON DELETE SET NULL / CASCADE behavior is tested. Demonstrated by the
--       BEGIN/ROLLBACK block in section 8: station and branch rows vanish while
--       their children go with them by CASCADE, and the Test users survive with
--       branch_id NULL by SET NULL.
--   [x] Invalid role/type/status values are rejected. Verified by attempting
--       INSERTs with role 'Hacker', type 'Xbox', status 'Broken' and negative
--       hourly_rate/deposit_amount/final_cost -- each was refused by a CHECK.
--   [x] CRUD operations work in pgAdmin. SELECT, INSERT, UPDATE and DELETE are
--       all present as runnable examples in section 8.
--   [x] JOIN queries produce the rows expected by the C++ records.
--   [x] Active-session query matches ActiveSessionView fields. Returns the eight
--       fields in struct order. See the open contract question in section 8
--       about whether elapsed_minutes comes from SQL or from the C++ layer.
--   [ ] Transactions leave the station/session consistent after failures.
--       Cannot be closed by M1 alone: the start/finish-session transactions in
--       section 12 are implemented by the C++ integration owner. The schema now
--       supports them, and the partial unique index on active sessions per
--       station acts as a backstop, but the actual rollback behaviour has to be
--       tested once that code exists.
--   [ ] libpqxx can connect using the final connection configuration.
--       Owned by the integration owner. No password is committed to this repo;
--       only the database name is recorded here, in section 0.

-- END OF M1 IMPLEMENTATION
-- The schema above is the final Game&Go database contract. Section 11 defines
-- the query contract the C++ adapter must implement against it.
