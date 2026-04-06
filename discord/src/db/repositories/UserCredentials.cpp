// Associated Header Include
#include "db/repositories/UserCredentials.hpp"

void UserCredentialsRepository::create(pqxx::transaction_base &txn, int user_id, std::string_view password_hash) {
  txn.exec(
      "INSERT INTO user_credentials (user_id, password_hash) VALUES ($1, $2)",
      pqxx::params(txn, user_id, password_hash));
}

std::optional<std::string> UserCredentialsRepository::findByUserId(pqxx::transaction_base &txn, int user_id) {
  auto result = txn.exec(
      "SELECT password_hash FROM user_credentials WHERE user_id = $1",
      pqxx::params(txn, user_id));

  if(result.empty()) {
    return std::nullopt;
  }

  return result[0]["password_hash"].as<std::string>();
}

void UserCredentialsRepository::update(pqxx::transaction_base &txn, int user_id, std::string_view password_hash) {
  txn.exec(
      "UPDATE user_credentials SET password_hash = $2 WHERE user_id = $1",
      pqxx::params(txn, user_id, password_hash));
}
