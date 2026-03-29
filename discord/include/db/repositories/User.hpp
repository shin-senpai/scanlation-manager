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
      pqxx::work &txn,
      const std::string_view &display_name,
      bool is_manager = false,
      bool is_supermanager = false);

  std::vector<User> listUsers(pqxx::read_transaction &txn);

  std::optional<User> findById(pqxx::read_transaction &txn, int id);
};