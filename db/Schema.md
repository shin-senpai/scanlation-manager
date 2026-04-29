# Database Schema

PostgreSQL. Migrations are applied manually in order from `db/migrations/`. The `citext` extension is required (enabled by migration 016).

---

## Tables

### `users`

Core identity record. Represents anyone who can use the platform.

| Column | Type | Nullable | Default | Notes |
|--------|------|----------|---------|-------|
| `id` | `SERIAL` | No | — | PK |
| `name` | `TEXT` | Yes | — | Unique webapp username. Nullable — Discord-only users don't need one. Once set, never deleted (prevents name recycling). |
| `display_name` | `TEXT` | No | — | Human-facing name shown in the bot |
| `permission_level` | `SMALLINT` | No | `0` | `0` = member, `1` = manager, `2` = supermanager |
| `joined_at` | `TIMESTAMPTZ` | No | `NOW()` | |
| `left_at` | `TIMESTAMPTZ` | Yes | — | `NULL` = currently active |

**Check constraints:**
- `permission_level IN (0, 1, 2)`

**Unique constraints:**
- `users_name_key` on `(name)` — webapp usernames are globally unique, even after a user leaves

**Triggers:**
- `enforce_supermanager_exists` — after any INSERT/UPDATE/DELETE, ensures at least one row exists with `permission_level = 2 AND left_at IS NULL`
- `enforce_user_name` — after INSERT/UPDATE, ensures a user with no active `discord_identities` row must have `name` set

---

### `discord_identities`

Links a Discord account to a `users` row. Supports unlinking — rows are soft-deleted via `unlinked_at` rather than removed, allowing re-linking later.

| Column | Type | Nullable | Default | Notes |
|--------|------|----------|---------|-------|
| `id` | `SERIAL` | No | — | PK |
| `discord_id` | `BIGINT` | No | — | Discord snowflake |
| `user_id` | `INT` | No | — | FK → `users(id)` |
| `linked_at` | `TIMESTAMPTZ` | No | `NOW()` | |
| `unlinked_at` | `TIMESTAMPTZ` | Yes | — | `NULL` = currently active |

**Unique indexes (partial, WHERE `unlinked_at IS NULL`):**
- `one_active_discord_id` on `(discord_id)` — a Discord account can only be linked to one user at a time
- `one_active_discord_per_user` on `(user_id)` — a user can only have one active Discord link at a time

**Triggers:**
- `enforce_discord_user_name` — after any INSERT/UPDATE/DELETE, blocks unlinking Discord from a user who has no `name` set (would leave them unreachable)

---

### `user_credentials`

Stores hashed passwords for webapp login. Optional — Discord-only users have no row here.

| Column | Type | Nullable | Default | Notes |
|--------|------|----------|---------|-------|
| `user_id` | `INT` | No | — | PK, FK → `users(id)` |
| `password_hash` | `TEXT` | No | — | |

---

### `user_aliases`

Tracks the alias a user goes by when completing work (e.g. the name that appears in a release credit). Only one active alias per user at a time.

| Column | Type | Nullable | Default | Notes |
|--------|------|----------|---------|-------|
| `id` | `SERIAL` | No | — | PK |
| `user_id` | `INT` | No | — | FK → `users(id)` |
| `alias` | `TEXT` | No | — | |
| `created_at` | `TIMESTAMPTZ` | No | `NOW()` | |
| `retired_at` | `TIMESTAMPTZ` | Yes | — | `NULL` = currently active |

**Unique indexes (partial, WHERE `retired_at IS NULL`):**
- `one_active_alias` on `(alias)` — no two users can hold the same active alias
- `one_active_alias_per_user` on `(user_id)` — a user can only have one active alias at a time

---

### `roles`

Custom capability labels (e.g. TL, TS, CLRD, QC, PR, RP). Describes what a user can do. Case-insensitive — `PR` and `pr` are treated as the same value.

| Column | Type | Nullable | Default | Notes |
|--------|------|----------|---------|-------|
| `id` | `SERIAL` | No | — | PK |
| `name` | `CITEXT` | No | — | Unique |

**Unique constraints:**
- `roles_name_key` on `(name)`

---

### `user_roles`

Which roles a user holds — their general capabilities, independent of any specific series or chapter.

| Column | Type | Nullable | Default | Notes |
|--------|------|----------|---------|-------|
| `user_id` | `INT` | No | — | FK → `users(id)` |
| `role_id` | `INT` | No | — | FK → `roles(id)` ON DELETE CASCADE |

**PK:** `(user_id, role_id)`

---

### `series`

A manga/manhwa/manhua title being worked on.

| Column | Type | Nullable | Default | Notes |
|--------|------|----------|---------|-------|
| `id` | `SERIAL` | No | — | PK |
| `name` | `CITEXT` | No | — | Unique, case-insensitive |
| `status` | `TEXT` | No | `'active'` | See check constraint below |
| `added_at` | `TIMESTAMPTZ` | No | `NOW()` | |
| `closed_at` | `TIMESTAMPTZ` | Yes | — | `NULL` = still ongoing. Set when status becomes `completed`, `dropped`, or `hiatus`. |

**Check constraints:**
- `series_status_check`: `status IN ('active', 'completed', 'dropped', 'hiatus')`

**Unique constraints:**
- `series_name_key` on `(name)`

---

### `chapters`

Individual chapters belonging to a series.

| Column | Type | Nullable | Default | Notes |
|--------|------|----------|---------|-------|
| `id` | `SERIAL` | No | — | PK |
| `series_id` | `INT` | No | — | FK → `series(id)` |
| `volume` | `INT` | Yes | — | Volume number, if the series uses a volume structure. `NULL` = not part of a named volume (e.g. web release). Not unique — multiple chapters share the same volume. |
| `number` | `NUMERIC(6,2)` | No | — | Sortable chapter number (e.g. `51`, `51.10`). Unique per series. |
| `name` | `CITEXT` | No | — | Display name (e.g. `"Ch 51.1"`), case-insensitive. Unique per series. |
| `status` | `TEXT` | No | `'in_progress'` | See check constraint below |
| `added_at` | `TIMESTAMPTZ` | No | `NOW()` | |
| `closed_at` | `TIMESTAMPTZ` | Yes | — | `NULL` = still in progress |

**Check constraints:**
- `chapters_status_check`: `status IN ('in_progress', 'released', 'dropped', 'hiatus')`

**Unique constraints:**
- `chapters_series_id_name_key` on `(series_id, name)` — display names are unique within a series
- `chapters_series_id_number_key` on `(series_id, number)` — chapter numbers are unique within a series

---

### `tasks`

Custom task types that must be completed before a chapter can be released (e.g. Translation, Typesetting, QC). Case-insensitive.

| Column | Type | Nullable | Default | Notes |
|--------|------|----------|---------|-------|
| `id` | `SERIAL` | No | — | PK |
| `name` | `CITEXT` | No | — | Unique |
| `retired_at` | `TIMESTAMPTZ` | Yes | — | `NULL` = active. Set when the task has completion history and cannot be hard-deleted. |

**Unique constraints:**
- `tasks_name_key` on `(name)`

**Soft-delete behaviour:** A task with any completed `chapter_assignments` row cannot be hard-deleted — it must be retired instead (`retired_at = NOW()`). Retired tasks are excluded from `/list-tasks` by default and cannot be assigned to new work, but their completion history is preserved.

---

### `task_dependencies`

Defines which tasks must be completed before others can start.

| Column | Type | Nullable | Default | Notes |
|--------|------|----------|---------|-------|
| `task_id` | `INT` | No | — | FK → `tasks(id)` ON DELETE CASCADE — the task that has a prerequisite |
| `depends_on_task_id` | `INT` | No | — | FK → `tasks(id)` ON DELETE CASCADE — the task that must be done first |

**PK:** `(task_id, depends_on_task_id)`

**Indexes:**
- `idx_task_dependencies_depends_on_task_id` on `(depends_on_task_id)` — the PK only covers lookups by `task_id`; this index covers the reverse direction (`listDependentsOf`)

---

### `role_tasks`

Maps which tasks a role is responsible for. Used to validate task assignments — when assigning a user to a series or chapter task, the task must be mapped to one of that user's roles.

| Column | Type | Nullable | Default | Notes |
|--------|------|----------|---------|-------|
| `role_id` | `INT` | No | — | FK → `roles(id)` ON DELETE CASCADE |
| `task_id` | `INT` | No | — | FK → `tasks(id)` ON DELETE CASCADE |

**PK:** `(role_id, task_id)`

**Indexes:**
- `idx_role_tasks_task_id` on `(task_id)` — the PK only covers lookups by `role_id`; this index covers lookups by `task_id` (`listByTask`)

---

### `series_assignments`

Which users are assigned to a series for a given task — the default crew. Used to auto-populate `chapter_assignments` when a new chapter is created.

| Column | Type | Nullable | Default | Notes |
|--------|------|----------|---------|-------|
| `user_id` | `INT` | No | — | FK → `users(id)` |
| `series_id` | `INT` | No | — | FK → `series(id)` |
| `task_id` | `INT` | No | — | FK → `tasks(id)` ON DELETE CASCADE |

**PK:** `(user_id, series_id, task_id)`

**Indexes:**
- `idx_series_assignments_series_id` on `(series_id)` — the PK only covers lookups by `user_id`; this index covers `listBySeries`

---

### `chapter_assignments`

Which users are assigned to a specific chapter for a given task. Doubles as both the to-do list and the completion record.

| Column | Type | Nullable | Default | Notes |
|--------|------|----------|---------|-------|
| `user_id` | `INT` | No | — | FK → `users(id)` |
| `chapter_id` | `INT` | No | — | FK → `chapters(id)` |
| `task_id` | `INT` | No | — | FK → `tasks(id)` |
| `completed_at` | `TIMESTAMPTZ` | Yes | — | `NULL` = outstanding; non-null = done, value is the completion timestamp |

**PK:** `(user_id, chapter_id, task_id)`

**Indexes:**
- `idx_chapter_assignments_chapter_id` on `(chapter_id)` — the PK only covers lookups by `user_id`; this index covers `listByChapter`

**Triggers:**
- `enforce_completed_assignment_immutable` — before DELETE, raises if `completed_at IS NOT NULL`. Completed rows are permanent historical records; only outstanding rows may be removed.

---

## Triggers

All triggers are `DEFERRABLE INITIALLY DEFERRED` — they fire at the end of the transaction, not immediately after each row change. This allows multi-step operations (e.g. inserting a user and their Discord identity in the same transaction) without intermediate constraint violations.

| Trigger | Table | Events | Behaviour |
|---------|-------|--------|-----------|
| `enforce_supermanager_exists` | `users` | INSERT, UPDATE, DELETE | Raises if no active user (`left_at IS NULL`) has `permission_level = 2` |
| `enforce_user_name` | `users` | INSERT, UPDATE | Raises if the user has no active `discord_identities` row and `name IS NULL` |
| `enforce_discord_user_name` | `discord_identities` | INSERT, UPDATE, DELETE | Raises if the affected user would be left with no active Discord link and no `name` |
| `enforce_completed_assignment_immutable` | `chapter_assignments` | DELETE | Raises if the row being deleted has `completed_at IS NOT NULL` — completed records are permanent |
| `roles_normalize_name` | `roles` | INSERT, UPDATE | Sets `NEW.name = UPPER(NEW.name)` before write — role names are always stored uppercase |
| `tasks_normalize_name` | `tasks` | INSERT, UPDATE | Sets `NEW.name = UPPER(NEW.name)` before write — task names are always stored uppercase |

---

## Extensions

| Extension | Purpose |
|-----------|---------|
| `citext` | Case-insensitive text type — used for `roles.name`, `tasks.name`, `series.name`, `chapters.name` |

---

## Design Notes

- **User identity:** A user is reachable via `users.name` (webapp login) or an active `discord_identities` row (bot), or both. At least one must always be present.
- **Aliases vs names:** `users.name` is the login/identity name. `user_aliases.alias` is the public credit name that appears in release history — separate because users may want a different display name from their login.
- **Permissions:** A single `permission_level` integer (`0`/`1`/`2`) replaces the old `is_manager`/`is_supermanager` boolean flags. At least one active supermanager (`2`) must always exist.
- **Roles vs tasks:** `roles` describe what a user *can* do. `tasks` describe the steps a chapter requires. `role_tasks` connects them for assignment validation. Assignments themselves (`series_assignments`, `chapter_assignments`) are task-scoped — not role-scoped — to avoid ambiguity.
- **Task tracking:** `chapter_assignments.completed_at` is both the "done" flag and the completion timestamp in one column. To-do list = `WHERE completed_at IS NULL`; history = `WHERE completed_at IS NOT NULL`.
- **Soft deletes:** Users (`left_at`), aliases (`retired_at`), and Discord identities (`unlinked_at`) are never hard-deleted — deactivation is recorded while historical data is preserved. Tasks follow a hybrid approach: hard-deleted if they have no completion history, soft-deleted (`retired_at`) otherwise to preserve the record.
- **Cascading deletes:** Deleting a role cascades to `user_roles` and `role_tasks`. Deleting a task cascades to `role_tasks`, `series_assignments`, and `task_dependencies`. `chapter_assignments` is intentionally excluded from cascade — it is historical data and must be managed explicitly (retire the task instead of deleting it).
- **Case insensitivity:** `citext` columns compare and enforce uniqueness case-insensitively but store values exactly as inserted. `PR` and `pr` cannot coexist; a lookup for either finds the same row.
- **Name normalization:** `roles.name` and `tasks.name` are additionally normalized to uppercase on write via a `BEFORE INSERT OR UPDATE` trigger. Any casing passed in (`pR`, `pr`, `PR`) is stored and returned as `PR`. `series.name` and `chapters.name` are excluded — those are display names where casing is meaningful.
