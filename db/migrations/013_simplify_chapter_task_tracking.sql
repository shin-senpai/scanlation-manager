-- 013_simplify_chapter_task_tracking.sql

-- chapter_task_completions and chapter_tasks are removed in favour of a
-- completed_at column directly on chapter_assignments. The to-do list for a
-- chapter is chapter_assignments WHERE completed_at IS NULL; completion history
-- is WHERE completed_at IS NOT NULL.

DROP TABLE chapter_task_completions;
DROP TABLE chapter_tasks;

ALTER TABLE chapter_assignments ADD COLUMN completed_at TIMESTAMPTZ;  -- NULL = not yet done
