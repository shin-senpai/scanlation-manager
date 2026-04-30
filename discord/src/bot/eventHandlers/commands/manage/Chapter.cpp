// Associated Header Include
#include "bot/eventHandlers/commands/manage/Chapter.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/ChapterAssignments.hpp"
#include "db/repositories/Chapters.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/RoleTasks.hpp"
#include "db/repositories/Series.hpp"
#include "db/repositories/SeriesAssignments.hpp"
#include "db/repositories/Tasks.hpp"
#include "db/repositories/User.hpp"
#include "db/repositories/UserRoles.hpp"
#include "types/ChapterStatus.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <iostream>
#include <optional>
#include <string>

// Third Party Includes
#include <dpp/appcommand.h>
#include <dpp/dispatcher.h>
#include <dpp/snowflake.h>
#include <pqxx/pqxx>

namespace {

// Extracts the value of a named (non-focused) option from a subcommand autocomplete event.
// Used to read already-filled options (e.g. the series name) when completing a later option.
std::string getAutocompleteContext(const dpp::autocomplete_t &event, const std::string &option_name) {
  if(event.options.empty() || event.options[0].type != dpp::co_sub_command)
    return {};
  for(const auto &opt : event.options[0].options) {
    if(opt.name == option_name && !opt.focused) {
      try {
        return std::get<std::string>(opt.value);
      } catch(...) {
      }
    }
  }
  return {};
}

void doAdd(const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  ChaptersRepository chapters_repo;
  SeriesAssignmentsRepository series_assignments_repo;
  ChapterAssignmentsRepository chapter_assignments_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("series"));
  const double number = std::get<double>(event.get_parameter("number"));
  const std::string name = std::get<std::string>(event.get_parameter("name"));

  std::optional<int> volume = std::nullopt;
  try {
    volume = static_cast<int>(std::get<int64_t>(event.get_parameter("volume")));
  } catch(...) {
    // Optional parameter not provided
  }

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  const auto default_assignments = series_assignments_repo.listBySeries(session.wtx(), maybe_series->id);

  try {
    const int chapter_id = chapters_repo.create(session.wtx(), maybe_series->id, number, name, volume);
    for(const auto &assignment : default_assignments) {
      chapter_assignments_repo.create(session.wtx(), assignment.user_id, chapter_id, assignment.task_id);
    }
    session.commit();
    event.edit_original_response(dpp::message("Chapter **" + name + "** added to **" + series_name + "** with ID `" + std::to_string(chapter_id) + "`."));
  } catch(const pqxx::unique_violation &) {
    event.edit_original_response(dpp::message("A chapter with that number or name already exists in **" + series_name + "**."));
  }
}

void doSetStatus(const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  ChaptersRepository chapters_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("series"));
  const std::string chapter_name = std::get<std::string>(event.get_parameter("chapter"));
  const std::string status_str = std::get<std::string>(event.get_parameter("status"));

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  const auto maybe_chapter = chapters_repo.findByName(session.wtx(), maybe_series->id, chapter_name);
  if(!maybe_chapter) {
    event.edit_original_response(dpp::message("Chapter **" + chapter_name + "** does not exist in **" + series_name + "**."));
    return;
  }

  chapters_repo.updateStatus(session.wtx(), maybe_chapter->id, chapterStatusFromString(status_str));
  session.commit();

  event.edit_original_response(dpp::message(
      "Chapter **" + chapter_name + "** status set to **" + status_str + "**."));
}

void doAssign(const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  ChaptersRepository chapters_repo;
  ChapterAssignmentsRepository assignments_repo;
  TasksRepository tasks_repo;
  RoleTasksRepository role_tasks_repo;
  DiscordIdentityRepository identity_repo;
  UserRolesRepository user_roles_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("series"));
  const std::string chapter_name = std::get<std::string>(event.get_parameter("chapter"));
  const dpp::snowflake target_discord_id = std::get<dpp::snowflake>(event.get_parameter("user"));
  const std::string task_name = std::get<std::string>(event.get_parameter("task"));

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  const auto maybe_chapter = chapters_repo.findByName(session.wtx(), maybe_series->id, chapter_name);
  if(!maybe_chapter) {
    event.edit_original_response(dpp::message("Chapter **" + chapter_name + "** does not exist in **" + series_name + "**."));
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
    assignments_repo.create(session.wtx(), *maybe_target_id, maybe_chapter->id, maybe_task->id);
    session.commit();
    event.edit_original_response(dpp::message(
        "<@" + std::to_string(target_discord_id) + "> assigned to **" + chapter_name + "** for **" + task_name + "**."));
  } catch(const pqxx::unique_violation &) {
    event.edit_original_response(dpp::message("That user is already assigned to that chapter for that task."));
  }
}

void doUnassign(const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  ChaptersRepository chapters_repo;
  ChapterAssignmentsRepository assignments_repo;
  TasksRepository tasks_repo;
  DiscordIdentityRepository identity_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("series"));
  const std::string chapter_name = std::get<std::string>(event.get_parameter("chapter"));
  const dpp::snowflake target_discord_id = std::get<dpp::snowflake>(event.get_parameter("user"));
  const std::string task_name = std::get<std::string>(event.get_parameter("task"));

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  const auto maybe_chapter = chapters_repo.findByName(session.wtx(), maybe_series->id, chapter_name);
  if(!maybe_chapter) {
    event.edit_original_response(dpp::message("Chapter **" + chapter_name + "** does not exist in **" + series_name + "**."));
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

  if(!assignments_repo.exists(session.wtx(), *maybe_target_id, maybe_chapter->id, maybe_task->id)) {
    event.edit_original_response(dpp::message("That user is not assigned to that chapter for that task."));
    return;
  }

  if(assignments_repo.exists(session.wtx(), *maybe_target_id, maybe_chapter->id, maybe_task->id, true)) {
    event.edit_original_response(dpp::message("That assignment is already completed and cannot be removed."));
    return;
  }

  assignments_repo.remove(session.wtx(), *maybe_target_id, maybe_chapter->id, maybe_task->id);
  session.commit();

  event.edit_original_response(dpp::message(
      "<@" + std::to_string(target_discord_id) + "> removed from **" + chapter_name + "** for **" + task_name + "**."));
}

void doUncomplete(const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  ChaptersRepository chapters_repo;
  ChapterAssignmentsRepository assignments_repo;
  TasksRepository tasks_repo;
  DiscordIdentityRepository identity_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("series"));
  const std::string chapter_name = std::get<std::string>(event.get_parameter("chapter"));
  const dpp::snowflake target_discord_id = std::get<dpp::snowflake>(event.get_parameter("user"));
  const std::string task_name = std::get<std::string>(event.get_parameter("task"));

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  const auto maybe_chapter = chapters_repo.findByName(session.wtx(), maybe_series->id, chapter_name);
  if(!maybe_chapter) {
    event.edit_original_response(dpp::message("Chapter **" + chapter_name + "** does not exist in **" + series_name + "**."));
    return;
  }

  if(maybe_chapter->status != ChapterStatus::in_progress) {
    event.edit_original_response(dpp::message(
        "Chapter **" + chapter_name + "** is not in progress. Set it back to in-progress before uncompleting assignments."));
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

  if(!assignments_repo.exists(session.wtx(), *maybe_target_id, maybe_chapter->id, maybe_task->id, true)) {
    event.edit_original_response(dpp::message("That assignment is not completed."));
    return;
  }

  assignments_repo.clearCompleted(session.wtx(), *maybe_target_id, maybe_chapter->id, maybe_task->id);
  session.commit();

  event.edit_original_response(dpp::message(
      "**" + task_name + "** marked as outstanding for <@" + std::to_string(target_discord_id) + "> on **" + chapter_name + "**."));
}

} // namespace

void Commands::chapter(Bot &bot, const dpp::slashcommand_t &event) {
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
    else if(sub == "uncomplete")
      doUncomplete(event, session);
  } catch(const std::exception &e) {
    std::cerr << "chapter/" << sub << " failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("An error occurred. Contact the administrator to resolve this issue."));
  }
}

void Commands::chapterAutocomplete(Bot &bot, const std::string &key, const std::string &input, const dpp::autocomplete_t &event) {
  dpp::interaction_response r(dpp::ir_autocomplete_reply);

  try {
    DbSession session(bot.getPool());

    auto to_lower = [](std::string s) {
      std::transform(s.begin(), s.end(), s.begin(), ::tolower);
      return s;
    };
    const std::string lower_input = to_lower(input);

    // Series name options
    if(key == "add/series" || key == "set-status/series" || key == "assign/series" || key == "unassign/series" || key == "uncomplete/series") {
      SeriesRepository series_repo;
      for(const auto &s : series_repo.list(session.wtx())) {
        if(lower_input.empty() || to_lower(s.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(s.name, s.name));
        }
      }
    }
    // Chapter name options — filters based on the already-typed series
    else if(key == "set-status/chapter" || key == "assign/chapter" || key == "unassign/chapter" || key == "uncomplete/chapter") {
      const std::string series_ctx = getAutocompleteContext(event, "series");
      if(!series_ctx.empty()) {
        SeriesRepository series_repo;
        ChaptersRepository chapters_repo;
        const auto maybe_series = series_repo.findByName(session.wtx(), series_ctx);
        if(maybe_series) {
          for(const auto &c : chapters_repo.listBySeries(session.wtx(), maybe_series->id)) {
            if(lower_input.empty() || to_lower(c.name).find(lower_input) != std::string::npos) {
              r.add_autocomplete_choice(dpp::command_option_choice(c.name, c.name));
            }
          }
        }
      }
    }
    // Task name options
    else if(key == "assign/task" || key == "unassign/task" || key == "uncomplete/task") {
      TasksRepository tasks_repo;
      for(const auto &t : tasks_repo.listAll(session.wtx())) {
        if(lower_input.empty() || to_lower(t.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(t.name, t.name));
        }
      }
    }
  } catch(const std::exception &e) {
    std::cerr << "chapterAutocomplete failed for key=" << key << ": " << e.what() << std::endl;
  }

  bot.getCore().interaction_response_create(event.command.id, event.command.token, r);
}
