# CLAUDE.md

This file provides guidance for working in the `scanlation-manager` monorepo.

## Project Overview

A management platform for scanlation groups. Built as a monorepo where a shared PostgreSQL database is the source of truth. Two interfaces are planned — a Discord bot (actively developed) and a web app (not yet started) — that operate against the same schema simultaneously.

## Repo Structure

```
scanlation-manager/
├── discord/     # C++ Discord bot — primary active module
├── db/          # PostgreSQL schema (migrations + Docker Compose)
├── backend/     # REST API — not yet implemented (placeholder)
└── frontend/    # Web UI — not yet implemented (placeholder)
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
cmake --preset default
cmake --build --preset default
./build/scanlation-manager   # config.json is auto-copied to build/ on build
```

Dependencies D++ and libpqxx are fetched automatically by CMake on first build. nlohmann_json and libcurl must be installed on the system.

### Configuration
The bot reads `discord/config.json` (copied to `build/config.json` at build time). Copy from example and fill in values:
```bash
cp discord/config.json.example discord/config.json
```

Required keys: `discord_bot_token`, `guild_id`, `database`
Optional keys: `work_progress_channel`, `gsheet_auth_token`, `gsheet_priv_api_url`

The `database` value is a libpq connection string, e.g.:
`"host=localhost port=5432 dbname=scanlation user=postgres password=secret"`

### Source Layout
```
discord/
├── include/
│   ├── bot/
│   │   ├── Bot.hpp
│   │   ├── eventHandlers/
│   │   │   ├── commands/         # One header per slash command
│   │   │   └── triggers/         # One header per message trigger
│   │   └── utils/                # BotUtils, ChannelUtils
│   ├── db/
│   │   ├── DbSession.hpp
│   │   ├── ConnectionPool.hpp
│   │   ├── repositories/         # One header per DB table/entity
│   │   └── utils/                # PqxxErrors (constraint name extraction)
│   ├── models/                   # Plain data structs (no logic)
│   ├── types/                    # Shared enums: Permission, SeriesStatus, ChapterStatus
│   └── utils/                    # ConfigManager, HttpUtils
└── src/                          # Mirrors include/ structure
```

### Adding a New Slash Command
1. Add a header in `include/bot/eventHandlers/commands/MyCommand.hpp` and implementation in `src/bot/eventHandlers/commands/MyCommand.cpp`
2. Register the command in `Bot::fillCommandMap()` in `Bot.cpp`
3. CMake picks up new `.cpp` files automatically via `GLOB_RECURSE`

### Adding a New Repository
1. Add header/impl under `include/db/repositories/` and `src/db/repositories/`
2. Use a `DbSession` obtained from `Bot::getDb().session()` — call `session->commit()` on success; the destructor releases the connection automatically

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

### Migrations
Applied manually in order. There is no migration runner yet.
```bash
psql "$DATABASE_URL" -f db/migrations/001_initial.sql
psql "$DATABASE_URL" -f db/migrations/002_add_supermanager.sql
psql "$DATABASE_URL" -f db/migrations/003_add_user_credentials.sql
psql "$DATABASE_URL" -f db/migrations/004_add_discord_identities.sql
psql "$DATABASE_URL" -f db/migrations/005_enforce_webapp_name.sql
psql "$DATABASE_URL" -f db/migrations/006_add_hiatus_to_series_check_constraint.sql
psql "$DATABASE_URL" -f db/migrations/007_add_unlinked_at_to_discord_identities.sql
psql "$DATABASE_URL" -f db/migrations/008_add_chapter_status.sql
psql "$DATABASE_URL" -f db/migrations/009_role_task_mapping_and_assignment_refactor.sql
psql "$DATABASE_URL" -f db/migrations/010_fix_supermanager_check.sql
psql "$DATABASE_URL" -f db/migrations/011_replace_manager_flags_with_permission_level.sql
psql "$DATABASE_URL" -f db/migrations/012_add_closed_at_to_chapters.sql
psql "$DATABASE_URL" -f db/migrations/013_simplify_chapter_task_tracking.sql
psql "$DATABASE_URL" -f db/migrations/014_add_hiatus_to_chapter_status.sql
psql "$DATABASE_URL" -f db/migrations/015_add_missing_indexes.sql
psql "$DATABASE_URL" -f db/migrations/016_citext_name_columns.sql
psql "$DATABASE_URL" -f db/migrations/017_add_chapter_number.sql
```

### Key Schema Notes
- `users.name` is nullable — users can exist with only a Discord identity
- Every user must have either a `name` (webapp) or a row in `discord_identities` — enforced by a constraint trigger
- At least one supermanager must always exist — enforced by a constraint trigger
- `chapter_assignments.completed_at` is both the "done" flag and the completion record — `NULL` means outstanding, non-null means done

---

## Current Command Status (Discord Bot)

| Command | Status |
|---------|--------|
| `/ping` | Done |
| `/register` | Done |
| `/set-progress-channel` | Done |
| `/set-alias` | Done |
| Work progress message trigger | Parses & echoes (no DB write yet) |
| `/work-update` | In progress — autocomplete hardcoded |
| Google Sheets sync | Not started |
