// Associated Header Include
#include "db/repositories/User.hpp"
#include "models/ModelUser.hpp"

int UserRepository::create(pqxx::work &txn, const std::string &display_name) {
  auto result = txn.exec(
      "INSERT INTO users (display_name) VALUES ($1) RETURNING id",
      pqxx::params{display_name});

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
        results[row][0].as<int>(),
        results[row][1].is_null() ? std::nullopt : std::make_optional(results[row][1].as<std::string>()),
        results[row][2].as<std::string>(),
        results[row][3].as<bool>(),
        results[row][4].as<bool>());
  }

  return users;
}

std::optional<User> UserRepository::findById(pqxx::read_transaction &txn, int id) {
  auto result = txn.exec(
      "SELECT id, name, display_name, is_manager, is_supermanager "
      "FROM users WHERE id = $1",
      pqxx::params{id});

  if(result.empty()) {
    return std::nullopt;
  }

  return User{
      result[0][0].as<int>(),
      result[0][1].is_null() ? std::nullopt : std::make_optional(result[0][1].as<std::string>()),
      result[0][2].as<std::string>(),
      result[0][3].as<bool>(),
      result[0][4].as<bool>(),
  };
}