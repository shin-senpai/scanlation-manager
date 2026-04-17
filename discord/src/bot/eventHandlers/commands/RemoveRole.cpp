// Associated Header Include
#include "bot/eventHandlers/commands/RemoveRole.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/Roles.hpp"
#include "db/repositories/User.hpp"
#include "db/repositories/UserRoles.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <iostream>
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <dpp/snowflake.h>
#include <pqxx/pqxx>

void Commands::removeRole(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository identity_repo;
    UserRepository user_repo;
    RolesRepository roles_repo;
    UserRolesRepository user_roles_repo;

    const auto maybe_user_id = identity_repo.findUserIdByDiscordId(session.wtx(), discord_id);
    if(!maybe_user_id) {
      event.edit_original_response(dpp::message("You are not registered. Please run /register first."));
      return;
    }
    const int64_t user_id = *maybe_user_id;

    if(user_repo.getPermissionLevel(session.wtx(), user_id) < Permission::manager) {
      event.edit_original_response(dpp::message("You lack the permission to perform this action."));
      return;
    }

    const dpp::snowflake target_discord_id = std::get<dpp::snowflake>(event.get_parameter("user"));
    const std::string role_name = std::get<std::string>(event.get_parameter("role"));

    const auto maybe_target_id = identity_repo.findUserIdByDiscordId(session.wtx(), static_cast<int64_t>(target_discord_id));
    if(!maybe_target_id) {
      event.edit_original_response(dpp::message("That user is not registered."));
      return;
    }
    const int64_t target_user_id = *maybe_target_id;

    const auto maybe_role = roles_repo.findByName(session.wtx(), role_name);
    if(!maybe_role) {
      event.edit_original_response(dpp::message("Role **" + role_name + "** does not exist."));
      return;
    }

    const auto assigned_roles = user_roles_repo.listByUser(session.wtx(), target_user_id);
    bool has_role = false;
    for(const auto &ur : assigned_roles) {
      if(ur.role_id == maybe_role->id) {
        has_role = true;
        break;
      }
    }

    if(!has_role) {
      event.edit_original_response(dpp::message("<@" + std::to_string(target_discord_id) + "> does not have the role **" + role_name + "**."));
      return;
    }

    user_roles_repo.remove(session.wtx(), target_user_id, maybe_role->id);
    session.commit();

    event.edit_original_response(dpp::message("Role **" + role_name + "** removed from <@" + std::to_string(target_discord_id) + ">."));
  } catch(const std::exception &e) {
    std::cerr << "removeRole failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to remove role. Contact the administrator to resolve this issue."));
  }
}
