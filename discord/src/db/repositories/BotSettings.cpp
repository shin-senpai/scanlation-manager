// Associated Header Include
#include "db/repositories/BotSettings.hpp"

std::optional<std::string> BotSettingsRepository::get(pqxx::transaction_base &txn, std::string_view key) {
  auto result = txn.exec(
      "SELECT value FROM bot_settings WHERE key = $1",
      pqxx::params(txn, std::string(key)));

  if(result.empty()) {
    return std::nullopt;
  }
  return result[0]["value"].as<std::string>();
}

void BotSettingsRepository::set(pqxx::transaction_base &txn, std::string_view key, std::string_view value) {
  txn.exec(
      "INSERT INTO bot_settings (key, value) VALUES ($1, $2)"
      " ON CONFLICT (key) DO UPDATE SET value = EXCLUDED.value",
      pqxx::params(txn, std::string(key), std::string(value)));
}

void BotSettingsRepository::remove(pqxx::transaction_base &txn, std::string_view key) {
  txn.exec(
      "DELETE FROM bot_settings WHERE key = $1",
      pqxx::params(txn, std::string(key)));
}
