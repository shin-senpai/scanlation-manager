-- 009_role_task_mapping_and_assignment_refactor.sql

-- Maps which tasks a role is responsible for.
-- Used to validate task assignments when assigning users to a series or chapter.
CREATE TABLE role_tasks (
    role_id INT NOT NULL REFERENCES roles(id),
    task_id INT NOT NULL REFERENCES tasks(id),
    PRIMARY KEY (role_id, task_id)
);

-- Replace role-based assignments with task-based assignments.

DROP TABLE series_assignments;
CREATE TABLE series_assignments (
    user_id INT NOT NULL REFERENCES users(id),
    series_id INT NOT NULL REFERENCES series(id),
    task_id INT NOT NULL REFERENCES tasks(id),
    PRIMARY KEY (user_id, series_id, task_id)
);

DROP TABLE chapter_assignments;
CREATE TABLE chapter_assignments (
    user_id INT NOT NULL REFERENCES users(id),
    chapter_id INT NOT NULL REFERENCES chapters(id),
    task_id INT NOT NULL REFERENCES tasks(id),
    PRIMARY KEY (user_id, chapter_id, task_id)
);
