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

bool TaskDependenciesRepository::exists(pqxx::transaction_base &txn, int task_id, int depends_on_task_id) {
  auto result = txn.exec(
      "SELECT task_id, depends_on_task_id FROM task_dependencies WHERE task_id = $1 AND depends_on_task_id = $2 LIMIT 1",
      pqxx::params(txn, task_id, depends_on_task_id));

  return !result.empty();
}

std::vector<int> TaskDependenciesRepository::listDependenciesOf(pqxx::transaction_base &txn, int task_id) {
  auto results = txn.exec(
      "SELECT depends_on_task_id FROM task_dependencies WHERE task_id = $1",
      pqxx::params(txn, task_id));

  std::vector<int> deps;
  deps.reserve(results.size());
  for(const auto &row : results) {
    deps.emplace_back(row["depends_on_task_id"].as<int>());
  }

  return deps;
}

std::vector<int> TaskDependenciesRepository::listDependentsOf(pqxx::transaction_base &txn, int task_id) {
  auto results = txn.exec(
      "SELECT task_id FROM task_dependencies WHERE depends_on_task_id = $1",
      pqxx::params(txn, task_id));

  std::vector<int> deps;
  deps.reserve(results.size());
  for(const auto &row : results) {
    deps.emplace_back(row["task_id"].as<int>());
  }

  return deps;
}

std::vector<std::string> TaskDependenciesRepository::listDependencyNamesOf(pqxx::transaction_base &txn, int task_id) {
  auto results = txn.exec(
      "SELECT t.name FROM task_dependencies td"
      " JOIN tasks t ON t.id = td.depends_on_task_id"
      " WHERE td.task_id = $1"
      " ORDER BY t.name",
      pqxx::params(txn, task_id));
  std::vector<std::string> names;
  names.reserve(results.size());
  for(const auto &row : results) {
    names.emplace_back(row[0].as<std::string>());
  }
  return names;
}

std::vector<std::string> TaskDependenciesRepository::listDependentNamesOf(pqxx::transaction_base &txn, int task_id) {
  auto results = txn.exec(
      "SELECT t.name FROM task_dependencies td"
      " JOIN tasks t ON t.id = td.task_id"
      " WHERE td.depends_on_task_id = $1"
      " ORDER BY t.name",
      pqxx::params(txn, task_id));
  std::vector<std::string> names;
  names.reserve(results.size());
  for(const auto &row : results) {
    names.emplace_back(row[0].as<std::string>());
  }
  return names;
}

std::vector<std::pair<std::string, std::vector<std::string>>> TaskDependenciesRepository::listAllWithDependencyNames(pqxx::transaction_base &txn) {
  auto results = txn.exec(
      "SELECT t.name AS task_name, dep.name AS dep_name"
      " FROM tasks t"
      " LEFT JOIN task_dependencies td ON td.task_id = t.id"
      " LEFT JOIN tasks dep ON dep.id = td.depends_on_task_id"
      " WHERE t.retired_at IS NULL"
      " ORDER BY t.name, dep.name",
      pqxx::params(txn));

  std::vector<std::pair<std::string, std::vector<std::string>>> out;
  for(const auto &row : results) {
    const std::string task_name = row["task_name"].as<std::string>();
    if(out.empty() || out.back().first != task_name) {
      out.emplace_back(task_name, std::vector<std::string>{});
    }
    if(!row["dep_name"].is_null()) {
      out.back().second.emplace_back(row["dep_name"].as<std::string>());
    }
  }
  return out;
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
  // A prerequisite blocks if it has an outstanding assignment OR a placeholder (unfilled vacancy).
  auto result = txn.exec(
      "SELECT t.name"
      " FROM task_dependencies td"
      " JOIN tasks t ON t.id = td.depends_on_task_id"
      " WHERE td.task_id = $1"
      " AND ("
      "   EXISTS ("
      "     SELECT 1 FROM chapter_assignments ca"
      "     WHERE ca.task_id = td.depends_on_task_id AND ca.chapter_id = $2 AND ca.completed_at IS NULL"
      "   )"
      "   OR EXISTS ("
      "     SELECT 1 FROM chapter_assignment_placeholders cap"
      "     WHERE cap.task_id = td.depends_on_task_id AND cap.chapter_id = $2"
      "   )"
      " )"
      " LIMIT 1",
      pqxx::params(task_id, chapter_id));
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
      " AND ca.completed_at IS NULL AND di.unlinked_at IS NULL"
      // Don't ping if any prerequisite still has an incomplete assignment OR a placeholder.
      " AND NOT EXISTS ("
      "   SELECT 1 FROM task_dependencies td2"
      "   WHERE td2.task_id = ca.task_id"
      "   AND ("
      "     EXISTS ("
      "       SELECT 1 FROM chapter_assignments blocker"
      "       WHERE blocker.task_id = td2.depends_on_task_id"
      "         AND blocker.chapter_id = $2 AND blocker.completed_at IS NULL"
      "     )"
      "     OR EXISTS ("
      "       SELECT 1 FROM chapter_assignment_placeholders cap"
      "       WHERE cap.task_id = td2.depends_on_task_id AND cap.chapter_id = $2"
      "     )"
      "   )"
      " )",
      pqxx::params(txn, depends_on_task_id, chapter_id));
  std::vector<std::pair<int64_t, std::string>> assignees;
  assignees.reserve(result.size());
  for(const auto &row : result) {
    assignees.emplace_back(row[0].as<int64_t>(), row[1].as<std::string>());
  }
  return assignees;
}
