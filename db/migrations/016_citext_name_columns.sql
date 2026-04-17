-- 016_citext_name_columns.sql
--
-- Makes name columns on roles, tasks, series, and chapters case-insensitive
-- using the citext extension. Existing UNIQUE constraints automatically adopt
-- citext comparison rules after the type change — no need to recreate them.
-- Values are stored as-is (case preserved); only comparisons are case-insensitive.

CREATE EXTENSION IF NOT EXISTS citext;

ALTER TABLE roles ALTER COLUMN name TYPE citext;
ALTER TABLE tasks ALTER COLUMN name TYPE citext;
ALTER TABLE series ALTER COLUMN name TYPE citext;
ALTER TABLE chapters ALTER COLUMN name TYPE citext;
