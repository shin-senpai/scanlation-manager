#pragma once

// Standard Includes
#include <string>

// Third Party Includes
#include <pqxx/pqxx>

class ChapterAssignmentPlaceholdersRepository {
public:
  // Add one placeholder slot for (chapter, task).
  // Multiple placeholders are allowed — each represents one unfilled vacancy.
  void create(pqxx::transaction_base &txn, int chapter_id, int task_id);

  // Remove ONE placeholder for (chapter, task) — the oldest row (lowest id).
  // No-op if none exist. Used when a user is assigned, consuming one vacancy.
  void removeOne(pqxx::transaction_base &txn, int chapter_id, int task_id);

  // Remove ALL placeholders for (chapter, task).
  // Used by the remove-placeholder command.
  void removeAll(pqxx::transaction_base &txn, int chapter_id, int task_id);

  // Count of placeholder rows for (chapter, task).
  int count(pqxx::transaction_base &txn, int chapter_id, int task_id);

  // Returns true if any placeholder exists for this chapter (any task).
  // Used by the auto-release guard.
  bool existsForChapter(pqxx::transaction_base &txn, int chapter_id);

  // Remove ALL placeholders for a specific task across all chapters in a series.
  // Called when the last series-level vacancy is filled or by remove-placeholder.
  void removeAllForTaskInSeries(pqxx::transaction_base &txn, int series_id, int task_id);

  // Remove ONE placeholder per chapter for a specific task across all chapters in a series.
  // Called when a single series-level vacancy is filled (not the last one).
  void removeOnePerChapterForTaskInSeries(pqxx::transaction_base &txn, int series_id, int task_id);

  // Remove ALL placeholders for a task across the entire database.
  // Called on task retirement.
  void removeAllByTask(pqxx::transaction_base &txn, int task_id);
};
