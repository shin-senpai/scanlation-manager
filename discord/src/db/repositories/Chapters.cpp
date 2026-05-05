// Associated Header Include
#include "db/repositories/Chapters.hpp"

namespace {
Chapter rowToChapter(const pqxx::row &row) {
  return Chapter{
      row["id"].as<int>(),
      row["series_id"].as<int>(),
      row["volume"].is_null() ? std::nullopt : std::make_optional(row["volume"].as<int>()),
      row["number"].as<double>(),
      row["name"].as<std::string>(),
      chapterStatusFromString(row["status"].as<std::string>()),
      row["added_at"].as<std::string>(),
      row["closed_at"].is_null() ? std::nullopt : std::make_optional(row["closed_at"].as<std::string>())};
}
}

int ChaptersRepository::create(pqxx::transaction_base &txn, int series_id, double number, std::string_view name, std::optional<int> volume) {
  auto result = txn.exec(
      "INSERT INTO chapters (series_id, number, name, volume) VALUES ($1, $2, $3, $4) RETURNING id",
      pqxx::params(txn, series_id, number, name, volume));

  return result[0]["id"].as<int>();
}

std::optional<Chapter> ChaptersRepository::findById(pqxx::transaction_base &txn, int id) {
  auto result = txn.exec(
      "SELECT id, series_id, volume, number, name, status, added_at, closed_at FROM chapters WHERE id = $1",
      pqxx::params(txn, id));

  if(result.empty()) {
    return std::nullopt;
  }

  return rowToChapter(result[0]);
}

std::optional<Chapter> ChaptersRepository::findByName(pqxx::transaction_base &txn, int series_id, std::string_view name) {
  auto result = txn.exec(
      "SELECT id, series_id, volume, number, name, status, added_at, closed_at FROM chapters WHERE series_id = $1 AND name = $2",
      pqxx::params(txn, series_id, name));

  if(result.empty()) {
    return std::nullopt;
  }

  return rowToChapter(result[0]);
}

std::optional<Chapter> ChaptersRepository::findByNumber(pqxx::transaction_base &txn, int series_id, double number) {
  auto result = txn.exec(
      "SELECT id, series_id, volume, number, name, status, added_at, closed_at FROM chapters WHERE series_id = $1 AND number = $2",
      pqxx::params(txn, series_id, number));

  if(result.empty()) {
    return std::nullopt;
  }

  return rowToChapter(result[0]);
}

std::vector<Chapter> ChaptersRepository::listBySeries(pqxx::transaction_base &txn, int series_id, std::optional<std::variant<ChapterStatus, bool>> filter) {
  std::string query = "SELECT id, series_id, volume, number, name, status, added_at, closed_at FROM chapters WHERE series_id = $1";
  pqxx::result results;

  if(filter && std::holds_alternative<ChapterStatus>(*filter)) {
    query += " AND status = $2 ORDER BY number";
    results = txn.exec(query, pqxx::params(txn, series_id, chapterStatusToString(std::get<ChapterStatus>(*filter))));
  } else {
    if(filter) query += std::get<bool>(*filter) ? " AND closed_at IS NOT NULL" : " AND closed_at IS NULL";
    query += " ORDER BY number";
    results = txn.exec(query, pqxx::params(txn, series_id));
  }

  std::vector<Chapter> chapters;
  chapters.reserve(results.size());
  for(const auto &row : results) {
    chapters.emplace_back(rowToChapter(row));
  }

  return chapters;
}

void ChaptersRepository::updateStatus(pqxx::transaction_base &txn, int id, ChapterStatus status) {
  txn.exec(
      "UPDATE chapters SET status = $2, closed_at = CASE WHEN $2 IN ('in_progress', 'hiatus') THEN NULL ELSE NOW() END WHERE id = $1",
      pqxx::params(txn, id, chapterStatusToString(status)));
}

void ChaptersRepository::remove(pqxx::transaction_base &txn, int chapter_id) {
  txn.exec(
      "DELETE FROM chapters WHERE id = $1",
      pqxx::params(txn, chapter_id));
}

std::vector<ChapterWithStats> ChaptersRepository::listWithStats(
    pqxx::transaction_base &txn,
    std::optional<int> series_id,
    std::optional<ChapterStatus> status_filter,
    bool sort_chronological) {
  std::string where_clause;
  if(series_id && status_filter) {
    where_clause = " WHERE c.series_id = $1 AND c.status::text = $2";
  } else if(series_id) {
    where_clause = " WHERE c.series_id = $1";
  } else if(status_filter) {
    where_clause = " WHERE c.status::text = $1";
  }

  const std::string order_clause = sort_chronological
                                       ? " ORDER BY c.closed_at DESC NULLS LAST, c.number ASC"
                                       : " ORDER BY c.number ASC";

  const std::string query =
      "SELECT c.id, c.series_id, c.volume, c.number, c.name, c.status,"
      " c.added_at, TO_CHAR(c.closed_at, 'YYYY-MM-DD') AS closed_at,"
      " s.name AS series_name,"
      " COUNT(DISTINCT ca.task_id) AS total_tasks,"
      " COUNT(DISTINCT CASE WHEN ca.completed_at IS NOT NULL THEN ca.task_id END) AS completed_tasks"
      " FROM chapters c"
      " JOIN series s ON s.id = c.series_id"
      " LEFT JOIN chapter_assignments ca ON ca.chapter_id = c.id" +
      where_clause +
      " GROUP BY c.id, c.series_id, c.volume, c.number, c.name, c.status,"
      " c.added_at, c.closed_at, s.name" +
      order_clause;

  pqxx::result results;
  if(series_id && status_filter) {
    results = txn.exec(query, pqxx::params(txn, *series_id, chapterStatusToString(*status_filter)));
  } else if(series_id) {
    results = txn.exec(query, pqxx::params(txn, *series_id));
  } else if(status_filter) {
    results = txn.exec(query, pqxx::params(txn, chapterStatusToString(*status_filter)));
  } else {
    results = txn.exec(query);
  }

  std::vector<ChapterWithStats> chapters;
  chapters.reserve(results.size());
  for(const auto &row : results) {
    chapters.emplace_back(ChapterWithStats{
        row["id"].as<int>(),
        row["series_id"].as<int>(),
        row["series_name"].as<std::string>(),
        row["volume"].is_null() ? std::nullopt : std::make_optional(row["volume"].as<int>()),
        row["number"].as<double>(),
        row["name"].as<std::string>(),
        chapterStatusFromString(row["status"].as<std::string>()),
        row["added_at"].as<std::string>(),
        row["closed_at"].is_null() ? std::nullopt : std::make_optional(row["closed_at"].as<std::string>()),
        row["total_tasks"].as<int>(),
        row["completed_tasks"].as<int>()});
  }

  return chapters;
}

