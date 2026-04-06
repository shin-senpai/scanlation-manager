// Associated Header Include
#include "db/repositories/Tasks.hpp"

int TasksRepository::create(pqxx::transaction_base &txn, std::string_view name) {
  auto result = txn.exec(
      "INSERT INTO tasks (name) VALUES ($1) RETURNING id",
      pqxx::params(txn, name));

  return result[0]["id"].as<int>();
}

std::optional<Task> TasksRepository::findById(pqxx::transaction_base &txn, int id) {
  auto result = txn.exec(
      "SELECT id, name FROM tasks WHERE id = $1",
      pqxx::params(txn, id));

  if(result.empty()) {
    return std::nullopt;
  }

  return Task{result[0]["id"].as<int>(), result[0]["name"].as<std::string>()};
}

std::optional<Task> TasksRepository::findByName(pqxx::transaction_base &txn, std::string_view name) {
  auto result = txn.exec(
      "SELECT id, name FROM tasks WHERE name = $1",
      pqxx::params(txn, name));

  if(result.empty()) {
    return std::nullopt;
  }

  return Task{result[0]["id"].as<int>(), result[0]["name"].as<std::string>()};
}

std::vector<Task> TasksRepository::listAll(pqxx::transaction_base &txn) {
  auto results = txn.exec("SELECT id, name FROM tasks");

  std::vector<Task> tasks;
  tasks.reserve(results.size());
  for(const auto &row : results) {
    tasks.emplace_back(row["id"].as<int>(), row["name"].as<std::string>());
  }

  return tasks;
}
