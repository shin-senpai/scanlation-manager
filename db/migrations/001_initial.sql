-- 001_initial.sql

CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    display_name TEXT NOT NULL,
    is_manager BOOLEAN NOT NULL DEFAULT FALSE,
    joined_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    left_at TIMESTAMPTZ  -- NULL means currently active
);

-- name is never deleted from the users table, so the UNIQUE constraint
-- on name naturally prevents any other user from ever taking it

CREATE TABLE user_aliases (
    id SERIAL PRIMARY KEY,
    user_id INT NOT NULL REFERENCES users(id),
    alias TEXT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    retired_at TIMESTAMPTZ  -- NULL means currently active
);

-- Only one active alias per alias text across all users
CREATE UNIQUE INDEX one_active_alias
ON user_aliases (alias)
WHERE retired_at IS NULL;

-- Only one active alias per user at a time
CREATE UNIQUE INDEX one_active_alias_per_user
ON user_aliases (user_id)
WHERE retired_at IS NULL;

CREATE TABLE roles (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE  -- TL, TS, CLRD, QC, PR, RP
);

CREATE TABLE series (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    status TEXT NOT NULL DEFAULT 'active' CHECK (status IN ('active', 'completed', 'dropped')),
    added_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    closed_at TIMESTAMPTZ  -- NULL means still active, set when completed or dropped
);

CREATE TABLE chapters (
    id SERIAL PRIMARY KEY,
    series_id INT NOT NULL REFERENCES series(id),
    name TEXT NOT NULL,  -- "Ch 51.1"
    added_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (series_id, name)
);

CREATE TABLE tasks (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE
);

CREATE TABLE task_dependencies (
    task_id INT NOT NULL REFERENCES tasks(id),
    depends_on_task_id INT NOT NULL REFERENCES tasks(id),
    PRIMARY KEY (task_id, depends_on_task_id)
);

-- Auto-populated when a chapter is created or a new task is added
-- Tracks the todo list for each chapter
CREATE TABLE chapter_tasks (
    chapter_id INT NOT NULL REFERENCES chapters(id),
    task_id INT NOT NULL REFERENCES tasks(id),
    completed_at TIMESTAMPTZ,  -- NULL means not yet completed
    PRIMARY KEY (chapter_id, task_id)
);

-- Tracks which users completed a chapter task
-- Populated from chapter_assignments at the time of completion
CREATE TABLE chapter_task_completions (
    chapter_id INT NOT NULL REFERENCES chapters(id),
    task_id INT NOT NULL REFERENCES tasks(id),
    user_id INT NOT NULL REFERENCES users(id),
    completed_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    PRIMARY KEY (chapter_id, task_id, user_id),
    FOREIGN KEY (chapter_id, task_id) REFERENCES chapter_tasks(chapter_id, task_id)
);

CREATE TABLE user_roles (
    user_id INT NOT NULL REFERENCES users(id),
    role_id INT NOT NULL REFERENCES roles(id),
    PRIMARY KEY (user_id, role_id)
);

CREATE TABLE series_assignments (
    user_id INT NOT NULL REFERENCES users(id),
    series_id INT NOT NULL REFERENCES series(id),
    role_id INT NOT NULL REFERENCES roles(id),
    PRIMARY KEY (user_id, series_id, role_id)
);

CREATE TABLE chapter_assignments (
    user_id INT NOT NULL REFERENCES users(id),
    chapter_id INT NOT NULL REFERENCES chapters(id),
    role_id INT NOT NULL REFERENCES roles(id),
    PRIMARY KEY (user_id, chapter_id, role_id)
);
