// Associated Header Include
#include "db/repositories/Tasks.hpp"

namespace {
Task rowToTask(const pqxx::row &row) {
  return Task{
      row["id"].as<int>(),
      row["name"].as<std::string>(),
      row["retired_at"].is_null() ? std::nullopt : std::make_optional(row["retired_at"].as<std::string>())};
}
}

int TasksRepository::create(pqxx::transaction_base &txn, std::string_view name) {
  auto result = txn.exec(
      "INSERT INTO tasks (name) VALUES ($1) RETURNING id",
      pqxx::params(txn, name));

  return result[0]["id"].as<int>();
}

void TasksRepository::remove(pqxx::transaction_base &txn, int id) {
  txn.exec(
      "DELETE FROM tasks WHERE id = $1",
      pqxx::params(txn, id));
}

void TasksRepository::retire(pqxx::transaction_base &txn, int id) {
  txn.exec(
      "UPDATE tasks SET retired_at = NOW() WHERE id = $1",
      pqxx::params(txn, id));
}

void TasksRepository::unretire(pqxx::transaction_base &txn, int id) {
  txn.exec(
      "UPDATE tasks SET retired_at = NULL WHERE id = $1",
      pqxx::params(txn, id));
}

std::optional<Task> TasksRepository::findById(pqxx::transaction_base &txn, int id) {
  auto result = txn.exec(
      "SELECT id, name, retired_at FROM tasks WHERE id = $1",
      pqxx::params(txn, id));

  if(result.empty()) {
    return std::nullopt;
  }

  return rowToTask(result[0]);
}

std::optional<Task> TasksRepository::findByName(pqxx::transaction_base &txn, std::string_view name) {
  auto result = txn.exec(
      "SELECT id, name, retired_at FROM tasks WHERE name = $1",
      pqxx::params(txn, name));

  if(result.empty()) {
    return std::nullopt;
  }

  return rowToTask(result[0]);
}

std::vector<Task> TasksRepository::listAll(pqxx::transaction_base &txn, bool include_retired) {
  std::string query = "SELECT id, name, retired_at FROM tasks";
  if(!include_retired) query += " WHERE retired_at IS NULL";
  query += " ORDER BY name";

  auto results = txn.exec(query);

  std::vector<Task> tasks;
  tasks.reserve(results.size());
  for(const auto &row : results) {
    tasks.emplace_back(rowToTask(row));
  }

  return tasks;
}
