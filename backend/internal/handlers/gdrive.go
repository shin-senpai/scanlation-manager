package handlers

import (
	"encoding/json"
	"fmt"
	"net/http"

	"scanlation-manager/backend/internal/services/gdrive"
)

type driveHandler struct {
	svc *gdrive.Service
}

// GET /drive/folders/{folderID}
func (h *driveHandler) list(w http.ResponseWriter, r *http.Request) {
	folderID := r.PathValue("folderID")
	if folderID == "" {
		http.Error(w, "missing folderID", http.StatusBadRequest)
		return
	}

	files, err := h.svc.List(r.Context(), folderID)
	if err != nil {
		http.Error(w, "failed to list folder: "+err.Error(), http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(files)
}

// GET /drive/files/{fileID}
func (h *driveHandler) download(w http.ResponseWriter, r *http.Request) {
	fileID := r.PathValue("fileID")
	if fileID == "" {
		http.Error(w, "missing fileID", http.StatusBadRequest)
		return
	}

	meta, err := h.svc.GetFile(r.Context(), fileID)
	if err != nil {
		http.Error(w, "failed to get file metadata: "+err.Error(), http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Disposition", fmt.Sprintf(`attachment; filename="%s"`, meta.Name))
	w.Header().Set("Content-Type", meta.MimeType)

	if err := h.svc.Download(r.Context(), fileID, w); err != nil {
		http.Error(w, "failed to download file: "+err.Error(), http.StatusInternalServerError)
		return
	}
}

// POST /drive/folders/{folderID}
// Query params: name, mimeType
func (h *driveHandler) upload(w http.ResponseWriter, r *http.Request) {
	folderID := r.PathValue("folderID")
	if folderID == "" {
		http.Error(w, "missing folderID", http.StatusBadRequest)
		return
	}

	name := r.URL.Query().Get("name")
	if name == "" {
		http.Error(w, "missing query param: name", http.StatusBadRequest)
		return
	}

	mimeType := r.URL.Query().Get("mimeType")
	if mimeType == "" {
		mimeType = "application/octet-stream"
	}

	defer r.Body.Close()

	fileID, err := h.svc.Upload(r.Context(), folderID, name, mimeType, r.Body)
	if err != nil {
		http.Error(w, "failed to upload file: "+err.Error(), http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusCreated)
	json.NewEncoder(w).Encode(map[string]string{"id": fileID})
}

// DELETE /drive/files/{fileID}
func (h *driveHandler) deleteFile(w http.ResponseWriter, r *http.Request) {
	fileID := r.PathValue("fileID")
	if fileID == "" {
		http.Error(w, "missing fileID", http.StatusBadRequest)
		return
	}

	if err := h.svc.Delete(r.Context(), fileID); err != nil {
		http.Error(w, "failed to delete file: "+err.Error(), http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusNoContent)
}
