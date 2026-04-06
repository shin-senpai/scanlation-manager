#pragma once

// User Defined Includes
#include "models/ModelRoleTask.hpp"

// Standard Includes
#include <vector>

// Third Party Includes
#include <pqxx/pqxx>

class RoleTasksRepository {
public:
  void create(pqxx::transaction_base &txn, int role_id, int task_id);

  void remove(pqxx::transaction_base &txn, int role_id, int task_id);

  std::vector<RoleTask> listByRole(pqxx::transaction_base &txn, int role_id);

  std::vector<RoleTask> listByTask(pqxx::transaction_base &txn, int task_id);
};
