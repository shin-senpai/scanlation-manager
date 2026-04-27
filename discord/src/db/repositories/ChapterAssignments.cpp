// Associated Header Include
#include "db/repositories/ChapterAssignments.hpp"

static std::vector<ChapterAssignment> buildResults(const pqxx::result &results) {
  std::vector<ChapterAssignment> assignments;
  assignments.reserve(results.size());
  for(const auto &row : results) {
    assignments.emplace_back(
        row["user_id"].as<int>(),
        row["chapter_id"].as<int>(),
        row["task_id"].as<int>(),
        row["completed_at"].is_null() ? std::nullopt : std::make_optional(row["completed_at"].as<std::string>()));
  }
  return assignments;
}

void ChapterAssignmentsRepository::create(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id) {
  txn.exec(
      "INSERT INTO chapter_assignments (user_id, chapter_id, task_id) VALUES ($1, $2, $3)",
      pqxx::params(txn, user_id, chapter_id, task_id));
}

void ChapterAssignmentsRepository::remove(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id) {
  txn.exec(
      "DELETE FROM chapter_assignments WHERE user_id = $1 AND chapter_id = $2 AND task_id = $3",
      pqxx::params(txn, user_id, chapter_id, task_id));
}

void ChapterAssignmentsRepository::removeOutstandingByTask(pqxx::transaction_base &txn, int task_id) {
  txn.exec(
      "DELETE FROM chapter_assignments WHERE task_id = $1 AND completed_at IS NULL",
      pqxx::params(txn, task_id));
}

bool ChapterAssignmentsRepository::hasCompletedByTask(pqxx::transaction_base &txn, int task_id) {
  auto result = txn.exec(
      "SELECT 1 FROM chapter_assignments WHERE task_id = $1 AND completed_at IS NOT NULL LIMIT 1",
      pqxx::params(txn, task_id));

  return !result.empty();
}

std::vector<ChapterAssignment> ChapterAssignmentsRepository::listByChapter(pqxx::transaction_base &txn, int chapter_id, std::optional<int> task_id, std::optional<bool> completed) {
  std::string query = "SELECT user_id, chapter_id, task_id, completed_at FROM chapter_assignments WHERE chapter_id = $1";
  if(completed) query += *completed ? " AND completed_at IS NOT NULL" : " AND completed_at IS NULL";
  pqxx::result results;
  if(task_id) {
    query += " AND task_id = $2";
    results = txn.exec(query, pqxx::params(txn, chapter_id, *task_id));
  } else {
    results = txn.exec(query, pqxx::params(txn, chapter_id));
  }

  return buildResults(results);
}

std::vector<ChapterAssignment> ChapterAssignmentsRepository::listByUser(pqxx::transaction_base &txn, int user_id, std::optional<int> task_id, std::optional<bool> completed) {
  std::string query = "SELECT user_id, chapter_id, task_id, completed_at FROM chapter_assignments WHERE user_id = $1";
  if(completed) query += *completed ? " AND completed_at IS NOT NULL" : " AND completed_at IS NULL";
  pqxx::result results;
  if(task_id) {
    query += " AND task_id = $2";
    results = txn.exec(query, pqxx::params(txn, user_id, *task_id));
  } else {
    results = txn.exec(query, pqxx::params(txn, user_id));
  }

  return buildResults(results);
}

bool ChapterAssignmentsRepository::exists(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id) {
  auto result = txn.exec(
      "SELECT 1 FROM chapter_assignments WHERE user_id = $1 AND chapter_id = $2 AND task_id = $3 LIMIT 1",
      pqxx::params(txn, user_id, chapter_id, task_id));

  return !result.empty();
}

void ChapterAssignmentsRepository::setCompleted(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id) {
  txn.exec(
      "UPDATE chapter_assignments SET completed_at = NOW() WHERE user_id = $1 AND chapter_id = $2 AND task_id = $3",
      pqxx::params(txn, user_id, chapter_id, task_id));
}
