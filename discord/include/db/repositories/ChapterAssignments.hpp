#pragma once

// User Defined Includes
#include "models/ModelChapterAssignment.hpp"

// Standard Includes
#include <optional>
#include <vector>

// Third Party Includes
#include <pqxx/pqxx>

class ChapterAssignmentsRepository {
public:
  void create(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id);

  void remove(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id);

  void removeAllByTask(pqxx::transaction_base &txn, int task_id);

  void removeOutstandingByTask(pqxx::transaction_base &txn, int task_id);

  bool exists(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id);

  bool hasCompletedByTask(pqxx::transaction_base &txn, int task_id);

  std::vector<ChapterAssignment> listByChapter(pqxx::transaction_base &txn, int chapter_id, std::optional<int> task_id = std::nullopt, std::optional<bool> completed = std::nullopt);

  std::vector<ChapterAssignment> listByUser(pqxx::transaction_base &txn, int user_id, std::optional<int> task_id = std::nullopt, std::optional<bool> completed = std::nullopt);

  void setCompleted(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id);
};
