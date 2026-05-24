#pragma once

// User Defined Includes
#include "models/ModelChapter.hpp"
#include "models/ModelChapterWithStats.hpp"
#include "types/ChapterStatus.hpp"

// Standard Includes
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

// Third Party Includes
#include <pqxx/pqxx>

class ChaptersRepository {
public:
  int create(pqxx::transaction_base &txn, int series_id, double number, std::optional<std::string> name = std::nullopt, std::optional<int> volume = std::nullopt, ChapterStatus status = ChapterStatus::in_progress);

  // Returns the ID of the lowest-numbered chapter after `after_number` in the series
  // that has `queued` status, or nullopt if none exists.
  std::optional<int> findNextQueuedId(pqxx::transaction_base &txn, int series_id, double after_number);

  std::optional<Chapter> findById(pqxx::transaction_base &txn, int id);

  std::optional<Chapter> findByName(pqxx::transaction_base &txn, int series_id, std::string_view name);

  std::optional<Chapter> findByNumber(pqxx::transaction_base &txn, int series_id, double number);

  std::vector<Chapter> listBySeries(pqxx::transaction_base &txn, int series_id, std::optional<std::variant<ChapterStatus, bool>> filter = std::nullopt);

  std::vector<Chapter> listBySeriesIds(pqxx::transaction_base &txn, const std::vector<int> &series_ids);

  std::vector<ChapterWithStats> listWithStats(
      pqxx::transaction_base &txn,
      std::optional<int> series_id = std::nullopt,
      std::optional<ChapterStatus> status_filter = std::nullopt,
      bool sort_chronological = false);

  // Looks up a chapter by name, falling back to "Ch.X" / "Vol.X Ch.X" number format for unnamed chapters.
  std::optional<Chapter> findByDisplayKey(pqxx::transaction_base &txn, int series_id, std::string_view key);

  void updateStatus(pqxx::transaction_base &txn, int id, ChapterStatus status);

  void remove(pqxx::transaction_base &txn, int chapter_id);
};
