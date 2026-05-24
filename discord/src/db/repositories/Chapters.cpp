// Associated Header Include
#include "db/repositories/Chapters.hpp"

// Standard Includes
#include <string>

namespace {
Chapter rowToChapter(const pqxx::row &row) {
  return Chapter{
      row["id"].as<int>(),
      row["series_id"].as<int>(),
      row["volume"].is_null() ? std::nullopt : std::make_optional(row["volume"].as<int>()),
      row["number"].as<double>(),
      row["name"].is_null() ? std::nullopt : std::make_optional(row["name"].as<std::string>()),
      chapterStatusFromString(row["status"].as<std::string>()),
      row["added_at"].as<std::string>(),
      row["closed_at"].is_null() ? std::nullopt : std::make_optional(row["closed_at"].as<std::string>())};
}
} // namespace

int ChaptersRepository::create(pqxx::transaction_base &txn, int series_id, double number, std::optional<std::string> name, std::optional<int> volume, ChapterStatus status) {
  auto result = txn.exec(
      "INSERT INTO chapters (series_id, number, name, volume, status) VALUES ($1, $2, $3, $4, $5) RETURNING id",
      pqxx::params(txn, series_id, number, name, volume, chapterStatusToString(status)));

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
    if(filter) {
      query += std::get<bool>(*filter) ? " AND closed_at IS NOT NULL" : " AND closed_at IS NULL";
    }
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

std::vector<Chapter> ChaptersRepository::listBySeriesIds(pqxx::transaction_base &txn, const std::vector<int> &series_ids) {
  if(series_ids.empty()) {
    return {};
}

  pqxx::params params(txn);
  std::string placeholders;
  for(size_t i = 0; i < series_ids.size(); ++i) {
    if(i > 0) {
      placeholders += ",";
}
    placeholders += "$" + std::to_string(i + 1);
    params.append(series_ids[i]);
  }

  auto results = txn.exec(
      "SELECT id, series_id, volume, number, name, status, added_at, closed_at"
      " FROM chapters WHERE series_id IN (" +
          placeholders + ") ORDER BY series_id, number",
      params);

  std::vector<Chapter> chapters;
  chapters.reserve(results.size());
  for(const auto &row : results) {
    chapters.emplace_back(rowToChapter(row));
  }
  return chapters;
}

std::optional<Chapter> ChaptersRepository::findByDisplayKey(pqxx::transaction_base &txn, int series_id, std::string_view key) {
  if(auto by_name = findByName(txn, series_id, key)) {
    return by_name;
  }
  const std::string key_str(key);
  const auto ch_pos = key_str.find("Ch.");
  if(ch_pos != std::string::npos) {
    try {
      return findByNumber(txn, series_id, std::stod(key_str.substr(ch_pos + 3)));
    } catch(...) {}
  }
  return std::nullopt;
}

void ChaptersRepository::updateStatus(pqxx::transaction_base &txn, int id, ChapterStatus status) {
  txn.exec(
      "UPDATE chapters SET status = $2, closed_at = CASE WHEN $2 IN ('in_progress', 'queued', 'hiatus') THEN NULL ELSE NOW() END WHERE id = $1",
      pqxx::params(txn, id, chapterStatusToString(status)));
}

std::optional<int> ChaptersRepository::findNextQueuedId(pqxx::transaction_base &txn, int series_id, double after_number) {
  const auto result = txn.exec(
      "SELECT id FROM chapters WHERE series_id = $1 AND number > $2 AND status = 'queued'"
      " ORDER BY number ASC LIMIT 1",
      pqxx::params(txn, series_id, after_number));
  if(result.empty()) {
    return std::nullopt;
  }
  return result[0][0].as<int>();
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
      " COUNT(DISTINCT ca.task_id) FILTER ("
      "   WHERE NOT EXISTS ("
      "     SELECT 1"
      "     FROM chapter_assignments ca2"
      "     WHERE ca2.chapter_id = c.id"
      "       AND ca2.task_id = ca.task_id"
      "       AND ca2.completed_at IS NULL"
      "   )"
      " ) AS completed_tasks"
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
        row["name"].is_null() ? std::nullopt : std::make_optional(row["name"].as<std::string>()),
        chapterStatusFromString(row["status"].as<std::string>()),
        row["added_at"].as<std::string>(),
        row["closed_at"].is_null() ? std::nullopt : std::make_optional(row["closed_at"].as<std::string>()),
        row["total_tasks"].as<int>(),
        row["completed_tasks"].as<int>()});
  }

  return chapters;
}
