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

  void remove(pqxx::transaction_base &txn, int id);

  void retire(pqxx::transaction_base &txn, int id);

  void unretire(pqxx::transaction_base &txn, int id);

  std::optional<Task> findById(pqxx::transaction_base &txn, int id);

  std::optional<Task> findByName(pqxx::transaction_base &txn, std::string_view name);

  // By default excludes retired tasks.
  std::vector<Task> listAll(pqxx::transaction_base &txn, bool include_retired = false);
};
