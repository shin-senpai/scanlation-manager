# CLAUDE.md

This file provides guidance for working in the `scanlation-manager` monorepo.

## Project Overview

A management platform for scanlation groups. Built as a monorepo where a shared PostgreSQL database is the source of truth. Two interfaces are planned — a Discord bot (actively developed) and a web app (not yet started) — that operate against the same schema simultaneously.

## Repo Structure

```
scanlation-manager/
├── discord/     # C++ Discord bot — primary active module
├── db/          # PostgreSQL schema (migrations + Docker Compose)
├── backend/     # Go REST API — external storage integrations (Google Drive, S3)
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

Required keys: `discord_bot_token`, `guild_id`, `db_connection_string`, `db_pool_size`
Optional keys: `work_progress_channel`, `staff_role_id`, `gsheet_auth_token`, `gsheet_priv_api_url`

The `db_connection_string` value is a libpq connection string, e.g.:
`"host=localhost port=5432 dbname=scanlation user=postgres password=secret"`

### Source Layout
```
discord/
├── include/
│   ├── bot/
│   │   ├── Bot.hpp
│   │   ├── eventHandlers/
│   │   │   ├── commands/
│   │   │   │   ├── add/          # Commands that create new records
│   │   │   │   ├── modify/       # Commands that update existing records
│   │   │   │   ├── list/         # Read-only / query commands
│   │   │   │   ├── remove/       # Commands that delete records
│   │   │   │   └── manage/       # Multi-operation entity commands (e.g. /series, /chapter)
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
1. Pick the right subdirectory under `commands/`: `add/`, `modify/`, `list/`, `remove/`, or `manage/` (for commands that span multiple operation types via subcommands)
2. Add a header in `include/bot/eventHandlers/commands/<subdir>/MyCommand.hpp` and implementation in `src/bot/eventHandlers/commands/<subdir>/MyCommand.cpp`
3. Register the command in `Bot::fillCommandMap()` in `Bot.cpp`
4. CMake picks up new `.cpp` files automatically via `GLOB_RECURSE`

### Adding a New Repository
1. Add header/impl under `include/db/repositories/` and `src/db/repositories/`
2. In a command handler, construct `DbSession session(bot.getPool())` — call `session.commit()` on success; the destructor releases the connection back to the pool automatically

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
psql "$DATABASE_URL" -f db/migrations/018_cascade_deletes_and_task_retirement.sql
psql "$DATABASE_URL" -f db/migrations/019_uppercase_role_task_names.sql
psql "$DATABASE_URL" -f db/migrations/020_add_volume_to_chapters.sql
```

### Key Schema Notes
- `users.name` is nullable — users can exist with only a Discord identity
- Every user must have either a `name` (webapp) or a row in `discord_identities` — enforced by a constraint trigger
- At least one supermanager must always exist — enforced by a constraint trigger
- `chapter_assignments.completed_at` is both the "done" flag and the completion record — `NULL` means outstanding, non-null means done
- Tasks can be hard-deleted only if they have no completed `chapter_assignments`; otherwise they must be retired (`tasks.retired_at`)
- Deleting a role or task cascades to related junction tables at the DB level (`ON DELETE CASCADE`); `chapter_assignments` is excluded from cascade to preserve history

---

## Current Command Status (Discord Bot)

| Command | Status |
|---------|--------|
| `/ping` | Done |
| `/register` | Done |
| `/set-progress-channel` | Done |
| `/set-staff-role` | Done |
| `/set-alias` | Done |
| `/add-role` | Done |
| `/add-task` | Done |
| `/assign-role` | Done |
| `/remove-role` | Done |
| `/delete-role` | Done |
| `/delete-task` | Done |
| `/retire-task` | Done |
| `/unretire-task` | Done |
| `/map-role-task` | Done |
| `/remove-role-task` | Done |
| `/list-roles` | Done |
| `/list-tasks` | Done |
| `/list-role-tasks` | Done |
| `/series` (add, set-status, assign, unassign) | Done |
| `/chapter` (add, set-status, assign, unassign, uncomplete) | Done |
| Work progress message trigger | Parses & echoes (no DB write yet) |
| `/work-update` | In progress — autocomplete hardcoded |
| Google Sheets sync | Not started |
