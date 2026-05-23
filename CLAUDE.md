# CLAUDE.md

This file provides guidance for working in the `scanlation-manager` monorepo.

## Project Overview

A management platform for scanlation groups. Built as a monorepo where a shared PostgreSQL database is the source of truth. Two interfaces are planned — a Discord bot (actively developed) and a web app (not yet started) — that operate against the same schema simultaneously.

## Repo Structure

```
scanlation-manager/
├── discord/     # C++ Discord bot — primary active module
├── db/          # PostgreSQL schema (migrations + Docker Compose)
├── backend/     # Go REST API — Google Drive, S3, and Google Sheets integrations
└── frontend/    # Web UI — not yet started
```

---

## discord/ — Discord Bot

### Tech Stack
- **Language:** C++20
- **Discord library:** D++ (libdpp) v10.1.4 — fetched via CMake FetchContent
- **Database client:** libpqxx 8.0.0 — fetched via CMake FetchContent
- **Other libs:** nlohmann_json, libcurl, pthreads
- **Build:** CMake 3.28+, system-installed: nlohmann_json, libcurl

### Build
```bash
cd discord

# Debug (default)
cmake --preset default
cmake --build --preset default
./build/debug/scanlation-manager   # config.json is auto-copied to build/debug/ on build

# Release
cmake --preset release
cmake --build --preset release
./build/release/scanlation-manager
```

Dependencies D++ and libpqxx are fetched automatically by CMake on first build. nlohmann_json and libcurl must be installed on the system.

### Configuration
The bot reads `discord/config.json` (copied to `build/config.json` at build time). Copy from example and fill in values:
```bash
cp discord/config.json.example discord/config.json
```

| Key | Required | Description |
|-----|----------|-------------|
| `discord_bot_token` | Yes | Discord bot token |
| `guild_id` | Yes | Discord server (guild) ID — used for command registration |
| `db_connection_string` | Yes | libpq connection string, e.g. `"host=localhost port=5432 dbname=scanlation_manager user=scanlation_manager password=secret"` |
| `db_pool_size` | Yes | Number of DB connections to maintain |
| `work_progress_channel` | No | Channel ID where work progress message trigger listens |
| `staff_role_id` | No | Discord role ID for @staff mentions |
| `backend_url` | No | Base URL of the Go backend (e.g. `http://localhost:8080`) — required for Google Sheets sync |
| `api_token` | No | Shared secret PSK for bot→backend requests — must match `api_token` in the backend config |

### Source Layout
```
discord/
├── include/
│   ├── bot/
│   │   ├── Bot.hpp                      # Bot class, PaginationState, CommandInfo
│   │   ├── eventHandlers/
│   │   │   ├── commands/
│   │   │   │   ├── add/                 # Commands that create records
│   │   │   │   ├── modify/              # Commands that update records
│   │   │   │   ├── list/                # Read-only / query commands
│   │   │   │   ├── remove/              # Commands that delete records
│   │   │   │   └── manage/              # Multi-subcommand entities (Series, Chapter, Gsheet)
│   │   │   └── triggers/                # One file per message trigger
│   │   └── utils/
│   │       ├── SheetSync.hpp            # Fire-and-forget Sheets sync
│   │       ├── ChannelUtils.hpp         # Channel mention parsing
│   │       └── GetAutoCompleteContext.hpp
│   ├── db/
│   │   ├── DbSession.hpp                # Transaction wrapper around a pooled connection
│   │   ├── ConnectionPool.hpp           # Thread-safe connection pool
│   │   └── repositories/               # One header per entity (22 classes)
│   ├── models/                          # Plain data structs (no logic)
│   ├── types/                           # Permission, SeriesStatus, ChapterStatus enums
│   └── utils/
│       ├── ConfigManager.hpp            # Thread-safe JSON config loader/writer
│       └── HttpUtils.hpp                # libcurl wrappers (httpGet, httpPost, CurlGlobalManager)
└── src/                                 # Mirrors include/ structure
```

### Adding a New Slash Command
1. Pick the right subdirectory: `add/`, `modify/`, `list/`, `remove/`, or `manage/`
2. Add `include/bot/eventHandlers/commands/<subdir>/MyCommand.hpp` and matching `.cpp`
3. Register the command in `Bot::fillCommandMap()` in `Bot.cpp`
4. CMake picks up new `.cpp` files automatically via `GLOB_RECURSE`

### Database Sessions
Construct `DbSession session(bot.getPool())` at the start of each handler:
- `session.wtx()` — returns a `pqxx::work&` write transaction
- `session.rtx()` — returns a `pqxx::read_transaction&` read transaction
- `session.commit()` — commits the write transaction
- Destructor automatically closes the open transaction and releases the connection back to the pool

### Google Sheets Sync
Mutations that affect sheet-visible data fire sync calls to the backend after committing. These are fire-and-forget (detached thread, errors logged to stderr, never propagated to the user).

`SheetSync` (`include/bot/utils/SheetSync.hpp`) provides three functions:
- `SheetSync::syncSeries(bot, series_name)` — rewrites the named series sheet tab
- `SheetSync::syncTodo(bot)` — rewrites the Todo sheet tab
- `SheetSync::deleteSeries(bot, series_name)` — removes the series sheet tab

Sync only fires when both `backend_url` and `api_token` are set in config **and** `gsheet_enabled = "1"` exists in the `bot_settings` DB table (toggled via `/gsheet enable`/`disable`).

Call placement after `session.commit()`:

| Command | Sync calls |
|---------|-----------|
| `/series add` | `syncSeries` |
| `/series set-status` | `syncSeries` + `syncTodo` |
| `/series assign` | `syncSeries` + `syncTodo` (when `sync_chapters` fires) |
| `/series unassign` | `syncSeries` + `syncTodo` (when `sync_chapters` fires) |
| `/series remove` | `deleteSeries` + `syncTodo` |
| `/chapter add` | `syncSeries` + `syncTodo` |
| `/chapter set-status` | `syncSeries` + `syncTodo` |
| `/chapter assign` | `syncSeries` + `syncTodo` |
| `/chapter unassign` | `syncSeries` + `syncTodo` |
| `/chapter uncomplete` | `syncSeries` + `syncTodo` |
| `/chapter remove` | `syncSeries` + `syncTodo` |
| `/work-update` | `syncSeries` + `syncTodo` |
| `/delete-task` | `syncSeries` (all affected series) + `syncTodo` |
| `/retire-task` | `syncSeries` (all affected series) + `syncTodo` |

### libcurl Usage Notes
- `CurlGlobalManager::curlManagerInit()` must be called once at startup (done in `main.cpp`) before any `httpGet`/`httpPost`
- `curl_easy_getinfo` with `CURLINFO_RESPONSE_CODE` requires a `long *`, **not** `int *` — on 64-bit Linux `sizeof(long) = 8`, so passing `int *` silently corrupts the stack in release builds (`-O3`) where the extra 4 bytes overwrite adjacent variables

### Code Style
Enforced by `.clang-format` (run `clang-format -i` on changed files):
- Based on LLVM style
- 2-space indentation
- No space before `(` in control flow (`if(`, `while(`, `for(`)
- Braces attach to the same line (Attach style)
- No column limit

---

## db/ — Database

### Setup
```bash
cd db
cp docker-compose.yml.example docker-compose.yml   # fill in credentials
docker compose up -d
```

### Applying Migrations
Apply via the Dockerised PostgreSQL container (run from the repo root):
```bash
docker exec -i scanlation-db-1 psql -U scanlation_manager -d scanlation_manager \
  < db/migrations/<migration_file>.sql
```

### Migration History

> **Note:** Two files are both named `022_*` due to a naming error. Apply them in the order listed below — `022_make_chapter_name_optional.sql` first, then `022_add_level_to_tasks.sql`.

```
001_initial.sql                               — Core schema: users, roles, series, chapters, tasks, assignments
002_add_supermanager.sql                      — is_supermanager flag; trigger enforces at least one always exists
003_add_user_credentials.sql                  — password_hash table for webapp auth
004_add_discord_identities.sql                — Discord account linking; users.name made nullable
005_enforce_webapp_name.sql                   — Triggers: user must have name OR Discord identity
006_add_hiatus_to_series_check_constraint.sql — series.status gains 'hiatus'
007_add_unlinked_at_to_discord_identities.sql — Soft-delete Discord links; partial unique indexes
008_add_chapter_status.sql                    — chapters.status ('in_progress'/'released'/'dropped')
009_role_task_mapping_and_assignment_refactor — role_tasks junction; series_assignments and chapter_assignments refactored to track tasks instead of roles
010_fix_supermanager_check.sql                — Supermanager trigger ignores left_at IS NOT NULL users
011_replace_manager_flags_with_permission_level — is_manager/is_supermanager → permission_level SMALLINT (0/1/2)
012_add_closed_at_to_chapters.sql             — chapters.closed_at TIMESTAMPTZ
013_simplify_chapter_task_tracking.sql        — Drops chapter_tasks/chapter_task_completions; adds completed_at to chapter_assignments
014_add_hiatus_to_chapter_status.sql          — chapters.status gains 'hiatus'
015_add_missing_indexes.sql                   — Performance indexes on FK columns
016_citext_name_columns.sql                   — roles.name, tasks.name, series.name, chapters.name → citext (case-insensitive compare, case-preserved storage)
017_add_chapter_number.sql                    — chapters.number NUMERIC(6,2); UNIQUE per series
018_cascade_deletes_and_task_retirement.sql   — ON DELETE CASCADE on role/task junction FKs; tasks.retired_at; immutability trigger on completed chapter_assignments
019_uppercase_role_task_names.sql             — BEFORE INSERT/UPDATE trigger normalises role and task names to UPPER
020_add_volume_to_chapters.sql                — chapters.volume INT (nullable)
021_cascade_series_and_chapter_deletes.sql    — ON DELETE CASCADE: chapters→series, chapter_assignments→chapter, series_assignments→series
022_make_chapter_name_optional.sql            — chapters.name DROP NOT NULL
022_add_level_to_tasks.sql                    — tasks.level INT (nullable) — ordering field used to prevent cyclic dependencies  [MISLABELLED: should be 023]
023_add_assigned_at_to_chapter_assignments.sql — chapter_assignments.assigned_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
024_add_bot_settings.sql                      — bot_settings (key TEXT PK, value TEXT) — runtime key-value config
025_add_todo_sheet_view.sql                   — outstanding_chapter_assignments VIEW: prerequisite-aware days_active
026_update_todo_sheet_view.sql                — Replaces view: adds pc.available, c.status='in_progress', s.status='active' filters
027_add_active_since_to_todo_view.sql         — Replaces view: swaps days_active (stale int) for active_since (TIMESTAMPTZ) so Days Active can be a live formula in the sheet
```

### Key Schema Notes
- `users.name` is nullable — users can exist with only a Discord identity
- Every user must have either a `name` (webapp) or an active row in `discord_identities` — enforced by constraint triggers
- At least one supermanager (`permission_level = 2`) among active users must always exist — enforced by a constraint trigger
- `chapter_assignments.completed_at` is both the "done" flag and the completion timestamp — `NULL` = outstanding, non-null = done
- `chapter_assignments.assigned_at` records when the assignment was created; used by `outstanding_chapter_assignments` to compute `active_since` (the timestamp from which Days Active counts)
- Tasks can be hard-deleted only if they have no completed `chapter_assignments`; otherwise use `tasks.retired_at` (soft-delete)
- Deleting a role or task cascades to junction tables (`role_tasks`, `user_roles`, `series_assignments`, `task_dependencies`) via `ON DELETE CASCADE`; `chapter_assignments` is intentionally excluded from cascade to preserve history
- Deleting a series or chapter cascades to child records; completed assignments must be cleared before deletion (the application handles this in the supermanager path before calling `remove()`)
- `bot_settings` is a simple `(key TEXT, value TEXT)` store — currently only `gsheet_enabled = "1"` is used
- `outstanding_chapter_assignments` (view, migrations 025+026) returns all actionable incomplete assignments: prerequisites satisfied (using only assignments that exist — unassigned prerequisites are ignored), chapter is `in_progress`, series is `active`. Used by the Todo sheet backend and referenced conceptually by `/todo` (which does equivalent filtering in-memory)

---

## backend/ — Go REST API

### Tech Stack
- **Language:** Go 1.26
- **Database client:** `github.com/jackc/pgx/v5` (pgxpool) — used only by the Sheets sync service
- **Google APIs:** `google.golang.org/api` (Drive v3 + Sheets v4)
- **S3:** `github.com/aws/aws-sdk-go-v2` — compatible with AWS S3, Cloudflare R2, Backblaze B2, MinIO
- **Config:** `config.json` in the working directory, or path overridden via `CONFIG_PATH` env var

### Build & Run
```bash
cd backend
go build ./...
go run ./cmd/api
```

### Configuration
```bash
cp backend/config.json.example backend/config.json
```

| Key | Required | Description |
|-----|----------|-------------|
| `port` | No (default `8080`) | HTTP listen port |
| `gdrive_credentials_file` | For Drive + Sheets | Path to service account JSON (same file for both) |
| `s3_endpoint` | For S3 | Leave empty for AWS S3; set custom URL for R2/B2/MinIO |
| `s3_region` | For S3 | `"auto"` for Cloudflare R2; AWS region string otherwise |
| `s3_access_key_id` | For S3 | Access key ID |
| `s3_secret_access_key` | For S3 | Secret access key |
| `s3_bucket` | For S3 | Bucket name |
| `gsheet_spreadsheet_id` | For Sheets | Google Spreadsheet ID (from URL) |
| `api_token` | For Sheets | Shared PSK — must match the bot's `api_token` in `discord/config.json` |
| `db_connection_string` | For Sheets | libpq-style connection string, e.g. `"host=localhost port=5432 dbname=scanlation_manager user=scanlation_manager password=secret"` |

Missing optional sections are skipped — the server starts without them (routes for that service won't be registered).

### Source Layout
```
backend/
├── cmd/api/main.go                        # Entry point — initialises services, registers routes, listens
├── config.json.example
├── internal/
│   ├── config/config.go                   # Config struct + JSON loader (Load())
│   ├── handlers/
│   │   ├── routes.go                      # RegisterRoutes — attaches handlers to mux conditionally
│   │   ├── api.go                         # GET /health — liveness + auth check
│   │   ├── gdrive.go                      # Drive list/download/upload/delete handlers
│   │   ├── s3.go                          # S3 list/download/upload/delete handlers
│   │   └── sheets.go                      # Sheets sync handlers + buildTodoGrid/buildSeriesGrid
│   └── services/
│       ├── gdrive/gdrive.go               # Drive API client (List, Download, Upload, Delete, GetFile)
│       ├── s3/s3.go                       # S3 client (List, Download, Upload, Delete)
│       ├── gsheets/gsheets.go             # Sheets client (ClearAndWrite, DeleteSheet, IsHealthy)
│       ├── sheetdb/sheetdb.go             # DB reader for sheet data (GetTodoRows, GetSeriesSheetData)
│       └── mangadex/mangadex.go           # MangaDex API client — stub, not yet implemented
```

### API Endpoints

All routes require `Authorization: Bearer <api_token>`.

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/health` | Backend liveness + auth check (200 if token valid) |
| `GET` | `/sheets/health` | Sheets API connectivity check |
| `POST` | `/sheets/sync/todo` | Rewrites the `"Todo"` sheet tab from `outstanding_chapter_assignments` view |
| `POST` | `/sheets/sync/series` | Body: `{"name":"..."}` — rewrites the named series sheet tab |
| `POST` | `/sheets/delete-series` | Body: `{"name":"..."}` — deletes the named series sheet tab |
| `GET` | `/drive/folders/{folderID}` | Lists files in a Google Drive folder |
| `GET` | `/drive/files/{fileID}` | Downloads a file from Google Drive |
| `POST` | `/drive/folders/{folderID}` | Query: `?name=&mimeType=` — uploads a file to a Drive folder |
| `DELETE` | `/drive/files/{fileID}` | Permanently deletes a Drive file |
| `GET` | `/s3/objects` | Query: `?prefix=` — lists S3 objects |
| `GET` | `/s3/objects/{key}` | Downloads an S3 object |
| `PUT` | `/s3/objects/{key}` | Query: `?mimeType=` — uploads an S3 object |
| `DELETE` | `/s3/objects/{key}` | Deletes an S3 object |

### Google Sheets — Sheet Structure

**Todo sheet** (tab name: `"Todo"`):
- Columns: `Series | Chapter | Task | Assigned To | Days Active`
- One row per actionable incomplete assignment (prerequisites met, chapter `in_progress`, series `active`)
- `Days Active` is written as a Google Sheets formula `=INT(TODAY()-DATE(Y,M,D))` where the date is `GREATEST(assigned_at, latest_prereq_completion)` fetched from the DB — the cell recalculates live every day without a sync
- Sorted: series name → chapter number → task level (NULLS LAST) → task name → assignee

**Series sheet** (one tab per series, tab named after the series):
```
Status  | <Active|Completed|Hiatus|Dropped>
(blank)
User    | Task
<crew member display name> | <task name>
...
(blank)
Chapter | Status | Closed At | <task1> | <task2> | ...
<Ch.X Name> | in_progress | N/A | <assignee or ✓ Name or N/A> | ...
```
- Task columns are all tasks that appear in at least one chapter assignment for the series, ordered by `tasks.level` then `tasks.name`
- Cell values: `N/A` (unassigned), display name (assigned, incomplete), `✓ Name` (completed)
- `Closed At` is `N/A` if `chapters.closed_at IS NULL`

### Manual Setup (one-time, before `/gsheet enable`)
1. Create a Google Spreadsheet and note its ID from the URL
2. Share it with the service account email (found inside `gdrive_credentials_file` JSON) as **Editor**
3. Fill in `backend/config.json`: `gsheet_spreadsheet_id`, `api_token`, `db_connection_string`
4. Start the backend
5. In Discord (as supermanager): `/gsheet enable` — verifies connectivity and stores `gsheet_enabled = "1"` in `bot_settings`

---

## Command Reference (Discord Bot)

### Permission levels
- `standard` (0) — any registered user
- `manager` (1) — can manage series, chapters, and view others' todo lists
- `supermanager` (2) — can delete series/chapters with completion history, manage `/gsheet`

### /ping
List command. Returns "Pong!". No auth required.

### /register [user]
Add command. Registers the calling user (or a target Discord user if manager+). Creates a `users` row and links it via `discord_identities`.

### /set-alias \<alias\>
Modify command. Sets the caller's active display name alias (used in credits). Retires the previous alias.

### /set-progress-channel \<channel\>
Modify command. Stores the channel ID in `config.json` via `ConfigManager`. Requires manager+.

### /set-staff-role \<role\>
Modify command. Stores the Discord role ID in `config.json`. Requires manager+.

### /promote \<user\>
Modify command. Elevates a user from standard → manager. Requires supermanager.

### /demote \<user\>
Modify command. Reduces a user from manager → standard. Requires supermanager.

### /add-role \<name\>
Add command. Creates a new role (e.g. `TL`, `QC`, `PR`). Name is normalised to uppercase. Requires manager+.

### /delete-role \<role\>
Remove command. Hard-deletes the role; cascades to `user_roles` and `role_tasks`. Requires manager+.

### /assign-role \<user\> \<role\>
Modify command. Assigns a Discord user to an app role (`user_roles`). Requires manager+.

### /remove-role \<user\> \<role\>
Remove command. Removes a user from an app role. Requires manager+.

### /sync-role \<discord_role\>
Add command. Syncs all members of a Discord role into the matching app role. Requires manager+.

### /add-task \<name\> \<level\>
Add command. Creates a task. Name is normalised to uppercase. `level` is an integer used to order tasks in sheets and prevent cyclic dependencies. Requires manager+.

### /delete-task \<task\>
Remove command. Hard-deletes the task. Fails if any completed `chapter_assignments` exist for this task — use `/retire-task` instead. Cascades to `role_tasks`, `task_dependencies`, outstanding `chapter_assignments`, and `series_assignments`. Fires `syncSeries` for all series that had assignments for the task, plus `syncTodo`. Requires manager+.

### /retire-task \<task\>
Modify command. Soft-deletes the task (sets `retired_at`). Removes all series-level and outstanding chapter-level assignments for the task. Fires `syncSeries` for all affected series, plus `syncTodo`. Retired tasks cannot be assigned. Requires manager+.

### /unretire-task \<task\>
Modify command. Clears `retired_at` on the task. Requires manager+.

### /map-role-task \<role\> \<task\>
Add command. Creates a `role_tasks` entry — users must hold a role with this mapping to be assigned the task. Requires manager+.

### /unmap-role-task \<role\> \<task\>
Remove command. Removes the `role_tasks` entry. Requires manager+.

### /set-task-dependency \<task\> \<depends_on\>
Add command. Creates a `task_dependencies` row — `task` cannot be marked complete until `depends_on` is complete for the same chapter. Requires manager+.

### /remove-task-dep \<task\> \<depends_on\>
Remove command. Deletes the dependency. Requires manager+.

### /list-roles
List command. Shows all roles and their mapped tasks.

### /list-tasks [include_retired]
List command. Shows all tasks ordered by level then name. Optionally includes retired tasks.

### /list-role-tasks \<role\>
List command. Shows all tasks mapped to a role.

### /list-task-deps \<task\>
List command. Shows the prerequisite graph for a task.

### /list-series [status]
List command. Lists series with optional status filter.

### /list-chapters \<series\> [status]
List command. Lists chapters in a series with optional status filter.

### /info \<user\>
List command. Shows a user's stats (total series count, recent history). Requires manager+ to view other users.

### /todo [user]
List command. Shows the calling user's outstanding actionable assignments (prerequisites met). Managers can pass a `user` parameter to view someone else's list. Uses in-memory dependency resolution against all incomplete assignments in the relevant chapters (equivalent logic to the `outstanding_chapter_assignments` view but applied per-user in the bot).

### /user-history \<user\> [series]
List command. Shows completed assignments for a user, optionally filtered to a series.

### /series \<subcommand\>
Manage command. Requires manager+.

| Subcommand | Description |
|-----------|-------------|
| `add <name>` | Creates the series; fires `syncSeries` |
| `set-status <name> <status>` | Updates status (`active`/`completed`/`hiatus`/`dropped`); fires `syncSeries` + `syncTodo` |
| `assign <name> <user> <task> [sync_chapters]` | Adds a series-level crew assignment (validates user has a capable role). When `sync_chapters` is true (default), also assigns the user to every non-released chapter in the series that doesn't already have them. Fires `syncSeries` + `syncTodo` (if synced). |
| `unassign <name> <user> <task> [sync_chapters]` | Removes a series-level crew assignment. When `sync_chapters` is true (default), also removes outstanding (not completed) chapter assignments for that user+task across all non-released chapters. Fires `syncSeries` + `syncTodo` (if synced). |
| `remove <name>` | Deletes the series and all chapters. If completed assignments exist, requires supermanager (clears `completed_at` first to bypass immutability trigger); fires `deleteSeries` + `syncTodo` |

### /chapter \<subcommand\>
Manage command. Requires manager+.

| Subcommand | Description |
|-----------|-------------|
| `add <series> <number> [name] [volume]` | Creates chapter; copies series-level crew assignments as chapter assignments; fires `syncSeries` + `syncTodo` |
| `set-status <series> <chapter> <status>` | Updates status (`in_progress`/`released`/`dropped`/`hiatus`); fires `syncSeries` + `syncTodo` |
| `assign <series> <chapter> <user> <task>` | Adds a chapter assignment (validates user has a capable role); fires `syncSeries` + `syncTodo` |
| `unassign <series> <chapter> <user> <task>` | Removes outstanding assignment (blocked if already completed); fires `syncSeries` + `syncTodo` |
| `uncomplete <series> <chapter> <user> <task>` | Clears `completed_at` on a completed assignment (chapter must be `in_progress`); fires `syncSeries` + `syncTodo` |
| `remove <series> <chapter>` | Deletes the chapter. Requires supermanager if completed assignments exist; fires `syncSeries` + `syncTodo` |
| `bulk-add <series> <chapters>` | Adds multiple chapters at once; `chapters` is a comma-separated list of numbers (e.g. `51,52,53.5`); copies series-level crew to each; skips numbers that already exist; fires `syncSeries` + `syncTodo` if any chapter was created |

### /work-update \<series\> \<chapter\> \<task\> [user]
Modify command. Marks a chapter assignment complete for the calling user (or a target user, if manager+).
- Checks that the assignment exists and is not already completed
- Checks that all task dependencies are satisfied for this chapter
- If dependents exist: pings assignees of tasks that are now **fully unblocked** (all their dependencies complete) — tasks still blocked by other prerequisites are not pinged
- If no dependents remain: auto-sets the chapter status to `released`
- Fires `syncSeries` + `syncTodo`

### /bulk-work-update \<series\> \<task\> \<chapters\> [user]
Modify command. Marks a task complete across multiple chapters in one command. `chapters` is a comma-separated list of chapter numbers (e.g. `51,52,53.5`). Input is normalized and validated before any DB write.
- All chapters are validated first (exists, assigned, not already complete, no blocking dependencies) — if any fail the entire command is aborted with a per-chapter error report
- On success: marks all complete, pings assignees of tasks now fully unblocked (deduplicated across chapters), auto-releases chapters with no remaining dependents
- Fires `syncSeries` + `syncTodo`

### /gsheet \<enable|disable\>
Manage command. Requires supermanager.
- `enable` — verifies `backend_url` and `api_token` are in config, calls `GET /health` then `GET /sheets/health`, stores `gsheet_enabled = "1"` in `bot_settings`
- `disable` — removes `gsheet_enabled` from `bot_settings`

### Work Progress Message Trigger
Listens in the configured `work_progress_channel`. Parses pipe-delimited messages matching one of these formats:
```
StaffName|#series-channel|chapter|task
StaffName|#series-channel|chapter|task|@next_role
```
Currently parses and echoes the parsed fields back. No DB write yet.

---

## Current Command Status

| Command | Status |
|---------|--------|
| `/ping` | Done |
| `/register` | Done |
| `/set-progress-channel` | Done |
| `/set-staff-role` | Done |
| `/set-alias` | Done |
| `/promote` | Done |
| `/demote` | Done |
| `/add-role` | Done |
| `/delete-role` | Done |
| `/assign-role` | Done |
| `/remove-role` | Done |
| `/sync-role` | Done |
| `/add-task` | Done |
| `/delete-task` | Done |
| `/retire-task` | Done |
| `/unretire-task` | Done |
| `/map-role-task` | Done |
| `/unmap-role-task` | Done |
| `/set-task-dependency` | Done |
| `/remove-task-dep` | Done |
| `/list-roles` | Done |
| `/list-tasks` | Done |
| `/list-role-tasks` | Done |
| `/list-task-deps` | Done |
| `/list-series` | Done |
| `/list-chapters` | Done |
| `/info` | Done |
| `/todo` | Done |
| `/user-history` | Done |
| `/work-update` | Done |
| `/bulk-work-update` | Done |
| `/series` (add, set-status, assign, unassign, remove) | Done |
| `/chapter` (add, set-status, assign, unassign, uncomplete, remove, bulk-add) | Done |
| `/gsheet` (enable, disable) | Done |
| Work progress message trigger | Parses & echoes (no DB write yet) |
| MangaDex integration (backend) | Stub only — not started |
