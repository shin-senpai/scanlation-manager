#pragma once

// User Defined Includes
#include "models/ModelUser.hpp"

// Standard Includes
#include <optional>
#include <string_view>

// Third Party Includes
#include <pqxx/pqxx>

class UserRepository {
public:
  // Creates a user with only display_name set (Discord registration path).
  // Returns the new user's id.
  int create(
      pqxx::transaction_base &txn,
      const std::string_view &display_name,
      Permission permission_level = Permission::standard);

  std::vector<User> listUsers(pqxx::transaction_base &txn);

  std::optional<User> findById(pqxx::transaction_base &txn, int id);
};