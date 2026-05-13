// Associated Header Include
#include "bot/eventHandlers/commands/list/ListTaskDeps.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/TaskDependencies.hpp"
#include "db/repositories/Tasks.hpp"
#include "db/repositories/User.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <algorithm>
#include <iostream>
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

void Commands::listTaskDeps(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository identity_repo;
    UserRepository user_repo;
    TasksRepository tasks_repo;
    TaskDependenciesRepository task_deps_repo;

    const auto maybe_user_id = identity_repo.findUserIdByDiscordId(session.rtx(), discord_id);
    if(!maybe_user_id) {
      event.edit_original_response(dpp::message("You are not registered. Please run /register first."));
      return;
    }

    if(user_repo.getPermissionLevel(session.rtx(), *maybe_user_id) < Permission::manager) {
      event.edit_original_response(dpp::message("You lack the permission to perform this action."));
      return;
    }

    const auto &task_param = event.get_parameter("task");
    const auto *task_name_ptr = std::get_if<std::string>(&task_param);

    if(!task_name_ptr || task_name_ptr->empty()) {
      const auto all = task_deps_repo.listAllWithDependencyNames(session.rtx());

      std::string out = "**Task Dependencies — All Tasks**\n\n";
      for(const auto &[tname, deps] : all) {
        out += "**" + tname + "** -> ";
        if(deps.empty()) {
          out += "(none)";
        } else {
          for(std::size_t i = 0; i < deps.size(); ++i) {
            if(i > 0) {
              out += ", ";
            }
            out += deps[i];
          }
        }
        out += "\n";
      }

      event.edit_original_response(dpp::message(out));
      return;
    }

    const std::string task_name = *task_name_ptr;

    const auto maybe_task = tasks_repo.findByName(session.rtx(), task_name);
    if(!maybe_task) {
      event.edit_original_response(dpp::message("Task **" + task_name + "** does not exist."));
      return;
    }

    const auto prerequisites = task_deps_repo.listDependencyNamesOf(session.rtx(), maybe_task->id);
    const auto dependents = task_deps_repo.listDependentNamesOf(session.rtx(), maybe_task->id);

    std::string out = "**Task Dependencies — " + task_name + "**\n";

    out += "\nPrerequisites (must complete before **" + task_name + "**):\n";
    if(prerequisites.empty()) {
      out += "(none)\n";
    } else {
      for(const auto &name : prerequisites) {
        out += "• " + name + "\n";
      }
    }

    out += "\nRequired by (tasks that need **" + task_name + "** first):\n";
    if(dependents.empty()) {
      out += "(none)\n";
    } else {
      for(const auto &name : dependents) {
        out += "• " + name + "\n";
      }
    }

    event.edit_original_response(dpp::message(out));

  } catch(const std::exception &e) {
    std::cerr << "listTaskDeps failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to list task dependencies. Contact the administrator to resolve this issue."));
  }
}

void Commands::listTaskDepsAutocomplete(Bot &bot, const std::string &key, const std::string &input, const dpp::autocomplete_t &event) {
  dpp::interaction_response r(dpp::ir_autocomplete_reply);

  try {
    DbSession session(bot.getPool());
    TasksRepository tasks_repo;

    auto to_lower = [](std::string s) {
      std::transform(s.begin(), s.end(), s.begin(), ::tolower);
      return s;
    };
    const std::string lower_input = to_lower(input);

    if(key == "task") {
      for(const auto &t : tasks_repo.listAll(session.rtx())) {
        if(lower_input.empty() || to_lower(t.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(t.name, t.name));
        }
      }
    }
  } catch(const std::exception &e) {
    std::cerr << "listTaskDepsAutocomplete failed for key=" << key << ": " << e.what() << std::endl;
  }

  bot.getCore().interaction_response_create(event.command.id, event.command.token, r);
}
