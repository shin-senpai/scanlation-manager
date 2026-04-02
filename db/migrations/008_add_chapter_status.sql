-- 008_add_chapter_status.sql

ALTER TABLE chapters ADD COLUMN status TEXT NOT NULL DEFAULT 'in_progress'
    CHECK (status IN ('in_progress', 'released', 'dropped'));
