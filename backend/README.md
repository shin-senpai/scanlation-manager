# scanlation-manager — Backend

> Part of the `scanlation-manager` monorepo. This module lives under `backend/`.

A Go REST API providing external service integrations and (eventually) the full scanlation data API consumed by the frontend.

---

## Structure

```
backend/
├── cmd/
│   └── api/
│       └── main.go          # Entry point — initializes services, starts server
├── internal/
│   ├── config/
│   │   └── config.go        # Loads config.json
│   ├── services/
│   │   ├── gdrive/
│   │   │   └── gdrive.go    # Google Drive client (list, download, upload, delete)
│   │   └── s3/
│   │       └── s3.go        # S3-compatible client (list, download, upload, delete)
│   └── handlers/
│       ├── routes.go        # Route registration
│       ├── gdrive.go        # Google Drive HTTP handlers
│       └── s3.go            # S3 HTTP handlers
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
| `port` | ❌ | Port to listen on (default: `8080`) |
| `gdrive_credentials_file` | ❌ | Path to a Google service account JSON key file |
| `s3_endpoint` | ❌ | S3-compatible endpoint URL (omit for AWS S3; for R2: `https://<account_id>.r2.cloudflarestorage.com`) |
| `s3_region` | ❌ | Region (`auto` for R2, standard region for AWS) |
| `s3_access_key_id` | ❌ | S3 access key ID |
| `s3_secret_access_key` | ❌ | S3 secret access key |
| `s3_bucket` | ❌ | S3 bucket name |

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

Run the binary from `backend/` so it can find `config.json` in the working directory, or set `CONFIG_PATH` explicitly.

---

## API

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

- [`google.golang.org/api`](https://pkg.go.dev/google.golang.org/api) — Google Drive API client
- [`github.com/aws/aws-sdk-go-v2`](https://pkg.go.dev/github.com/aws/aws-sdk-go-v2) — AWS SDK for S3-compatible storage
