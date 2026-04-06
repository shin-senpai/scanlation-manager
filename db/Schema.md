# Database Schema

PostgreSQL. Migrations are applied manually in order from `db/migrations/`.

---

## Tables

### `users`
Core identity record. Represents anyone who can use the platform.

| Column | Type | Notes |
|--------|------|-------|
| `id` | `SERIAL` | PK |
| `name` | `TEXT` | Unique webapp username. Nullable — Discord-only users don't need one until they set it. Once set, never deleted (prevents name recycling). |
| `display_name` | `TEXT` | Human-facing name |
| `permission_level` | `SMALLINT` | `0` = member, `1` = manager, `2` = supermanager. Default `0`. |
| `joined_at` | `TIMESTAMPTZ` | Default `now()` |
| `left_at` | `TIMESTAMPTZ` | `NULL` = currently active |

**Constraints:**
- A user must have either a `name` or an active `discord_identities` row — enforced by `enforce_user_name` trigger.
- At least one active user with `permission_level = 2` must always exist — enforced by `enforce_supermanager_exists` trigger.

---

### `discord_identities`
Links a Discord account to a `users` row. Supports unlinking — a row is soft-deleted via `unlinked_at` rather than removed, allowing re-linking later.

| Column | Type | Notes |
|--------|------|-------|
| `id` | `SERIAL` | PK |
| `discord_id` | `BIGINT` | Discord snowflake |
| `user_id` | `INT` | FK → `users(id)` |
| `linked_at` | `TIMESTAMPTZ` | Default `now()` |
| `unlinked_at` | `TIMESTAMPTZ` | `NULL` = currently active |

**Unique indexes (partial, scoped to active rows where `unlinked_at IS NULL`):**
- `one_active_discord_id` — a Discord account can only be linked to one user at a time.
- `one_active_discord_per_user` — a user can only have one active Discord link at a time.

**Constraints:**
- Unlinking a Discord identity from a user who has no `name` set is blocked by `enforce_discord_user_name` trigger.

---

### `user_credentials`
Stores hashed passwords for webapp login. One row per user, optional.

| Column | Type | Notes |
|--------|------|-------|
| `user_id` | `INT` | PK, FK → `users(id)` |
| `password_hash` | `TEXT` | |

---

### `user_aliases`
Tracks the alias a user goes by when completing work (e.g. the name that appears in a release credit). Only one active alias per user at a time.

| Column | Type | Notes |
|--------|------|-------|
| `id` | `SERIAL` | PK |
| `user_id` | `INT` | FK → `users(id)` |
| `alias` | `TEXT` | |
| `created_at` | `TIMESTAMPTZ` | Default `now()` |
| `retired_at` | `TIMESTAMPTZ` | `NULL` = currently active |

**Unique indexes (partial, scoped to active rows where `retired_at IS NULL`):**
- `one_active_alias` — no two users can hold the same active alias.
- `one_active_alias_per_user` — a user can only have one active alias at a time.

---

### `roles`
Custom capability labels (e.g. TL, TS, CLRD, QC, PR, RP) — used to describe what a user can do, not to control what they are assigned to.

| Column | Type | Notes |
|--------|------|-------|
| `id` | `SERIAL` | PK |
| `name` | `TEXT` | Unique |

---

### `user_roles`
Which roles a user holds (their general capabilities, independent of any specific series or chapter).

| Column | Type | Notes |
|--------|------|-------|
| `user_id` | `INT` | FK → `users(id)` |
| `role_id` | `INT` | FK → `roles(id)` |

PK: `(user_id, role_id)`

---

### `series`
A manga/manhwa/manhua title being worked on.

| Column | Type | Notes |
|--------|------|-------|
| `id` | `SERIAL` | PK |
| `name` | `TEXT` | Unique |
| `status` | `TEXT` | `'active'` \| `'completed'` \| `'dropped'` \| `'hiatus'` |
| `added_at` | `TIMESTAMPTZ` | Default `now()` |
| `closed_at` | `TIMESTAMPTZ` | `NULL` = still ongoing |

---

### `chapters`
Individual chapters belonging to a series.

| Column | Type | Notes |
|--------|------|-------|
| `id` | `SERIAL` | PK |
| `series_id` | `INT` | FK → `series(id)` |
| `name` | `TEXT` | e.g. `"Ch 51.1"` |
| `status` | `TEXT` | `'in_progress'` \| `'released'` \| `'dropped'` \| `'hiatus'`. Default `'in_progress'`. |
| `added_at` | `TIMESTAMPTZ` | Default `now()` |
| `closed_at` | `TIMESTAMPTZ` | `NULL` = still in progress |

Unique on `(series_id, name)`.

---

### `tasks`
Custom task types that must be completed before a chapter can be released (e.g. Translation, Typesetting, QC).

| Column | Type | Notes |
|--------|------|-------|
| `id` | `SERIAL` | PK |
| `name` | `TEXT` | Unique |

---

### `task_dependencies`
Defines which tasks must be completed before others can start.

| Column | Type | Notes |
|--------|------|-------|
| `task_id` | `INT` | FK → `tasks(id)` |
| `depends_on_task_id` | `INT` | FK → `tasks(id)` |

PK: `(task_id, depends_on_task_id)`

---

### `role_tasks`
Maps which tasks a role is responsible for. Used to validate task assignments — when assigning a user to a series or chapter for a given task, the task must be mapped to one of the user's roles.

| Column | Type | Notes |
|--------|------|-------|
| `role_id` | `INT` | FK → `roles(id)` |
| `task_id` | `INT` | FK → `tasks(id)` |

PK: `(role_id, task_id)`

---

### `series_assignments`
Which users are assigned to a series for a given task (the default crew for a series). Used to auto-populate `chapter_assignments` when a new chapter is created.

| Column | Type | Notes |
|--------|------|-------|
| `user_id` | `INT` | FK → `users(id)` |
| `series_id` | `INT` | FK → `series(id)` |
| `task_id` | `INT` | FK → `tasks(id)` |

PK: `(user_id, series_id, task_id)`

---

### `chapter_assignments`
Which users are assigned to a specific chapter for a given task. The to-do list for a chapter is rows where `completed_at IS NULL`; completion history is where `completed_at IS NOT NULL`.

| Column | Type | Notes |
|--------|------|-------|
| `user_id` | `INT` | FK → `users(id)` |
| `chapter_id` | `INT` | FK → `chapters(id)` |
| `task_id` | `INT` | FK → `tasks(id)` |
| `completed_at` | `TIMESTAMPTZ` | `NULL` = not yet done |

PK: `(user_id, chapter_id, task_id)`

---

## Triggers

| Trigger | Table | Event | Purpose |
|---------|-------|-------|---------|
| `enforce_supermanager_exists` | `users` | `INSERT`, `UPDATE`, `DELETE` | Ensures at least one active user with `permission_level = 2` always exists |
| `enforce_user_name` | `users` | `INSERT`, `UPDATE` | A user with no active Discord identity must have a `name` |
| `enforce_discord_user_name` | `discord_identities` | `INSERT`, `UPDATE`, `DELETE` | Blocks unlinking Discord from a user who has no `name` |

All triggers are `DEFERRABLE INITIALLY DEFERRED`.

---

## Key Design Notes

- **User identity:** A user exists in `users` and is reachable via either `name` (webapp) or an active `discord_identities` row (bot). Both can be active simultaneously.
- **Aliases vs names:** `users.name` is the login/identity name; `user_aliases` is the public credit alias used in release history.
- **Permissions:** `permission_level` is a single integer (`0`/`1`/`2`) replacing the old `is_manager`/`is_supermanager` boolean flags. Enforced via a `CHECK` constraint.
- **Roles:** `roles` and `user_roles` describe a user's general capabilities. `role_tasks` maps roles to tasks and is used only for assignment validation.
- **Assignments are task-scoped:** `series_assignments` and `chapter_assignments` link users to specific tasks, not roles. This eliminates ambiguity when validating work progress events — the check is a direct lookup on `(user, chapter, task)`.
- **Task tracking:** `chapter_assignments.completed_at` is both the "done" flag and the completion record. To-do list = `WHERE completed_at IS NULL`; completion history = `WHERE completed_at IS NOT NULL`.
- **Soft deletes:** Users, aliases, and Discord identities are never hard-deleted — `left_at`, `retired_at`, and `unlinked_at` mark deactivation, preserving historical completion records.
