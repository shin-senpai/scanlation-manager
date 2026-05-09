-- 022_add_level_to_tasks.sql
--
-- Adds a level column to tasks that is going to be used to prevent cyclic dependencies

ALTER TABLE tasks ADD COLUMN level INT;