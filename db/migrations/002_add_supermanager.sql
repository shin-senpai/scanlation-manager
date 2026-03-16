-- 002_add_supermanager.sql

ALTER TABLE users ADD COLUMN is_supermanager BOOLEAN NOT NULL DEFAULT FALSE;

CREATE OR REPLACE FUNCTION check_supermanager_exists()
RETURNS TRIGGER AS $$
BEGIN
    IF (SELECT COUNT(*) FROM users WHERE is_supermanager = TRUE) = 0 THEN
        RAISE EXCEPTION 'There must be at least one supermanager';
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE CONSTRAINT TRIGGER enforce_supermanager_exists
AFTER UPDATE OR DELETE ON users
DEFERRABLE INITIALLY DEFERRED
FOR EACH ROW
EXECUTE FUNCTION check_supermanager_exists();
