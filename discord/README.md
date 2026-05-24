# scanlation-manager — Discord Bot

> Part of the `scanlation-manager` monorepo. This module lives under `discord/`.

A Discord bot built with [D++](https://dpp.dev/) (libdpp) in C++ to help scanlation groups track work progress across series. Staff submit work updates via slash commands, managers configure the team and track progress, and the bot enforces task dependencies and pings downstream assignees automatically.

---

## Monorepo Structure

```
scanlation-manager/
├── discord/      ← this module
├── db/
├── frontend/
└── backend/
```

---

## Features

| Status | Feature |
|--------|---------|
| ✅ | `/ping` — Health check |
| ✅ | `/register` — Register yourself or another user (manager+) as a team member |
| ✅ | `/set-progress-channel` — Designate a channel for work progress messages |
| ✅ | `/set-staff-role` — Set the Discord role required to use the bot; auto-registers all current members with that role |
| ✅ | `/set-alias` — Set a display alias for release credits |
| ✅ | `/add-role` — Create a scanlation role |
| ✅ | `/add-task` — Create a task type with an ordering level |
| ✅ | `/sync-role` — Bulk-map a Discord role to an app role and assign it to all registered members who have it |
| ✅ | `/assign-role` — Assign a role to a user |
| ✅ | `/remove-role` — Remove a role from a user |
| ✅ | `/delete-role` — Delete a role and all its mappings |
| ✅ | `/delete-task` — Delete a task (only if no completed assignments) |
| ✅ | `/retire-task` — Soft-retire a task, preserving its history |
| ✅ | `/unretire-task` — Restore a retired task to active status |
| ✅ | `/map-role-task` — Map a role to a task it is responsible for |
| ✅ | `/unmap-role-task` — Remove a role-task mapping |
| ✅ | `/set-task-dependency` — Define a prerequisite relationship between tasks |
| ✅ | `/remove-task-dep` — Remove a task prerequisite |
| ✅ | `/list-roles` — List all roles |
| ✅ | `/list-tasks` — List all tasks |
| ✅ | `/list-role-tasks` — List all role-task mappings |
| ✅ | `/list-task-deps` — Show the prerequisite graph for a task |
| ✅ | `/series` — Manage series: add, set-status, assign/unassign/move-assignment default crew, add-placeholder, remove-placeholder, remove |
| ✅ | `/chapter` — Manage chapters: add, bulk-add, set-status, assign/unassign/uncomplete/remove, move-assignment, add-placeholder, remove-placeholder |
| ✅ | `/list-series` — List series with chapter counts and latest activity, optional status filter |
| ✅ | `/list-chapters` — List chapters with task completion stats, optional series/status/sort |
| ✅ | `/info` — Deep-dive view for a series (crew + chapters), chapter (assignments), or user profile |
| ✅ | `/promote` — Promote a user to the next permission level |
| ✅ | `/demote` — Demote a user to the previous permission level |
| ✅ | `/work-update` — Mark a chapter task as complete; blocks on unmet dependencies, pings downstream assignees |
| ✅ | `/bulk-work-update` — Mark a task complete across multiple chapters at once; same dependency/auto-release logic as `/work-update` |
| ✅ | `/todo` — Show outstanding tasks that are ready to start (dependencies satisfied) |
| ✅ | `/user-history` — Paginated log of all completed assignments, newest first, optionally filtered by series |
| ✅ | `/gsheet` — Enable or disable Google Sheets sync (supermanager only) |
| ✅ | Google Sheets integration — Sync series and todo data to a Google Spreadsheet via the backend |
| 🚧 | Work progress message trigger — Parses structured messages in the progress channel (no DB write yet) |

---

## Project Structure

```
discord/
├── src/
│   ├── main.cpp
│   └── bot/
│       ├── Bot.cpp                                # Core bot class, command & trigger registration
│       ├── eventHandlers/
│       │   ├── commands/
│       │   │   ├── add/       # /register, /add-role, /add-task, /sync-role, /map-role-task, /set-task-dependency
│       │   │   ├── modify/    # /work-update, /bulk-work-update, /set-alias, /set-progress-channel, /set-staff-role, /promote, /demote, /assign-role, /retire-task, /unretire-task, /remove-role
│       │   │   ├── list/      # /ping, /info, /list-*, /todo, /user-history
│       │   │   ├── remove/    # /delete-role, /delete-task, /unmap-role-task, /remove-task-dep
│       │   │   └── manage/    # /series, /chapter, /gsheet
│       │   └── triggers/
│       │       └── WorkProgress.cpp
│       └── utils/
│           ├── SheetSync.cpp                      # Fire-and-forget Sheets sync (syncSeries, syncTodo, deleteSeries)
│           ├── ChannelUtils.cpp
│           ├── DateUtils.cpp
│           └── GetAutoCompleteContext.cpp
├── include/                                       # Mirrors src/ structure
│   └── ...
├── db/
│   ├── DbSession.hpp / DbSession.cpp              # Transaction session (RAII)
│   ├── ConnectionPool.hpp / ConnectionPool.cpp    # Thread-safe connection pooling
│   ├── repositories/                              # One file pair per DB table
│   │   ├── User, DiscordIdentities, UserAliases, UserCredentials
│   │   ├── Roles, UserRoles, RoleTasks
│   │   ├── Tasks, TaskDependencies
│   │   ├── Series, SeriesAssignments, SeriesAssignmentPlaceholders
│   │   ├── Chapters, ChapterAssignments, ChapterAssignmentPlaceholders
│   │   └── BotSettings                           # Runtime key-value config (gsheet_enabled)
│   └── utils/
│       └── PqxxErrors.hpp / PqxxErrors.cpp        # Constraint name extraction from pqxx exceptions
├── models/                                        # Plain data structs (no logic)
├── types/                                         # Shared enums: Permission, SeriesStatus, ChapterStatus
└── utils/
    ├── ConfigManager.hpp / ConfigManager.cpp      # Thread-safe JSON config read/write
    └── HttpUtils.hpp / HttpUtils.cpp              # libcurl HTTP GET/POST wrappers (CurlGlobalManager)
```

---

## Configuration

The bot reads from a `config.json` file at the working directory (auto-copied from `discord/config.json` to the build directory at build time). Copy the example and fill in your values:

```bash
cp config.json.example config.json
```

| Key | Required | Type | Description |
|-----|----------|------|-------------|
| `discord_bot_token` | ✅ | `string` | Your Discord bot token |
| `guild_id` | ✅ | `uint64` | The Discord server (guild) ID to register commands to |
| `db_connection_string` | ✅ | `string` | PostgreSQL connection string (libpq format), e.g. `"host=localhost port=5432 dbname=scanlation_manager user=scanlation_manager password=secret"` |
| `db_pool_size` | ✅ | `size_t` | Number of database connections to maintain in the pool |
| `work_progress_channel` | ❌ | `uint64` | Channel ID for progress message trigger (can be set via `/set-progress-channel`) |
| `staff_role_id` | ❌ | `uint64` | Discord role ID required to use the bot (can be set via `/set-staff-role`) |
| `backend_url` | ❌ | `string` | Base URL of the Go backend, e.g. `http://localhost:8080` — required for Google Sheets sync |
| `api_token` | ❌ | `string` | Shared secret PSK for bot→backend requests — must match `api_token` in `backend/config.json` |

---

## Bot Invite

When inviting the bot to your server, make sure to include both the `bot` and `applications.commands` scopes:

```
https://discord.com/oauth2/authorize?client_id=YOUR_CLIENT_ID&scope=bot+applications.commands&permissions=139586816064
```

The bot requires the `Server Members Intent` privileged gateway intent to be enabled in the Discord Developer Portal. This is used by `/set-staff-role` and `/sync-role` to iterate the guild member cache.

---

## Dependencies

- [D++ (libdpp)](https://dpp.dev/) — Discord API wrapper (fetched via CMake FetchContent)
- [libpqxx](https://pqxx.org/) — PostgreSQL C++ client (fetched via CMake FetchContent)
- [nlohmann/json](https://github.com/nlohmann/json) — JSON parsing for config (system-installed)
- [libcurl](https://curl.se/libcurl/) — HTTP client for backend sync calls (system-installed)
- CMake 3.28+ (build system)

---

## Building

> D++ and libpqxx are fetched automatically by CMake on first build. You only need `nlohmann_json` and `libcurl` installed on the system. The database must be running and migrated before starting the bot (see `db/`).

```bash
cd discord

# Debug (default)
cmake --preset default
cmake --build --preset default
./build/debug/scanlation-manager

# Release
cmake --preset release
cmake --build --preset release
./build/release/scanlation-manager
```

---

## Permission Levels

| Level | Name | Description |
|-------|------|-------------|
| 0 | Standard | Default for all registered users. Can submit work updates and view their own to-do list. |
| 1 | Manager | Can manage team structure: create roles/tasks, assign users, manage series and chapters. Can view other users' todo lists and histories. |
| 2 | Supermanager | Full access. Can promote/demote users, change bot-wide settings, and delete entities with completion history. At least one supermanager must always exist — enforced at the database level. |

The first user to run `/register` is automatically granted Supermanager.

### Command permission summary

| Command | Minimum level |
|---------|--------------|
| `/ping` | Bot access (no registration required) |
| `/register` (self) | Bot access (no registration required) |
| `/register [user]` | Manager |
| `/set-alias` | Standard (registered) |
| `/work-update` | Standard (registered) |
| `/bulk-work-update` | Standard (registered; manager+ to mark for another user) |
| `/todo` (self) | Standard (registered) |
| `/todo [user]` | Manager |
| `/user-history` (self) | Standard (registered) |
| `/user-history [user]` | Manager |
| `/list-roles` | Standard (registered) |
| `/list-tasks` | Standard (registered) |
| `/list-role-tasks` | Standard (registered) |
| `/list-task-deps` | Standard (registered) |
| `/list-series` | Standard (registered) |
| `/list-chapters` | Standard (registered) |
| `/info user` (self) | Standard (registered) |
| `/info user [user]` | Manager |
| `/info series` | Manager |
| `/info chapter` | Manager |
| `/set-progress-channel` | Manager |
| `/add-role` | Manager |
| `/add-task` | Manager |
| `/sync-role` | Manager |
| `/assign-role` | Manager |
| `/remove-role` | Manager |
| `/delete-role` | Manager |
| `/delete-task` | Manager |
| `/retire-task` | Manager |
| `/unretire-task` | Manager |
| `/map-role-task` | Manager |
| `/unmap-role-task` | Manager |
| `/set-task-dependency` | Manager |
| `/remove-task-dep` | Manager |
| `/series` | Manager (`remove` with completion history requires Supermanager) |
| `/chapter` | Manager (`remove` with completion history requires Supermanager) |
| `/chapter add-placeholder` | Manager |
| `/chapter remove-placeholder` | Manager |
| `/chapter move-assignment` | Manager |
| `/series add-placeholder` | Manager |
| `/series remove-placeholder` | Manager |
| `/series move-assignment` | Manager |
| `/set-staff-role` | Supermanager |
| `/promote` | Supermanager |
| `/demote` | Supermanager |
| `/gsheet` | Supermanager |

---

## Slash Commands

### `/ping`
Health check. Responds with `Pong!`.

### `/register [user?]`
Registers a Discord user as a scanlation team member. Creates a user record and links their Discord identity.

- With no arguments: registers yourself.
- With a `user` argument (manager+): registers the specified Discord user on their behalf.
- The first user to register is automatically granted Supermanager.

### `/set-progress-channel [channel]`
Sets the channel where the bot watches for work progress messages. Persisted to `config.json`. Manager+.

### `/set-staff-role [role]`
Sets the Discord role that gates bot access. Persisted to `config.json`. After updating, the bot scans the guild member cache and auto-registers any members who have that role but are not yet in the system. Supermanager only.

### `/set-alias [alias]`
Sets a display alias for the calling user, used in release credits. Enforces uniqueness across active users.

### `/add-role [name]`
Creates a new scanlation role (e.g. `TL`, `QC`, `PR`). Name is normalised to uppercase. Manager+.

### `/add-task [name] [level]`
Creates a new task type. Name is normalised to uppercase. `level` is an integer used to order tasks in sheets and to prevent cyclic dependencies (a task can only depend on tasks with a strictly lower level). Manager+.

### `/sync-role [role]`
Takes a Discord role, creates a matching app role by the same name (or finds the existing one), then assigns that app role to every registered user who has the Discord role. Manager+.

### `/assign-role [user] [role]`
Assigns a scanlation role to a user. Manager+.

### `/remove-role [user] [role]`
Removes a scanlation role from a user. Manager+.

### `/delete-role [name]`
Deletes a role and cascades to all role-task mappings and user-role assignments. Manager+.

### `/delete-task [name]`
Deletes a task. Only allowed if the task has no completed chapter assignments — use `/retire-task` instead if it does. After deletion, fires a sheet sync for every series that had assignments for the task. Manager+.

### `/retire-task [name]`
Soft-retires a task (sets `retired_at`). Removes all series-level and outstanding chapter assignments for the task, **and all series-level and chapter-level placeholder slots** for that task, then syncs every affected series sheet. Completed assignment history is preserved. Retired tasks cannot be assigned. Manager+.

### `/unretire-task [name]`
Restores a retired task to active status. Manager+.

### `/map-role-task [role] [task]`
Maps a role to a task it is responsible for. Users must hold a role with this mapping to be assignable to that task. Manager+.

### `/unmap-role-task [role] [task]`
Removes a role-task mapping. Manager+.

### `/set-task-dependency [task] [depends_on]`
Declares that `task` cannot be marked complete until `depends_on` is complete for the same chapter. Manager+.

### `/remove-task-dep [task] [depends_on]`
Removes a task prerequisite. Manager+.

### `/list-roles`
Lists all scanlation roles and their mapped tasks.

### `/list-tasks [include_retired?]`
Lists all tasks ordered by level then name. Pass `include_retired: true` to also show retired tasks.

### `/list-role-tasks`
Lists all role-to-task mappings.

### `/list-task-deps [task]`
Shows the full prerequisite graph for a task — both what it depends on and what depends on it.

### `/series`
Multi-subcommand for managing series. Manager+.

- **`add [name]`** — Create a new series.
- **`set-status [name] [status]`** — Update series status (`active`, `hiatus`, `completed`, `dropped`).
- **`assign [name] [user] [task] [sync_chapters?]`** — Add a user to the series' default crew for a task. The user must hold a role mapped to that task. New chapters automatically inherit these assignments. Also clears the series-level placeholder for that task. When `sync_chapters` is `true` (default), also adds the assignment to every existing non-released chapter in the series that the user isn't already assigned to, and clears chapter-level placeholders for that task in the series.
- **`unassign [name] [user] [task] [sync_chapters?]`** — Remove a user from the default crew. When `sync_chapters` is `true` (default), also removes their outstanding (not yet completed) chapter assignments across all non-released chapters in the series. Completed assignments are never removed.
- **`remove [name]`** — Delete a series and all its chapters. Manager can remove series with no completed assignments; Supermanager can remove any (completed assignments are cleared first to bypass the immutability trigger).
- **`add-placeholder [name] [task] [sync_chapters?]`** — Add a series-level vacancy slot indicating that staff is needed for this task. Blocked if the task is retired. When `sync_chapters` is `true` (default), also creates one chapter-level placeholder per non-released chapter in the series.
- **`remove-placeholder [name] [task] [sync_chapters?]`** — Remove all series-level placeholder slots for the task. Reports an error if none exist. When `sync_chapters` is `true` (default), also removes all chapter-level placeholders for that task across all non-released chapters.

### `/chapter`
Multi-subcommand for managing chapters. Manager+.

- **`add [series] [number] [name?] [volume?]`** — Add a chapter. Automatically creates chapter assignments from the series' default crew.
- **`set-status [series] [chapter] [status]`** — Update chapter status (`in_progress`, `queued`, `released`, `hiatus`, `dropped`).
- **`assign [series] [chapter] [user] [task]`** — Assign a user to a chapter for a task. The user must hold a role mapped to that task. Automatically consumes one placeholder vacancy for that slot if any exist.
- **`unassign [series] [chapter] [user] [task]`** — Remove an outstanding assignment. Cannot remove a completed assignment — use `uncomplete` first.
- **`uncomplete [series] [chapter] [user] [task]`** — Mark a completed assignment as outstanding again. Only allowed when the chapter status is `in_progress`.
- **`remove [series] [chapter]`** — Delete a chapter and all its assignments. Manager can remove chapters with no completed assignments; Supermanager can remove any.
- **`bulk-add [series] [chapters]`** — Add multiple chapters at once. `chapters` is a comma-separated list of numbers (e.g. `51,52,53.5`). Input is normalized and validated. Each new chapter inherits the series' default crew. Numbers that already exist are skipped and reported.
- **`move-assignment [series] [chapter] [from_user] [to_user] [task]`** — Transfer an outstanding assignment from one user to another. To user must hold a capable role. Blocked if the assignment is already completed.
- **`add-placeholder [series] [chapter] [task]`** — Mark a chapter+task slot as needing staff without assigning anyone yet. Multiple placeholders per slot are allowed (e.g. if two people are needed for the same task). Appears as a red **TBD** cell in the series sheet. Blocked if the task is retired.
- **`remove-placeholder [series] [chapter] [task]`** — Remove all placeholder vacancies for a chapter+task slot. Reports an error if no placeholder exists.

All name fields support autocomplete.

### `/list-series [status?]`
Lists all series sorted chronologically by latest chapter activity. Each entry shows chapter count and the date of the latest relevant chapter as a Discord timestamp.

- **`status`** — Optional filter: `active`, `completed`, `dropped`, `hiatus`.
- Results are paginated (10 per page) with ◀ ▶ reactions.

### `/list-chapters [series?] [status?] [sort?]`
Lists chapters with task completion progress (`X/Y tasks`).

- **`series`** — Optional filter: restrict to one series (autocomplete).
- **`status`** — Optional filter: `in_progress`, `released`, `dropped`, `hiatus`.
- **`sort`** — `number` (default, ascending) or `chronological` (`closed_at DESC NULLS LAST`).
- Results are paginated (10 per page) with ◀ ▶ reactions.

### `/info`
Deep-dive view for a single entity. Autocomplete on all name fields.

- **`series [name]`** — Series status, default crew grouped by task, and up to 15 chapters with task completion. Manager+.
- **`chapter [series] [chapter]`** — Chapter metadata and all assignments grouped by task, marked ✅ (with completion date) or ⬜ (outstanding). Manager+.
- **`user [user?]`** — User profile: display name, alias, permission level, linked Discord, active series, and total series worked. Omitting `user` shows your own profile; manager+ to view others.

### `/promote [user]`
Promotes a user one permission level: Standard → Manager → Supermanager. Supermanager only.

### `/demote [user]`
Demotes a user one permission level: Supermanager → Manager → Standard. Blocked by the database if it would remove the last Supermanager. Supermanager only.

### `/work-update [series] [chapter] [task] [user?]`
Marks a chapter task assignment as complete. All fields support autocomplete — chapter list is filtered to the selected series, task list is filtered to outstanding assignments in that chapter.

- **`user`** (manager+) — Mark complete on behalf of another user.
- **Dependency checking:** Blocks until all prerequisite tasks for that chapter are complete.
- **Downstream pings:** Mentions all users whose tasks were unblocked by this completion.
- **Auto-release:** If no incomplete assignments remain **and no placeholder vacancies exist**, the chapter status is automatically set to `released`.

### `/bulk-work-update [series] [task] [chapters] [user?]`
Marks a task complete across multiple chapters in a single command. `chapters` is a comma-separated list of chapter numbers (e.g. `51,52,53.5`). Input is normalized and validated. Autocomplete is available for `series` and `task`; `task` is filtered to incomplete assignments in the selected series.

- **`user`** (manager+) — Mark complete on behalf of another user.
- **Validate-then-execute:** All chapters are checked first — any error (not found, not assigned, already complete, dependency blocked) aborts the entire command with a per-chapter report.
- **Downstream pings:** Mentions users whose tasks became fully unblocked, deduplicated across all chapters.
- **Auto-release:** Chapters with no remaining incomplete assignments **and no placeholder vacancies** are automatically set to `released`.

### `/todo [user?]`
Shows all outstanding task assignments that are ready to start (prerequisites satisfied), grouped by series and chapter.

- With no argument: shows your own to-do list.
- With a `user` argument (manager+): shows that user's to-do list.

### `/user-history [user?] [series?]`
Paginated log of all completed assignments, sorted newest first.

- With no arguments: shows your own history.
- **`user`** (manager+) — View another user's history.
- **`series`** — Optional filter (autocomplete).
- Results are paginated (10 per page) with ◀ ▶ reactions.

### `/gsheet [enable|disable]`
Toggles Google Sheets integration. Supermanager only.

- **`enable`** — Verifies that `backend_url` and `api_token` are set in `config.json`, calls `GET /health` (backend auth) then `GET /sheets/health` (Sheets API connectivity), and stores `gsheet_enabled = "1"` in the `bot_settings` table.
- **`disable`** — Removes `gsheet_enabled` from `bot_settings`.

Sync only fires when both config keys are present **and** `gsheet_enabled` is set. See `backend/README.md` for full setup instructions.

---

## Work Progress Message Format

The bot listens for messages in the configured progress channel that match this pipe-delimited format:

```
StaffName|<#channelId>|ChapterNumber|Task
StaffName|<#channelId>|ChapterNumber|Task|NextRole
```

- The channel field must be a Discord channel mention (e.g. `<#123456789>`).
- Messages with 3 pipes (4 fields) are accepted without a next role.
- Messages with 4 pipes (5 fields) are accepted with a next role.
- Any other format is silently ignored.
- Currently parses and echoes the message; DB write is not yet implemented.

---

## Notes

- Commands are registered per-guild (not globally) for faster propagation during development.
- All dates are returned as Discord Unix timestamps (`<t:UNIX>`) so Discord renders them in the user's local timezone.
- Autocomplete options use case-insensitive substring matching.
- `ConfigManager` is thread-safe and persists changes to disk on every `set` call.
- `ConnectionPool` is thread-safe. Callers construct `DbSession session(bot.getPool())` — provides `wtx()` (write transaction) and `rtx()` (read transaction). `session.commit()` commits; the destructor releases the connection back to the pool.
- `CurlGlobalManager::curlManagerInit()` must be called once at startup before any HTTP calls.
- The `i_guild_members` privileged intent must be enabled in the Discord Developer Portal for `/set-staff-role` and `/sync-role` to populate the guild member cache.
- Google Sheets sync calls (`SheetSync::syncSeries`, `syncTodo`, `deleteSeries`) are fire-and-forget — they run in a detached thread after `session.commit()`, log errors to stderr, and never propagate failures to the user.
