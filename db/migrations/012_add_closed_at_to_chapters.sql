-- 012_add_closed_at_to_chapters.sql

ALTER TABLE chapters ADD COLUMN closed_at TIMESTAMPTZ;  -- NULL = still in progress
