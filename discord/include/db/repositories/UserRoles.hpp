#pragma once

// User Defined Includes
#include "models/ModelUserRole.hpp"

// Standard Includes
#include <vector>

// Third Party Includes
#include <pqxx/pqxx>

class UserRolesRepository {
public:
  void create(pqxx::transaction_base &txn, int user_id, int role_id);

  void remove(pqxx::transaction_base &txn, int user_id, int role_id);

  std::vector<UserRole> listByUser(pqxx::transaction_base &txn, int user_id);
};
