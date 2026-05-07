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

void TaskDependenciesRepository::removeAllByTask(pqxx::transaction_base &txn, int task_id) {
  txn.exec(
      "DELETE FROM task_dependencies WHERE task_id = $1 OR depends_on_task_id = $1",
      pqxx::params(txn, task_id));
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

std::unordered_map<int, std::vector<int>> TaskDependenciesRepository::listAll(pqxx::transaction_base &txn) {
  auto results = txn.exec(
      "SELECT task_id, depends_on_task_id FROM task_dependencies",
      pqxx::params(txn));

  std::unordered_map<int, std::vector<int>> deps;
  for(const auto &row : results) {
    deps[row["task_id"].as<int>()]
        .push_back(row["depends_on_task_id"].as<int>());
  }

  return deps;
}

std::optional<std::string> TaskDependenciesRepository::findFirstBlockingDependency(pqxx::transaction_base &txn, int chapter_id, int task_id) {
  auto result = txn.exec(
      "SELECT t.name"
      " FROM chapter_assignments ca"
      " JOIN task_dependencies td ON ca.task_id = td.depends_on_task_id"
      " JOIN tasks t ON t.id = ca.task_id"
      " WHERE td.task_id = $1 AND ca.chapter_id = $2 AND ca.completed_at IS NULL"
      " LIMIT 1",
      pqxx::params(txn, task_id, chapter_id));
  if(result.empty()) {
    return std::nullopt;
  }
  return result[0][0].as<std::string>();
}

std::vector<std::pair<int64_t, std::string>> TaskDependenciesRepository::findDependentAssignees(pqxx::transaction_base &txn, int chapter_id, int depends_on_task_id) {
  auto result = txn.exec(
      "SELECT DISTINCT di.discord_id, t.name"
      " FROM chapter_assignments ca"
      " JOIN task_dependencies td ON ca.task_id = td.task_id"
      " JOIN discord_identities di ON di.user_id = ca.user_id"
      " JOIN tasks t ON t.id = ca.task_id"
      " WHERE td.depends_on_task_id = $1 AND ca.chapter_id = $2"
      " AND ca.completed_at IS NULL AND di.unlinked_at IS NULL",
      pqxx::params(txn, depends_on_task_id, chapter_id));
  std::vector<std::pair<int64_t, std::string>> assignees;
  assignees.reserve(result.size());
  for(const auto &row : result) {
    assignees.emplace_back(row[0].as<int64_t>(), row[1].as<std::string>());
  }
  return assignees;
}
