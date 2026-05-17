# scanlation-manager

A management platform for scanlation groups — tracking series, chapters, tasks, and contributors. The project is built as a monorepo with a shared PostgreSQL database, currently with an active Discord bot and a planned web app that will both operate against the same data.

> **Status: In Progress.** The Discord bot is the primary active module. The REST backend is active and provides Google Drive, S3, and Google Sheets integrations. The frontend is planned but not yet started.

---

## Overview

Scanlation groups juggle a lot: multiple series, multiple contributors each filling different roles, custom workflows per project, and the need to track who did what and when. This tool aims to centralize all of that.

The core idea is that both the Discord bot and the future web app share the same database — so a group can use whichever interface fits their workflow, or both at once.

---

## Features

The database schema models all of the following. Bot commands and web UI to expose them are progressively being built on top.

| Status | Feature |
|--------|---------|
| ✅ schema, ✅ db layer, ✅ bot | User registration — link Discord identities to team member accounts |
| ✅ schema, ✅ db layer, ✅ bot | User aliases — track contributors under different names over time |
| ✅ schema, ✅ db layer, ✅ bot | User permissions — standard, manager, and supermanager levels; promote/demote via bot |
| ✅ schema, ✅ db layer, ✅ bot | Custom roles — define capabilities for team members |
| ✅ schema, ✅ db layer, ✅ bot | Custom tasks — define the steps required before a chapter can be released |
| ✅ schema, ✅ db layer, ✅ bot | Task dependencies — specify which tasks block other tasks; enforced on work updates |
| ✅ schema, ✅ db layer, ✅ bot | Series & chapter tracking — track active series and per-chapter assignments |
| ✅ schema, ✅ db layer, ✅ bot | Contribution history — record who completed which task, for which chapter, and when |
| ✅ bot | To-do lists — see which tasks are ready to start (dependencies satisfied), per user |
| ✅ bot | Release summaries — query releases with sorting and filtering |
| ✅ backend | Google Drive — list, upload, download, and delete files via REST API |
| ✅ backend | S3-compatible storage — list, upload, download, and delete objects (AWS S3, Cloudflare R2, MinIO) |
| ✅ backend + bot | Google Sheets sync — automatically push series and todo data to a Google Spreadsheet |

---

## Monorepo Structure

```
scanlation-manager/
├── discord/       # Discord bot (C++/D++) — actively developed
├── db/            # Database migrations and Docker Compose setup
├── backend/       # REST API (Go) — in early development
└── frontend/      # Web UI — planned
```

### discord/

The Discord bot is the first interface into the system. Built with C++20 and [D++](https://dpp.dev/) (a Discord API library), backed by PostgreSQL via `libpqxx`.

See [`discord/README.md`](discord/README.md) for build instructions, configuration, and command documentation.

### db/

PostgreSQL 16 database, run via Docker Compose. Contains migration scripts that define the shared schema used by all services.

### backend/

A Go REST API providing external service integrations for the bot: Google Drive, S3-compatible storage (AWS S3, Cloudflare R2, MinIO), and Google Sheets sync. The Sheets sync service reads from the database and writes live progress data to a configured Google Spreadsheet.

See [`backend/README.md`](backend/README.md) for setup and usage.

### frontend/

Not yet started. Will be a web UI consuming the backend API.

---

## Tech Stack

| Layer | Technology |
|---|---|
| Discord bot | C++20, [D++](https://dpp.dev/) v10.1.4 |
| Database | PostgreSQL 18.3 (Docker) |
| DB client (C++) | libpqxx 8.0.0 |
| Build system | CMake 3.28+ |
| Backend | Go 1.26, stdlib `net/http` |
| Frontend | TBD |

---

## Getting Started

For now, setup means running the database and the Discord bot.

**1. Start the database**

```sh
cd db
cp docker-compose.yml.example docker-compose.yml  # fill in your values
docker compose up -d
```

**2. Build and run the Discord bot**

```sh
cd discord
cp config.json.example config.json  # fill in your bot token etc.
cmake --preset default
cmake --build --preset default
./build/debug/scanlation-manager
```

See [`discord/README.md`](discord/README.md) for full details.

---

## License

See [LICENSE](LICENSE).
