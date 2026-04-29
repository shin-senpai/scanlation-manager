// Associated Header Include
#include "bot/eventHandlers/commands/modify/RetireTask.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/Tasks.hpp"
#include "db/repositories/User.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <iostream>
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

void Commands::retireTask(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository identity_repo;
    UserRepository user_repo;
    TasksRepository tasks_repo;

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

    const std::string name = std::get<std::string>(event.get_parameter("name"));

    const auto maybe_task = tasks_repo.findByName(session.wtx(), name);
    if(!maybe_task) {
      event.edit_original_response(dpp::message("Task **" + name + "** does not exist."));
      return;
    }

    if(maybe_task->retired_at) {
      event.edit_original_response(dpp::message("Task **" + name + "** is already retired."));
      return;
    }

    tasks_repo.retire(session.wtx(), maybe_task->id);
    session.commit();

    event.edit_original_response(dpp::message("Task **" + name + "** retired. Completion history is preserved."));
  } catch(const std::exception &e) {
    std::cerr << "retireTask failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to retire task. Contact the administrator to resolve this issue."));
  }
}
