package main

import (
	"context"
	"fmt"
	"log"
	"net/http"

	"scanlation-manager/backend/internal/config"
	"scanlation-manager/backend/internal/handlers"
	"scanlation-manager/backend/internal/services/gdrive"
	"scanlation-manager/backend/internal/services/gsheets"
	"scanlation-manager/backend/internal/services/s3"
	"scanlation-manager/backend/internal/services/sheetdb"
)

func main() {
	cfg, err := config.Load()
	if err != nil {
		log.Fatalf("load config: %v", err)
	}

	ctx := context.Background()

	gdriveSvc, err := gdrive.NewWithServiceAccount(ctx, cfg.GDriveCredentials)
	if err != nil {
		log.Printf("init google drive: %v", err)
	}

	s3Svc, err := s3.New(ctx, cfg.S3Endpoint, cfg.S3Region, cfg.S3AccessKeyID, cfg.S3SecretAccessKey, cfg.S3Bucket)
	if err != nil {
		log.Printf("init s3: %v", err)
	}

	var sheetsSvc *gsheets.Client
	var dbSvc *sheetdb.Client
	if cfg.GSheetSpreadsheetID != "" && cfg.GDriveCredentials != "" {
		sheetsSvc, err = gsheets.New(ctx, cfg.GDriveCredentials, cfg.GSheetSpreadsheetID)
		if err != nil {
			log.Printf("init google sheets: %v", err)
		}
	}
	if cfg.DBConnectionString != "" && sheetsSvc != nil {
		dbSvc, err = sheetdb.New(ctx, cfg.DBConnectionString)
		if err != nil {
			log.Printf("init sheetdb: %v", err)
		}
	}
	if sheetsSvc != nil && dbSvc != nil {
		log.Printf("google sheets integration enabled (spreadsheet: %s)", cfg.GSheetSpreadsheetID)
	}

	mux := http.NewServeMux()
	handlers.RegisterRoutes(mux, gdriveSvc, s3Svc, sheetsSvc, dbSvc, cfg.APIToken)

	addr := fmt.Sprintf(":%d", cfg.Port)
	log.Printf("starting server on %s", addr)
	if err := http.ListenAndServe(addr, mux); err != nil {
		log.Fatalf("server error: %v", err)
	}
}
