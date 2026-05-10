// Associated Header Include
#include "bot/eventHandlers/commands/add/AddTask.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/Tasks.hpp"
#include "db/repositories/User.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <iostream>
#include <limits>
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

void Commands::addTask(Bot &bot, const dpp::slashcommand_t &event) {
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

    // The reason we do this roundabout method of getting an int is because the std::variant inside event.get_parameter(...) doesn't support int as a valid type
    // but our DB stores the level as an INT, so that's why we have this
    const auto &param = event.get_parameter("level");
    int level{};
    if(const auto p = std::get_if<long>(&param)) {
      if(*p >= std::numeric_limits<int>::min() && *p <= std::numeric_limits<int>::max()) {
        level = *p;
      } else {
        event.edit_original_response(dpp::message("Level has to be between **" + std::to_string(std::numeric_limits<int>::min()) + "** and **" + std::to_string(std::numeric_limits<int>::max()) + "**."));
        return;
      }
    } else {
      event.edit_original_response(dpp::message("You provided an invalid level."));
      return;
    }

    const int task_id = tasks_repo.create(session.wtx(), name, level);
    session.commit();

    event.edit_original_response(dpp::message("Task **" + name + "** created with ID `" + std::to_string(task_id) + "`."));
  } catch(const pqxx::unique_violation &) {
    event.edit_original_response(dpp::message("A task with that name already exists."));
  } catch(const std::exception &e) {
    std::cerr << "addTask failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to create task. Contact the administrator to resolve this issue."));
  }
}
