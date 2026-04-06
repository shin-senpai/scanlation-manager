#pragma once

// User Defined Includes
#include "models/ModelTask.hpp"

// Standard Includes
#include <optional>
#include <string_view>
#include <vector>

// Third Party Includes
#include <pqxx/pqxx>

class TasksRepository {
public:
  int create(pqxx::transaction_base &txn, std::string_view name);

  std::optional<Task> findById(pqxx::transaction_base &txn, int id);

  std::optional<Task> findByName(pqxx::transaction_base &txn, std::string_view name);

  std::vector<Task> listAll(pqxx::transaction_base &txn);
};
