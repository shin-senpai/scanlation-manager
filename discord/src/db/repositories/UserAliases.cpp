// Associated Header Include
#include "db/repositories/UserAliases.hpp"

void UserAliasesRepository::create(pqxx::transaction_base &txn, int user_id, std::string &alias) {
  txn.exec(
      "INSERT INTO user_aliases (user_id, alias) VALUES ($1, $2)",
      pqxx::params(txn, user_id, alias));
}

std::optional<std::string> UserAliasesRepository::read(pqxx::transaction_base &txn, int user_id) {
  auto result = txn.exec(
      "SELECT alias FROM user_aliases WHERE user_id = $1 AND retired_at IS NULL",
      pqxx::params(txn, user_id));

  if(result.empty()) {
    return std::nullopt;
  }

  return result[0][0].as<std::string>();
}

void UserAliasesRepository::retire(pqxx::transaction_base &txn, int user_id) {
  txn.exec(
      "UPDATE user_aliases SET retired_at = NOW() WHERE id = $1",
      pqxx::params(txn, user_id));
}