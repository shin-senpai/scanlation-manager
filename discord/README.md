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
| ✅ | `/ping` — Health check command |
| ✅ | `/register` — Register a Discord user as a team member |
| ✅ | `/set-progress-channel` — Designate a channel for work progress messages |
| ✅ | `/set-alias` — Set a display alias for release credits |
| ✅ | Work progress message trigger — Parses and echoes structured progress updates |
| 🚧 | `/work-update` — Slash command to submit a work progress update (autocomplete hardcoded) |
| 🚧 | Google Sheets integration — Sync progress data to a spreadsheet |

---

## Project Structure

```
discord/
├── main.cpp
├── bot/
│   ├── Bot.hpp / Bot.cpp                          # Core bot class, command & trigger registration
│   ├── eventHandlers/
│   │   ├── commands/
│   │   │   ├── Ping.hpp / Ping.cpp
│   │   │   ├── RegisterUser.hpp / RegisterUser.cpp
│   │   │   ├── SetAlias.hpp / SetAlias.cpp
│   │   │   ├── SetProgressChannel.hpp / SetProgressChannel.cpp
│   │   │   └── WorkProgress.hpp / WorkProgress.cpp
│   │   └── triggers/
│   │       └── WorkProgress.hpp / WorkProgress.cpp
│   └── utils/
│       └── ChannelUtils.hpp / ChannelUtils.cpp    # Channel mention parsing helpers
├── db/
│   ├── DbSession.hpp / DbSession.cpp              # Transaction session (RAII)
│   ├── ConnectionPool.hpp / ConnectionPool.cpp    # Thread-safe connection pooling
│   ├── repositories/                              # One file pair per DB table
│   │   ├── User.hpp / User.cpp
│   │   ├── DiscordIdentities.hpp / DiscordIdentities.cpp
│   │   ├── UserAliases.hpp / UserAliases.cpp
│   │   ├── UserCredentials.hpp / UserCredentials.cpp
│   │   ├── UserRoles.hpp / UserRoles.cpp
│   │   ├── Roles.hpp / Roles.cpp
│   │   ├── RoleTasks.hpp / RoleTasks.cpp
│   │   ├── Tasks.hpp / Tasks.cpp
│   │   ├── TaskDependencies.hpp / TaskDependencies.cpp
│   │   ├── Series.hpp / Series.cpp
│   │   ├── Chapters.hpp / Chapters.cpp
│   │   ├── SeriesAssignments.hpp / SeriesAssignments.cpp
│   │   └── ChapterAssignments.hpp / ChapterAssignments.cpp
│   └── utils/
│       └── PqxxErrors.hpp / PqxxErrors.cpp        # Constraint name extraction from pqxx exceptions
├── models/                                        # Plain data structs (no logic)
│   ├── ModelUser.hpp
│   ├── ModelSeries.hpp
│   ├── ModelChapter.hpp
│   ├── ModelRole.hpp
│   ├── ModelTask.hpp
│   ├── ModelTaskDependency.hpp
│   ├── ModelRoleTask.hpp
│   ├── ModelUserRole.hpp
│   ├── ModelSeriesAssignment.hpp
│   ├── ModelChapterAssignment.hpp
│   └── ModelWorkProgress.hpp
├── types/                                         # Shared enums
│   ├── Permission.hpp
│   ├── SeriesStatus.hpp
│   └── ChapterStatus.hpp
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
| `database` | ✅ | `string` | PostgreSQL connection string |
| `work_progress_channel` | ❌ | `uint64` | Channel ID for progress message trigger (can be set via `/set-progress-channel`) |
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

### `/ping`
Simple health check. Responds with `Pong! 🏓`.

### `/register`
Registers the calling Discord user as a scanlation team member. Creates a user record in the database and links their Discord identity to it. Returns a confirmation message, or an error if they are already registered.

### `/set-progress-channel [channel]`
Sets the channel where the bot will watch for work progress messages. The setting is persisted to `config.json`.

### `/set-alias [alias]`
Sets a display alias for the calling user, used in release credits. Enforces uniqueness — no two active users can share an alias, and each user can only have one active alias at a time.

### `/work-update` *(in progress)*
Allows staff to submit a work update via slash command with autocomplete for series, chapter, and task. Currently the autocomplete options are hardcoded placeholders.

---

## Notes

- Commands are registered per-guild (not globally) for faster propagation during development.
- The `ConfigManager` is thread-safe and persists changes to disk on every `set` call.
- The database layer uses a `ConnectionPool` (accessed via `Bot::getPool()`). Callers construct a `DbSession` from the pool directly — `DbSession session(bot.getPool())` — which provides `wtx()` (write transaction) and `rtx()` (read transaction). All queries use parameterized statements.
