-- Series-level placeholder slots. A placeholder at the series level means "we
-- need someone for this task on every chapter, but haven't found them yet."
-- When a new chapter is created, series-level placeholders cascade to it
-- automatically (same as series_assignments). Multiple placeholders per
-- (series_id, task_id) are supported — each represents one unfilled vacancy.
CREATE TABLE series_assignment_placeholders (
  id         SERIAL PRIMARY KEY,
  series_id  INT NOT NULL REFERENCES series(id) ON DELETE CASCADE,
  task_id    INT NOT NULL REFERENCES tasks(id)  ON DELETE CASCADE,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_sap_series_id   ON series_assignment_placeholders(series_id);
CREATE INDEX idx_sap_series_task ON series_assignment_placeholders(series_id, task_id);
