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

bool RoleTasksRepository::exists(pqxx::transaction_base &txn, int role_id, int task_id) {
  auto results = txn.exec(
      "SELECT 1 FROM role_tasks WHERE role_id = $1 AND task_id = $2 LIMIT 1",
      pqxx::params(txn, role_id, task_id));
  
  return !results.empty();
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

std::vector<int> RoleTasksRepository::listTaskIdsByRole(pqxx::transaction_base &txn, int role_id) {
  auto results = txn.exec(
      "SELECT task_id FROM role_tasks WHERE role_id = $1",
      pqxx::params(txn, role_id));

  std::vector<int> tasks;
  tasks.reserve(results.size());
  for(const auto &row : results) {
    tasks.emplace_back(row["task_id"].as<int>());
  }

  return tasks;
}

std::vector<int> RoleTasksRepository::listRoleIdsByTask(pqxx::transaction_base &txn, int task_id) {
  auto results = txn.exec(
      "SELECT role_id FROM role_tasks WHERE task_id = $1",
      pqxx::params(txn, task_id));

  std::vector<int> roles;
  roles.reserve(results.size());
  for(const auto &row : results) {
    roles.emplace_back(row["role_id"].as<int>());
  }

  return roles;
}

std::vector<RoleTask> RoleTasksRepository::listAll(pqxx::transaction_base &txn) {
  auto results = txn.exec(
      "SELECT role_id, task_id FROM role_tasks",
      pqxx::params(txn));

  std::vector<RoleTask> role_tasks;
  role_tasks.reserve(results.size());
  for(const auto &row : results) {
    role_tasks.emplace_back(row["role_id"].as<int>(), row["task_id"].as<int>());
  }

  return role_tasks;
}
