#pragma once

// User Defined Includes
#include "models/ModelSeries.hpp"
#include "types/SeriesStatus.hpp"

// Standard Includes
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

// Third Party Includes
#include <pqxx/pqxx>

class SeriesRepository {
public:
  int create(pqxx::transaction_base &txn, std::string_view name);

  std::optional<Series> findById(pqxx::transaction_base &txn, int id);

  std::optional<Series> findByName(pqxx::transaction_base &txn, std::string_view name);

  std::vector<Series> list(pqxx::transaction_base &txn, std::optional<std::variant<SeriesStatus, bool>> filter = std::nullopt);

  // Also updates closed_at: sets it to NOW() when status is not active, clears it when active.
  void updateStatus(pqxx::transaction_base &txn, int id, SeriesStatus status);
};
