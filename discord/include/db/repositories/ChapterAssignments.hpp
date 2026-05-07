#pragma once

// User Defined Includes
#include "models/ModelAssignmentDetail.hpp"
#include "models/ModelChapterAssignment.hpp"
#include "models/ModelUserHistoryEntry.hpp"

// Standard Includes
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Third Party Includes
#include <pqxx/pqxx>

class ChapterAssignmentsRepository {
public:
  void create(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id);

  void remove(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id);

  void removeAllByTask(pqxx::transaction_base &txn, int task_id);

  void removeOutstandingByTask(pqxx::transaction_base &txn, int task_id);

  bool exists(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id, std::optional<bool> completed = std::nullopt);

  bool hasCompletedByTask(pqxx::transaction_base &txn, int task_id);

  bool hasCompletedByChapter(pqxx::transaction_base &txn, int chapter_id);

  bool hasCompletedBySeries(pqxx::transaction_base &txn, int series_id);

  void clearAllCompletedByChapter(pqxx::transaction_base &txn, int chapter_id);

  void clearAllCompletedBySeries(pqxx::transaction_base &txn, int series_id);

  std::vector<ChapterAssignment> listByChapter(pqxx::transaction_base &txn, int chapter_id, std::optional<int> task_id = std::nullopt, std::optional<bool> completed = std::nullopt);

  std::vector<ChapterAssignment> listByUser(pqxx::transaction_base &txn, int user_id, std::optional<int> task_id = std::nullopt, std::optional<bool> completed = std::nullopt);

  std::vector<ChapterAssignment> listBySeries(pqxx::transaction_base &txn, std::vector<int> series_ids, std::optional<std::vector<int>> chapter_ids = std::nullopt, std::optional<bool> completed = std::nullopt);

  std::vector<int> listDistinctSeriesByUser(pqxx::transaction_base &txn, int user_id, std::optional<bool> completed);

  void setCompleted(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id);

  void clearCompleted(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id);

  std::vector<AssignmentDetail> listByChapterWithDetails(pqxx::transaction_base &txn, int chapter_id);

  // Returns (series_id, series_name) for series where user has at least one outstanding assignment.
  std::vector<std::pair<int, std::string>> listActiveSeriesForUser(pqxx::transaction_base &txn, int user_id);

  // Returns the number of distinct series the user has any assignment in (completed or not).
  int countTotalSeriesForUser(pqxx::transaction_base &txn, int user_id);

  // Returns all completed assignments for a user, newest first, with full series/chapter/task details.
  std::vector<UserHistoryEntry> listCompletedByUserWithDetails(pqxx::transaction_base &txn, int user_id, std::optional<int> series_id = std::nullopt);
};
