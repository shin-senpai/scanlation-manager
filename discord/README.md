# scanlation-manager — Discord Bot

> Part of the `scanlation-manager` monorepo. This module lives under `discord/`.

A Discord bot built with [D++](https://dpp.dev/) (libdpp) in C++ to help scanlation groups track work progress across series. It listens for structured progress messages in a designated channel and exposes slash commands for managing state.

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
| ✅ | `/register` — Register a Discord user as a team member |
| ✅ | `/set-progress-channel` — Designate a channel for work progress messages |
| ✅ | `/set-staff-role` — Set the Discord role required to use the bot |
| ✅ | `/set-alias` — Set a display alias for release credits |
| ✅ | `/add-role` — Create a scanlation role |
| ✅ | `/add-task` — Create a task type |
| ✅ | `/assign-role` — Assign a role to a user |
| ✅ | `/remove-role` — Remove a role from a user |
| ✅ | `/delete-role` — Delete a role and all its mappings |
| ✅ | `/delete-task` — Delete a task (only if no completed assignments) |
| ✅ | `/retire-task` — Soft-retire a task, preserving its history |
| ✅ | `/unretire-task` — Restore a retired task to active status |
| ✅ | `/map-role-task` — Map a role to a task it is responsible for |
| ✅ | `/remove-role-task` — Remove a role-task mapping |
| ✅ | `/list-roles` — List all roles |
| ✅ | `/list-tasks` — List all tasks |
| ✅ | `/list-role-tasks` — List all role-task mappings |
| ✅ | `/series` — Manage series: add, set-status, assign/unassign default crew |
| ✅ | `/chapter` — Manage chapters: add, set-status, assign/unassign/uncomplete |
| ✅ | Work progress message trigger — Parses and echoes structured progress updates |
| 🚧 | `/work-update` — Slash command to submit a work progress update (autocomplete hardcoded) |
| 🚧 | Google Sheets integration — Sync progress data to a spreadsheet |

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
│       │   │   ├── add/       # Commands that create new records
│       │   │   ├── modify/    # Commands that update existing records
│       │   │   ├── list/      # Read-only / query commands
│       │   │   ├── remove/    # Commands that delete records
│       │   │   └── manage/    # Multi-subcommand entity commands (/series, /chapter)
│       │   └── triggers/
│       │       └── WorkProgress.cpp
│       └── utils/
│           └── ChannelUtils.cpp
├── include/                                       # Mirrors src/ structure
│   └── ...
├── db/
│   ├── DbSession.hpp / DbSession.cpp              # Transaction session (RAII)
│   ├── ConnectionPool.hpp / ConnectionPool.cpp    # Thread-safe connection pooling
│   ├── repositories/                              # One file pair per DB table
│   │   ├── User, DiscordIdentities, UserAliases, UserCredentials
│   │   ├── Roles, UserRoles, RoleTasks
│   │   ├── Tasks, TaskDependencies
│   │   ├── Series, SeriesAssignments
│   │   └── Chapters, ChapterAssignments
│   └── utils/
│       └── PqxxErrors.hpp / PqxxErrors.cpp        # Constraint name extraction from pqxx exceptions
├── models/                                        # Plain data structs (no logic)
├── types/                                         # Shared enums: Permission, SeriesStatus, ChapterStatus
└── utils/
    ├── ConfigManager.hpp / ConfigManager.cpp      # Thread-safe JSON config read/write
    └── HttpUtils.hpp / HttpUtils.cpp              # libcurl HTTP GET/POST wrappers
```

---

## Configuration

The bot reads from a `config.json` file at the working directory. The following keys are used:

| Key | Required | Type | Description |
|-----|----------|------|-------------|
| `discord_bot_token` | ✅ | `string` | Your Discord bot token |
| `guild_id` | ✅ | `uint64` | The Discord server (guild) ID to register commands to |
| `db_connection_string` | ✅ | `string` | PostgreSQL connection string (libpq format) |
| `db_pool_size` | ✅ | `int` | Number of database connections to maintain in the pool |
| `work_progress_channel` | ❌ | `uint64` | Channel ID for progress message trigger (can be set via `/set-progress-channel`) |
| `staff_role_id` | ❌ | `uint64` | Discord role ID required to use the bot (can be set via `/set-staff-role`) |
| `gsheet_auth_token` | ❌ | `string` | Auth token for Google Sheets private API (future use) |
| `gsheet_priv_api_url` | ❌ | `string` | Endpoint URL for Google Sheets private API (future use) |

Copy `config.json.example` and fill in your values.

---

## Bot Invite

When inviting the bot to your server, make sure to include both the `bot` and `applications.commands` scopes:

```
https://discord.com/oauth2/authorize?client_id=YOUR_CLIENT_ID&scope=bot+applications.commands&permissions=139586816064
```

---

## Dependencies

- [D++ (libdpp)](https://dpp.dev/) — Discord API wrapper
- [libpqxx](https://pqxx.org/) — PostgreSQL C++ client
- [nlohmann/json](https://github.com/nlohmann/json) — JSON parsing for config
- [libcurl](https://curl.se/libcurl/) — HTTP client for external API calls
- CMake 3.28+ (build system)

---

## Building

> D++ and libpqxx are fetched automatically by CMake on first build. You only need to have `nlohmann_json` and `libcurl` installed on the system. The database must also be running and migrated before starting the bot (see `db/`).

```bash
cd discord
cmake --preset default
cmake --build --preset default
./build/scanlation-manager
```

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

---

## Slash Commands

All commands except `/ping` and `/register` require the user to be registered. Most management commands require `manager` or `supermanager` permission level.

### `/ping`
Health check. Responds with `Pong!`.

### `/register`
Registers the calling Discord user as a scanlation team member. Creates a user record and links their Discord identity. Errors if already registered.

### `/set-progress-channel [channel]`
Sets the channel where the bot watches for work progress messages. Persisted to `config.json`. Requires server admin or the configured staff role.

### `/set-staff-role [role]`
Sets the Discord role that is allowed to use the bot. Persisted to `config.json`. Requires server admin.

### `/set-alias [alias]`
Sets a display alias for the calling user, used in release credits. Enforces uniqueness — no two active users can share an alias.

### `/add-role [name]`
Creates a new scanlation role (e.g. "Translator", "Cleaner"). Manager+.

### `/add-task [name]`
Creates a new task type (e.g. "Translation", "Cleaning"). Manager+.

### `/assign-role [user] [role]`
Assigns a scanlation role to a user. Manager+.

### `/remove-role [user] [role]`
Removes a scanlation role from a user. Manager+.

### `/delete-role [name]`
Deletes a role and cascades to all role-task mappings and user-role assignments. Manager+.

### `/delete-task [name]`
Deletes a task. Only allowed if the task has no completed chapter assignments; otherwise use `/retire-task`. Manager+.

### `/retire-task [name]`
Soft-retires a task — it is hidden from assignment but its completion history is preserved. Manager+.

### `/unretire-task [name]`
Restores a retired task to active status. Manager+.

### `/map-role-task [role] [task]`
Maps a role to a task it is responsible for. A user must have a role with this mapping to be assigned to that task. Manager+.

### `/remove-role-task [role] [task]`
Removes a role-task mapping. Manager+.

### `/list-roles`
Lists all scanlation roles. Manager+.

### `/list-tasks`
Lists all tasks, including retired ones. Manager+.

### `/list-role-tasks`
Lists all role-to-task mappings. Manager+.

### `/series`
Multi-subcommand for managing series. Manager+.

- **`add [name]`** — Create a new series.
- **`set-status [name] [status]`** — Update series status (`active`, `hiatus`, `completed`, `dropped`). Autocomplete on name.
- **`assign [name] [user] [task]`** — Add a user to the series' default crew for a task. The user must have a role mapped to that task. New chapters added to this series will automatically inherit these assignments.
- **`unassign [name] [user] [task]`** — Remove a user from the default crew.

### `/chapter`
Multi-subcommand for managing chapters. Manager+.

- **`add [series] [number] [name] [volume?]`** — Add a chapter to a series. Automatically creates chapter assignments from the series' default crew. Volume is optional.
- **`set-status [series] [chapter] [status]`** — Update chapter status (`in_progress`, `released`, `hiatus`, `dropped`). Autocomplete on series and chapter name.
- **`assign [series] [chapter] [user] [task]`** — Assign a user to a chapter for a specific task. The user must have a role mapped to that task.
- **`unassign [series] [chapter] [user] [task]`** — Remove an assignment. Cannot remove a completed assignment — use `uncomplete` first.
- **`uncomplete [series] [chapter] [user] [task]`** — Mark a completed assignment as outstanding again. Only allowed when the chapter status is `in_progress`.

### `/work-update` *(in progress)*
Allows staff to submit a work update via slash command with autocomplete for series, chapter, and task. Autocomplete options are currently hardcoded placeholders.

---

## Notes

- Commands are registered per-guild (not globally) for faster propagation during development.
- The `ConfigManager` is thread-safe and persists changes to disk on every `set` call.
- The database layer uses a `ConnectionPool` (accessed via `Bot::getPool()`). Callers construct a `DbSession` from the pool directly — `DbSession session(bot.getPool())` — which provides `wtx()` (write transaction) and `rtx()` (read transaction). All queries use parameterized statements.
