// Associated Header Include
#include "bot/eventHandlers/commands/manage/RoleCheck.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/BotSettings.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/User.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <iostream>
#include <string>

// Third Party Includes
#include <dpp/appcommand.h>
#include <dpp/dispatcher.h>

namespace {

void doEnable(const dpp::slashcommand_t &event, DbSession &session) {
  BotSettingsRepository settings_repo;
  if(settings_repo.get(session.wtx(), "role_check_enabled")) {
    event.edit_original_response(dpp::message("Role checking is already enabled."));
    return;
  }
  settings_repo.set(session.wtx(), "role_check_enabled", "1");
  session.commit();
  event.edit_original_response(dpp::message(
      "Role checking enabled. Users must now hold a role mapped to a task before being assigned to it."));
}

void doDisable(const dpp::slashcommand_t &event, DbSession &session) {
  BotSettingsRepository settings_repo;
  if(!settings_repo.get(session.wtx(), "role_check_enabled")) {
    event.edit_original_response(dpp::message("Role checking is already disabled."));
    return;
  }
  settings_repo.remove(session.wtx(), "role_check_enabled");
  session.commit();
  event.edit_original_response(dpp::message(
      "Role checking disabled. Users can now be assigned to any task regardless of their roles."));
}

} // namespace

void Commands::roleCheck(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  const dpp::command_interaction cmd_data = event.command.get_command_interaction();
  if(cmd_data.options.empty()) {
    return;
  }
  const std::string sub = cmd_data.options[0].name;

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

    if(sub == "enable") {
      doEnable(event, session);
    } else if(sub == "disable") {
      doDisable(event, session);
    }
  } catch(const std::exception &e) {
    std::cerr << "role-check/" << sub << " failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("An error occurred. Contact the administrator to resolve this issue."));
  }
}
