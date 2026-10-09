-- ADDED: migrate an EXISTING LOCAL Game&Go PostgreSQL database without dropping records.
-- Run this against the local `gameandgo` database once. Safe to rerun in normal cases.
BEGIN;
CREATE EXTENSION IF NOT EXISTS pgcrypto;

ALTER TABLE users ADD COLUMN IF NOT EXISTS username VARCHAR(64);
ALTER TABLE users ADD COLUMN IF NOT EXISTS password_hash TEXT;

-- Keep expected usernames for known development seed accounts. Other old accounts
-- receive a predictable user<ID> login so they can be reset after migration.
UPDATE users
SET username = CASE lower(name)
    WHEN 'dev-alice admin' THEN 'alice.admin'
    WHEN 'dev-john admin' THEN 'john.admin'
    WHEN 'dev-jane admin' THEN 'jane.admin'
    WHEN 'dev-bob staff' THEN 'bob.staff'
    WHEN 'dev-malak staff' THEN 'malak.staff'
    WHEN 'dev-eve staff' THEN 'eve.staff'
    WHEN 'dev-frank staff' THEN 'frank.staff'
    WHEN 'dev-grace staff' THEN 'grace.staff'
    WHEN 'dev-heidi staff' THEN 'heidi.staff'
    WHEN 'dev-charlie customer' THEN 'charlie.customer'
    WHEN 'dev-adam customer' THEN 'adam.customer'
    WHEN 'dev-sandy customer' THEN 'sandy.customer'
    WHEN 'dev-oscar customer' THEN 'oscar.customer'
    WHEN 'dev-diana customer' THEN 'diana.customer'
    ELSE 'user' || id::text
END
WHERE username IS NULL OR btrim(username) = '';

-- ADDED: password for legacy accounts without credentials is temporary and shared
-- for development only. Replace it after first login before production use.
UPDATE users
SET password_hash = crypt('Welcome123!', gen_salt('bf'))
WHERE password_hash IS NULL OR btrim(password_hash) = '';

ALTER TABLE users ALTER COLUMN username SET NOT NULL;
ALTER TABLE users ALTER COLUMN password_hash SET NOT NULL;
CREATE UNIQUE INDEX IF NOT EXISTS idx_users_username_lower ON users (lower(username));
COMMIT;
