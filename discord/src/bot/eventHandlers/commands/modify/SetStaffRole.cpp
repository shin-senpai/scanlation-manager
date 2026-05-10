// Associated Header Include
#include "bot/eventHandlers/commands/modify/SetStaffRole.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/User.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <algorithm>
#include <exception>
#include <iostream>
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>

void Commands::setStaffRole(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository discord_id_repo;
    UserRepository user_repo;

    const auto maybe_user_id = discord_id_repo.findUserIdByDiscordId(session.wtx(), discord_id);
    if(!maybe_user_id) {
      event.edit_original_response(dpp::message("User not Found! Please try after running the /register command"));
      return;
    }
    const int64_t user_id = *maybe_user_id;

    Permission permission_level = user_repo.getPermissionLevel(session.wtx(), user_id);
    if(permission_level < Permission::supermanager) {
      event.edit_original_response(dpp::message("You lack the permission to perform this action"));
      return;
    }

    dpp::snowflake role_id = std::get<dpp::snowflake>(event.get_parameter("role"));
    if(role_id == bot.getStaffRole()) {
      event.edit_original_response(dpp::message("That Role has already been set as the Staff Role"));
      return;
    }
    bot.setStaffRole(role_id);

    dpp::guild *guild = dpp::find_guild(event.command.guild_id);
    if(!guild || guild->members.empty()) {
      event.edit_original_response(dpp::message("Staff Role updated, but guild members are not in cache — auto-registration skipped."));
      return;
    }

    int registered = 0;
    int skipped = 0;

    for(const auto &[_, member] : guild->members) {
      const auto &roles = member.get_roles();
      if(std::find(roles.begin(), roles.end(), role_id) == roles.end()) {
        continue;
}

      dpp::user *u = member.get_user();
      if(!u || u->is_bot()) {
        continue;
}

      const int64_t member_discord_id = static_cast<int64_t>(member.user_id);

      try {
        DiscordIdentityRepository id_repo;
        UserRepository u_repo;

        if(id_repo.findUserIdByDiscordId(session.wtx(), member_discord_id)) {
          ++skipped;
          continue;
        }

        const int new_user_id = u_repo.create(session.wtx(), u->username);
        id_repo.create(session.wtx(), member_discord_id, new_user_id);
        session.commit();
        ++registered;
      } catch(const std::exception &e) {
        std::cerr << "Auto-register failed for member (" << member_discord_id << "): " << e.what() << std::endl;
      }
    }

    std::string msg = "Staff Role updated! Auto-registered " + std::to_string(registered) + " member(s)";
    if(skipped > 0) {
      msg += " (" + std::to_string(skipped) + " already registered)";
}
    msg += ".";
    event.edit_original_response(dpp::message(msg));

  } catch(std::exception &e) {
    event.edit_original_response(dpp::message("Failed to set Staff Role. Contact the administrator to resolve this issue"));
    std::cerr << "Staff Role was failed to be set by user (" << discord_id << ") due to exception: " << e.what() << std::endl;
  }
}