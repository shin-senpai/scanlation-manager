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
| ✅ | `/set-progress-channel` — Designate a channel for work progress messages |
| ✅ | Work progress message trigger — Parses and echoes structured progress updates |
| 🚧 | `/work-update` — Slash command to submit a work progress update (autocomplete for series/chapter/task in progress) |
| 🚧 | Google Sheets integration — Sync progress data to a spreadsheet |
| 🚧 | Dynamic autocomplete — Series, chapters, and tasks currently hardcoded |

---

## Project Structure

```
discord/
├── main.cpp
├── bot/
│   ├── Bot.hpp / Bot.cpp                  # Core bot class, command & trigger registration
│   └── eventHandlers/
│       ├── commands/
│       │   ├── Ping.hpp / Ping.cpp
│       │   ├── SetProgressChannel.hpp / SetProgressChannel.cpp
│       │   └── WorkProgress.hpp / WorkProgress.cpp
│       └── triggers/
│           └── WorkProgress.hpp / WorkProgress.cpp
├── models/
│   └── ModelWorkProgress.hpp              # WorkProgress data struct
└── utils/
    ├── ChannelUtils.hpp / ChannelUtils.cpp  # Channel mention parsing helpers
    ├── ConfigManager.hpp / ConfigManager.cpp # JSON config read/write
    └── HttpUtils.hpp / HttpUtils.cpp         # libcurl HTTP GET/POST wrappers
```

---

## Configuration

The bot reads from a `config.json` file at the working directory. The following keys are used:

| Key | Required | Type | Description |
|-----|----------|------|-------------|
| `discord_bot_token` | ✅ | `string` | Your Discord bot token |
| `guild_id` | ✅ | `uint64` | The Discord server (guild) ID to register commands to |
| `work_progress_channel` | ❌ | `uint64` | Channel ID for progress message trigger (can be set via `/set-progress-channel`) |
| `gsheet_auth_token` | ❌ | `string` | Auth token for Google Sheets private API (future use) |
| `gsheet_priv_api_url` | ❌ | `string` | Endpoint URL for Google Sheets private API (future use) |

**Example `config.json`:**
```json
{
  "discord_bot_token": "YOUR_TOKEN_HERE",
  "guild_id": 123456789012345678,
  "work_progress_channel": 987654321098765432
}
```

---

## Bot Invite

When inviting the bot to your server, make sure to include both the `bot` and `applications.commands` scopes:

```
https://discord.com/oauth2/authorize?client_id=YOUR_CLIENT_ID&scope=bot+applications.commands&permissions=139586816064
```

---

## Dependencies

- [D++ (libdpp)](https://dpp.dev/) — Discord API wrapper
- [nlohmann/json](https://github.com/nlohmann/json) — JSON parsing for config
- [libcurl](https://curl.se/libcurl/) — HTTP client for external API calls
- CMake (build system)

---

## Building

> Build instructions depend on the CMake setup in `discord/`. Ensure all dependencies are installed and available to CMake.

```bash
cd discord
mkdir build && cd build
cmake ..
make
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

### `/set-progress-channel [channel]`
Sets the channel where the bot will watch for work progress messages. The setting is persisted to `config.json`.

### `/work-update` *(in progress)*
Allows staff to submit a work update via slash command with autocomplete for series, chapter, and task. Currently the autocomplete options are hardcoded placeholders.

---

## Notes

- Commands are registered per-guild (not globally) for faster propagation during development.
- The `ConfigManager` is thread-safe and persists changes to disk on every `set` call.