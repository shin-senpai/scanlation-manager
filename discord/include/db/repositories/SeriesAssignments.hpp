#pragma once

// User Defined Includes
#include "models/ModelAssignmentDetail.hpp"
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

  bool exists(pqxx::transaction_base &txn, int user_id, int series_id, int task_id);

  void removeAllByTask(pqxx::transaction_base &txn, int task_id);

  // Returns distinct series names that have a series-level or chapter-level assignment
  // for the given task. Must be called before deletion so cascade hasn't removed the rows yet.
  std::vector<std::string> listSeriesNamesByTask(pqxx::transaction_base &txn, int task_id);

  // Returns distinct series names that have a series-level or chapter-level assignment
  // for the given user. Must be called before deletion so cascade hasn't removed the rows yet.
  std::vector<std::string> listSeriesNamesByUser(pqxx::transaction_base &txn, int user_id);

  std::vector<SeriesAssignment> listBySeries(pqxx::transaction_base &txn, int series_id, std::optional<int> task_id = std::nullopt);

  std::vector<SeriesAssignment> listByUser(pqxx::transaction_base &txn, int user_id, std::optional<int> task_id = std::nullopt);

  std::vector<int> listDistinctSeriesByUser(pqxx::transaction_base &txn, int user_id);

  std::vector<CrewDetail> listBySeriesWithDetails(pqxx::transaction_base &txn, int series_id);
};
