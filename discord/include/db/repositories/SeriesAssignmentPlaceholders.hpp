#pragma once

// Standard Includes
#include <vector>

// Third Party Includes
#include <pqxx/pqxx>

class SeriesAssignmentPlaceholdersRepository {
public:
  // Add one placeholder slot for (series, task).
  void create(pqxx::transaction_base &txn, int series_id, int task_id);

  // Remove ONE placeholder for (series, task) — the oldest row (lowest id).
  // No-op if none exist. Used when a user is assigned, consuming one vacancy.
  void removeOne(pqxx::transaction_base &txn, int series_id, int task_id);

  // Remove ALL placeholders for (series, task).
  void removeAll(pqxx::transaction_base &txn, int series_id, int task_id);

  // Count of placeholders for (series, task).
  int count(pqxx::transaction_base &txn, int series_id, int task_id);

  // Returns one task_id entry per placeholder row for the series (with duplicates).
  // Used when creating a new chapter to cascade series-level placeholders.
  std::vector<int> listTaskIdsBySeries(pqxx::transaction_base &txn, int series_id);

  // Remove ALL placeholders for a task across all series.
  // Called on task retirement.
  void removeAllByTask(pqxx::transaction_base &txn, int task_id);
};
