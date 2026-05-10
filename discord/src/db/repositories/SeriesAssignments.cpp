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

bool SeriesAssignmentsRepository::exists(pqxx::transaction_base &txn, int user_id, int series_id, int task_id) {
  auto result = txn.exec(
      "SELECT 1 FROM series_assignments WHERE user_id = $1 AND series_id = $2 AND task_id = $3 LIMIT 1",
      pqxx::params(txn, user_id, series_id, task_id));

  return !result.empty();
}

void SeriesAssignmentsRepository::removeAllByTask(pqxx::transaction_base &txn, int task_id) {
  txn.exec(
      "DELETE FROM series_assignments WHERE task_id = $1",
      pqxx::params(txn, task_id));
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

std::vector<int> SeriesAssignmentsRepository::listDistinctSeriesByUser(pqxx::transaction_base &txn, int user_id) {
  auto results = txn.exec(
      "SELECT DISTINCT series_id FROM series_assignments WHERE user_id = $1",
      pqxx::params(txn, user_id));

  std::vector<int> series;
  series.reserve(results.size());
  for(const auto &row : results) {
    series.emplace_back(row["series_id"].as<int>());
  }

  return series;
}

std::vector<CrewDetail> SeriesAssignmentsRepository::listBySeriesWithDetails(pqxx::transaction_base &txn, int series_id) {
  auto results = txn.exec(
      "SELECT t.name AS task_name, u.display_name AS user_display"
      " FROM series_assignments sa"
      " JOIN tasks t ON t.id = sa.task_id"
      " JOIN users u ON u.id = sa.user_id"
      " WHERE sa.series_id = $1"
      " ORDER BY t.name ASC, u.display_name ASC",
      pqxx::params(txn, series_id));

  std::vector<CrewDetail> crew;
  crew.reserve(results.size());
  for(const auto &row : results) {
    crew.emplace_back(CrewDetail{row["task_name"].as<std::string>(), row["user_display"].as<std::string>()});
  }

  return crew;
}
