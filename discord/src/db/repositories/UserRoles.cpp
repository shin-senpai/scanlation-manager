// Associated Header Include
#include "db/repositories/UserRoles.hpp"

void UserRolesRepository::create(pqxx::transaction_base &txn, int user_id, int role_id) {
  txn.exec(
      "INSERT INTO user_roles (user_id, role_id) VALUES ($1, $2)",
      pqxx::params(txn, user_id, role_id));
}

void UserRolesRepository::remove(pqxx::transaction_base &txn, int user_id, int role_id) {
  txn.exec(
      "DELETE FROM user_roles WHERE user_id = $1 AND role_id = $2",
      pqxx::params(txn, user_id, role_id));
}

std::vector<UserRole> UserRolesRepository::listByUser(pqxx::transaction_base &txn, int user_id) {
  auto results = txn.exec(
      "SELECT user_id, role_id FROM user_roles WHERE user_id = $1",
      pqxx::params(txn, user_id));

  std::vector<UserRole> user_roles;
  user_roles.reserve(results.size());
  for(const auto &row : results) {
    user_roles.emplace_back(row["user_id"].as<int>(), row["role_id"].as<int>());
  }

  return user_roles;
}
