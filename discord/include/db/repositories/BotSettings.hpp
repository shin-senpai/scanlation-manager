#pragma once

// Standard Includes
#include <optional>
#include <string>
#include <string_view>

// Third Party Includes
#include <pqxx/pqxx>

class BotSettingsRepository {
public:
  std::optional<std::string> get(pqxx::transaction_base &txn, std::string_view key);

  void set(pqxx::transaction_base &txn, std::string_view key, std::string_view value);

  void remove(pqxx::transaction_base &txn, std::string_view key);
};
