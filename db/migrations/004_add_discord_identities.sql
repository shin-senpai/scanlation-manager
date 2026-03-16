-- 004_add_discord_identities.sql

-- name becomes nullable since Discord users don't need a webapp username
-- until they explicitly set one
ALTER TABLE users ALTER COLUMN name DROP NOT NULL;

CREATE TABLE discord_identities (
    id SERIAL PRIMARY KEY,
    discord_id BIGINT NOT NULL UNIQUE,
    user_id INT NOT NULL REFERENCES users(id),
    linked_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
