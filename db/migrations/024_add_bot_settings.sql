-- 024_add_bot_settings.sql
-- Key-value store for bot-level configuration that is managed via Discord commands
-- (e.g. enabling/disabling the Google Sheets integration).
CREATE TABLE bot_settings (
  key   TEXT PRIMARY KEY,
  value TEXT NOT NULL
);
