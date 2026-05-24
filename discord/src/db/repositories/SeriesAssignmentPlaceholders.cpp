// Associated Header Include
#include "db/repositories/SeriesAssignmentPlaceholders.hpp"

void SeriesAssignmentPlaceholdersRepository::create(pqxx::transaction_base &txn, int series_id, int task_id) {
  txn.exec(
      "INSERT INTO series_assignment_placeholders (series_id, task_id) VALUES ($1, $2)",
      pqxx::params(series_id, task_id));
}

void SeriesAssignmentPlaceholdersRepository::removeOne(pqxx::transaction_base &txn, int series_id, int task_id) {
  txn.exec(
      "DELETE FROM series_assignment_placeholders WHERE id = ("
      "  SELECT id FROM series_assignment_placeholders"
      "  WHERE series_id = $1 AND task_id = $2 ORDER BY id LIMIT 1"
      ")",
      pqxx::params(series_id, task_id));
}

void SeriesAssignmentPlaceholdersRepository::removeAll(pqxx::transaction_base &txn, int series_id, int task_id) {
  txn.exec(
      "DELETE FROM series_assignment_placeholders WHERE series_id = $1 AND task_id = $2",
      pqxx::params(series_id, task_id));
}

int SeriesAssignmentPlaceholdersRepository::count(pqxx::transaction_base &txn, int series_id, int task_id) {
  const auto result = txn.exec(
      "SELECT COUNT(*) FROM series_assignment_placeholders WHERE series_id = $1 AND task_id = $2",
      pqxx::params(series_id, task_id));
  return result[0][0].as<int>();
}

std::vector<int> SeriesAssignmentPlaceholdersRepository::listTaskIdsBySeries(pqxx::transaction_base &txn, int series_id) {
  const auto result = txn.exec(
      "SELECT task_id FROM series_assignment_placeholders WHERE series_id = $1 ORDER BY id",
      pqxx::params(series_id));
  std::vector<int> task_ids;
  task_ids.reserve(result.size());
  for(const auto &row : result) {
    task_ids.push_back(row[0].as<int>());
  }
  return task_ids;
}

void SeriesAssignmentPlaceholdersRepository::removeAllByTask(pqxx::transaction_base &txn, int task_id) {
  txn.exec(
      "DELETE FROM series_assignment_placeholders WHERE task_id = $1",
      pqxx::params(task_id));
}
