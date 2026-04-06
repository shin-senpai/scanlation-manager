// Associated Header Include
#include "db/repositories/Series.hpp"

namespace {
Series rowToSeries(const pqxx::row &row) {
  return Series{
      row["id"].as<int>(),
      row["name"].as<std::string>(),
      seriesStatusFromString(row["status"].as<std::string>()),
      row["added_at"].as<std::string>(),
      row["closed_at"].is_null() ? std::nullopt : std::make_optional(row["closed_at"].as<std::string>())};
}
}

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
    if(filter) query += std::get<bool>(*filter) ? " WHERE closed_at IS NOT NULL" : " WHERE closed_at IS NULL";
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
