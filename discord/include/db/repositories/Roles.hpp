#pragma once

// User Defined Includes
#include "models/ModelRole.hpp"

// Standard Includes
#include <optional>
#include <string_view>
#include <vector>

// Third Party Includes
#include <pqxx/pqxx>

class RolesRepository {
public:
  int create(pqxx::transaction_base &txn, std::string_view name);

  std::optional<Role> findById(pqxx::transaction_base &txn, int id);

  std::optional<Role> findByName(pqxx::transaction_base &txn, std::string_view name);

  std::vector<Role> listAll(pqxx::transaction_base &txn);
};
