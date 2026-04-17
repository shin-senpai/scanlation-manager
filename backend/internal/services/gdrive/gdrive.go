package gdrive

import (
	"context"
	"fmt"
	"io"
	"net/http"

	"google.golang.org/api/drive/v3"
	"google.golang.org/api/option"
)

// File is a simplified representation of a Google Drive file/folder.
type File struct {
	ID       string
	Name     string
	MimeType string
	Size     int64
}

// Service handles communication with the Google Drive API.
type Service struct {
	client *drive.Service
}

// NewWithServiceAccount creates a Service authenticated via a service account credentials file.
func NewWithServiceAccount(ctx context.Context, credentialsFile string) (*Service, error) {
	client, err := drive.NewService(ctx,
		option.WithAuthCredentialsFile(option.ServiceAccount, credentialsFile),
		option.WithScopes(drive.DriveScope),
	)
	if err != nil {
		return nil, fmt.Errorf("create drive client: %w", err)
	}

	return &Service{client: client}, nil
}

// List returns the files and folders inside a given folder.
func (s *Service) List(ctx context.Context, folderID string) ([]File, error) {
	query := fmt.Sprintf("'%s' in parents and trashed = false", folderID)

	resp, err := s.client.Files.List().
		Context(ctx).
		Q(query).
		Fields("files(id, name, mimeType, size)").
		SupportsAllDrives(true).
		IncludeItemsFromAllDrives(true).
		Do()
	if err != nil {
		return nil, fmt.Errorf("list files in folder %s: %w", folderID, err)
	}

	files := make([]File, 0, len(resp.Files))
	for _, f := range resp.Files {
		files = append(files, File{
			ID:       f.Id,
			Name:     f.Name,
			MimeType: f.MimeType,
			Size:     f.Size,
		})
	}

	return files, nil
}

// GetFile returns the metadata for a single file by ID.
func (s *Service) GetFile(ctx context.Context, fileID string) (*File, error) {
	f, err := s.client.Files.Get(fileID).
		Context(ctx).
		Fields("id, name, mimeType, size").
		SupportsAllDrives(true).
		Do()
	if err != nil {
		return nil, fmt.Errorf("get file metadata %s: %w", fileID, err)
	}

	return &File{
		ID:       f.Id,
		Name:     f.Name,
		MimeType: f.MimeType,
		Size:     f.Size,
	}, nil
}

// Download fetches the content of a file by ID and writes it to the provided writer.
func (s *Service) Download(ctx context.Context, fileID string, w io.Writer) error {
	resp, err := s.client.Files.Get(fileID).
		Context(ctx).
		SupportsAllDrives(true).
		Download()
	if err != nil {
		return fmt.Errorf("download file %s: %w", fileID, err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return fmt.Errorf("download file %s: unexpected status %d", fileID, resp.StatusCode)
	}

	if _, err := io.Copy(w, resp.Body); err != nil {
		return fmt.Errorf("read file %s: %w", fileID, err)
	}

	return nil
}

// Delete permanently deletes a file by ID.
func (s *Service) Delete(ctx context.Context, fileID string) error {
	err := s.client.Files.Delete(fileID).
		Context(ctx).
		SupportsAllDrives(true).
		Do()
	if err != nil {
		return fmt.Errorf("delete file %s: %w", fileID, err)
	}

	return nil
}

// Upload uploads content from a reader to the given folder with the given filename and MIME type.
// Returns the ID of the newly created file.
func (s *Service) Upload(ctx context.Context, folderID string, name string, mimeType string, r io.Reader) (string, error) {
	metadata := &drive.File{
		Name:    name,
		Parents: []string{folderID},
	}

	resp, err := s.client.Files.Create(metadata).
		Context(ctx).
		Media(r).
		SupportsAllDrives(true).
		Fields("id").
		Do()
	if err != nil {
		return "", fmt.Errorf("upload file %s: %w", name, err)
	}

	return resp.Id, nil
}
