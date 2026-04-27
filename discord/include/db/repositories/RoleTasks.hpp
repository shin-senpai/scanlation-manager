#pragma once

// User Defined Includes
#include "db/repositories/Series.hpp"
#include "models/ModelRoleTask.hpp"

// Standard Includes
#include <vector>

// Third Party Includes
#include <pqxx/pqxx>

class RoleTasksRepository {
public:
  void create(pqxx::transaction_base &txn, int role_id, int task_id);

  void remove(pqxx::transaction_base &txn, int role_id, int task_id);

  bool exists(pqxx::transaction_base &txn, int role_id, int task_id);

  void removeAllByRole(pqxx::transaction_base &txn, int role_id);

  void removeAllByTask(pqxx::transaction_base &txn, int task_id);

  std::vector<int> listTaskIdsByRole(pqxx::transaction_base &txn, int role_id);

  std::vector<int> listRoleIdsByTask(pqxx::transaction_base &txn, int task_id);

  std::vector<RoleTask> listAll(pqxx::transaction_base &txn);
};
