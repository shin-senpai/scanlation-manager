// Associated Header Include
#include "bot/eventHandlers/commands/manage/Series.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "bot/utils/SheetSync.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/ChapterAssignmentPlaceholders.hpp"
#include "db/repositories/ChapterAssignments.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/RoleTasks.hpp"
#include "db/repositories/Series.hpp"
#include "db/repositories/SeriesAssignmentPlaceholders.hpp"
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

void doAdd(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;

  const std::string name = std::get<std::string>(event.get_parameter("name"));

  try {
    const int id = series_repo.create(session.wtx(), name);
    session.commit();
    SheetSync::syncSeries(bot, name);
    event.edit_original_response(dpp::message("Series **" + name + "** created with ID `" + std::to_string(id) + "`."));
  } catch(const pqxx::unique_violation &) {
    event.edit_original_response(dpp::message("A series with that name already exists."));
  }
}

void doSetStatus(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
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
  SheetSync::syncSeries(bot, name);
  SheetSync::syncTodo(bot);

  event.edit_original_response(dpp::message("Series **" + name + "** status set to **" + status_str + "**."));
}

void doAssign(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  SeriesAssignmentsRepository assignments_repo;
  ChapterAssignmentsRepository chapter_assignments_repo;
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

  bool sync_chapters = true;
  const auto sync_param = event.get_parameter("sync_chapters");
  if(const auto *b = std::get_if<bool>(&sync_param)) {
    sync_chapters = *b;
  }

  try {
    ChapterAssignmentPlaceholdersRepository chapter_placeholder_repo;
    SeriesAssignmentPlaceholdersRepository series_placeholder_repo;
    assignments_repo.create(session.wtx(), *maybe_target_id, maybe_series->id, maybe_task->id);
    // Assigning a user fills one series-level vacancy — consume the oldest placeholder slot.
    series_placeholder_repo.removeOne(session.wtx(), maybe_series->id, maybe_task->id);
    int chapters_affected = 0;
    if(sync_chapters) {
      chapters_affected = chapter_assignments_repo.createForSeriesIfMissing(
          session.wtx(), *maybe_target_id, maybe_series->id, maybe_task->id);
      // Consume one chapter-level placeholder per chapter (mirrors the single series slot consumed).
      chapter_placeholder_repo.removeOnePerChapterForTaskInSeries(session.wtx(), maybe_series->id, maybe_task->id);
    }
    session.commit();
    SheetSync::syncSeries(bot, series_name);
    if(sync_chapters) {
      SheetSync::syncTodo(bot);
    }
    std::string msg = "<@" + std::to_string(target_discord_id) + "> assigned to **" + series_name + "** for **" + task_name + "**.";
    if(sync_chapters) {
      msg += chapters_affected > 0
                 ? "\n(Synced to " + std::to_string(chapters_affected) + " existing chapter(s).)"
                 : "\n(No existing chapters to sync.)";
    }
    event.edit_original_response(dpp::message(msg));
  } catch(const pqxx::unique_violation &) {
    event.edit_original_response(dpp::message("That user is already assigned to that series for that task."));
  }
}

void doUnassign(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  SeriesAssignmentsRepository assignments_repo;
  ChapterAssignmentsRepository chapter_assignments_repo;
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

  bool sync_chapters = true;
  const auto sync_param = event.get_parameter("sync_chapters");
  if(const auto *b = std::get_if<bool>(&sync_param)) {
    sync_chapters = *b;
  }

  assignments_repo.remove(session.wtx(), *maybe_target_id, maybe_series->id, maybe_task->id);
  int chapters_affected = 0;
  if(sync_chapters) {
    chapters_affected = chapter_assignments_repo.removeOutstandingForUserInSeries(
        session.wtx(), *maybe_target_id, maybe_series->id, maybe_task->id);
  }
  session.commit();
  SheetSync::syncSeries(bot, series_name);
  if(sync_chapters) {
    SheetSync::syncTodo(bot);
  }

  std::string msg = "<@" + std::to_string(target_discord_id) + "> removed from **" + series_name + "** for **" + task_name + "**.";
  if(sync_chapters) {
    msg += chapters_affected > 0
               ? "\n(Removed from " + std::to_string(chapters_affected) + " existing chapter(s); completed assignments preserved.)"
               : "\n(No outstanding chapter assignments to remove.)";
  }
  event.edit_original_response(dpp::message(msg));
}

void doMoveAssignment(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  SeriesAssignmentsRepository assignments_repo;
  ChapterAssignmentsRepository chapter_assignments_repo;
  TasksRepository tasks_repo;
  RoleTasksRepository role_tasks_repo;
  DiscordIdentityRepository identity_repo;
  UserRolesRepository user_roles_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("name"));
  const dpp::snowflake from_discord_id = std::get<dpp::snowflake>(event.get_parameter("from_user"));
  const dpp::snowflake to_discord_id = std::get<dpp::snowflake>(event.get_parameter("to_user"));
  const std::string task_name = std::get<std::string>(event.get_parameter("task"));

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  const auto maybe_from_id = identity_repo.findUserIdByDiscordId(session.wtx(), static_cast<int64_t>(from_discord_id));
  if(!maybe_from_id) {
    event.edit_original_response(dpp::message("From user is not registered."));
    return;
  }

  const auto maybe_to_id = identity_repo.findUserIdByDiscordId(session.wtx(), static_cast<int64_t>(to_discord_id));
  if(!maybe_to_id) {
    event.edit_original_response(dpp::message("To user is not registered."));
    return;
  }

  if(*maybe_from_id == *maybe_to_id) {
    event.edit_original_response(dpp::message("Cannot move an assignment to the same user."));
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

  const auto user_roles = user_roles_repo.listByUser(session.wtx(), *maybe_to_id);
  const auto capable_roles = role_tasks_repo.listRoleIdsByTask(session.wtx(), maybe_task->id);
  bool has_valid_role = false;
  for(const auto &ur : user_roles) {
    if(std::find(capable_roles.begin(), capable_roles.end(), ur.role_id) != capable_roles.end()) {
      has_valid_role = true;
      break;
    }
  }
  if(!has_valid_role) {
    event.edit_original_response(dpp::message("To user does not have a role that allows them to be assigned to **" + task_name + "**."));
    return;
  }

  if(!assignments_repo.exists(session.wtx(), *maybe_from_id, maybe_series->id, maybe_task->id)) {
    event.edit_original_response(dpp::message("<@" + std::to_string(from_discord_id) + "> is not assigned to **" + series_name + "** for **" + task_name + "**."));
    return;
  }

  if(assignments_repo.exists(session.wtx(), *maybe_to_id, maybe_series->id, maybe_task->id)) {
    event.edit_original_response(dpp::message("<@" + std::to_string(to_discord_id) + "> is already assigned to **" + series_name + "** for **" + task_name + "**."));
    return;
  }

  bool sync_chapters = true;
  const auto sync_param = event.get_parameter("sync_chapters");
  if(const auto *b = std::get_if<bool>(&sync_param)) {
    sync_chapters = *b;
  }

  assignments_repo.remove(session.wtx(), *maybe_from_id, maybe_series->id, maybe_task->id);
  assignments_repo.create(session.wtx(), *maybe_to_id, maybe_series->id, maybe_task->id);

  int removed_count = 0;
  int added_count = 0;
  if(sync_chapters) {
    removed_count = chapter_assignments_repo.removeOutstandingForUserInSeries(session.wtx(), *maybe_from_id, maybe_series->id, maybe_task->id);
    added_count = chapter_assignments_repo.createForSeriesIfMissing(session.wtx(), *maybe_to_id, maybe_series->id, maybe_task->id);
  }
  session.commit();
  SheetSync::syncSeries(bot, series_name);
  if(sync_chapters) {
    SheetSync::syncTodo(bot);
  }

  std::string msg = "Moved **" + series_name + "** / **" + task_name + "** assignment from <@" + std::to_string(from_discord_id) + "> to <@" + std::to_string(to_discord_id) + ">.";
  if(sync_chapters) {
    msg += "\n(Removed from " + std::to_string(removed_count) + " chapter(s); added to " + std::to_string(added_count) + " chapter(s).)";
  }
  event.edit_original_response(dpp::message(msg));
}

void doRemove(Bot &bot, const dpp::slashcommand_t &event, DbSession &session, Permission user_perm) {
  SeriesRepository series_repo;
  ChapterAssignmentsRepository chapter_assignments_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("name"));

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  if(user_perm < Permission::supermanager && chapter_assignments_repo.hasCompletedBySeries(session.wtx(), maybe_series->id)) {
    event.edit_original_response(dpp::message(
        "Series **" + series_name + "** has chapters with completed assignments. Only a supermanager can delete it."));
    return;
  }

  if(user_perm >= Permission::supermanager) {
    // Clear completed_at on all chapter_assignments in this series before the
    // delete so the immutability trigger doesn't fire on the cascaded removal.
    chapter_assignments_repo.clearAllCompletedBySeries(session.wtx(), maybe_series->id);
  }

  // Cascades: series → series_assignments, series → chapters → chapter_assignments.
  series_repo.remove(session.wtx(), maybe_series->id);
  session.commit();
  SheetSync::deleteSeries(bot, series_name);
  SheetSync::syncTodo(bot);

  event.edit_original_response(dpp::message(
      "Series **" + series_name + "** and all its chapters have been deleted."));
}

void doAddPlaceholder(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  TasksRepository tasks_repo;
  SeriesAssignmentPlaceholdersRepository series_placeholder_repo;
  ChapterAssignmentPlaceholdersRepository chapter_placeholder_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("name"));
  const std::string task_name = std::get<std::string>(event.get_parameter("task"));

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  const auto maybe_task = tasks_repo.findByName(session.wtx(), task_name);
  if(!maybe_task) {
    event.edit_original_response(dpp::message("Task **" + task_name + "** does not exist."));
    return;
  }

  if(maybe_task->retired_at) {
    event.edit_original_response(dpp::message("Task **" + task_name + "** is retired and cannot have placeholders added."));
    return;
  }

  bool sync_chapters = true;
  const auto sync_param = event.get_parameter("sync_chapters");
  if(const auto *b = std::get_if<bool>(&sync_param)) {
    sync_chapters = *b;
  }

  series_placeholder_repo.create(session.wtx(), maybe_series->id, maybe_task->id);
  const int total = series_placeholder_repo.count(session.wtx(), maybe_series->id, maybe_task->id);

  int chapters_affected = 0;
  if(sync_chapters) {
    // Add one chapter-level placeholder to every non-released chapter in the series.
    const auto result = session.wtx().exec(
        "SELECT id FROM chapters WHERE series_id = $1 AND status != 'released'",
        pqxx::params(maybe_series->id));
    for(const auto &row : result) {
      chapter_placeholder_repo.create(session.wtx(), row[0].as<int>(), maybe_task->id);
      ++chapters_affected;
    }
  }

  session.commit();
  SheetSync::syncSeries(bot, series_name);
  SheetSync::syncTodo(bot);

  std::string msg = "Added placeholder for **" + task_name + "** on **" + series_name + "** (" + std::to_string(total) + " total).";
  if(sync_chapters) {
    msg += chapters_affected > 0
               ? "\n(Added to " + std::to_string(chapters_affected) + " existing chapter(s).)"
               : "\n(No existing chapters to sync.)";
  }
  event.edit_original_response(dpp::message(msg));
}

void doRemovePlaceholder(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  TasksRepository tasks_repo;
  SeriesAssignmentPlaceholdersRepository series_placeholder_repo;
  ChapterAssignmentPlaceholdersRepository chapter_placeholder_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("name"));
  const std::string task_name = std::get<std::string>(event.get_parameter("task"));

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  const auto maybe_task = tasks_repo.findByName(session.wtx(), task_name);
  if(!maybe_task) {
    event.edit_original_response(dpp::message("Task **" + task_name + "** does not exist."));
    return;
  }

  if(series_placeholder_repo.count(session.wtx(), maybe_series->id, maybe_task->id) == 0) {
    event.edit_original_response(dpp::message("No placeholder exists for **" + task_name + "** on **" + series_name + "**."));
    return;
  }

  bool sync_chapters = true;
  const auto sync_param = event.get_parameter("sync_chapters");
  if(const auto *b = std::get_if<bool>(&sync_param)) {
    sync_chapters = *b;
  }

  series_placeholder_repo.removeAll(session.wtx(), maybe_series->id, maybe_task->id);
  if(sync_chapters) {
    chapter_placeholder_repo.removeAllForTaskInSeries(session.wtx(), maybe_series->id, maybe_task->id);
  }

  session.commit();
  SheetSync::syncSeries(bot, series_name);
  SheetSync::syncTodo(bot);

  std::string msg = "Removed all placeholders for **" + task_name + "** on **" + series_name + "**.";
  if(sync_chapters) {
    msg += "\n(Chapter-level placeholders for this task also cleared.)";
  }
  event.edit_original_response(dpp::message(msg));
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

    const Permission user_perm = user_repo.getPermissionLevel(session.wtx(), *maybe_user_id);
    if(user_perm < Permission::manager) {
      event.edit_original_response(dpp::message("You lack the permission to perform this action."));
      return;
    }

    if(sub == "add") {
      doAdd(bot, event, session);
    } else if(sub == "set-status") {
      doSetStatus(bot, event, session);
    } else if(sub == "assign") {
      doAssign(bot, event, session);
    } else if(sub == "unassign") {
      doUnassign(bot, event, session);
    } else if(sub == "remove") {
      doRemove(bot, event, session, user_perm);
    } else if(sub == "move-assignment") {
      doMoveAssignment(bot, event, session);
    } else if(sub == "add-placeholder") {
      doAddPlaceholder(bot, event, session);
    } else if(sub == "remove-placeholder") {
      doRemovePlaceholder(bot, event, session);
    }
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

    if(key == "set-status/name" || key == "assign/name" || key == "unassign/name" || key == "remove/name" || key == "move-assignment/name" || key == "add-placeholder/name" || key == "remove-placeholder/name") {
      SeriesRepository series_repo;
      for(const auto &s : series_repo.list(session.wtx())) {
        if(lower_input.empty() || to_lower(s.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(s.name, s.name));
        }
      }
    } else if(key == "assign/task" || key == "unassign/task" || key == "move-assignment/task" || key == "add-placeholder/task" || key == "remove-placeholder/task") {
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
