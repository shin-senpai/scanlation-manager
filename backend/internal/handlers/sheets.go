package handlers

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"strings"

	"scanlation-manager/backend/internal/services/gsheets"
	"scanlation-manager/backend/internal/services/sheetdb"
)

type sheetsHandler struct {
	sheets *gsheets.Client
	db     *sheetdb.Client
	token  string
}

// authenticate checks the Authorization: Bearer header against the configured token.
func (h *sheetsHandler) authenticate(r *http.Request) bool {
	auth := r.Header.Get("Authorization")
	const prefix = "Bearer "
	if !strings.HasPrefix(auth, prefix) {
		return false
	}
	return strings.TrimPrefix(auth, prefix) == h.token
}

func (h *sheetsHandler) health(w http.ResponseWriter, r *http.Request) {
	if !h.authenticate(r) {
		http.Error(w, "unauthorized", http.StatusUnauthorized)
		return
	}
	if !h.sheets.IsHealthy(r.Context()) {
		http.Error(w, "google sheets not reachable", http.StatusServiceUnavailable)
		return
	}
	w.WriteHeader(http.StatusOK)
}

func (h *sheetsHandler) syncTodo(w http.ResponseWriter, r *http.Request) {
	if !h.authenticate(r) {
		http.Error(w, "unauthorized", http.StatusUnauthorized)
		return
	}

	rows, err := h.db.GetTodoRows(r.Context())
	if err != nil {
		log.Printf("sheets/sync/todo: db error: %v", err)
		http.Error(w, "db error", http.StatusInternalServerError)
		return
	}

	values := buildTodoGrid(rows)
	if err := h.sheets.ClearAndWrite(r.Context(), "Todo", values); err != nil {
		log.Printf("sheets/sync/todo: sheets error: %v", err)
		http.Error(w, "sheets error", http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
}

func (h *sheetsHandler) syncSeries(w http.ResponseWriter, r *http.Request) {
	if !h.authenticate(r) {
		http.Error(w, "unauthorized", http.StatusUnauthorized)
		return
	}

	var body struct {
		Name string `json:"name"`
	}
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil || body.Name == "" {
		http.Error(w, "bad request: missing name", http.StatusBadRequest)
		return
	}

	data, err := h.db.GetSeriesSheetData(r.Context(), body.Name)
	if err != nil {
		log.Printf("sheets/sync/series %q: db error: %v", body.Name, err)
		http.Error(w, "db error", http.StatusInternalServerError)
		return
	}
	if data == nil {
		http.Error(w, fmt.Sprintf("series %q not found", body.Name), http.StatusNotFound)
		return
	}

	values := buildSeriesGrid(data)
	if err := h.sheets.ClearAndWrite(r.Context(), body.Name, values); err != nil {
		log.Printf("sheets/sync/series %q: sheets error: %v", body.Name, err)
		http.Error(w, "sheets error", http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
}

func (h *sheetsHandler) deleteSeries(w http.ResponseWriter, r *http.Request) {
	if !h.authenticate(r) {
		http.Error(w, "unauthorized", http.StatusUnauthorized)
		return
	}

	var body struct {
		Name string `json:"name"`
	}
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil || body.Name == "" {
		http.Error(w, "bad request: missing name", http.StatusBadRequest)
		return
	}

	if err := h.sheets.DeleteSheet(context.Background(), body.Name); err != nil {
		log.Printf("sheets/delete-series %q: sheets error: %v", body.Name, err)
		http.Error(w, "sheets error", http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
}

// buildTodoGrid builds the values grid for the Todo sheet.
func buildTodoGrid(rows []sheetdb.TodoRow) [][]interface{} {
	values := make([][]interface{}, 0, len(rows)+1)
	values = append(values, []interface{}{"Series", "Chapter", "Task", "Assigned To", "Days Active"})
	for _, r := range rows {
		values = append(values, []interface{}{r.Series, r.Chapter, r.Task, r.AssignedTo, r.DaysActive})
	}
	return values
}

// buildSeriesGrid builds the values grid for a series sheet.
func buildSeriesGrid(data *sheetdb.SeriesSheetData) [][]interface{} {
	var values [][]interface{}

	// Series status header
	values = append(values, []interface{}{"Status", data.Status})
	values = append(values, []interface{}{})

	// Section 1: crew list
	values = append(values, []interface{}{"User", "Task"})
	for _, c := range data.Crew {
		values = append(values, []interface{}{c.UserName, c.TaskName})
	}

	// Blank separator row
	values = append(values, []interface{}{})

	// Section 2: chapter table header
	header := make([]interface{}, 3, 3+len(data.Tasks))
	header[0] = "Chapter"
	header[1] = "Status"
	header[2] = "Closed At"
	for _, t := range data.Tasks {
		header = append(header, t.Name)
	}
	values = append(values, header)

	// Chapter rows
	for _, ch := range data.Chapters {
		closedAt := "N/A"
		if ch.ClosedAt != nil {
			closedAt = *ch.ClosedAt
		}
		row := make([]interface{}, 3, 3+len(data.Tasks))
		row[0] = ch.Label
		row[1] = ch.Status
		row[2] = closedAt
		taskAssignments := data.Assignments[ch.ID]
		for _, t := range data.Tasks {
			entries, ok := taskAssignments[t.ID]
			if !ok || len(entries) == 0 {
				row = append(row, "N/A")
				continue
			}
			parts := make([]string, 0, len(entries))
			for _, e := range entries {
				if e.Completed {
					parts = append(parts, "✓ "+e.DisplayName)
				} else {
					parts = append(parts, e.DisplayName)
				}
			}
			row = append(row, strings.Join(parts, ", "))
		}
		values = append(values, row)
	}

	return values
}
