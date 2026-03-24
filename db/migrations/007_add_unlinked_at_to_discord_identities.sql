-- 007_add_unlinked_at_to_discord_identities.sql

-- 1. Add the unlinked_at column
ALTER TABLE discord_identities
    ADD COLUMN unlinked_at TIMESTAMPTZ;

-- 2. Drop the hard unique constraint on discord_id (column-level UNIQUE),
--    and replace both it and the missing user_id uniqueness with partial
--    indexes scoped to active (unlinked_at IS NULL) rows only.
--    This allows a discord_id or user_id to be re-linked after unlinking.
ALTER TABLE discord_identities
    DROP CONSTRAINT discord_identities_discord_id_key;

CREATE UNIQUE INDEX one_active_discord_id
    ON discord_identities (discord_id)
    WHERE unlinked_at IS NULL;

CREATE UNIQUE INDEX one_active_discord_per_user
    ON discord_identities (user_id)
    WHERE unlinked_at IS NULL;

-- 3. Update trigger functions from 005 to only consider active links

CREATE OR REPLACE FUNCTION check_user_name()
RETURNS TRIGGER AS $$
BEGIN
    IF (
        NOT EXISTS (
            SELECT 1 FROM discord_identities
            WHERE user_id = NEW.id
              AND unlinked_at IS NULL
        )
        AND NEW.name IS NULL
    ) THEN
        RAISE EXCEPTION 'User must have a name if not linked to Discord';
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION check_discord_user_name()
RETURNS TRIGGER AS $$
BEGIN
    IF (
        NOT EXISTS (
            SELECT 1 FROM discord_identities
            WHERE user_id = COALESCE(NEW.user_id, OLD.user_id)
              AND unlinked_at IS NULL
        )
        AND NOT EXISTS (
            SELECT 1 FROM users
            WHERE id = COALESCE(NEW.user_id, OLD.user_id)
              AND name IS NOT NULL
        )
    ) THEN
        RAISE EXCEPTION 'Cannot unlink Discord from a user with no name set';
    END IF;
    RETURN COALESCE(NEW, OLD);
END;
$$ LANGUAGE plpgsql;