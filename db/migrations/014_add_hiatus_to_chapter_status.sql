-- 014_add_hiatus_to_chapter_status.sql

ALTER TABLE chapters DROP CONSTRAINT chapters_status_check;

ALTER TABLE chapters
ADD CONSTRAINT chapters_status_check
CHECK (status IN ('in_progress', 'released', 'dropped', 'hiatus'));
