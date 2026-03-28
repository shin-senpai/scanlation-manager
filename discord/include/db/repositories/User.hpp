#pragma once

// User Defined Includes
#include "models/ModelUser.hpp"

// Standard Includes
#include <optional>
#include <string>

// Third Party Includes
#include <pqxx/pqxx>

class UserRepository {
public:
  // Creates a user with only display_name set (Discord registration path).
  // Returns the new user's id.
  int create(pqxx::work &txn, const std::string &display_name);
  
  std::optional<User> findById(pqxx::read_transaction &txn, int id);
};