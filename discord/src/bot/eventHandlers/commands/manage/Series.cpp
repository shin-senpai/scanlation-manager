// Associated Header Include
#include "bot/eventHandlers/commands/manage/Series.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/RoleTasks.hpp"
#include "db/repositories/Series.hpp"
#include "db/repositories/SeriesAssignments.hpp"
#include "db/repositories/Tasks.hpp"
#include "db/repositories/User.hpp"
#include "db/repositories/UserRoles.hpp"
#include "types/Permission.hpp"
#include "types/SeriesStatus.hpp"

// Standard Includes
#include <algorithm>
#include <iostream>
#include <string>

// Third Party Includes
#include <dpp/appcommand.h>
#include <dpp/dispatcher.h>
#include <dpp/snowflake.h>
#include <pqxx/pqxx>

namespace {

void doAdd(const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;

  const std::string name = std::get<std::string>(event.get_parameter("name"));

  try {
    const int id = series_repo.create(session.wtx(), name);
    session.commit();
    event.edit_original_response(dpp::message("Series **" + name + "** created with ID `" + std::to_string(id) + "`."));
  } catch(const pqxx::unique_violation &) {
    event.edit_original_response(dpp::message("A series with that name already exists."));
  }
}

void doSetStatus(const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;

  const std::string name = std::get<std::string>(event.get_parameter("name"));
  const std::string status_str = std::get<std::string>(event.get_parameter("status"));

  const auto maybe_series = series_repo.findByName(session.wtx(), name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + name + "** does not exist."));
    return;
  }

  series_repo.updateStatus(session.wtx(), maybe_series->id, seriesStatusFromString(status_str));
  session.commit();

  event.edit_original_response(dpp::message("Series **" + name + "** status set to **" + status_str + "**."));
}

void doAssign(const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  SeriesAssignmentsRepository assignments_repo;
  TasksRepository tasks_repo;
  RoleTasksRepository role_tasks_repo;
  DiscordIdentityRepository identity_repo;
  UserRolesRepository user_roles_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("name"));
  const dpp::snowflake target_discord_id = std::get<dpp::snowflake>(event.get_parameter("user"));
  const std::string task_name = std::get<std::string>(event.get_parameter("task"));

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  const auto maybe_target_id = identity_repo.findUserIdByDiscordId(session.wtx(), static_cast<int64_t>(target_discord_id));
  if(!maybe_target_id) {
    event.edit_original_response(dpp::message("That user is not registered."));
    return;
  }

  const auto maybe_task = tasks_repo.findByName(session.wtx(), task_name);
  if(!maybe_task) {
    event.edit_original_response(dpp::message("Task **" + task_name + "** does not exist."));
    return;
  }

  if(maybe_task->retired_at) {
    event.edit_original_response(dpp::message("Task **" + task_name + "** is retired and cannot be assigned."));
    return;
  }

  const auto user_roles = user_roles_repo.listByUser(session.wtx(), *maybe_target_id);
  const auto capable_roles = role_tasks_repo.listRoleIdsByTask(session.wtx(), maybe_task->id);
  bool has_valid_role = false;
  for(const auto &user_role : user_roles) {
    if(std::find(capable_roles.begin(), capable_roles.end(), user_role.role_id) != capable_roles.end()) {
      has_valid_role = true;
      break;
    }
  }
  if(!has_valid_role) {
    event.edit_original_response(dpp::message("User does not have a Role that allows them to be assigned to **" + task_name + "**"));
    return;
  }

  try {
    assignments_repo.create(session.wtx(), *maybe_target_id, maybe_series->id, maybe_task->id);
    session.commit();
    event.edit_original_response(dpp::message(
        "<@" + std::to_string(target_discord_id) + "> assigned to **" + series_name + "** for **" + task_name + "**."));
  } catch(const pqxx::unique_violation &) {
    event.edit_original_response(dpp::message("That user is already assigned to that series for that task."));
  }
}

void doUnassign(const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  SeriesAssignmentsRepository assignments_repo;
  TasksRepository tasks_repo;
  DiscordIdentityRepository identity_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("name"));
  const dpp::snowflake target_discord_id = std::get<dpp::snowflake>(event.get_parameter("user"));
  const std::string task_name = std::get<std::string>(event.get_parameter("task"));

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  const auto maybe_target_id = identity_repo.findUserIdByDiscordId(session.wtx(), static_cast<int64_t>(target_discord_id));
  if(!maybe_target_id) {
    event.edit_original_response(dpp::message("That user is not registered."));
    return;
  }

  const auto maybe_task = tasks_repo.findByName(session.wtx(), task_name);
  if(!maybe_task) {
    event.edit_original_response(dpp::message("Task **" + task_name + "** does not exist."));
    return;
  }

  if(!assignments_repo.exists(session.wtx(), *maybe_target_id, maybe_series->id, maybe_task->id)) {
    event.edit_original_response(dpp::message("That user is not assigned to that series for that task."));
    return;
  }

  assignments_repo.remove(session.wtx(), *maybe_target_id, maybe_series->id, maybe_task->id);
  session.commit();

  event.edit_original_response(dpp::message(
      "<@" + std::to_string(target_discord_id) + "> removed from **" + series_name + "** for **" + task_name + "**."));
}

} // namespace

void Commands::series(Bot &bot, const dpp::slashcommand_t &event) {
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

    if(sub == "add")
      doAdd(event, session);
    else if(sub == "set-status")
      doSetStatus(event, session);
    else if(sub == "assign")
      doAssign(event, session);
    else if(sub == "unassign")
      doUnassign(event, session);
  } catch(const std::exception &e) {
    std::cerr << "series/" << sub << " failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("An error occurred. Contact the administrator to resolve this issue."));
  }
}

void Commands::seriesAutocomplete(Bot &bot, const std::string &key, const std::string &input, const dpp::autocomplete_t &event) {
  dpp::interaction_response r(dpp::ir_autocomplete_reply);

  try {
    DbSession session(bot.getPool());

    auto to_lower = [](std::string s) {
      std::transform(s.begin(), s.end(), s.begin(), ::tolower);
      return s;
    };
    const std::string lower_input = to_lower(input);

    if(key == "set-status/name" || key == "assign/name" || key == "unassign/name") {
      SeriesRepository series_repo;
      for(const auto &s : series_repo.list(session.wtx())) {
        if(lower_input.empty() || to_lower(s.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(s.name, s.name));
        }
      }
    } else if(key == "assign/task" || key == "unassign/task") {
      TasksRepository tasks_repo;
      for(const auto &t : tasks_repo.listAll(session.wtx())) {
        if(lower_input.empty() || to_lower(t.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(t.name, t.name));
        }
      }
    }
  } catch(const std::exception &e) {
    std::cerr << "seriesAutocomplete failed for key=" << key << ": " << e.what() << std::endl;
  }

  bot.getCore().interaction_response_create(event.command.id, event.command.token, r);
}
