-- 027_add_active_since_to_todo_view.sql
-- Replaces days_active (a stale integer) with active_since (a timestamp).
-- The Go backend writes active_since into the Todo sheet as a DATE() formula so
-- that "Days Active" recalculates live in Google Sheets without needing a sync.
DROP VIEW IF EXISTS outstanding_chapter_assignments;
CREATE VIEW outstanding_chapter_assignments AS
WITH prereq_check AS (
  SELECT
    ca.chapter_id,
    ca.task_id,
    ca.user_id,
    CASE
      WHEN NOT EXISTS (
        SELECT 1 FROM task_dependencies td WHERE td.task_id = ca.task_id
      ) THEN TRUE
      WHEN EXISTS (
        SELECT 1 FROM task_dependencies td
        JOIN chapter_assignments prereq_ca
          ON prereq_ca.chapter_id = ca.chapter_id
         AND prereq_ca.task_id    = td.depends_on_task_id
        WHERE td.task_id = ca.task_id
          AND prereq_ca.completed_at IS NULL
      ) THEN FALSE
      ELSE TRUE
    END AS available,
    (
      SELECT MAX(prereq_ca.completed_at)
      FROM task_dependencies td
      JOIN chapter_assignments prereq_ca
        ON prereq_ca.chapter_id = ca.chapter_id
       AND prereq_ca.task_id    = td.depends_on_task_id
      WHERE td.task_id = ca.task_id
    ) AS latest_prereq_completed
  FROM chapter_assignments ca
  WHERE ca.completed_at IS NULL
)
SELECT
  s.name        AS series_name,
  c.number      AS chapter_number,
  c.name        AS chapter_name,
  c.volume      AS chapter_volume,
  t.name        AS task_name,
  t.level       AS task_level,
  u.display_name,
  GREATEST(ca.assigned_at, COALESCE(pc.latest_prereq_completed, ca.assigned_at)) AS active_since
FROM chapter_assignments ca
JOIN prereq_check pc ON pc.chapter_id = ca.chapter_id
                    AND pc.task_id    = ca.task_id
                    AND pc.user_id    = ca.user_id
JOIN chapters     c  ON c.id          = ca.chapter_id
JOIN series       s  ON s.id          = c.series_id
JOIN tasks        t  ON t.id          = ca.task_id
JOIN users        u  ON u.id          = ca.user_id
WHERE ca.completed_at IS NULL
  AND pc.available
  AND c.status = 'in_progress'
  AND s.status = 'active';
