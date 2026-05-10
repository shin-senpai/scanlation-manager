-- 019_uppercase_role_task_names.sql
--
-- Role and task names are short identifiers (e.g. TL, QC, PR) that should
-- always be stored in uppercase for consistency. A BEFORE INSERT OR UPDATE
-- trigger normalizes the name column on roles and tasks so callers can pass
-- any casing and always read back the canonical uppercase form.
--
-- series.name and chapters.name are intentionally excluded — those are
-- human-readable display names where casing is meaningful.

CREATE OR REPLACE FUNCTION normalize_name_to_upper()
RETURNS TRIGGER AS $$
BEGIN
  NEW.name = UPPER(NEW.name);
  RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER roles_normalize_name
  BEFORE INSERT OR UPDATE OF name ON roles
  FOR EACH ROW EXECUTE FUNCTION normalize_name_to_upper();

CREATE TRIGGER tasks_normalize_name
  BEFORE INSERT OR UPDATE OF name ON tasks
  FOR EACH ROW EXECUTE FUNCTION normalize_name_to_upper();

-- Normalize any existing rows
UPDATE roles SET name = UPPER(name);
UPDATE tasks SET name = UPPER(name);
