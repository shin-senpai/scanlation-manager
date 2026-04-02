-- 010_fix_supermanager_check.sql

-- Fix the supermanager existence check to only consider active users (left_at IS NULL).
-- Previously it counted all supermanagers including ones who have left, meaning
-- the last active supermanager could leave without being blocked.

CREATE OR REPLACE FUNCTION check_supermanager_exists()
RETURNS TRIGGER AS $$
BEGIN
    IF (SELECT COUNT(*) FROM users WHERE is_supermanager = TRUE AND left_at IS NULL) = 0 THEN
        RAISE EXCEPTION 'There must be at least one active supermanager';
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;
