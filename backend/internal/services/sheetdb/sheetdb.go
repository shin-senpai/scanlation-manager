package sheetdb

import (
	"context"
	"errors"
	"fmt"
	"math"
	"strconv"
	"time"

	"github.com/jackc/pgx/v5"
	"github.com/jackc/pgx/v5/pgxpool"
)

// Client holds a pgx connection pool for reading sheet data.
type Client struct {
	pool *pgxpool.Pool
}

// New creates a Client backed by a pgx connection pool using a libpq-style connection string.
func New(ctx context.Context, connStr string) (*Client, error) {
	pool, err := pgxpool.New(ctx, connStr)
	if err != nil {
		return nil, fmt.Errorf("create pool: %w", err)
	}
	if err := pool.Ping(ctx); err != nil {
		pool.Close()
		return nil, fmt.Errorf("ping db: %w", err)
	}
	return &Client{pool: pool}, nil
}

// Close releases the connection pool.
func (c *Client) Close() {
	c.pool.Close()
}

// TodoRow is one row in the Todo sheet.
type TodoRow struct {
	Series      string
	Chapter     string
	Task        string
	AssignedTo  string
	ActiveSince time.Time // when the task became actionable (assigned_at or latest prereq completion)
}

// GetTodoRows returns all incomplete assignments ordered by series, chapter number, task.
// See migration 027_add_active_since_to_todo_view.sql for the active_since calculation logic.
func (c *Client) GetTodoRows(ctx context.Context) ([]TodoRow, error) {
	const q = `
		SELECT series_name, chapter_number, chapter_name, chapter_volume,
		       task_name, display_name, active_since
		FROM outstanding_chapter_assignments
		ORDER BY series_name, chapter_number, task_level NULLS LAST, task_name, display_name`

	rows, err := c.pool.Query(ctx, q)
	if err != nil {
		return nil, fmt.Errorf("query todo rows: %w", err)
	}
	defer rows.Close()

	var result []TodoRow
	for rows.Next() {
		var seriesName, taskName, displayName string
		var number float64
		var chapterName *string
		var volume *int64
		var activeSince time.Time

		if err := rows.Scan(&seriesName, &number, &chapterName, &volume, &taskName, &displayName, &activeSince); err != nil {
			return nil, fmt.Errorf("scan todo row: %w", err)
		}

		result = append(result, TodoRow{
			Series:      seriesName,
			Chapter:     formatChapterLabel(number, chapterName, volume),
			Task:        taskName,
			AssignedTo:  displayName,
			ActiveSince: activeSince,
		})
	}
	return result, rows.Err()
}

// AssignmentEntry represents one user's assignment to a chapter+task.
type AssignmentEntry struct {
	DisplayName string
	Completed   bool
}

// CrewEntry represents one series-level crew assignment.
type CrewEntry struct {
	UserName string
	TaskName string
}

// TaskColumn describes a task that appears as a column in the series sheet.
type TaskColumn struct {
	ID    int
	Name  string
	Level *int64 // nullable
}

// ChapterRow describes one chapter in the series sheet.
type ChapterRow struct {
	ID       int
	Label    string
	Status   string
	ClosedAt *string // nil when the chapter has no closed_at date
}

// SeriesSheetData holds everything needed to render a series sheet.
type SeriesSheetData struct {
	Status           string
	Crew             []CrewEntry
	CrewPlaceholders []CrewEntry               // TBD rows for series-level placeholder slots
	Tasks            []TaskColumn
	Chapters         []ChapterRow
	Assignments      map[int]map[int][]AssignmentEntry // [chapterID][taskID]
	Placeholders     map[int]map[int]int               // [chapterID][taskID] = count of unfilled vacancies
}

// GetSeriesSheetData fetches all data needed to render the series sheet for the given series name.
// Returns nil if the series does not exist.
func (c *Client) GetSeriesSheetData(ctx context.Context, seriesName string) (*SeriesSheetData, error) {
	// Resolve series ID and status.
	var seriesID int
	var seriesStatus string
	err := c.pool.QueryRow(ctx, `
		SELECT id,
		       CASE status
		           WHEN 'active'    THEN 'Active'
		           WHEN 'completed' THEN 'Completed'
		           WHEN 'hiatus'    THEN 'Hiatus'
		           WHEN 'dropped'   THEN 'Dropped'
		           ELSE status
		       END
		FROM series WHERE name = $1`, seriesName).Scan(&seriesID, &seriesStatus)
	if errors.Is(err, pgx.ErrNoRows) {
		return nil, nil
	}
	if err != nil {
		return nil, fmt.Errorf("find series: %w", err)
	}

	data := &SeriesSheetData{
		Status:       seriesStatus,
		Assignments:  make(map[int]map[int][]AssignmentEntry),
		Placeholders: make(map[int]map[int]int),
	}

	// 1. Crew (series-level assignments)
	crewRows, err := c.pool.Query(ctx, `
		SELECT u.display_name, t.name
		FROM series_assignments sa
		JOIN users u ON sa.user_id = u.id
		JOIN tasks t ON sa.task_id = t.id
		WHERE sa.series_id = $1
		ORDER BY t.level NULLS LAST, t.name, u.display_name`, seriesID)
	if err != nil {
		return nil, fmt.Errorf("query crew: %w", err)
	}
	defer crewRows.Close()
	for crewRows.Next() {
		var e CrewEntry
		if err := crewRows.Scan(&e.UserName, &e.TaskName); err != nil {
			return nil, fmt.Errorf("scan crew: %w", err)
		}
		data.Crew = append(data.Crew, e)
	}
	if err := crewRows.Err(); err != nil {
		return nil, err
	}

	// 1b. Series-level crew placeholders — one TBD entry per placeholder row
	sapRows, err := c.pool.Query(ctx, `
		SELECT t.name, COUNT(*) AS cnt
		FROM series_assignment_placeholders sap
		JOIN tasks t ON sap.task_id = t.id
		WHERE sap.series_id = $1
		GROUP BY t.id, t.name, t.level
		ORDER BY t.level NULLS LAST, t.name`, seriesID)
	if err != nil {
		return nil, fmt.Errorf("query crew placeholders: %w", err)
	}
	defer sapRows.Close()
	for sapRows.Next() {
		var taskName string
		var cnt int
		if err := sapRows.Scan(&taskName, &cnt); err != nil {
			return nil, fmt.Errorf("scan crew placeholder: %w", err)
		}
		for i := 0; i < cnt; i++ {
			data.CrewPlaceholders = append(data.CrewPlaceholders, CrewEntry{UserName: "TBD", TaskName: taskName})
		}
	}
	if err := sapRows.Err(); err != nil {
		return nil, err
	}

	// 2. Task columns (all tasks with at least one assignment or placeholder in any chapter
	//    of this series, or a series-level placeholder for this series)
	taskRows, err := c.pool.Query(ctx, `
		SELECT DISTINCT t.id, t.name, t.level
		FROM (
			SELECT ca.task_id FROM chapter_assignments ca
			JOIN chapters c ON ca.chapter_id = c.id WHERE c.series_id = $1
			UNION
			SELECT cap.task_id FROM chapter_assignment_placeholders cap
			JOIN chapters c ON cap.chapter_id = c.id WHERE c.series_id = $1
			UNION
			SELECT sap.task_id FROM series_assignment_placeholders sap
			WHERE sap.series_id = $1
		) combined
		JOIN tasks t ON t.id = combined.task_id
		ORDER BY t.level NULLS LAST, t.name`, seriesID)
	if err != nil {
		return nil, fmt.Errorf("query task columns: %w", err)
	}
	defer taskRows.Close()
	for taskRows.Next() {
		var tc TaskColumn
		if err := taskRows.Scan(&tc.ID, &tc.Name, &tc.Level); err != nil {
			return nil, fmt.Errorf("scan task column: %w", err)
		}
		data.Tasks = append(data.Tasks, tc)
	}
	if err := taskRows.Err(); err != nil {
		return nil, err
	}

	// 3. Chapters ordered by number
	chapterRows, err := c.pool.Query(ctx, `
		SELECT id, number, name, volume,
		       CASE status
		           WHEN 'in_progress' THEN 'In Progress'
		           WHEN 'queued'      THEN 'Queued'
		           WHEN 'released'    THEN 'Released'
		           WHEN 'hiatus'      THEN 'Hiatus'
		           WHEN 'dropped'     THEN 'Dropped'
		           ELSE status
		       END,
		       closed_at::date::text
		FROM chapters
		WHERE series_id = $1
		ORDER BY number`, seriesID)
	if err != nil {
		return nil, fmt.Errorf("query chapters: %w", err)
	}
	defer chapterRows.Close()
	for chapterRows.Next() {
		var id int
		var number float64
		var name *string
		var volume *int64
		var status string
		var closedAt *string
		if err := chapterRows.Scan(&id, &number, &name, &volume, &status, &closedAt); err != nil {
			return nil, fmt.Errorf("scan chapter: %w", err)
		}
		data.Chapters = append(data.Chapters, ChapterRow{
			ID:       id,
			Label:    formatChapterLabel(number, name, volume),
			Status:   status,
			ClosedAt: closedAt,
		})
		data.Assignments[id] = make(map[int][]AssignmentEntry)
	}
	if err := chapterRows.Err(); err != nil {
		return nil, err
	}

	if len(data.Chapters) == 0 {
		return data, nil
	}

	// 4. Assignments for all chapters in this series
	assignRows, err := c.pool.Query(ctx, `
		SELECT ca.chapter_id, ca.task_id, u.display_name,
		       ca.completed_at IS NOT NULL AS completed
		FROM chapter_assignments ca
		JOIN chapters c ON ca.chapter_id = c.id
		JOIN users u ON ca.user_id = u.id
		WHERE c.series_id = $1
		ORDER BY ca.chapter_id, ca.task_id, u.display_name`, seriesID)
	if err != nil {
		return nil, fmt.Errorf("query assignments: %w", err)
	}
	defer assignRows.Close()
	for assignRows.Next() {
		var chapterID, taskID int
		var displayName string
		var completed bool
		if err := assignRows.Scan(&chapterID, &taskID, &displayName, &completed); err != nil {
			return nil, fmt.Errorf("scan assignment: %w", err)
		}
		if _, ok := data.Assignments[chapterID]; !ok {
			data.Assignments[chapterID] = make(map[int][]AssignmentEntry)
		}
		data.Assignments[chapterID][taskID] = append(data.Assignments[chapterID][taskID], AssignmentEntry{
			DisplayName: displayName,
			Completed:   completed,
		})
	}
	if err := assignRows.Err(); err != nil {
		return nil, err
	}

	// 5. Placeholder counts per (chapter, task)
	phRows, err := c.pool.Query(ctx, `
		SELECT cap.chapter_id, cap.task_id, COUNT(*) AS cnt
		FROM chapter_assignment_placeholders cap
		JOIN chapters c ON cap.chapter_id = c.id
		WHERE c.series_id = $1
		GROUP BY cap.chapter_id, cap.task_id`, seriesID)
	if err != nil {
		return nil, fmt.Errorf("query placeholders: %w", err)
	}
	defer phRows.Close()
	for phRows.Next() {
		var chapterID, taskID, cnt int
		if err := phRows.Scan(&chapterID, &taskID, &cnt); err != nil {
			return nil, fmt.Errorf("scan placeholder: %w", err)
		}
		if _, ok := data.Placeholders[chapterID]; !ok {
			data.Placeholders[chapterID] = make(map[int]int)
		}
		data.Placeholders[chapterID][taskID] = cnt
	}
	return data, phRows.Err()
}

// formatChapterLabel builds the display label for a chapter row.
// Format: "Vol.X Ch.Y" if volume is set, else "Ch.Y". Appends name if present.
func formatChapterLabel(number float64, name *string, volume *int64) string {
	numStr := formatChapterNumber(number)
	label := "Ch." + numStr
	if volume != nil {
		label = fmt.Sprintf("Vol.%d Ch.%s", *volume, numStr)
	}
	if name != nil && *name != "" {
		label += " " + *name
	}
	return label
}

// formatChapterNumber formats a chapter number as an integer string when it has no
// fractional part (51 → "51") or as a decimal otherwise (51.5 → "51.5").
func formatChapterNumber(n float64) string {
	if n == math.Trunc(n) {
		return strconv.Itoa(int(n))
	}
	return strconv.FormatFloat(n, 'f', -1, 64)
}
