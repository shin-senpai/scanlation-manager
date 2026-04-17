-- 015_add_missing_indexes.sql
--
-- The PKs on these tables have the frequently-queried column in a non-leftmost
-- position, so PostgreSQL cannot use the PK index for those lookups.

CREATE INDEX idx_series_assignments_series_id ON series_assignments (series_id);
CREATE INDEX idx_chapter_assignments_chapter_id ON chapter_assignments (chapter_id);
CREATE INDEX idx_task_dependencies_depends_on_task_id ON task_dependencies (depends_on_task_id);
CREATE INDEX idx_role_tasks_task_id ON role_tasks (task_id);
