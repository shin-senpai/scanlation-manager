package handlers

import (
	"net/http"
	"strings"
)

// makeHealthHandler returns an authenticated health handler that simply
// verifies the bearer token and returns 200 OK. It is the generic
// "is the backend up and is my token valid?" check, independent of any
// specific integration.
func makeHealthHandler(token string) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		auth := r.Header.Get("Authorization")
		const prefix = "Bearer "
		if !strings.HasPrefix(auth, prefix) || strings.TrimPrefix(auth, prefix) != token {
			http.Error(w, "unauthorized", http.StatusUnauthorized)
			return
		}
		w.WriteHeader(http.StatusOK)
	}
}
