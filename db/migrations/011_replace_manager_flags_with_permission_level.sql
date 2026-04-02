-- 011_replace_manager_flags_with_permission_level.sql

-- Replace is_manager and is_supermanager boolean flags with a single
-- permission_level column.
--
-- 0 = regular member
-- 1 = manager
-- 2 = supermanager

ALTER TABLE users DROP COLUMN is_manager;
ALTER TABLE users DROP COLUMN is_supermanager;

ALTER TABLE users ADD COLUMN permission_level SMALLINT NOT NULL DEFAULT 0
    CHECK (permission_level IN (0, 1, 2));

-- Drop the old supermanager enforcement trigger and function.
DROP TRIGGER enforce_supermanager_exists ON users;
DROP FUNCTION check_supermanager_exists();

-- Ensure at least one active user with permission_level = 2 always exists.
CREATE OR REPLACE FUNCTION check_supermanager_exists()
RETURNS TRIGGER AS $$
BEGIN
    IF (SELECT COUNT(*) FROM users WHERE permission_level = 2 AND left_at IS NULL) = 0 THEN
        RAISE EXCEPTION 'There must be at least one active supermanager (permission_level = 2)';
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE CONSTRAINT TRIGGER enforce_supermanager_exists
AFTER INSERT OR UPDATE OR DELETE ON users
DEFERRABLE INITIALLY DEFERRED
FOR EACH ROW
EXECUTE FUNCTION check_supermanager_exists();
