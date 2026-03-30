// Associated Header Include
#include "bot//eventHandlers/commands/SetAlias.hpp"

// Standard Includes
#include <iostream>
#include <string>

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/ConnectionPool.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/UserAliases.hpp"
#include "db/utils/PqxxErrors.hpp"

// Third Party Includes
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

void Commands::setAlias(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());

    DiscordIdentityRepository identity_repo;
    UserAliasesRepository alias_repo;

    const auto maybe_user_id = identity_repo.findUserIdByDiscordId(session.rtx(), discord_id);
    if(!maybe_user_id) {
      event.edit_original_response(dpp::message("User not Found! Please try after running the /register command"));
      return;
    }
    const int64_t user_id = *maybe_user_id;

    session.closeTx(); // Closing the read transaction so that we can use the connection for a write

    std::string alias = std::get<std::string>(event.get_parameter("alias"));

    alias_repo.create(session.wtx(), user_id, alias);
    session.commit();

    event.edit_original_response(dpp::message("Your alias has been set"));
  } catch(const pqxx::unique_violation &e) {
    auto constraint = Db::Utils::extractConstraintName(e);
    if(constraint == "one_active_alias") {
      event.edit_original_response(dpp::message("This alias is already being used"));
    } else if(constraint == "one_active_alias_per_user") {
      event.edit_original_response(dpp::message("You already have an alias"));
    } else {
      std::cerr << "Alias for user (" << discord_id << ") " << "failed to be set due to exception: " << e.what() << std::endl;
      event.edit_original_response(dpp::message("Unable to set alias. Contact the administrator to resolve this issue"));
    }

  } catch(const std::exception &e) {
    std::cerr << "Alias for user (" << discord_id << ") " << "failed to be set due to exception: " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Unable to set alias. Contact the administrator to resolve this issue"));
  }
}