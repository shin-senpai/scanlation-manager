-- 023_add_assigned_at_to_chapter_assignments.sql
-- Tracks when a chapter assignment was created so we can calculate how long a task
-- has been active (e.g. for the Google Sheets "Days Active" column).
-- Existing rows receive the current timestamp as a best-effort fallback.
ALTER TABLE chapter_assignments
  ADD COLUMN assigned_at TIMESTAMPTZ NOT NULL DEFAULT NOW();
