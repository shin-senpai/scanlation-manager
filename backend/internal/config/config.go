package config

import (
	"encoding/json"
	"fmt"
	"os"
)

type Config struct {
	Port              int    `json:"port"`
	GDriveCredentials string `json:"gdrive_credentials_file"` // path to service account JSON

	S3Endpoint        string `json:"s3_endpoint"`         // leave empty for AWS; set for R2, B2, MinIO, etc.
	S3Region          string `json:"s3_region"`           // "auto" for R2, standard AWS region otherwise
	S3AccessKeyID     string `json:"s3_access_key_id"`
	S3SecretAccessKey string `json:"s3_secret_access_key"`
	S3Bucket          string `json:"s3_bucket"`

	// Google Sheets integration (optional)
	GSheetSpreadsheetID string `json:"gsheet_spreadsheet_id"` // ID of the target spreadsheet
	APIToken            string `json:"api_token"`             // PSK checked on all /sheets/* requests from the bot

	// PostgreSQL connection (used by Google Sheets sync to read current state)
	DBConnectionString string `json:"db_connection_string"` // libpq-style connection string
}

func Load() (*Config, error) {
	path := "config.json"
	if p := os.Getenv("CONFIG_PATH"); p != "" {
		path = p
	}

	f, err := os.Open(path)
	if err != nil {
		return nil, fmt.Errorf("open config: %w", err)
	}
	defer f.Close()

	var cfg Config
	if err := json.NewDecoder(f).Decode(&cfg); err != nil {
		return nil, fmt.Errorf("decode config: %w", err)
	}

	if cfg.Port == 0 {
		cfg.Port = 8080
	}

	return &cfg, nil
}
