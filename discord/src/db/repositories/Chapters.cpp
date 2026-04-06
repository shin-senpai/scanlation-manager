// Associated Header Include
#include "db/repositories/Chapters.hpp"

namespace {
Chapter rowToChapter(const pqxx::row &row) {
  return Chapter{
      row["id"].as<int>(),
      row["series_id"].as<int>(),
      row["name"].as<std::string>(),
      chapterStatusFromString(row["status"].as<std::string>()),
      row["added_at"].as<std::string>(),
      row["closed_at"].is_null() ? std::nullopt : std::make_optional(row["closed_at"].as<std::string>())};
}
}

int ChaptersRepository::create(pqxx::transaction_base &txn, int series_id, std::string_view name) {
  auto result = txn.exec(
      "INSERT INTO chapters (series_id, name) VALUES ($1, $2) RETURNING id",
      pqxx::params(txn, series_id, name));

  return result[0]["id"].as<int>();
}

std::optional<Chapter> ChaptersRepository::findById(pqxx::transaction_base &txn, int id) {
  auto result = txn.exec(
      "SELECT id, series_id, name, status, added_at, closed_at FROM chapters WHERE id = $1",
      pqxx::params(txn, id));

  if(result.empty()) {
    return std::nullopt;
  }

  return rowToChapter(result[0]);
}

std::optional<Chapter> ChaptersRepository::findByName(pqxx::transaction_base &txn, int series_id, std::string_view name) {
  auto result = txn.exec(
      "SELECT id, series_id, name, status, added_at, closed_at FROM chapters WHERE series_id = $1 AND name = $2",
      pqxx::params(txn, series_id, name));

  if(result.empty()) {
    return std::nullopt;
  }

  return rowToChapter(result[0]);
}

std::vector<Chapter> ChaptersRepository::listBySeries(pqxx::transaction_base &txn, int series_id, std::optional<std::variant<ChapterStatus, bool>> filter) {
  std::string query = "SELECT id, series_id, name, status, added_at, closed_at FROM chapters WHERE series_id = $1";
  pqxx::result results;
  // if filter is empty, we don't need to dereference it
  if(filter && std::holds_alternative<ChapterStatus>(*filter)) {
    query += " AND status = $2";
    results = txn.exec(query, pqxx::params(txn, series_id, chapterStatusToString(std::get<ChapterStatus>(*filter))));
  } else {
    if(filter) query += std::get<bool>(*filter) ? " AND closed_at IS NOT NULL" : " AND closed_at IS NULL";
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
