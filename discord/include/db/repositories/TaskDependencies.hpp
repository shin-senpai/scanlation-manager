#pragma once

// User Defined Includes
#include "models/ModelTaskDependency.hpp"

// Standard Includes
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
};
