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
| `is_manager` | `BOOLEAN` | Default `false` |
| `is_supermanager` | `BOOLEAN` | Default `false`. At least one must always exist (enforced by trigger). |
| `joined_at` | `TIMESTAMPTZ` | Default `now()` |
| `left_at` | `TIMESTAMPTZ` | `NULL` = currently active |

**Constraints:**
- A user must have either a `name` or a linked `discord_identities` row — enforced by `enforce_user_name` trigger.
- Removing the last supermanager is blocked by `enforce_supermanager_exists` trigger.

---

### `discord_identities`
Links a Discord account to a `users` row. A user may have at most one Discord identity.

| Column | Type | Notes |
|--------|------|-------|
| `id` | `SERIAL` | PK |
| `discord_id` | `BIGINT` | Unique Discord snowflake |
| `user_id` | `INT` | FK → `users(id)` |
| `linked_at` | `TIMESTAMPTZ` | Default `now()` |

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

**Unique indexes:**
- `one_active_alias` — no two users can hold the same active alias.
- `one_active_alias_per_user` — a user can only have one active alias at a time.

---

### `roles`
Custom capability labels (e.g. TL, TS, CLRD, QC, PR, RP).

| Column | Type | Notes |
|--------|------|-------|
| `id` | `SERIAL` | PK |
| `name` | `TEXT` | Unique |

---

### `user_roles`
Which roles a user holds (their general capabilities, independent of any specific series).

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
| `added_at` | `TIMESTAMPTZ` | Default `now()` |

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
Defines which tasks must be done before others can start.

| Column | Type | Notes |
|--------|------|-------|
| `task_id` | `INT` | FK → `tasks(id)` |
| `depends_on_task_id` | `INT` | FK → `tasks(id)` |

PK: `(task_id, depends_on_task_id)`

---

### `chapter_tasks`
The to-do list for each chapter — one row per `(chapter, task)` pair. Auto-populated when a chapter is created or a new task is added.

| Column | Type | Notes |
|--------|------|-------|
| `chapter_id` | `INT` | FK → `chapters(id)` |
| `task_id` | `INT` | FK → `tasks(id)` |
| `completed_at` | `TIMESTAMPTZ` | `NULL` = not yet done. This is the canonical "is this task done?" flag. |

PK: `(chapter_id, task_id)`

---

### `chapter_task_completions`
Immutable history of who completed each chapter task and when. Populated from `chapter_assignments` at the time of completion.

| Column | Type | Notes |
|--------|------|-------|
| `chapter_id` | `INT` | FK → `chapters(id)` |
| `task_id` | `INT` | FK → `tasks(id)` |
| `user_id` | `INT` | FK → `users(id)` |
| `completed_at` | `TIMESTAMPTZ` | Default `now()` |

PK: `(chapter_id, task_id, user_id)`
Also has a composite FK on `(chapter_id, task_id)` → `chapter_tasks`.

---

### `series_assignments`
Which users are assigned to a series in a given role (the "default" crew for a series).

| Column | Type | Notes |
|--------|------|-------|
| `user_id` | `INT` | FK → `users(id)` |
| `series_id` | `INT` | FK → `series(id)` |
| `role_id` | `INT` | FK → `roles(id)` |

PK: `(user_id, series_id, role_id)`

---

### `chapter_assignments`
Which users are assigned to a specific chapter in a given role. More granular than `series_assignments` and is the source of truth used when recording completions.

| Column | Type | Notes |
|--------|------|-------|
| `user_id` | `INT` | FK → `users(id)` |
| `chapter_id` | `INT` | FK → `chapters(id)` |
| `role_id` | `INT` | FK → `roles(id)` |

PK: `(user_id, chapter_id, role_id)`

---

## Triggers

| Trigger | Table | Event | Purpose |
|---------|-------|-------|---------|
| `enforce_supermanager_exists` | `users` | `UPDATE`, `DELETE` | Blocks the last supermanager from being removed |
| `enforce_user_name` | `users` | `INSERT`, `UPDATE` | A user with no Discord identity must have a `name` |
| `enforce_discord_user_name` | `discord_identities` | `INSERT`, `UPDATE`, `DELETE` | Blocks unlinking Discord from a user who has no `name` |

All triggers are `DEFERRABLE INITIALLY DEFERRED`.

---

## Key Design Notes

- **User identity:** A user exists in `users` and is reachable via either `name` (webapp) or `discord_identities` (bot). Both can be active simultaneously.
- **Aliases vs names:** `users.name` is the login/identity name; `user_aliases` is the public credit alias used in release history.
- **Task tracking split:** `chapter_tasks.completed_at` is the live "done" flag. `chapter_task_completions` is the append-only audit log of who did the work.
- **Soft deletes:** Users and aliases are never hard-deleted — `left_at` / `retired_at` mark deactivation, preserving historical completion records.
