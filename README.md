# scanlation-manager

A management platform for scanlation groups — tracking series, chapters, tasks, and contributors. The project is built as a monorepo with a shared PostgreSQL database, currently with an active Discord bot and a planned web app that will both operate against the same data.

> **Status: In Progress.** The Discord bot is the primary focus right now. The web app (backend + frontend) is planned but not yet implemented.

---

## Overview

Scanlation groups juggle a lot: multiple series, multiple contributors each filling different roles, custom workflows per project, and the need to track who did what and when. This tool aims to centralize all of that.

The core idea is that both the Discord bot and the future web app share the same database — so a group can use whichever interface fits their workflow, or both at once.

---

## Planned Features

- **User permissions** — Basic flags for supermanager and manager roles
- **Custom roles** — Define what capabilities a user has within the group
- **Custom tasks** — Define the steps that must be completed before a chapter can be released
- **Task dependencies** — Specify which tasks block other tasks
- **Series & chapter tracking** — Track which series are active and who is assigned to what at the chapter level
- **Contribution history** — Record who completed which task, for which chapter, under which alias, and when
- **To-do lists** — See which tasks are outstanding and who is responsible for them
- **Release summaries** — Query releases with sorting and filtering

---

## Monorepo Structure

```
scanlation-manager/
├── discord/       # Discord bot (C++/D++) — actively developed
├── db/            # Database migrations and Docker Compose setup
├── backend/       # REST API / server — planned
└── frontend/      # Web UI — planned
```

### discord/

The Discord bot is the first interface into the system. Built with C++20 and [D++](https://dpp.dev/) (a Discord API library), backed by PostgreSQL via `libpqxx`.

See [`discord/README.md`](discord/README.md) for build instructions, configuration, and command documentation.

### db/

PostgreSQL 16 database, run via Docker Compose. Contains migration scripts that define the shared schema used by all services.

### backend/ & frontend/

Not yet implemented. The backend will expose the same data as the Discord bot through a REST (or similar) API; the frontend will be a web UI consuming that API.

---

## Tech Stack

| Layer | Technology |
|---|---|
| Discord bot | C++20, [D++](https://dpp.dev/) v10.1.4 |
| Database | PostgreSQL 16 (Docker) |
| DB client (C++) | libpqxx 8.0.0 |
| Build system | CMake 3.28+ |
| Backend | TBD |
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
cmake -B build && cmake --build build
./build/scanlation-bot
```

See [`discord/README.md`](discord/README.md) for full details.

---

## License

See [LICENSE](LICENSE).
