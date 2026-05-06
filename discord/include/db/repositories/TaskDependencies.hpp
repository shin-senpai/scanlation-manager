#pragma once

// User Defined Includes
#include "models/ModelTaskDependency.hpp"

// Standard Includes
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Third Party Includes
#include <pqxx/pqxx>

class TaskDependenciesRepository {
public:
  void create(pqxx::transaction_base &txn, int task_id, int depends_on_task_id);

  void remove(pqxx::transaction_base &txn, int task_id, int depends_on_task_id);

  void removeAllByTask(pqxx::transaction_base &txn, int task_id);

  // Returns the tasks that task_id depends on.
  std::vector<TaskDependency> listDependenciesOf(pqxx::transaction_base &txn, int task_id);

  // Returns the tasks that depend on task_id.
  std::vector<TaskDependency> listDependentsOf(pqxx::transaction_base &txn, int task_id);

  // Returns the name of the first prerequisite task that still has outstanding assignments
  // for chapter_id, or nullopt if all dependencies are satisfied.
  std::optional<std::string> findFirstBlockingDependency(pqxx::transaction_base &txn, int chapter_id, int task_id);

  // Returns (discord_id, task_name) pairs for users with outstanding assignments on tasks
  // that depend on depends_on_task_id within chapter_id.
  std::vector<std::pair<int64_t, std::string>> findDependentAssignees(pqxx::transaction_base &txn, int chapter_id, int depends_on_task_id);
};
