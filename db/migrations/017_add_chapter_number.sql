-- 017_add_chapter_number.sql
--
-- Adds a numeric chapter number used for sorting. The display name (chapters.name)
-- is kept separate — number is purely for ordering and identity.
--
-- NOTE: This column is NOT NULL with no default. If the table has existing rows,
-- you must populate them before applying this migration or it will fail.

ALTER TABLE chapters ADD COLUMN number NUMERIC(6,2) NOT NULL;

CREATE UNIQUE INDEX chapters_series_id_number_key
    ON chapters (series_id, number);
