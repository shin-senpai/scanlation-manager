package s3

import (
	"context"
	"fmt"
	"io"

	"github.com/aws/aws-sdk-go-v2/aws"
	"github.com/aws/aws-sdk-go-v2/config"
	"github.com/aws/aws-sdk-go-v2/credentials"
	"github.com/aws/aws-sdk-go-v2/service/s3"
)

// Object is a simplified representation of an S3 object.
type Object struct {
	Key  string
	Size int64
}

// Service handles communication with an S3-compatible storage backend.
type Service struct {
	client *s3.Client
	bucket string
}

// New creates a Service pointed at any S3-compatible endpoint.
// For AWS S3, set endpoint to "".
// For Cloudflare R2, set endpoint to "https://<account_id>.r2.cloudflarestorage.com" and region to "auto".
func New(ctx context.Context, endpoint, region, accessKeyID, secretAccessKey, bucket string) (*Service, error) {
	opts := []func(*config.LoadOptions) error{
		config.WithRegion(region),
		config.WithCredentialsProvider(
			credentials.NewStaticCredentialsProvider(accessKeyID, secretAccessKey, ""),
		),
	}

	cfg, err := config.LoadDefaultConfig(ctx, opts...)
	if err != nil {
		return nil, fmt.Errorf("load s3 config: %w", err)
	}

	clientOpts := []func(*s3.Options){}
	if endpoint != "" {
		clientOpts = append(clientOpts, func(o *s3.Options) {
			o.BaseEndpoint = aws.String(endpoint)
		})
	}

	return &Service{
		client: s3.NewFromConfig(cfg, clientOpts...),
		bucket: bucket,
	}, nil
}

// List returns all objects under the given prefix (use "" for the entire bucket).
func (s *Service) List(ctx context.Context, prefix string) ([]Object, error) {
	input := &s3.ListObjectsV2Input{
		Bucket: aws.String(s.bucket),
	}
	if prefix != "" {
		input.Prefix = aws.String(prefix)
	}

	resp, err := s.client.ListObjectsV2(ctx, input)
	if err != nil {
		return nil, fmt.Errorf("list objects (prefix=%q): %w", prefix, err)
	}

	objects := make([]Object, 0, len(resp.Contents))
	for _, obj := range resp.Contents {
		objects = append(objects, Object{
			Key:  aws.ToString(obj.Key),
			Size: aws.ToInt64(obj.Size),
		})
	}

	return objects, nil
}

// Download fetches an object by key and writes it to the provided writer.
func (s *Service) Download(ctx context.Context, key string, w io.Writer) error {
	resp, err := s.client.GetObject(ctx, &s3.GetObjectInput{
		Bucket: aws.String(s.bucket),
		Key:    aws.String(key),
	})
	if err != nil {
		return fmt.Errorf("get object %q: %w", key, err)
	}
	defer resp.Body.Close()

	if _, err := io.Copy(w, resp.Body); err != nil {
		return fmt.Errorf("read object %q: %w", key, err)
	}

	return nil
}

// Upload stores content from a reader under the given key.
func (s *Service) Upload(ctx context.Context, key string, mimeType string, r io.Reader) error {
	_, err := s.client.PutObject(ctx, &s3.PutObjectInput{
		Bucket:      aws.String(s.bucket),
		Key:         aws.String(key),
		Body:        r,
		ContentType: aws.String(mimeType),
	})
	if err != nil {
		return fmt.Errorf("put object %q: %w", key, err)
	}

	return nil
}

// Delete permanently removes an object by key.
func (s *Service) Delete(ctx context.Context, key string) error {
	_, err := s.client.DeleteObject(ctx, &s3.DeleteObjectInput{
		Bucket: aws.String(s.bucket),
		Key:    aws.String(key),
	})
	if err != nil {
		return fmt.Errorf("delete object %q: %w", key, err)
	}

	return nil
}
