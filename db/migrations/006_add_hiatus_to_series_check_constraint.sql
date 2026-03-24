-- 006_add_hiatus_to_series_check_constraint.sql

ALTER TABLE series DROP CONSTRAINT series_status_check;

ALTER TABLE series
ADD CONSTRAINT series_status_check
CHECK (status IN ('active', 'completed', 'dropped', 'hiatus'));