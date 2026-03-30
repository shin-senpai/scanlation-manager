// Associated Header Include
#include "db/repositories/User.hpp"

// User Defined Includes
#include "models/ModelUser.hpp"

int UserRepository::create(pqxx::work &txn, const std::string_view &display_name, bool is_manager, bool is_supermanager) {
  auto result = txn.exec(
      "INSERT INTO users (display_name, is_manager, is_supermanager) VALUES ($1, $2, $3) RETURNING id",
      pqxx::params{txn, display_name, is_manager, is_supermanager});

  return result[0][0].as<int>();
}

std::vector<User> UserRepository::listUsers(pqxx::read_transaction &txn) {
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
        results[row]["is_manager"].as<bool>(),
        results[row]["joined_at"].as<std::string>(),
        results[row]["left_at"].is_null() ? std::nullopt : std::make_optional(results[row]["left_at"].as<std::string>()),
        results[row]["is_supermanager"].as<bool>());
  }

  return users;
}

std::optional<User> UserRepository::findById(pqxx::read_transaction &txn, int id) {
  auto result = txn.exec(
      "SELECT id, name, display_name, is_manager, is_supermanager "
      "FROM users WHERE id = $1",
      pqxx::params{txn, id});

  if(result.empty()) {
    return std::nullopt;
  }

  return User{
      result[0]["id"].as<int>(),
      result[0]["name"].is_null() ? std::nullopt : std::make_optional(result[0][1].as<std::string>()),
      result[0]["display_name"].as<std::string>(),
      result[0]["is_manager"].as<bool>(),
      result[0]["joined_at"].as<std::string>(),
      result[0]["left_at"].is_null() ? std::nullopt : std::make_optional(result[0]["left_at"].as<std::string>()),
      result[0]["is_supermanager"].as<bool>()};
}