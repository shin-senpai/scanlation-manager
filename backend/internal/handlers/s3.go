package handlers

import (
	"encoding/json"
	"net/http"

	"scanlation-manager/backend/internal/services/s3"
)

type s3Handler struct {
	svc *s3.Service
}

// GET /s3/objects?prefix=some/path
func (h *s3Handler) list(w http.ResponseWriter, r *http.Request) {
	prefix := r.URL.Query().Get("prefix")

	objects, err := h.svc.List(r.Context(), prefix)
	if err != nil {
		http.Error(w, "failed to list objects: "+err.Error(), http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(objects)
}

// GET /s3/objects/{key}
func (h *s3Handler) download(w http.ResponseWriter, r *http.Request) {
	key := r.PathValue("key")
	if key == "" {
		http.Error(w, "missing key", http.StatusBadRequest)
		return
	}

	w.Header().Set("Content-Disposition", "attachment")
	w.Header().Set("Content-Type", "application/octet-stream")

	if err := h.svc.Download(r.Context(), key, w); err != nil {
		http.Error(w, "failed to download object: "+err.Error(), http.StatusInternalServerError)
		return
	}
}

// PUT /s3/objects/{key}
// Query params: mimeType
func (h *s3Handler) upload(w http.ResponseWriter, r *http.Request) {
	key := r.PathValue("key")
	if key == "" {
		http.Error(w, "missing key", http.StatusBadRequest)
		return
	}

	mimeType := r.URL.Query().Get("mimeType")
	if mimeType == "" {
		mimeType = "application/octet-stream"
	}

	defer r.Body.Close()

	if err := h.svc.Upload(r.Context(), key, mimeType, r.Body); err != nil {
		http.Error(w, "failed to upload object: "+err.Error(), http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusCreated)
}

// DELETE /s3/objects/{key}
func (h *s3Handler) deleteObject(w http.ResponseWriter, r *http.Request) {
	key := r.PathValue("key")
	if key == "" {
		http.Error(w, "missing key", http.StatusBadRequest)
		return
	}

	if err := h.svc.Delete(r.Context(), key); err != nil {
		http.Error(w, "failed to delete object: "+err.Error(), http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusNoContent)
}
