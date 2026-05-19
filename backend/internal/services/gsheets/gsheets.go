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
		ValueInputOption("USER_ENTERED").
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

// SeriesSheetLayout describes the number of rows in each section of a series sheet,
// which is needed to compute cell ranges for formatting.
type SeriesSheetLayout struct {
	CrewCount    int
	TaskCount    int
	ChapterCount int
}

// FormatSeriesSheet applies visual formatting to a series sheet in one batchUpdate call:
// bold dark-blue headers on the Status row, Crew header, and Chapter table header;
// a frozen first column; colour-coded chapter Status cells; and green/grey backgrounds
// on completed/unassigned task cells.
func (c *Client) FormatSeriesSheet(ctx context.Context, name string, l SeriesSheetLayout) error {
	sheetID, err := c.getSheetID(ctx, name)
	if err != nil {
		return err
	}
	if sheetID < 0 {
		return fmt.Errorf("sheet %q not found", name)
	}

	// Row indices (0-based):
	//   0          : Status header
	//   1          : blank
	//   2          : Crew section header ("User", "Task")
	//   3…2+C      : crew data rows
	//   3+C        : blank
	//   4+C        : Chapter table header
	//   5+C…4+C+Ch : chapter data rows
	C := int64(l.CrewCount)
	T := int64(l.TaskCount)
	Ch := int64(l.ChapterCount)
	chapterHeaderRow := 4 + C
	chapterDataStart := 5 + C
	chapterDataEnd := chapterDataStart + Ch // exclusive
	totalCols := 3 + T

	headerFmt := headerCellFormat()
	headerFields := "userEnteredFormat(backgroundColor,textFormat)"

	var reqs []*sheets.Request

	// (a) Status row header (row 0, cols 0–1)
	reqs = append(reqs, repeatCell(sheetID, 0, 1, 0, 2, headerFmt, headerFields))

	// (b) Crew section header (row 2, cols 0–1)
	reqs = append(reqs, repeatCell(sheetID, 2, 3, 0, 2, headerFmt, headerFields))

	// (c) Chapter table header row
	reqs = append(reqs, repeatCell(sheetID, chapterHeaderRow, chapterHeaderRow+1, 0, totalCols, headerFmt, headerFields))

	// (e) & (f) conditional formatting for chapter data rows (skip if no chapters)
	if Ch > 0 {
		statusCol := int64(1)
		// (e) Status column colour by value
		statusColors := []struct {
			value string
			color *sheets.Color
		}{
			{"Released", rgb(200, 230, 201)},
			{"In Progress", rgb(255, 249, 196)},
			{"Hiatus", rgb(255, 224, 178)},
			{"Dropped", rgb(255, 205, 210)},
		}
		for _, sc := range statusColors {
			reqs = append(reqs, condFmtTextEq(sheetID, chapterDataStart, chapterDataEnd, statusCol, statusCol+1, sc.value, sc.color))
		}

		// (f) Task columns: ✓ cells green, N/A cells grey
		if T > 0 {
			reqs = append(reqs,
				condFmtTextStartsWith(sheetID, chapterDataStart, chapterDataEnd, 3, totalCols, "✓", rgb(220, 237, 200)),
				condFmtTextEq(sheetID, chapterDataStart, chapterDataEnd, 3, totalCols, "N/A", rgb(238, 238, 238)),
			)
		}
	}

	_, err = c.srv.Spreadsheets.BatchUpdate(c.spreadsheetID, &sheets.BatchUpdateSpreadsheetRequest{
		Requests: reqs,
	}).Context(ctx).Do()
	if err != nil {
		return fmt.Errorf("format series sheet %q: %w", name, err)
	}
	return nil
}

// headerCellFormat returns the CellFormat used for all header rows.
func headerCellFormat() *sheets.CellFormat {
	return &sheets.CellFormat{
		BackgroundColor: rgb(21, 101, 192), // #1565C0
		TextFormat: &sheets.TextFormat{
			Bold:            true,
			ForegroundColor: rgb(255, 255, 255),
		},
	}
}

// rgb constructs a *sheets.Color from 0–255 component values.
func rgb(r, g, b uint8) *sheets.Color {
	return &sheets.Color{
		Red:   float64(r) / 255,
		Green: float64(g) / 255,
		Blue:  float64(b) / 255,
	}
}

// repeatCell returns a RepeatCell request that stamps the given CellFormat onto a range.
func repeatCell(sheetID, r0, r1, c0, c1 int64, fmt *sheets.CellFormat, fields string) *sheets.Request {
	return &sheets.Request{
		RepeatCell: &sheets.RepeatCellRequest{
			Range: &sheets.GridRange{
				SheetId:          sheetID,
				StartRowIndex:    r0,
				EndRowIndex:      r1,
				StartColumnIndex: c0,
				EndColumnIndex:   c1,
			},
			Cell:   &sheets.CellData{UserEnteredFormat: fmt},
			Fields: fields,
		},
	}
}

// condFmtTextEq returns an AddConditionalFormatRule request that colours cells whose
// text exactly equals value.
func condFmtTextEq(sheetID, r0, r1, c0, c1 int64, value string, bg *sheets.Color) *sheets.Request {
	return &sheets.Request{
		AddConditionalFormatRule: &sheets.AddConditionalFormatRuleRequest{
			Rule: &sheets.ConditionalFormatRule{
				Ranges: []*sheets.GridRange{{
					SheetId:          sheetID,
					StartRowIndex:    r0,
					EndRowIndex:      r1,
					StartColumnIndex: c0,
					EndColumnIndex:   c1,
				}},
				BooleanRule: &sheets.BooleanRule{
					Condition: &sheets.BooleanCondition{
						Type:   "TEXT_EQ",
						Values: []*sheets.ConditionValue{{UserEnteredValue: value}},
					},
					Format: &sheets.CellFormat{BackgroundColor: bg},
				},
			},
		},
	}
}

// condFmtTextStartsWith returns an AddConditionalFormatRule request that colours cells
// whose text starts with the given prefix.
func condFmtTextStartsWith(sheetID, r0, r1, c0, c1 int64, prefix string, bg *sheets.Color) *sheets.Request {
	return &sheets.Request{
		AddConditionalFormatRule: &sheets.AddConditionalFormatRuleRequest{
			Rule: &sheets.ConditionalFormatRule{
				Ranges: []*sheets.GridRange{{
					SheetId:          sheetID,
					StartRowIndex:    r0,
					EndRowIndex:      r1,
					StartColumnIndex: c0,
					EndColumnIndex:   c1,
				}},
				BooleanRule: &sheets.BooleanRule{
					Condition: &sheets.BooleanCondition{
						Type:   "TEXT_STARTS_WITH",
						Values: []*sheets.ConditionValue{{UserEnteredValue: prefix}},
					},
					Format: &sheets.CellFormat{BackgroundColor: bg},
				},
			},
		},
	}
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
