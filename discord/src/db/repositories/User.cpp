// Associated Header Include
#include "db/repositories/User.hpp"

// User Defined Includes
#include "models/ModelUser.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <optional>

int UserRepository::create(pqxx::transaction_base &txn, const std::string_view &display_name, Permission permission_level) {
  auto result = txn.exec(
      "INSERT INTO users (display_name, permission_level) VALUES ($1, $2) RETURNING id",
      pqxx::params{txn, display_name, static_cast<int>(permission_level)});

  return result[0]["id"].as<int>();
}

std::vector<User> UserRepository::listUsers(pqxx::transaction_base &txn) {
  auto results = txn.exec(
      "SELECT * FROM users");

  size_t rows = results.size();

  std::vector<User> users;
  users.reserve(rows);
  for(size_t row = 0; row < rows; row++) {
    users.emplace_back(
        results[row]["id"].as<int>(),
        results[row]["name"].is_null() ? std::nullopt : std::make_optional(results[row]["name"].as<std::string>()),
        results[row]["display_name"].as<std::string>(),
        results[row]["joined_at"].as<std::string>(),
        results[row]["left_at"].is_null() ? std::nullopt : std::make_optional(results[row]["left_at"].as<std::string>()),
        static_cast<Permission>(results[row]["permission_level"].as<int>()));
  }

  return users;
}

std::optional<User> UserRepository::findById(pqxx::transaction_base &txn, int id) {
  auto result = txn.exec(
      "SELECT * FROM users WHERE id = $1",
      pqxx::params{txn, id});

  if(result.empty()) {
    return std::nullopt;
  }

  return User{
      result[0]["id"].as<int>(),
      result[0]["name"].is_null() ? std::nullopt : std::make_optional(result[0][1].as<std::string>()),
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

std::optional<Permission> UserRepository::getPermissionLevel(pqxx::transaction_base &txn, int id) {
  auto result = txn.exec(
      "SELECT permission_level FROM users WHERE id = $1",
      pqxx::params(txn, id));

  if(result.empty()) {
    return std::nullopt;
  }

  return static_cast<Permission>(result[0]["permission_level"].as<int>());
}