-- 003_add_user_credentials.sql

CREATE TABLE user_credentials (
    user_id INT NOT NULL REFERENCES users(id),
    password_hash TEXT NOT NULL,
    PRIMARY KEY (user_id)
);
