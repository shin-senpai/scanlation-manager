package handlers

import (
	"net/http"

	"scanlation-manager/backend/internal/services/gdrive"
	"scanlation-manager/backend/internal/services/s3"
)

func RegisterRoutes(mux *http.ServeMux, gdriveSvc *gdrive.Service, s3Svc *s3.Service) {
	if gdriveSvc != nil {
		drive := &driveHandler{svc: gdriveSvc}
		mux.HandleFunc("GET /drive/folders/{folderID}", drive.list)
		mux.HandleFunc("GET /drive/files/{fileID}", drive.download)
		mux.HandleFunc("POST /drive/folders/{folderID}", drive.upload)
		mux.HandleFunc("DELETE /drive/files/{fileID}", drive.deleteFile)
	}

	if s3Svc != nil {
		s3h := &s3Handler{svc: s3Svc}
		mux.HandleFunc("GET /s3/objects", s3h.list)
		mux.HandleFunc("GET /s3/objects/{key...}", s3h.download)
		mux.HandleFunc("PUT /s3/objects/{key...}", s3h.upload)
		mux.HandleFunc("DELETE /s3/objects/{key...}", s3h.deleteObject)
	}
}
