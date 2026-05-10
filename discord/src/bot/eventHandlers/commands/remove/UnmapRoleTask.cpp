// Associated Header Include
#include "bot/eventHandlers/commands/remove/UnmapRoleTask.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/RoleTasks.hpp"
#include "db/repositories/Roles.hpp"
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

void Commands::unmapRoleTask(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository identity_repo;
    UserRepository user_repo;
    RolesRepository roles_repo;
    TasksRepository tasks_repo;
    RoleTasksRepository role_tasks_repo;

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

    const std::string role_name = std::get<std::string>(event.get_parameter("role"));
    const std::string task_name = std::get<std::string>(event.get_parameter("task"));

    const auto maybe_role = roles_repo.findByName(session.wtx(), role_name);
    if(!maybe_role) {
      event.edit_original_response(dpp::message("Role **" + role_name + "** does not exist."));
      return;
    }

    const auto maybe_task = tasks_repo.findByName(session.wtx(), task_name);
    if(!maybe_task) {
      event.edit_original_response(dpp::message("Task **" + task_name + "** does not exist."));
      return;
    }

    if(!role_tasks_repo.exists(session.wtx(), maybe_role->id, maybe_task->id)) {
      event.edit_original_response(dpp::message("Role **" + role_name + "** is not mapped to task **" + task_name + "**."));
      return;
    }

    role_tasks_repo.remove(session.wtx(), maybe_role->id, maybe_task->id);
    session.commit();

    event.edit_original_response(dpp::message("Removed mapping between role **" + role_name + "** and task **" + task_name + "**."));
  } catch(const std::exception &e) {
    std::cerr << "unmapRoleTask failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to remove role-task mapping. Contact the administrator to resolve this issue."));
  }
}

void Commands::unmapRoleTaskAutocomplete(Bot &bot, const std::string &key, const std::string &input, const dpp::autocomplete_t &event) {
  dpp::interaction_response r(dpp::ir_autocomplete_reply);
  try {
    DbSession session(bot.getPool());
    auto to_lower = [](std::string s) {
      std::transform(s.begin(), s.end(), s.begin(), ::tolower);
      return s;
    };
    const std::string lower_input = to_lower(input);
    if(key == "role") {
      RolesRepository roles_repo;
      for(const auto &role : roles_repo.listAll(session.rtx())) {
        if(lower_input.empty() || to_lower(role.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(role.name, role.name));
        }
      }
    } else if(key == "task") {
      TasksRepository tasks_repo;
      for(const auto &t : tasks_repo.listAll(session.rtx())) {
        if(lower_input.empty() || to_lower(t.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(t.name, t.name));
        }
      }
    }
  } catch(const std::exception &e) {
    std::cerr << "unmapRoleTaskAutocomplete failed for key=" << key << ": " << e.what() << std::endl;
  }
  bot.getCore().interaction_response_create(event.command.id, event.command.token, r);
}
