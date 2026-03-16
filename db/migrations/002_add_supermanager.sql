-- 002_add_supermanager.sql

ALTER TABLE users ADD COLUMN is_supermanager BOOLEAN NOT NULL DEFAULT FALSE;
