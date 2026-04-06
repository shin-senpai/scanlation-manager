// Associated Header Include
#include "db/repositories/SeriesAssignments.hpp"

void SeriesAssignmentsRepository::create(pqxx::transaction_base &txn, int user_id, int series_id, int task_id) {
  txn.exec(
      "INSERT INTO series_assignments (user_id, series_id, task_id) VALUES ($1, $2, $3)",
      pqxx::params(txn, user_id, series_id, task_id));
}

void SeriesAssignmentsRepository::remove(pqxx::transaction_base &txn, int user_id, int series_id, int task_id) {
  txn.exec(
      "DELETE FROM series_assignments WHERE user_id = $1 AND series_id = $2 AND task_id = $3",
      pqxx::params(txn, user_id, series_id, task_id));
}

std::vector<SeriesAssignment> SeriesAssignmentsRepository::listBySeries(pqxx::transaction_base &txn, int series_id, std::optional<int> task_id) {
  std::string query = "SELECT user_id, series_id, task_id FROM series_assignments WHERE series_id = $1";
  pqxx::result results;
  if(task_id) {
    query += " AND task_id = $2";
    results = txn.exec(query, pqxx::params(txn, series_id, *task_id));
  } else {
    results = txn.exec(query, pqxx::params(txn, series_id));
  }

  std::vector<SeriesAssignment> assignments;
  assignments.reserve(results.size());
  for(const auto &row : results) {
    assignments.emplace_back(row["user_id"].as<int>(), row["series_id"].as<int>(), row["task_id"].as<int>());
  }

  return assignments;
}

std::vector<SeriesAssignment> SeriesAssignmentsRepository::listByUser(pqxx::transaction_base &txn, int user_id, std::optional<int> task_id) {
  std::string query = "SELECT user_id, series_id, task_id FROM series_assignments WHERE user_id = $1";
  pqxx::result results;
  if(task_id) {
    query += " AND task_id = $2";
    results = txn.exec(query, pqxx::params(txn, user_id, *task_id));
  } else {
    results = txn.exec(query, pqxx::params(txn, user_id));
  }

  std::vector<SeriesAssignment> assignments;
  assignments.reserve(results.size());
  for(const auto &row : results) {
    assignments.emplace_back(row["user_id"].as<int>(), row["series_id"].as<int>(), row["task_id"].as<int>());
  }

  return assignments;
}
