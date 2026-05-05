// Associated Header Include
#include "db/repositories/Series.hpp"

// Standard Includes
#include <string>

namespace {
Series rowToSeries(const pqxx::row &row) {
  return Series{
      row["id"].as<int>(),
      row["name"].as<std::string>(),
      seriesStatusFromString(row["status"].as<std::string>()),
      row["added_at"].as<std::string>(),
      row["closed_at"].is_null() ? std::nullopt : std::make_optional(row["closed_at"].as<std::string>())};
}
} // namespace

int SeriesRepository::create(pqxx::transaction_base &txn, std::string_view name) {
  auto result = txn.exec(
      "INSERT INTO series (name) VALUES ($1) RETURNING id",
      pqxx::params(txn, name));

  return result[0]["id"].as<int>();
}

std::optional<Series> SeriesRepository::findById(pqxx::transaction_base &txn, int id) {
  auto result = txn.exec(
      "SELECT id, name, status, added_at, closed_at FROM series WHERE id = $1",
      pqxx::params(txn, id));

  if(result.empty()) {
    return std::nullopt;
  }

  return rowToSeries(result[0]);
}

std::optional<Series> SeriesRepository::findByName(pqxx::transaction_base &txn, std::string_view name) {
  auto result = txn.exec(
      "SELECT id, name, status, added_at, closed_at FROM series WHERE name = $1",
      pqxx::params(txn, name));

  if(result.empty()) {
    return std::nullopt;
  }

  return rowToSeries(result[0]);
}

std::vector<Series> SeriesRepository::list(pqxx::transaction_base &txn, std::optional<std::variant<SeriesStatus, bool>> filter) {
  std::string query = "SELECT id, name, status, added_at, closed_at FROM series";
  pqxx::result results;
  if(filter && std::holds_alternative<SeriesStatus>(*filter)) {
    query += " WHERE status = $1";
    results = txn.exec(query, pqxx::params(txn, seriesStatusToString(std::get<SeriesStatus>(*filter))));
  } else {
    if(filter)
      query += std::get<bool>(*filter) ? " WHERE closed_at IS NOT NULL" : " WHERE closed_at IS NULL";
    results = txn.exec(query);
  }

  std::vector<Series> series;
  series.reserve(results.size());
  for(const auto &row : results) {
    series.emplace_back(rowToSeries(row));
  }

  return series;
}

void SeriesRepository::updateStatus(pqxx::transaction_base &txn, int id, SeriesStatus status) {
  txn.exec(
      "UPDATE series SET status = $2, closed_at = CASE WHEN $2 in ('active', 'hiatus') THEN NULL ELSE NOW() END WHERE id = $1",
      pqxx::params(txn, id, seriesStatusToString(status)));
}

void SeriesRepository::remove(pqxx::transaction_base &txn, int series_id) {
  txn.exec(
      "DELETE FROM series WHERE id = $1",
      pqxx::params(txn, series_id));
}

std::vector<SeriesWithStats> SeriesRepository::listWithStats(
    pqxx::transaction_base &txn,
    std::optional<SeriesStatus> series_filter,
    std::optional<std::string> chapter_status_for_ts) {
  std::string query =
      "SELECT s.id, s.name, s.status, s.added_at, s.closed_at,"
      " COUNT(c.id) AS chapter_count,"
      " TO_CHAR(MAX(CASE WHEN $1::text IS NULL OR c.status::text = $1 THEN c.closed_at END), 'YYYY-MM-DD') AS latest_chapter_at"
      " FROM series s LEFT JOIN chapters c ON c.series_id = s.id";

  if(series_filter) {
    query += " WHERE s.status = $2";
  }

  query +=
      " GROUP BY s.id, s.name, s.status, s.added_at, s.closed_at"
      " ORDER BY MAX(CASE WHEN $1::text IS NULL OR c.status::text = $1 THEN c.closed_at END) DESC NULLS LAST, s.name ASC";

  pqxx::result results;
  if(series_filter) {
    results = txn.exec(query, pqxx::params(txn, chapter_status_for_ts, seriesStatusToString(*series_filter)));
  } else {
    results = txn.exec(query, pqxx::params(txn, chapter_status_for_ts));
  }

  std::vector<SeriesWithStats> series_list;
  series_list.reserve(results.size());
  for(const auto &row : results) {
    series_list.emplace_back(SeriesWithStats{
        row["id"].as<int>(),
        row["name"].as<std::string>(),
        seriesStatusFromString(row["status"].as<std::string>()),
        row["added_at"].as<std::string>(),
        row["closed_at"].is_null() ? std::nullopt : std::make_optional(row["closed_at"].as<std::string>()),
        row["chapter_count"].as<int>(),
        row["latest_chapter_at"].is_null() ? std::nullopt : std::make_optional(row["latest_chapter_at"].as<std::string>())});
  }

  return series_list;
}
