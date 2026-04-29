-- 020_add_volume_to_chapters.sql
--
-- Adds an optional volume number to chapters for series that use a volume
-- structure. NULL means the chapter is not associated with a specific volume
-- (e.g. web releases). No uniqueness constraint — multiple chapters can share
-- the same volume number.

ALTER TABLE chapters ADD COLUMN volume INT;
