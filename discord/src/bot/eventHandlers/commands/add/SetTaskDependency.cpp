// Associated Header Include
#include "bot/eventHandlers/commands/add/SetTaskDependency.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/TaskDependencies.hpp"
#include "db/repositories/Tasks.hpp"
#include "db/repositories/User.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <iostream>
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

void Commands::setTaskDependency(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository identity_repo;
    UserRepository user_repo;
    TasksRepository tasks_repo;
    TaskDependenciesRepository task_deps_repo;

    const auto task_name = std::get<std::string>(event.get_parameter("task"));
    const auto depends_on_task_name = std::get<std::string>(event.get_parameter("depends_on_task"));

    const auto maybe_user_id = identity_repo.findUserIdByDiscordId(session.wtx(), discord_id);
    if(!maybe_user_id) {
      event.edit_original_response(dpp::message("You are not registered. Please run /register first."));
      return;
    }

    if(user_repo.getPermissionLevel(session.wtx(), *maybe_user_id) < Permission::supermanager) {
      event.edit_original_response(dpp::message("You lack the permission to perform this action."));
      return;
    }

    if(task_name == depends_on_task_name) {
      event.edit_original_response(dpp::message("**" + task_name + "** cannot depend on itself."));
      return;
    }

    const auto maybe_task = tasks_repo.findByName(session.wtx(), task_name);
    if(!maybe_task) {
      event.edit_original_response(dpp::message("Task **" + task_name + "** does not exist."));
      return;
    }

    const auto maybe_depends_on_task = tasks_repo.findByName(session.wtx(), depends_on_task_name);
    if(!maybe_depends_on_task) {
      event.edit_original_response(dpp::message("Task **" + task_name + "** does not exist."));
      return;
    }

    if(maybe_task->level <= maybe_depends_on_task->level) {
      event.edit_original_response(dpp::message("**" + task_name + "** cannot depend on a task that is on a higher or equal level."));
      return;
    }

    if(task_deps_repo.exists(session.wtx(), maybe_task->id, maybe_depends_on_task->id)) {
      event.edit_original_response(dpp::message("**" + task_name + "** already depends on **" + depends_on_task_name + "**."));
      return;
    }

    if(task_deps_repo.exists(session.wtx(), maybe_depends_on_task->id, maybe_task->id)) {
      event.edit_original_response(dpp::message(
          "Cannot make **" + task_name +
          "** depend on **" + depends_on_task_name +
          "** because **" + depends_on_task_name +
          "** already depends on **" + task_name + "**."));
      return;
    }

    task_deps_repo.create(session.wtx(), maybe_task->id, maybe_depends_on_task->id);
    session.commit();
    event.edit_original_response(dpp::message("**" + task_name + "** now depends on **" + depends_on_task_name + "**."));

  } catch(const std::exception &e) {
    std::cerr << "setTaskDependency failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to set Task Dependency. Contact the administrator to resolve this issue."));
  }
}

void Commands::setTaskDependencyAutocomplete(Bot &bot, const std::string &key, const std::string &input, const dpp::autocomplete_t &event) {
  dpp::interaction_response r(dpp::ir_autocomplete_reply);

  try {
    DbSession session(bot.getPool());
    TasksRepository tasks_repo;
    auto to_lower = [](std::string s) {
      std::transform(s.begin(), s.end(), s.begin(), ::tolower);
      return s;
    };

    const std::string lower_input = to_lower(input);
    if(key == "task" || key == "depends_on_task") {
      for(const auto &t : tasks_repo.listAll(session.rtx())) {
        if(lower_input.empty() || to_lower(t.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(t.name, t.name));
        }
      }
    }

  } catch(const std::exception &e) {
    std::cerr << "mapRoleTaskAutocomplete failed for key=" << key << ": " << e.what() << std::endl;
  }
  bot.getCore().interaction_response_create(event.command.id, event.command.token, r);
}