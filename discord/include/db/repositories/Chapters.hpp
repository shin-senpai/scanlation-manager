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
  int create(pqxx::transaction_base &txn, int series_id, double number, std::string_view name, std::optional<int> volume = std::nullopt);

  std::optional<Chapter> findById(pqxx::transaction_base &txn, int id);

  std::optional<Chapter> findByName(pqxx::transaction_base &txn, int series_id, std::string_view name);

  std::optional<Chapter> findByNumber(pqxx::transaction_base &txn, int series_id, double number);

  std::vector<Chapter> listBySeries(pqxx::transaction_base &txn, int series_id, std::optional<std::variant<ChapterStatus, bool>> filter = std::nullopt);

  std::vector<ChapterWithStats> listWithStats(
      pqxx::transaction_base &txn,
      std::optional<int> series_id = std::nullopt,
      std::optional<ChapterStatus> status_filter = std::nullopt,
      bool sort_chronological = false);

  void updateStatus(pqxx::transaction_base &txn, int id, ChapterStatus status);

  void remove(pqxx::transaction_base &txn, int chapter_id);
};
