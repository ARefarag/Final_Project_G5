--Rolling back as any transaction error that happened in the middle of execution must be rolled back to be fixed 
ROLLBACK;
-- FRESH-DEVELOPMENT-DB ONLY: this script drops and recreates the tables below.
-- For an existing local database, use migrations/001_auth_management.sql instead.
DROP TABLE IF EXISTS sessions, reservations, stations, users, branches CASCADE;
CREATE EXTENSION IF NOT EXISTS pgcrypto;
-- ============================================================================
-- GAME&GO - GAMING LOUNGE TIME & TAB MANAGER
-- ============================================================================
-- 1. TABLE: branches
-- ============================================================================
-- PURPOSE:
--   Stores each physical Game&Go lounge branch.
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
 CREATE TABLE users (
     id SERIAL PRIMARY KEY,
     name VARCHAR(100) NOT NULL,
     role VARCHAR(50) NOT NULL,
     phone VARCHAR(20),
     branch_id INT REFERENCES branches(id) ON DELETE SET NULL,
     username VARCHAR(64) NOT NULL UNIQUE,
     password_hash TEXT NOT NULL,
        CHECK (role IN ('Admin', 'Staff', 'Customer'))
 );

-- ============================================================================
-- 3. TABLE: stations
-- ============================================================================
-- PURPOSE:
--   Stores the physical gaming stations in every branch.
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
-- 6. DEVELOPMENT SEED DATA FOR TESTING WITH GUI
-- ============================================================================
INSERT INTO branches (branch_name, street_address, district, location_city, location_country) VALUES
('dev-Main Branch', '111 Sidi-gaber St', 'Smouha', 'Alexandria', 'Egypt'),
('dev-Uptown Branch', '456 El-eqbal St', 'Victoria', 'Alexandria', 'Egypt'),
('dev-Maadi Branch', '789 Maadi St', 'Maadi', 'Cairo', 'Egypt');


-- ADDED: all seeded accounts share this development-only password: Welcome123!
INSERT INTO users (name, role, phone, branch_id, username, password_hash) VALUES
('dev-Alice Admin', 'Admin', '123-456-7890', NULL, 'alice.admin', crypt('Welcome123!', gen_salt('bf'))),
('dev-John Admin', 'Admin', '987-654-3210', NULL, 'john.admin', crypt('Welcome123!', gen_salt('bf'))),
('dev-Jane Admin', 'Admin', '555-555-5555', NULL, 'jane.admin', crypt('Welcome123!', gen_salt('bf'))),
('dev-Bob Staff', 'Staff', '234-567-8901', 1, 'bob.staff', crypt('Welcome123!', gen_salt('bf'))),
('dev-Malak Staff','Staff','342-578-9012',1, 'malak.staff', crypt('Welcome123!', gen_salt('bf'))),
('dev-Eve Staff', 'Staff', '567-890-1234', 2, 'eve.staff', crypt('Welcome123!', gen_salt('bf'))),
('dev-Frank Staff', 'Staff', '678-901-2345', 2, 'frank.staff', crypt('Welcome123!', gen_salt('bf'))),
('dev-Grace Staff', 'Staff', '789-012-3456', 3, 'grace.staff', crypt('Welcome123!', gen_salt('bf'))),
('dev-Heidi Staff', 'Staff', '890-123-4567', 3, 'heidi.staff', crypt('Welcome123!', gen_salt('bf'))),
('dev-Charlie Customer', 'Customer', '345-678-9012', 1, 'charlie.customer', crypt('Welcome123!', gen_salt('bf'))),
('dev-Adam Customer', 'Customer', '012-759-880', 1, 'adam.customer', crypt('Welcome123!', gen_salt('bf'))),
('dev-Sandy Customer', 'Customer', '015-888-333', 2, 'sandy.customer', crypt('Welcome123!', gen_salt('bf'))),
('dev-Oscar Customer', 'Customer', '019-777-444', 3, 'oscar.customer', crypt('Welcome123!', gen_salt('bf'))),
('dev-Diana Customer', 'Customer', '456-789-0123', 3, 'diana.customer', crypt('Welcome123!', gen_salt('bf')));


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
-- 7. INDEX WORK
-- ============================================================================
--speed up queries filterred by branch
CREATE INDEX idx_users_branch ON users(branch_id);
CREATE UNIQUE INDEX idx_users_username_lower ON users (lower(username)); 
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

--Save our changes so we can rollback if anything goes wrong
COMMIT;

-- ============================================================================
-- 8. CRUD / SELECT WORK
-- ============================================================================
-- The training requirements require the team to demonstrate database design,
-- INSERT, UPDATE, DELETE, SELECT, JOINs, keys/relationships and PostgreSQL
-- integration with C++.

--SELECT Tests
SELECT * FROM branches;
SELECT id, name, role, phone, branch_id, username FROM users;
SELECT * FROM stations WHERE branch_id = 1; -- test branch id
SELECT * FROM reservations WHERE status = 'Pending';
SELECT * FROM sessions WHERE end_time IS NULL;

--JOIN Tests

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

-- AGGREGATE Tests

SELECT status, COUNT(*) FROM stations GROUP BY status;
SELECT status, COUNT(*) FROM reservations GROUP BY status;
SELECT SUM(final_cost) FROM sessions WHERE end_time IS NOT NULL;

--UPDATE Tests
-- The transaction is rolled back so the demo data remains unchanged.
BEGIN;

UPDATE stations
SET status = 'Maintenance'
WHERE id = 1;

UPDATE reservations
SET status = 'Confirmed'
WHERE id = 1;

UPDATE users
SET phone = '000-000-0000'
WHERE id = 10;

SELECT id, status
FROM stations
WHERE id = 1;

SELECT id, status
FROM reservations
WHERE id = 1;

SELECT id, phone
FROM users
WHERE id = 10;

ROLLBACK;

--DELETE Tests
-- The transaction is rolled back so the demo data remains unchanged.
BEGIN;

DELETE FROM branches
WHERE id = 3;

SELECT id
FROM branches
WHERE id = 3;

SELECT COUNT(*) AS remaining_branch_stations
FROM stations
WHERE branch_id = 3;

SELECT COUNT(*) AS remaining_branch_reservations
FROM reservations
WHERE branch_id = 3;

SELECT id, branch_id
FROM users
WHERE id IN (8, 9, 13, 14)
ORDER BY id;

ROLLBACK;

