// Associated Header Include
#include "bot/eventHandlers/commands/SetProgressChannel.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/User.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <exception>

// Third Party Includes
#include <dpp/dispatcher.h>

void Commands::setProgressChannel(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository discord_id_repo;
    UserRepository user_repo;

    const auto maybe_user_id = discord_id_repo.findUserIdByDiscordId(session.rtx(), discord_id);
    if(!maybe_user_id) {
      event.edit_original_response(dpp::message("User not Found! Please try after running the /register command"));
      return;
    }
    const int64_t user_id = *maybe_user_id;

    Permission permission_level = user_repo.getPermissionLevel(session.rtx(), user_id).value();
    if(permission_level < Permission::manager) {
      event.edit_original_response(dpp::message("You lack the permission to perform this action"));
      return;
    }

    dpp::snowflake channel_id = std::get<dpp::snowflake>(event.get_parameter("channel"));
    bot.setWorkProgressChannel(channel_id);
    event.edit_original_response(dpp::message("Work progress channel updated!"));
  } catch(std::exception &e) {
    event.edit_original_response(dpp::message("Failed to set channel for Work-Progess. Contact the administrator to resolve this issue"));
    std::cerr << "work progress channel was failed to be set by user (" << discord_id << ") due to exception: " << e.what() << std::endl;
  }
}