// Associated Header Include
#include "db/repositories/TaskDependencies.hpp"

void TaskDependenciesRepository::create(pqxx::transaction_base &txn, int task_id, int depends_on_task_id) {
  txn.exec(
      "INSERT INTO task_dependencies (task_id, depends_on_task_id) VALUES ($1, $2)",
      pqxx::params(txn, task_id, depends_on_task_id));
}

void TaskDependenciesRepository::remove(pqxx::transaction_base &txn, int task_id, int depends_on_task_id) {
  txn.exec(
      "DELETE FROM task_dependencies WHERE task_id = $1 AND depends_on_task_id = $2",
      pqxx::params(txn, task_id, depends_on_task_id));
}

std::vector<TaskDependency> TaskDependenciesRepository::listDependenciesOf(pqxx::transaction_base &txn, int task_id) {
  auto results = txn.exec(
      "SELECT task_id, depends_on_task_id FROM task_dependencies WHERE task_id = $1",
      pqxx::params(txn, task_id));

  std::vector<TaskDependency> deps;
  deps.reserve(results.size());
  for(const auto &row : results) {
    deps.emplace_back(row["task_id"].as<int>(), row["depends_on_task_id"].as<int>());
  }

  return deps;
}

std::vector<TaskDependency> TaskDependenciesRepository::listDependentsOf(pqxx::transaction_base &txn, int task_id) {
  auto results = txn.exec(
      "SELECT task_id, depends_on_task_id FROM task_dependencies WHERE depends_on_task_id = $1",
      pqxx::params(txn, task_id));

  std::vector<TaskDependency> deps;
  deps.reserve(results.size());
  for(const auto &row : results) {
    deps.emplace_back(row["task_id"].as<int>(), row["depends_on_task_id"].as<int>());
  }

  return deps;
}
