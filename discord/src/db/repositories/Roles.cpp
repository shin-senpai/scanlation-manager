// Associated Header Include
#include "db/repositories/Roles.hpp"

int RolesRepository::create(pqxx::transaction_base &txn, std::string_view name) {
  auto result = txn.exec(
      "INSERT INTO roles (name) VALUES ($1) RETURNING id",
      pqxx::params(txn, name));

  return result[0]["id"].as<int>();
}

std::optional<Role> RolesRepository::findById(pqxx::transaction_base &txn, int id) {
  auto result = txn.exec(
      "SELECT id, name FROM roles WHERE id = $1",
      pqxx::params(txn, id));

  if(result.empty()) {
    return std::nullopt;
  }

  return Role{result[0]["id"].as<int>(), result[0]["name"].as<std::string>()};
}

std::optional<Role> RolesRepository::findByName(pqxx::transaction_base &txn, std::string_view name) {
  auto result = txn.exec(
      "SELECT id, name FROM roles WHERE name = $1",
      pqxx::params(txn, name));

  if(result.empty()) {
    return std::nullopt;
  }

  return Role{result[0]["id"].as<int>(), result[0]["name"].as<std::string>()};
}

std::vector<Role> RolesRepository::listAll(pqxx::transaction_base &txn) {
  auto results = txn.exec("SELECT id, name FROM roles");

  std::vector<Role> roles;
  roles.reserve(results.size());
  for(const auto &row : results) {
    roles.emplace_back(row["id"].as<int>(), row["name"].as<std::string>());
  }

  return roles;
}
