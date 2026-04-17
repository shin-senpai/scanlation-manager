#pragma once

// User Defined Includes
#include "models/ModelChapter.hpp"
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
  int create(pqxx::transaction_base &txn, int series_id, double number, std::string_view name);

  std::optional<Chapter> findById(pqxx::transaction_base &txn, int id);

  std::optional<Chapter> findByName(pqxx::transaction_base &txn, int series_id, std::string_view name);

  std::optional<Chapter> findByNumber(pqxx::transaction_base &txn, int series_id, double number);

  std::vector<Chapter> listBySeries(pqxx::transaction_base &txn, int series_id, std::optional<std::variant<ChapterStatus, bool>> filter = std::nullopt);

  void updateStatus(pqxx::transaction_base &txn, int id, ChapterStatus status);
};
