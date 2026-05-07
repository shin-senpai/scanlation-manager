// Associated Header Include
#include "db/repositories/ChapterAssignments.hpp"
#include "models/ModelChapterAssignment.hpp"
#include <string>

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

bool ChapterAssignmentsRepository::hasCompletedByChapter(pqxx::transaction_base &txn, int chapter_id) {
  auto result = txn.exec(
      "SELECT 1 FROM chapter_assignments WHERE chapter_id = $1 AND completed_at IS NOT NULL LIMIT 1",
      pqxx::params(txn, chapter_id));

  return !result.empty();
}

bool ChapterAssignmentsRepository::hasCompletedBySeries(pqxx::transaction_base &txn, int series_id) {
  auto result = txn.exec(
      "SELECT 1 FROM chapter_assignments ca "
      "JOIN chapters c ON c.id = ca.chapter_id "
      "WHERE c.series_id = $1 AND ca.completed_at IS NOT NULL LIMIT 1",
      pqxx::params(txn, series_id));

  return !result.empty();
}

void ChapterAssignmentsRepository::clearAllCompletedByChapter(pqxx::transaction_base &txn, int chapter_id) {
  txn.exec(
      "UPDATE chapter_assignments SET completed_at = NULL WHERE chapter_id = $1",
      pqxx::params(txn, chapter_id));
}

void ChapterAssignmentsRepository::clearAllCompletedBySeries(pqxx::transaction_base &txn, int series_id) {
  txn.exec(
      "UPDATE chapter_assignments SET completed_at = NULL "
      "WHERE chapter_id IN (SELECT id FROM chapters WHERE series_id = $1)",
      pqxx::params(txn, series_id));
}

std::vector<ChapterAssignment> ChapterAssignmentsRepository::listByChapter(pqxx::transaction_base &txn, int chapter_id, std::optional<int> task_id, std::optional<bool> completed) {
  std::string query = "SELECT user_id, chapter_id, task_id, completed_at FROM chapter_assignments WHERE chapter_id = $1";
  if(completed) {
    query += *completed ? " AND completed_at IS NOT NULL" : " AND completed_at IS NULL";
}
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
  if(completed) {
    query += *completed ? " AND completed_at IS NOT NULL" : " AND completed_at IS NULL";
}
  pqxx::result results;
  if(task_id) {
    query += " AND task_id = $2";
    results = txn.exec(query, pqxx::params(txn, user_id, *task_id));
  } else {
    results = txn.exec(query, pqxx::params(txn, user_id));
  }

  return buildResults(results);
}

std::vector<ChapterAssignment> ChapterAssignmentsRepository::listBySeries(pqxx::transaction_base &txn, std::vector<int> series_ids, std::optional<std::vector<int>> chapter_ids, std::optional<bool> completed) {
  if(series_ids.empty()) {
    return {};
}

  pqxx::params params(txn);
  int param_idx = 1;

  std::string series_placeholders;
  for(size_t i = 0; i < series_ids.size(); ++i) {
    if(i > 0) {
      series_placeholders += ",";
}
    series_placeholders += "$" + std::to_string(param_idx++);
    params.append(series_ids[i]);
  }

  std::string chapter_placeholders;
  if(chapter_ids && !chapter_ids->empty()) {
    for(size_t i = 0; i < chapter_ids->size(); ++i) {
      if(i > 0) {
        chapter_placeholders += ",";
}
      chapter_placeholders += "$" + std::to_string(param_idx++);
      params.append((*chapter_ids)[i]);
    }
  }

  std::string query =
      "SELECT user_id, chapter_id, task_id, completed_at"
      " FROM chapters"
      " JOIN chapter_assignments ON chapters.id = chapter_assignments.chapter_id"
      " WHERE chapters.series_id IN (" +
      series_placeholders + ")";

  if(!chapter_placeholders.empty()) {
    query += " AND chapters.id IN (" + chapter_placeholders + ")";
}

  if(completed) {
    query += *completed ? " AND completed_at IS NOT NULL" : " AND completed_at IS NULL";
}

  auto results = txn.exec(query, params);

  return buildResults(results);
}

std::vector<int> ChapterAssignmentsRepository::listDistinctSeriesByUser(pqxx::transaction_base &txn, int user_id, std::optional<bool> completed) {
  std::string query =
      "SELECT DISTINCT series_id"
      " FROM chapters"
      " JOIN chapter_assignments ON chapters.id = chapter_assignments.chapter_id"
      " WHERE chapter_assignments.user_id = $1";

  if(completed) {
    query += (*completed) ? " AND completed_at IS NOT NULL" : " AND completed_at IS NULL";
  }

  auto results = txn.exec(query, pqxx::params(txn, user_id));

  std::vector<int> series;
  series.reserve(results.size());
  for(const auto &row : results) {
    series.emplace_back(row["series_id"].as<int>());
  }

  return series;
}

bool ChapterAssignmentsRepository::exists(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id, std::optional<bool> completed) {
  std::string query = "SELECT 1 FROM chapter_assignments WHERE user_id = $1 AND chapter_id = $2 AND task_id = $3";
  if(completed) {
    query += *completed ? " AND completed_at IS NOT NULL" : " AND completed_at IS NULL";
}
  query += " LIMIT 1";

  auto result = txn.exec(query, pqxx::params(txn, user_id, chapter_id, task_id));
  return !result.empty();
}

void ChapterAssignmentsRepository::setCompleted(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id) {
  txn.exec(
      "UPDATE chapter_assignments SET completed_at = NOW() WHERE user_id = $1 AND chapter_id = $2 AND task_id = $3",
      pqxx::params(txn, user_id, chapter_id, task_id));
}

void ChapterAssignmentsRepository::clearCompleted(pqxx::transaction_base &txn, int user_id, int chapter_id, int task_id) {
  txn.exec(
      "UPDATE chapter_assignments SET completed_at = NULL WHERE user_id = $1 AND chapter_id = $2 AND task_id = $3",
      pqxx::params(txn, user_id, chapter_id, task_id));
}

std::vector<AssignmentDetail> ChapterAssignmentsRepository::listByChapterWithDetails(pqxx::transaction_base &txn, int chapter_id) {
  auto results = txn.exec(
      "SELECT t.name AS task_name, u.display_name AS user_display,"
      " TO_CHAR(ca.completed_at, 'YYYY-MM-DD') AS completed_at"
      " FROM chapter_assignments ca"
      " JOIN tasks t ON t.id = ca.task_id"
      " JOIN users u ON u.id = ca.user_id"
      " WHERE ca.chapter_id = $1"
      " ORDER BY t.name ASC, u.display_name ASC",
      pqxx::params(txn, chapter_id));

  std::vector<AssignmentDetail> assignments;
  assignments.reserve(results.size());
  for(const auto &row : results) {
    assignments.emplace_back(AssignmentDetail{
        row["task_name"].as<std::string>(),
        row["user_display"].as<std::string>(),
        row["completed_at"].is_null() ? std::nullopt : std::make_optional(row["completed_at"].as<std::string>())});
  }

  return assignments;
}
