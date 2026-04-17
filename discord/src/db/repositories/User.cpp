// Associated Header Include
#include "db/repositories/User.hpp"

// User Defined Includes
#include "models/ModelUser.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <optional>

int UserRepository::create(pqxx::transaction_base &txn, std::string_view display_name, Permission permission_level) {
  auto result = txn.exec(
      "INSERT INTO users (display_name, permission_level) VALUES ($1, $2) RETURNING id",
      pqxx::params(txn, display_name, static_cast<int>(permission_level)));

  return result[0]["id"].as<int>();
}

std::vector<User> UserRepository::listUsers(pqxx::transaction_base &txn, std::optional<Permission> permission_level, bool active_only) {
  std::string query = "SELECT id, name, display_name, joined_at, left_at, permission_level FROM users";
  bool has_where = false;

  if(active_only) {
    query += " WHERE left_at IS NULL";
    has_where = true;
  }
  if(permission_level) {
    query += has_where ? " AND" : " WHERE";
    query += " permission_level = " + std::to_string(static_cast<int>(*permission_level));
  }

  auto results = txn.exec(query);

  std::vector<User> users;
  users.reserve(results.size());
  for(const auto &row : results) {
    users.emplace_back(
        row["id"].as<int>(),
        row["name"].is_null() ? std::nullopt : std::make_optional(row["name"].as<std::string>()),
        row["display_name"].as<std::string>(),
        row["joined_at"].as<std::string>(),
        row["left_at"].is_null() ? std::nullopt : std::make_optional(row["left_at"].as<std::string>()),
        static_cast<Permission>(row["permission_level"].as<int>()));
  }

  return users;
}

std::optional<User> UserRepository::findById(pqxx::transaction_base &txn, int id) {
  auto result = txn.exec(
      "SELECT id, name, display_name, joined_at, left_at, permission_level FROM users WHERE id = $1",
      pqxx::params(txn, id));

  if(result.empty()) {
    return std::nullopt;
  }

  return User{
      result[0]["id"].as<int>(),
      result[0]["name"].is_null() ? std::nullopt : std::make_optional(result[0]["name"].as<std::string>()),
      result[0]["display_name"].as<std::string>(),
      result[0]["joined_at"].as<std::string>(),
      result[0]["left_at"].is_null() ? std::nullopt : std::make_optional(result[0]["left_at"].as<std::string>()),
      static_cast<Permission>(result[0]["permission_level"].as<int>())};
}

void UserRepository::setPermissionLevel(pqxx::transaction_base &txn, int id, Permission permission_level) {
  txn.exec(
      "UPDATE users SET permission_level = $2 WHERE id = $1",
      pqxx::params(txn, id, static_cast<int>(permission_level)));
}

Permission UserRepository::getPermissionLevel(pqxx::transaction_base &txn, int id) {
  auto result = txn.exec(
      "SELECT permission_level FROM users WHERE id = $1",
      pqxx::params(txn, id));

  return static_cast<Permission>(result[0]["permission_level"].as<int>());
}