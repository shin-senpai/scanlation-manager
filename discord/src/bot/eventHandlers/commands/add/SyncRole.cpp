// Associated Header Include
#include "bot/eventHandlers/commands/add/SyncRole.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/Roles.hpp"
#include "db/repositories/User.hpp"
#include "db/repositories/UserRoles.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <algorithm>
#include <iostream>
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <dpp/snowflake.h>
#include <pqxx/pqxx>

void Commands::syncRole(Bot &bot, const dpp::slashcommand_t &event) {
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

    if(user_repo.getPermissionLevel(session.wtx(), *maybe_user_id) < Permission::manager) {
      event.edit_original_response(dpp::message("You lack the permission to perform this action."));
      return;
    }

    const dpp::snowflake discord_role_id = std::get<dpp::snowflake>(event.get_parameter("role"));
    const dpp::role &discord_role = event.command.get_resolved_role(discord_role_id);
    const std::string role_name = discord_role.name;

    // Create app role if it doesn't exist yet
    RolesRepository roles_repo;
    bool role_created = false;
    int app_role_id;
    const auto maybe_existing = roles_repo.findByName(session.wtx(), role_name);
    if(maybe_existing) {
      app_role_id = maybe_existing->id;
    } else {
      app_role_id = roles_repo.create(session.wtx(), role_name);
      session.commit();
      role_created = true;
    }

    // Walk the guild member cache and assign to all registered users with this Discord role
    dpp::guild *guild = dpp::find_guild(event.command.guild_id);
    if(!guild || guild->members.empty()) {
      const std::string role_msg = role_created ? "Created" : "Found existing";
      event.edit_original_response(dpp::message(
          role_msg + " app role **" + role_name + "**, but guild members are not in cache — user assignments skipped."));
      return;
    }

    int assigned = 0;
    int already_had = 0;
    int not_registered = 0;

    for(const auto &[_, member] : guild->members) {
      const auto &roles = member.get_roles();
      if(std::find(roles.begin(), roles.end(), discord_role_id) == roles.end()) continue;

      dpp::user *u = member.get_user();
      if(!u || u->is_bot()) continue;

      const int64_t member_discord_id = static_cast<int64_t>(member.user_id);

      try {
        DbSession assign_session(bot.getPool());
        DiscordIdentityRepository id_repo;
        UserRolesRepository user_roles_repo;

        const auto maybe_app_user_id = id_repo.findUserIdByDiscordId(assign_session.wtx(), member_discord_id);
        if(!maybe_app_user_id) {
          ++not_registered;
          continue;
        }

        user_roles_repo.create(assign_session.wtx(), *maybe_app_user_id, app_role_id);
        assign_session.commit();
        ++assigned;
      } catch(const pqxx::unique_violation &) {
        ++already_had;
      } catch(const std::exception &e) {
        std::cerr << "syncRole assignment failed for member (" << member_discord_id << "): " << e.what() << std::endl;
      }
    }

    const std::string role_msg = role_created ? "Created" : "Found existing";
    std::string msg = role_msg + " app role **" + role_name + "**. "
        + "Assigned to " + std::to_string(assigned) + " user(s)";
    if(already_had > 0) msg += ", " + std::to_string(already_had) + " already had it";
    if(not_registered > 0) msg += ", " + std::to_string(not_registered) + " not registered";
    msg += ".";
    event.edit_original_response(dpp::message(msg));

  } catch(const std::exception &e) {
    std::cerr << "syncRole failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to sync role. Contact the administrator to resolve this issue."));
  }
}
