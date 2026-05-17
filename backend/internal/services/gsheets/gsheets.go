package gsheets

import (
	"context"
	"fmt"

	"google.golang.org/api/option"
	"google.golang.org/api/sheets/v4"
)

// Client wraps the Google Sheets API for the scanlation manager.
type Client struct {
	srv           *sheets.Service
	spreadsheetID string
}

// New creates a Client authenticated via a service account credentials file.
// The service account must have Editor access to the target spreadsheet.
func New(ctx context.Context, credentialsFile, spreadsheetID string) (*Client, error) {
	srv, err := sheets.NewService(ctx,
		option.WithAuthCredentialsFile(option.ServiceAccount, credentialsFile),
		option.WithScopes(sheets.SpreadsheetsScope),
	)
	if err != nil {
		return nil, fmt.Errorf("create sheets client: %w", err)
	}
	return &Client{srv: srv, spreadsheetID: spreadsheetID}, nil
}

// IsHealthy returns true if the spreadsheet is reachable and accessible.
func (c *Client) IsHealthy(ctx context.Context) bool {
	_, err := c.srv.Spreadsheets.Get(c.spreadsheetID).Context(ctx).Fields("spreadsheetId").Do()
	return err == nil
}

// getSheetID returns the numeric sheet ID for a tab by name, or -1 if not found.
func (c *Client) getSheetID(ctx context.Context, name string) (int64, error) {
	sp, err := c.srv.Spreadsheets.Get(c.spreadsheetID).Context(ctx).Fields("sheets(properties(sheetId,title))").Do()
	if err != nil {
		return -1, fmt.Errorf("get spreadsheet metadata: %w", err)
	}
	for _, sh := range sp.Sheets {
		if sh.Properties.Title == name {
			return sh.Properties.SheetId, nil
		}
	}
	return -1, nil
}

// getOrCreateSheetID returns the sheet ID for a tab, creating it if absent.
func (c *Client) getOrCreateSheetID(ctx context.Context, name string) (int64, error) {
	id, err := c.getSheetID(ctx, name)
	if err != nil {
		return -1, err
	}
	if id >= 0 {
		return id, nil
	}

	resp, err := c.srv.Spreadsheets.BatchUpdate(c.spreadsheetID, &sheets.BatchUpdateSpreadsheetRequest{
		Requests: []*sheets.Request{
			{AddSheet: &sheets.AddSheetRequest{Properties: &sheets.SheetProperties{Title: name}}},
		},
	}).Context(ctx).Do()
	if err != nil {
		return -1, fmt.Errorf("add sheet %q: %w", name, err)
	}
	return resp.Replies[0].AddSheet.Properties.SheetId, nil
}

// ClearAndWrite clears the named sheet tab and writes values starting at A1.
// Creates the tab if it does not exist.
func (c *Client) ClearAndWrite(ctx context.Context, name string, values [][]interface{}) error {
	if _, err := c.getOrCreateSheetID(ctx, name); err != nil {
		return err
	}

	sheetRange := fmt.Sprintf("'%s'", name)

	if _, err := c.srv.Spreadsheets.Values.Clear(c.spreadsheetID, sheetRange, &sheets.ClearValuesRequest{}).Context(ctx).Do(); err != nil {
		return fmt.Errorf("clear sheet %q: %w", name, err)
	}

	if len(values) == 0 {
		return nil
	}

	rows := make([]*sheets.RowData, 0, len(values))
	_ = rows // values.Update path is simpler; use the values API directly

	vr := &sheets.ValueRange{Values: toInterfaceSlices(values)}
	_, err := c.srv.Spreadsheets.Values.Update(c.spreadsheetID, sheetRange, vr).
		ValueInputOption("RAW").
		Context(ctx).Do()
	if err != nil {
		return fmt.Errorf("write sheet %q: %w", name, err)
	}
	return nil
}

// DeleteSheet removes the named tab from the spreadsheet.
// If the tab does not exist this is a no-op.
func (c *Client) DeleteSheet(ctx context.Context, name string) error {
	id, err := c.getSheetID(ctx, name)
	if err != nil {
		return err
	}
	if id < 0 {
		return nil // already gone
	}

	_, err = c.srv.Spreadsheets.BatchUpdate(c.spreadsheetID, &sheets.BatchUpdateSpreadsheetRequest{
		Requests: []*sheets.Request{
			{DeleteSheet: &sheets.DeleteSheetRequest{SheetId: id}},
		},
	}).Context(ctx).Do()
	if err != nil {
		return fmt.Errorf("delete sheet %q: %w", name, err)
	}
	return nil
}

// toInterfaceSlices converts [][]interface{} to the nested slice the Sheets API expects.
func toInterfaceSlices(in [][]interface{}) [][]interface{} {
	out := make([][]interface{}, len(in))
	for i, row := range in {
		out[i] = make([]interface{}, len(row))
		copy(out[i], row)
	}
	return out
}
