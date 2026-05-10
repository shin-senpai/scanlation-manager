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
| ✅ | `/add-task` — Create a task type |
| ✅ | `/sync-role` — Bulk-map a Discord role to an app role and assign it to all registered members who have it |
| ✅ | `/assign-role` — Assign a role to a user |
| ✅ | `/remove-role` — Remove a role from a user |
| ✅ | `/delete-role` — Delete a role and all its mappings |
| ✅ | `/delete-task` — Delete a task (only if no completed assignments) |
| ✅ | `/retire-task` — Soft-retire a task, preserving its history |
| ✅ | `/unretire-task` — Restore a retired task to active status |
| ✅ | `/map-role-task` — Map a role to a task it is responsible for |
| ✅ | `/unmap-role-task` — Remove a role-task mapping |
| ✅ | `/list-roles` — List all roles |
| ✅ | `/list-tasks` — List all tasks |
| ✅ | `/list-role-tasks` — List all role-task mappings |
| ✅ | `/series` — Manage series: add, set-status, assign/unassign default crew, remove |
| ✅ | `/chapter` — Manage chapters: add, set-status, assign/unassign/uncomplete/remove |
| ✅ | `/list-series` — List series with chapter counts and latest activity, optional status filter |
| ✅ | `/list-chapters` — List chapters with task completion stats, optional series/status/sort |
| ✅ | `/info` — Deep-dive view for a series (crew + chapters), chapter (assignments), or user profile |
| ✅ | `/promote` — Promote a user to the next permission level |
| ✅ | `/demote` — Demote a user to the previous permission level |
| ✅ | `/work-update` — Mark a chapter task as complete; blocks on unmet dependencies, pings downstream assignees |
| ✅ | `/todo` — Show outstanding tasks that are ready to start (dependencies satisfied) |
| ✅ | `/user-history` — Paginated log of all completed assignments, newest first, optionally filtered by series |
| 🚧 | Work progress message trigger — Parses structured messages in the progress channel (no DB write yet) |
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
| `db_pool_size` | ✅ | `size_t` | Number of database connections to maintain in the pool |
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

The bot requires the `Server Members Intent` privileged gateway intent to be enabled in the Discord Developer Portal. This is used by `/set-staff-role` and `/sync-role` to iterate the guild member cache.

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

# Debug (default)
cmake --preset default
cmake --build --preset default
./build/debug/scanlation-manager   # config.json is auto-copied to build/debug/ on build

# Release
cmake --preset release
cmake --build --preset release
./build/release/scanlation-manager
```

---

## Permission Levels

The bot has three internal permission levels assigned per user:

| Level | Name | Description |
|-------|------|-------------|
| 0 | Standard | Default for all registered users. Can submit work updates and view their own to-do list. |
| 1 | Manager | Can manage team structure: create roles/tasks, assign users, manage series and chapters. |
| 2 | Supermanager | Full access. Can promote/demote users and change bot-wide settings. At least one supermanager must always exist — enforced at the database level. |

The first user to run `/register` is automatically granted Supermanager.

**Bot access gate:** Before any command is processed, the bot checks that the user has the configured staff Discord role or is a server admin (has `Manage Server` or `Administrator` permissions). Users who fail this check receive an ephemeral error and the command is not dispatched.

### Command permission summary

| Command | Minimum level |
|---------|--------------|
| `/ping` | Bot access (no registration required) |
| `/register` (self) | Bot access (no registration required) |
| `/register [user]` | Manager |
| `/set-alias` | Standard (registered) |
| `/work-update` | Standard (registered) |
| `/todo` | Standard (registered) |
| `/user-history` | Standard (registered; manager+ to view others) |
| `/set-progress-channel` | Manager |
| `/list-roles` | Standard (registered) |
| `/list-tasks` | Standard (registered) |
| `/list-role-tasks` | Standard (registered) |
| `/list-series` | Standard (registered) |
| `/list-chapters` | Standard (registered) |
| `/info user` | Standard (registered; manager+ to view others) |
| `/info series` | Manager |
| `/info chapter` | Manager |
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
| `/series` | Manager |
| `/chapter` | Manager |
| `/set-staff-role` | Supermanager |
| `/promote` | Supermanager |
| `/demote` | Supermanager |

---

## Slash Commands

### `/ping`
Health check. Responds with `Pong!`.

### `/register [user?]`
Registers a Discord user as a scanlation team member. Creates a user record and links their Discord identity.

- With no arguments: registers yourself.
- With a `user` argument (manager+): registers the specified Discord user on their behalf — useful for onboarding members who haven't run the command themselves.
- The first user to register is automatically granted Supermanager.
- Errors if the target is already registered.

### `/set-progress-channel [channel]`
Sets the channel where the bot watches for work progress messages. Persisted to `config.json`.

### `/set-staff-role [role]`
Sets the Discord role that gates bot access. Persisted to `config.json`. After updating the role, the bot scans the guild member cache and auto-registers any members who have that role but are not yet in the system. Reports how many were registered and how many were already present. Requires Supermanager.

### `/set-alias [alias]`
Sets a display alias for the calling user, used in release credits. Enforces uniqueness — no two active users can share an alias.

### `/add-role [name]`
Creates a new scanlation role (e.g. "Translator", "Cleaner"). Manager+.

### `/add-task [name]`
Creates a new task type (e.g. "Translation", "Cleaning"). Manager+.

### `/sync-role [role]`
Takes a Discord role, creates a matching app role by the same name (or finds the existing one), then walks the guild member cache and assigns that app role to every registered user who has the Discord role. Reports assigned/already-had/not-registered counts. Manager+.

### `/assign-role [user] [role]`
Assigns a scanlation role to a user. Manager+.

### `/remove-role [user] [role]`
Removes a scanlation role from a user. Manager+.

### `/delete-role [name]`
Deletes a role and cascades to all role-task mappings and user-role assignments. Manager+.

### `/delete-task [name]`
Deletes a task. Only allowed if the task has no completed chapter assignments — if it does, use `/retire-task` instead. Manager+.

### `/retire-task [name]`
Soft-retires a task — it is hidden from new assignment but its completion history is preserved. Manager+.

### `/unretire-task [name]`
Restores a retired task to active status. Manager+.

### `/map-role-task [role] [task]`
Maps a role to a task it is responsible for. A user must hold a role with this mapping to be assignable to that task. Manager+.

### `/unmap-role-task [role] [task]`
Removes a role-task mapping. Manager+.

### `/list-roles`
Lists all scanlation roles.

### `/list-tasks`
Lists all tasks, including retired ones (marked separately).

### `/list-role-tasks`
Lists all role-to-task mappings.

### `/series`
Multi-subcommand for managing series. Manager+.

- **`add [name]`** — Create a new series.
- **`set-status [name] [status]`** — Update series status (`active`, `hiatus`, `completed`, `dropped`). Autocomplete on name.
- **`assign [name] [user] [task]`** — Add a user to the series' default crew for a task. The user must have a role mapped to that task. New chapters added to this series will automatically inherit these assignments.
- **`unassign [name] [user] [task]`** — Remove a user from the default crew.
- **`remove [name]`** — Delete a series and all its chapters. Manager can remove series with no completed assignments; Supermanager can remove any series (completed assignments are cleared first).

### `/chapter`
Multi-subcommand for managing chapters. Manager+.

- **`add [series] [number] [name] [volume?]`** — Add a chapter to a series. Automatically creates chapter assignments from the series' default crew. Volume is optional.
- **`set-status [series] [chapter] [status]`** — Update chapter status (`in_progress`, `released`, `hiatus`, `dropped`).
- **`assign [series] [chapter] [user] [task]`** — Assign a user to a chapter for a specific task. The user must have a role mapped to that task.
- **`unassign [series] [chapter] [user] [task]`** — Remove an assignment. Cannot remove a completed assignment — use `uncomplete` first.
- **`uncomplete [series] [chapter] [user] [task]`** — Mark a completed assignment as outstanding again. Only allowed when the chapter status is `in_progress`.
- **`remove [series] [chapter]`** — Delete a chapter and all its assignments. Manager can remove chapters with no completed assignments; Supermanager can remove any chapter.

All name fields support autocomplete.

### `/list-series [status?]`
Lists all series sorted chronologically by latest chapter activity (falls back to alphabetical when no chapters exist). Each entry shows chapter count and the date of the latest relevant chapter as a Discord timestamp.

- **`status`** — Optional filter: `active`, `completed`, `dropped`, `hiatus`.
- Results are paginated (10 per page). Use the ◀ ▶ reactions to navigate — only the user who ran the command can page through results.

### `/list-chapters [series?] [status?] [sort?]`
Lists chapters with task completion progress (`X/Y tasks`).

- **`series`** — Optional filter: restrict to one series (autocomplete).
- **`status`** — Optional filter: `in_progress`, `released`, `dropped`, `hiatus`.
- **`sort`** — `number` (default, ascending by chapter number) or `chronological` (`closed_at DESC NULLS LAST`).
- When no series filter is applied, each entry is prefixed with the series name.
- Results are paginated (10 per page) with ◀ ▶ reactions.

### `/info`
Deep-dive view for a single entity. Autocomplete on all name fields.

- **`series [name]`** — Shows series status, default crew grouped by task, and up to 15 chapters with task completion (`X/Y tasks`). A hint is shown when there are more than 15 chapters. Manager+.
- **`chapter [series] [chapter]`** — Shows chapter metadata (volume, number, status, dates as Discord timestamps) and all assignments grouped by task, each marked ✅ (with completion date) or ⬜ (outstanding). Manager+.
- **`user [user?]`** — Shows a user's profile: display name, username (N/A if unset), alias (N/A if unset), permission level, joined/left dates, linked Discord account, series currently assigned to, and total series worked on. Omitting `user` shows your own profile. Manager+ required to look up others.

### `/promote [user]`
Promotes a user one permission level: Standard → Manager → Supermanager. Supermanager only.

### `/demote [user]`
Demotes a user one permission level: Supermanager → Manager → Standard. Supermanager only. Blocked by the database if it would remove the last Supermanager.

### `/work-update [series] [chapter] [task]`
Marks one of your outstanding chapter task assignments as complete. All fields support autocomplete — the chapter list is filtered to the selected series, and the task list is filtered to your outstanding assignments in that chapter.

- **Dependency checking:** If the task has dependencies defined (via task dependency mappings), the command blocks completion until all assignments for those prerequisite tasks in that chapter are finished, naming the blocking task.
- **Downstream pings:** After marking complete, the bot looks up all users assigned to tasks that depend on the just-completed task in that chapter (and whose assignments are still outstanding), and mentions them in the response with the specific task they can now proceed with. Users assigned to multiple unblocked tasks receive separate mentions per task.

### `/todo [user?]`
Shows all outstanding task assignments that are ready to start — i.e. whose dependencies are fully satisfied. Results are grouped by series and chapter.

- With no argument: shows your own to-do list.
- With a `user` argument: shows that user's to-do list (any registered user can view anyone's).
- Returns "no pending tasks" if everything is blocked or nothing is assigned.

### `/user-history [user?] [series?]`
Paginated log of all completed assignments for a user, sorted newest first.

- With no arguments: shows your own history.
- **`user`** — View another user's history (manager+).
- **`series`** — Optional filter: restrict results to one series (autocomplete).
- Each entry shows series, volume/chapter number, chapter name, task name, and completion date as a Discord timestamp.
- Results are paginated (10 per page) with ◀ ▶ reactions.

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
- The `ConfigManager` is thread-safe and persists changes to disk on every `set` call.
- The database layer uses a `ConnectionPool` (accessed via `Bot::getPool()`). Callers construct a `DbSession` from the pool directly — `DbSession session(bot.getPool())` — which provides `wtx()` (write transaction) and `rtx()` (read transaction). All queries use parameterized statements.
- The `i_guild_members` privileged intent must be enabled in the Discord Developer Portal for `/set-staff-role` and `/sync-role` to populate the guild member cache. Without it, those commands will report that members are not in cache and skip auto-assignment.
