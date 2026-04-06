#pragma once

// Standard Includes
#include <optional>
#include <string>
#include <string_view>

// Third Party Includes
#include <pqxx/pqxx>

class UserCredentialsRepository {
public:
  void create(pqxx::transaction_base &txn, int user_id, std::string_view password_hash);

  std::optional<std::string> findByUserId(pqxx::transaction_base &txn, int user_id);

  void update(pqxx::transaction_base &txn, int user_id, std::string_view password_hash);
};
