// Associated Header Include
#include "db/repositories/DiscordIdentities.hpp"

void DiscordIdentityRepository::create(pqxx::work &txn, int64_t discord_id, int user_id) {
  txn.exec(
      "INSERT INTO discord_identities (discord_id, user_id) VALUES ($1, $2)",
      pqxx::params{discord_id, user_id}    
    );
}

std::optional<int> DiscordIdentityRepository::findUserIdByDiscordId(pqxx::work &txn, int64_t discord_id) {
  auto result = txn.exec(
      "SELECT user_id FROM discord_identities WHERE discord_id = $1",
      pqxx::params{discord_id}
    );

  if(result.empty()) return std::nullopt;

  return result[0][0].as<int>();
}