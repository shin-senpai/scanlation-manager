-- 018_cascade_deletes_and_task_retirement.sql
--
-- 1. Adds ON DELETE CASCADE to FK constraints where the child rows have no
--    historical value and should be cleaned up automatically when the parent
--    role or task is deleted.
--
--    chapter_assignments.task_id is intentionally excluded — those rows are
--    historical completion records and must be preserved. The application
--    checks for completed records before deleting a task; if any exist, it
--    retires the task instead.
--
-- 2. Adds retired_at to tasks for soft-deletion when completion history exists.

-- user_roles: cascade on role deletion
ALTER TABLE user_roles
  DROP CONSTRAINT user_roles_role_id_fkey,
  ADD CONSTRAINT user_roles_role_id_fkey
    FOREIGN KEY (role_id) REFERENCES roles(id) ON DELETE CASCADE;

-- role_tasks: cascade on role or task deletion
ALTER TABLE role_tasks
  DROP CONSTRAINT role_tasks_role_id_fkey,
  ADD CONSTRAINT role_tasks_role_id_fkey
    FOREIGN KEY (role_id) REFERENCES roles(id) ON DELETE CASCADE;

ALTER TABLE role_tasks
  DROP CONSTRAINT role_tasks_task_id_fkey,
  ADD CONSTRAINT role_tasks_task_id_fkey
    FOREIGN KEY (task_id) REFERENCES tasks(id) ON DELETE CASCADE;

-- series_assignments: cascade on task deletion
ALTER TABLE series_assignments
  DROP CONSTRAINT series_assignments_task_id_fkey,
  ADD CONSTRAINT series_assignments_task_id_fkey
    FOREIGN KEY (task_id) REFERENCES tasks(id) ON DELETE CASCADE;

-- task_dependencies: cascade on task deletion (both directions)
ALTER TABLE task_dependencies
  DROP CONSTRAINT task_dependencies_task_id_fkey,
  ADD CONSTRAINT task_dependencies_task_id_fkey
    FOREIGN KEY (task_id) REFERENCES tasks(id) ON DELETE CASCADE;

ALTER TABLE task_dependencies
  DROP CONSTRAINT task_dependencies_depends_on_task_id_fkey,
  ADD CONSTRAINT task_dependencies_depends_on_task_id_fkey
    FOREIGN KEY (depends_on_task_id) REFERENCES tasks(id) ON DELETE CASCADE;

-- Soft-delete support for tasks
ALTER TABLE tasks ADD COLUMN retired_at TIMESTAMPTZ;

-- Prevent deletion of completed chapter_assignments rows.
-- Completed rows are historical records and must never be removed.
-- Outstanding rows (completed_at IS NULL) can still be deleted freely
-- (e.g. when cleaning up before a hard task delete).
CREATE OR REPLACE FUNCTION prevent_completed_assignment_delete()
RETURNS TRIGGER AS $$
BEGIN
  IF OLD.completed_at IS NOT NULL THEN
    RAISE EXCEPTION
      'Cannot delete a completed chapter assignment (user_id=%, chapter_id=%, task_id=%)',
      OLD.user_id, OLD.chapter_id, OLD.task_id;
  END IF;
  RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER enforce_completed_assignment_immutable
  BEFORE DELETE ON chapter_assignments
  FOR EACH ROW EXECUTE FUNCTION prevent_completed_assignment_delete();
