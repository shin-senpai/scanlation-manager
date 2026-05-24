-- Add 'queued' chapter status.
-- A queued chapter is functionally equivalent to in_progress (all operations work,
-- including work-updates, assignments, and placeholders), but it is excluded from
-- the /todo command and the GSheet Todo tab. The intent is that staff should focus
-- on the current in_progress chapter before seeing subsequent ones.
--
-- Auto-assignment: when a new chapter is added and an in_progress chapter already
-- exists in the series, the new chapter defaults to queued.
-- Auto-promotion: when an in_progress chapter is released/dropped/set to hiatus,
-- the next chapter (by number) that has queued status is promoted to in_progress.

ALTER TABLE chapters DROP CONSTRAINT chapters_status_check;
ALTER TABLE chapters
  ADD CONSTRAINT chapters_status_check
  CHECK (status IN ('in_progress', 'queued', 'released', 'dropped', 'hiatus'));
