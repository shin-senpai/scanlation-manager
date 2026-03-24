-- 005_enforce_webapp_name.sql

CREATE OR REPLACE FUNCTION check_user_name()
RETURNS TRIGGER AS $$
BEGIN
    -- Check if the user has no discord identity and no name
    IF (
        NOT EXISTS (SELECT 1 FROM discord_identities WHERE user_id = NEW.id)
        AND NEW.name IS NULL
    ) THEN
        RAISE EXCEPTION 'User must have a name if not linked to Discord';
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE CONSTRAINT TRIGGER enforce_user_name
AFTER INSERT OR UPDATE ON users
DEFERRABLE INITIALLY DEFERRED
FOR EACH ROW
EXECUTE FUNCTION check_user_name();

CREATE OR REPLACE FUNCTION check_discord_user_name()
RETURNS TRIGGER AS $$
BEGIN
    -- When unlinking discord, ensure the user has a name
    IF (
        NOT EXISTS (SELECT 1 FROM discord_identities WHERE user_id = NEW.user_id)
        AND NOT EXISTS (SELECT 1 FROM users WHERE id = NEW.user_id AND name IS NOT NULL)
    ) THEN
        RAISE EXCEPTION 'Cannot unlink Discord from a user with no name set';
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE CONSTRAINT TRIGGER enforce_discord_user_name
AFTER INSERT OR UPDATE OR DELETE ON discord_identities
DEFERRABLE INITIALLY DEFERRED
FOR EACH ROW
EXECUTE FUNCTION check_discord_user_name();
