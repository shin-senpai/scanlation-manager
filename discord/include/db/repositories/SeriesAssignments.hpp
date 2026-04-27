#pragma once

// User Defined Includes
#include "models/ModelSeriesAssignment.hpp"

// Standard Includes
#include <optional>
#include <vector>

// Third Party Includes
#include <pqxx/pqxx>

class SeriesAssignmentsRepository {
public:
  void create(pqxx::transaction_base &txn, int user_id, int series_id, int task_id);

  void remove(pqxx::transaction_base &txn, int user_id, int series_id, int task_id);

  void removeAllByTask(pqxx::transaction_base &txn, int task_id);

  std::vector<SeriesAssignment> listBySeries(pqxx::transaction_base &txn, int series_id, std::optional<int> task_id = std::nullopt);

  std::vector<SeriesAssignment> listByUser(pqxx::transaction_base &txn, int user_id, std::optional<int> task_id = std::nullopt);
};
