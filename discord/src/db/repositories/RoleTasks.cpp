// Associated Header Include
#include "db/repositories/RoleTasks.hpp"

void RoleTasksRepository::create(pqxx::transaction_base &txn, int role_id, int task_id) {
  txn.exec(
      "INSERT INTO role_tasks (role_id, task_id) VALUES ($1, $2)",
      pqxx::params(txn, role_id, task_id));
}

void RoleTasksRepository::remove(pqxx::transaction_base &txn, int role_id, int task_id) {
  txn.exec(
      "DELETE FROM role_tasks WHERE role_id = $1 AND task_id = $2",
      pqxx::params(txn, role_id, task_id));
}

void RoleTasksRepository::removeAllByRole(pqxx::transaction_base &txn, int role_id) {
  txn.exec(
      "DELETE FROM role_tasks WHERE role_id = $1",
      pqxx::params(txn, role_id));
}

void RoleTasksRepository::removeAllByTask(pqxx::transaction_base &txn, int task_id) {
  txn.exec(
      "DELETE FROM role_tasks WHERE task_id = $1",
      pqxx::params(txn, task_id));
}

std::vector<RoleTask> RoleTasksRepository::listByRole(pqxx::transaction_base &txn, int role_id) {
  auto results = txn.exec(
      "SELECT role_id, task_id FROM role_tasks WHERE role_id = $1",
      pqxx::params(txn, role_id));

  std::vector<RoleTask> role_tasks;
  role_tasks.reserve(results.size());
  for(const auto &row : results) {
    role_tasks.emplace_back(row["role_id"].as<int>(), row["task_id"].as<int>());
  }

  return role_tasks;
}

std::vector<RoleTask> RoleTasksRepository::listByTask(pqxx::transaction_base &txn, int task_id) {
  auto results = txn.exec(
      "SELECT role_id, task_id FROM role_tasks WHERE task_id = $1",
      pqxx::params(txn, task_id));

  std::vector<RoleTask> role_tasks;
  role_tasks.reserve(results.size());
  for(const auto &row : results) {
    role_tasks.emplace_back(row["role_id"].as<int>(), row["task_id"].as<int>());
  }

  return role_tasks;
}
