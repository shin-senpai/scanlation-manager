# scanlation-manager — Backend

> Part of the `scanlation-manager` monorepo. This module lives under `backend/`.

A Go REST API providing external service integrations (Google Drive, S3-compatible storage, Google Sheets) for the scanlation manager Discord bot and (eventually) the full data API consumed by the frontend.

---

## Structure

```
backend/
├── cmd/
│   └── api/
│       └── main.go               # Entry point — initializes services, starts server
├── internal/
│   ├── config/
│   │   └── config.go             # Loads config.json
│   ├── services/
│   │   ├── gdrive/
│   │   │   └── gdrive.go         # Google Drive client (list, download, upload, delete)
│   │   ├── s3/
│   │   │   └── s3.go             # S3-compatible client (list, download, upload, delete)
│   │   ├── gsheets/
│   │   │   └── gsheets.go        # Google Sheets client (ClearAndWrite, DeleteSheet, IsHealthy, GetSheetURLs, FormatSeriesSheet, FormatSeriesListSheet)
│   │   ├── sheetdb/
│   │   │   └── sheetdb.go        # PostgreSQL reader for sheet sync data
│   │   └── mangadex/
│   │       └── mangadex.go       # MangaDex API client — stub, not yet implemented
│   └── handlers/
│       ├── routes.go             # Route registration (conditional on enabled services)
│       ├── api.go                # GET /health — liveness + auth check
│       ├── gdrive.go             # Google Drive HTTP handlers
│       ├── s3.go                 # S3 HTTP handlers
│       └── sheets.go             # Google Sheets sync handlers + grid builders
└── config.json.example
```

---

## Configuration

Copy the example and fill in your values:

```bash
cp config.json.example config.json
```

| Key | Required | Description |
|-----|----------|-------------|
| `port` | No (default `8080`) | Port to listen on |
| `gdrive_credentials_file` | For Drive + Sheets | Path to a Google service account JSON key file |
| `s3_endpoint` | For S3 | S3-compatible endpoint URL — omit for AWS S3; for R2: `https://<account_id>.r2.cloudflarestorage.com` |
| `s3_region` | For S3 | Region (`auto` for R2, standard AWS region string otherwise) |
| `s3_access_key_id` | For S3 | S3 access key ID |
| `s3_secret_access_key` | For S3 | S3 secret access key |
| `s3_bucket` | For S3 | S3 bucket name |
| `gsheet_spreadsheet_id` | For Sheets | Google Spreadsheet ID (from the URL) |
| `api_token` | For Sheets | Shared PSK — must match `api_token` in `discord/config.json` |
| `db_connection_string` | For Sheets | libpq-style connection string for reading DB state, e.g. `host=localhost port=5432 dbname=scanlation_manager user=scanlation_manager password=secret` |

All service keys are optional — the server starts without a service if its credentials are absent, and its routes are simply not registered.

The config path can be overridden with the `CONFIG_PATH` environment variable.

---

## Running

```bash
go run ./cmd/api/
```

Or build a binary:

```bash
go build -o bin/api ./cmd/api/
./bin/api
```

Run from `backend/` so it finds `config.json` in the working directory, or set `CONFIG_PATH` explicitly.

---

## API

All routes require `Authorization: Bearer <api_token>`.

### Health

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/health` | Backend liveness + auth check — 200 if reachable and token is valid |

### Google Sheets Sync

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/sheets/health` | Sheets API connectivity check |
| `POST` | `/sheets/sync/todo` | Rewrites the `"Todo"` sheet tab from DB state |
| `POST` | `/sheets/sync/series` | Body: `{"name":"..."}` — rewrites the named series sheet tab (deletes tab if series is not active) |
| `POST` | `/sheets/sync/series-list` | Rewrites the `"Series"` overview tab (all series, with hyperlinks for active ones) |
| `POST` | `/sheets/delete-series` | Body: `{"name":"..."}` — deletes the named series sheet tab |

### Google Drive

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/drive/folders/{folderID}` | List files in a folder |
| `GET` | `/drive/files/{fileID}` | Download a file |
| `POST` | `/drive/folders/{folderID}` | Upload a file — query params: `name`, `mimeType` |
| `DELETE` | `/drive/files/{fileID}` | Delete a file |

### S3-compatible Storage

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/s3/objects` | List objects — query param: `prefix` (optional) |
| `GET` | `/s3/objects/{key...}` | Download an object |
| `PUT` | `/s3/objects/{key...}` | Upload an object — query param: `mimeType` (optional) |
| `DELETE` | `/s3/objects/{key...}` | Delete an object |

Keys support slashes (e.g. `chapters/vol1/ch1.zip`).

---

## Dependencies

- [`google.golang.org/api`](https://pkg.go.dev/google.golang.org/api) — Google Drive v3 + Sheets v4 API clients
- [`github.com/aws/aws-sdk-go-v2`](https://pkg.go.dev/github.com/aws/aws-sdk-go-v2) — AWS SDK for S3-compatible storage
- [`github.com/jackc/pgx/v5`](https://pkg.go.dev/github.com/jackc/pgx/v5) — PostgreSQL client (pgxpool) for reading DB state during sheet sync
