DROP TABLE IF EXISTS sessions, reservations, stations, users, branches CASCADE;
-- ============================================================================
-- GAME&GO - GAMING LOUNGE TIME & TAB MANAGER
-- POSTGRESQL DECLARATION / TODO TEMPLATE ONLY
--
-- IMPORTANT:
--   This file is intentionally NON-EXECUTABLE.
--   It does NOT create the database and does NOT create the tables for you.
--   Each DDL statement is shown as a commented declaration so Member 1 can
--   implement it after agreeing on the final schema.
--
-- FILE OWNER MAP
--   Member 1: THIS ENTIRE FILE. Member 1 owns all PostgreSQL/database work
--             represented by the TODOs below, including schema, constraints,
--             seed data, SQL queries, indexes, and database-side testing.
--
-- DATABASE CONTRACT
--   The names/types below MUST match main.cpp and the SRS.
--   If the team changes a field, update the C++ shared record and SRS together.
-- ============================================================================

-- ============================================================================
-- 0. DATABASE SETUP
-- ============================================================================
-- TODO [M1]: Create/select the development PostgreSQL database in pgAdmin.
-- TODO [M1]: Record the final database name used by the team.
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
-- TODO [M1]: Write the CREATE TABLE statement using exactly these columns.
-- TODO [M1]: Decide whether any additional uniqueness rule is actually needed.
--
-- Declaration shape only:
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
-- TODO [M1]: Write the CREATE TABLE statement.
-- TODO [M1]: Add a CHECK constraint so role only accepts the agreed values.
-- TODO [M1]: Decide whether phone uniqueness is required by the final rules.
--
-- Declaration shape only:
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
-- TODO [M1]: Write the CREATE TABLE statement.
-- TODO [M1]: Add CHECK constraints for type, status, and hourly_rate >= 0.
-- TODO [M1]: Decide whether station naming/numbering beyond SERIAL id is needed.
--
-- Declaration shape only:
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
-- TODO [M1]: Write the CREATE TABLE statement.
-- TODO [M1]: Add CHECK constraints for station_type, status, deposit >= 0.
-- TODO [M1]: Decide how your project should validate overlapping reservations.
--             Do not invent a different rule without documenting it in the SRS.
-- NOTE: Application-level reservation validation must use the same status and
--       station-type values documented here.
--
-- Declaration shape only:
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
-- TODO [M1]: Write the CREATE TABLE statement.
-- TODO [M1]: Add a CHECK rule so final_cost is never negative.
-- TODO [M1]: Decide/document whether the final schema will enforce only one
--             active session per station. If yes, add and test the database rule.
-- NOTE: The C++ application treats end_time = NULL as an open session and
--       the final integration must preserve that meaning.
--
-- Declaration shape only:
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

-- ============================================================================
-- 6. RELATIONSHIP MAP
-- ============================================================================
-- TODO [M1]: Ensure these relationships exist in the final implementation.
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
-- TODO [M1]: Implement and test constraints for:
--   [ ] Primary key on every table
--   [ ] Foreign key on every relationship
--   [ ] role is Admin / Staff / Customer
--   [ ] station type is PC / PS4 / PS5
--   [ ] station status is Available / InUse / Maintenance
--   [ ] reservation station_type is PC / PS4 / PS5
--   [ ] reservation status is Pending / Confirmed / Canceled
--   [ ] station hourly_rate is non-negative
--   [ ] reservation deposit_amount is non-negative
--   [ ] session final_cost is non-negative when supplied
--   [ ] Required text fields are NOT NULL
--
-- TODO [M1]: Record any additional constraints in the SRS before coding against them.

-- ============================================================================
-- 8. CRUD / SELECT WORK REQUIRED BY MEMBER 1
-- ============================================================================
-- The training requirements require the team to demonstrate database design,
-- INSERT, UPDATE, DELETE, SELECT, JOINs, keys/relationships and PostgreSQL
-- integration with C++.
--
-- TODO [M1]: Implement actual working examples for each of the following.
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
SELECT s.id, u.name, st.type, s.start_time, st.hourly_rate
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
-- TODO [M1]: Add indexes only where the implemented queries benefit from them.
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
-- TODO [M1]: Create realistic demo data sufficient for the final GUI demo:
--   * at least 2 branches
--   * at least 1 Admin
--   * at least 1 Staff
--   * several Customers
--   * multiple PC / PS4 / PS5 stations
--   * a mixture of station statuses
--   * Pending / Confirmed / Canceled reservations
--   * active and completed sessions for testing
--
-- TODO [M1]: Keep seed/test records clearly identifiable as development data.
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
-- TODO [M1]: Keep these exact textual values in the final SQL.
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
-- TODO [M1]: Before integration, verify the final SQL schema and test data:
-- NOTE: The corresponding C++ integration must be checked against this final schema.
-- The integration implementation is maintained separately by the C++ database integration owner.
--   [ ] All five required tables exist.
--   [ ] All columns match the SRS exactly.
--   [ ] PK/FK relationships work.
--   [ ] ON DELETE SET NULL / CASCADE behavior is tested.
--   [ ] Invalid role/type/status values are rejected.
--   [ ] CRUD operations work in pgAdmin.
--   [ ] JOIN queries produce the rows expected by the C++ records.
--   [ ] Active-session query matches ActiveSessionView fields.
--   [ ] Transactions leave the station/session consistent after failures.
--   [ ] libpqxx can connect using the final connection configuration.

-- END OF DECLARATION-ONLY TEMPLATE
-- The final executable schema must be implemented by Member 1.
