-- Make chapters.name nullable to support chapters identified by number only.
ALTER TABLE chapters ALTER COLUMN name DROP NOT NULL;
