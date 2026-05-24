-- Placeholder slots for chapter assignments. A placeholder indicates that a staff
-- member is needed for a given chapter+task but has not been found yet. Multiple
-- placeholders per (chapter_id, task_id) are supported — each represents one
-- unfilled vacancy. When a user is assigned, one placeholder is consumed.
CREATE TABLE chapter_assignment_placeholders (
  id         SERIAL PRIMARY KEY,
  chapter_id INT NOT NULL REFERENCES chapters(id) ON DELETE CASCADE,
  task_id    INT NOT NULL REFERENCES tasks(id)    ON DELETE CASCADE,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_cap_chapter_id   ON chapter_assignment_placeholders(chapter_id);
CREATE INDEX idx_cap_chapter_task ON chapter_assignment_placeholders(chapter_id, task_id);
