#pragma once

// Standard Includes
#include <cstdint>
#include <optional>

// Third Party Includes
#include <pqxx/pqxx>

class DiscordIdentityRepository {
public:
  // Links a Discord user to an existing users row.
  // Throws pqxx::unique_violation if discord_id is already registered.
  void create(pqxx::work &txn, int64_t discord_id, int user_id);

  std::optional<int> findUserIdByDiscordId(pqxx::work &txn, int64_t discord_id);
};