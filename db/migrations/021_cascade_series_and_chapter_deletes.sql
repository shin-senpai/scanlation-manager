-- 021_cascade_series_and_chapter_deletes.sql
--
-- Adds ON DELETE CASCADE to three FK constraints so that deleting a series or
-- chapter automatically cleans up subordinate rows:
--
--   series  → chapters            (chapters.series_id)
--   series  → series_assignments  (series_assignments.series_id)
--   chapter → chapter_assignments (chapter_assignments.chapter_id)
--
-- The chapter_assignments immutability trigger (enforce_completed_assignment_immutable)
-- still fires on cascaded deletes, so application code must clear completed_at
-- on chapter_assignments before deleting a chapter/series that has completed
-- history (supermanager path).

-- chapters: cascade on series deletion
ALTER TABLE chapters
  DROP CONSTRAINT chapters_series_id_fkey,
  ADD CONSTRAINT chapters_series_id_fkey
    FOREIGN KEY (series_id) REFERENCES series(id) ON DELETE CASCADE;

-- series_assignments: cascade on series deletion
ALTER TABLE series_assignments
  DROP CONSTRAINT series_assignments_series_id_fkey,
  ADD CONSTRAINT series_assignments_series_id_fkey
    FOREIGN KEY (series_id) REFERENCES series(id) ON DELETE CASCADE;

-- chapter_assignments: cascade on chapter deletion
ALTER TABLE chapter_assignments
  DROP CONSTRAINT chapter_assignments_chapter_id_fkey,
  ADD CONSTRAINT chapter_assignments_chapter_id_fkey
    FOREIGN KEY (chapter_id) REFERENCES chapters(id) ON DELETE CASCADE;
