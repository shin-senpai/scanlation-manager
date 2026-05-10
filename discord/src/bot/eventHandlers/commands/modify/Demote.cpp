// Associated Header Include
#include "bot/eventHandlers/commands/modify/Demote.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/User.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <iostream>
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <dpp/snowflake.h>
#include <pqxx/pqxx>

namespace {
std::string permissionName(Permission p) {
  switch(p) {
    case Permission::standard:
      return "Standard";
    case Permission::manager:
      return "Manager";
    case Permission::supermanager:
      return "Supermanager";
  }
  return "";
}
} // namespace

void Commands::demote(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository identity_repo;
    UserRepository user_repo;

    const auto maybe_user_id = identity_repo.findUserIdByDiscordId(session.wtx(), discord_id);
    if(!maybe_user_id) {
      event.edit_original_response(dpp::message("You are not registered. Please run /register first."));
      return;
    }

    if(user_repo.getPermissionLevel(session.wtx(), *maybe_user_id) < Permission::supermanager) {
      event.edit_original_response(dpp::message("You lack the permission to perform this action."));
      return;
    }

    const dpp::snowflake target_discord_id = std::get<dpp::snowflake>(event.get_parameter("user"));
    const auto maybe_target_id = identity_repo.findUserIdByDiscordId(session.wtx(), static_cast<int64_t>(target_discord_id));
    if(!maybe_target_id) {
      event.edit_original_response(dpp::message("That user is not registered."));
      return;
    }

    const Permission current = user_repo.getPermissionLevel(session.wtx(), *maybe_target_id);
    if(current == Permission::standard) {
      event.edit_original_response(dpp::message("<@" + std::to_string(target_discord_id) + "> is already Standard."));
      return;
    }

    const Permission next = (current == Permission::supermanager) ? Permission::manager : Permission::standard;
    user_repo.setPermissionLevel(session.wtx(), *maybe_target_id, next);
    session.commit();

    event.edit_original_response(dpp::message(
        "<@" + std::to_string(target_discord_id) + "> demoted from " +
        permissionName(current) + " to **" + permissionName(next) + "**."));
  } catch(const pqxx::sql_error &e) {
    std::cerr << "demote failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Cannot demote: at least one Supermanager must remain."));
  } catch(const std::exception &e) {
    std::cerr << "demote failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to demote user. Contact the administrator to resolve this issue."));
  }
}
