// Associated Header Include
#include "bot/eventHandlers/commands/manage/Chapter.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "bot/utils/GetAutoCompleteContext.hpp"
#include "bot/utils/SheetSync.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/BotSettings.hpp"
#include "db/repositories/ChapterAssignmentPlaceholders.hpp"
#include "db/repositories/ChapterAssignments.hpp"
#include "db/repositories/Chapters.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/RoleTasks.hpp"
#include "db/repositories/Series.hpp"
#include "db/repositories/SeriesAssignmentPlaceholders.hpp"
#include "db/repositories/SeriesAssignments.hpp"
#include "db/repositories/Tasks.hpp"
#include "db/repositories/User.hpp"
#include "db/repositories/UserRoles.hpp"
#include "types/ChapterStatus.hpp"
#include "types/Permission.hpp"
#include "types/SeriesStatus.hpp"

// User Defined Utils
#include "bot/utils/ParseChapterNumbers.hpp"

// Standard Includes
#include <cmath>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

// Third Party Includes
#include <dpp/appcommand.h>
#include <dpp/dispatcher.h>
#include <dpp/snowflake.h>
#include <pqxx/pqxx>

namespace {

std::string fmtChapterNumber(double n) {
  if(n == std::floor(n)) {
    return std::to_string(static_cast<int>(n));
  }
  std::ostringstream oss;
  oss << n;
  return oss.str();
}

void doAdd(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  ChaptersRepository chapters_repo;
  SeriesAssignmentsRepository series_assignments_repo;
  ChapterAssignmentsRepository chapter_assignments_repo;
  SeriesAssignmentPlaceholdersRepository series_placeholder_repo;
  ChapterAssignmentPlaceholdersRepository chapter_placeholder_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("series"));
  std::optional<std::string> name;
  const auto &name_param = event.get_parameter("name");
  if(const auto *p = std::get_if<std::string>(&name_param); p && !p->empty()) {
    name = *p;
  }

  const double number = std::get<double>(event.get_parameter("number"));
  if(number < 0) {
    event.edit_original_response(dpp::message("Chapter number cannot be negative."));
    return;
  }

  std::optional<int> volume;
  const auto &volume_param = event.get_parameter("volume");
  if(const auto *p = std::get_if<int64_t>(&volume_param)) {
    volume = static_cast<int>(*p);
  }

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  if(maybe_series->status != SeriesStatus::active) {
    event.edit_original_response(dpp::message(
        "Series **" + series_name + "** is not active. Set its status to Active before adding chapters."));
    return;
  }

  const auto default_assignments = series_assignments_repo.listBySeries(session.wtx(), maybe_series->id);

  // Default to queued if an in_progress chapter already exists in the series.
  const bool has_in_progress = !chapters_repo.listBySeries(session.wtx(), maybe_series->id, ChapterStatus::in_progress).empty();
  const ChapterStatus initial_status = has_in_progress ? ChapterStatus::queued : ChapterStatus::in_progress;

  try {
    const int chapter_id = chapters_repo.create(session.wtx(), maybe_series->id, number, name, volume, initial_status);
    for(const auto &assignment : default_assignments) {
      chapter_assignments_repo.create(session.wtx(), assignment.user_id, chapter_id, assignment.task_id);
    }
    // Cascade series-level placeholders to the new chapter.
    for(const int task_id : series_placeholder_repo.listTaskIdsBySeries(session.wtx(), maybe_series->id)) {
      chapter_placeholder_repo.create(session.wtx(), chapter_id, task_id);
    }
    session.commit();
    SheetSync::syncSeries(bot, series_name);
    SheetSync::syncTodo(bot);
    const std::string display = name ? "**" + *name + "**" : "**Ch." + fmtChapterNumber(number) + "**";
    event.edit_original_response(dpp::message("Chapter " + display + " added to **" + series_name + "** with ID `" + std::to_string(chapter_id) + "`."));
  } catch(const pqxx::unique_violation &) {
    event.edit_original_response(dpp::message("A chapter with that number or name already exists in **" + series_name + "**."));
  }
}

void doSetStatus(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
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

  const auto maybe_chapter = chapters_repo.findByDisplayKey(session.wtx(), maybe_series->id, chapter_name);
  if(!maybe_chapter) {
    event.edit_original_response(dpp::message("Chapter **" + chapter_name + "** does not exist in **" + series_name + "**."));
    return;
  }

  const ChapterStatus new_status = chapterStatusFromString(status_str);
  chapters_repo.updateStatus(session.wtx(), maybe_chapter->id, new_status);
  // When a chapter is closed (released/dropped/hiatus), promote the next queued
  // chapter in line (lowest number > this one with queued status) to in_progress.
  if(new_status == ChapterStatus::released || new_status == ChapterStatus::dropped || new_status == ChapterStatus::hiatus) {
    const auto next_id = chapters_repo.findNextQueuedId(session.wtx(), maybe_series->id, maybe_chapter->number);
    if(next_id) {
      chapters_repo.updateStatus(session.wtx(), *next_id, ChapterStatus::in_progress);
    }
  }
  session.commit();
  SheetSync::syncSeries(bot, series_name);
  SheetSync::syncTodo(bot);

  event.edit_original_response(dpp::message(
      "Chapter **" + chapter_name + "** status set to **" + status_str + "**."));
}

void doAssign(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
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

  if(maybe_series->status != SeriesStatus::active) {
    event.edit_original_response(dpp::message(
        "Series **" + series_name + "** is not active. Set its status to Active before modifying assignments."));
    return;
  }

  const auto maybe_chapter = chapters_repo.findByDisplayKey(session.wtx(), maybe_series->id, chapter_name);
  if(!maybe_chapter) {
    event.edit_original_response(dpp::message("Chapter **" + chapter_name + "** does not exist in **" + series_name + "**."));
    return;
  }

  if(maybe_chapter->status != ChapterStatus::in_progress && maybe_chapter->status != ChapterStatus::queued) {
    event.edit_original_response(dpp::message(
        "Chapter **" + chapter_name + "** is not in progress or queued. Set its status to In Progress or Queued before modifying assignments."));
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

  BotSettingsRepository settings_repo;
  if(settings_repo.get(session.wtx(), "role_check_enabled")) {
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
  }

  try {
    ChapterAssignmentPlaceholdersRepository placeholder_repo;
    assignments_repo.create(session.wtx(), *maybe_target_id, maybe_chapter->id, maybe_task->id);
    // Consume one placeholder vacancy for this slot (no-op if none exist).
    placeholder_repo.removeOne(session.wtx(), maybe_chapter->id, maybe_task->id);
    session.commit();
    SheetSync::syncSeries(bot, series_name);
    SheetSync::syncTodo(bot);
    event.edit_original_response(dpp::message(
        "<@" + std::to_string(target_discord_id) + "> assigned to **" + chapter_name + "** for **" + task_name + "**."));
  } catch(const pqxx::unique_violation &) {
    event.edit_original_response(dpp::message("That user is already assigned to that chapter for that task."));
  }
}

void doUnassign(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
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

  if(maybe_series->status != SeriesStatus::active) {
    event.edit_original_response(dpp::message(
        "Series **" + series_name + "** is not active. Set its status to Active before modifying assignments."));
    return;
  }

  const auto maybe_chapter = chapters_repo.findByDisplayKey(session.wtx(), maybe_series->id, chapter_name);
  if(!maybe_chapter) {
    event.edit_original_response(dpp::message("Chapter **" + chapter_name + "** does not exist in **" + series_name + "**."));
    return;
  }

  if(maybe_chapter->status != ChapterStatus::in_progress && maybe_chapter->status != ChapterStatus::queued) {
    event.edit_original_response(dpp::message(
        "Chapter **" + chapter_name + "** is not in progress or queued. Set its status to In Progress or Queued before modifying assignments."));
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
  SheetSync::syncSeries(bot, series_name);
  SheetSync::syncTodo(bot);

  event.edit_original_response(dpp::message(
      "<@" + std::to_string(target_discord_id) + "> removed from **" + chapter_name + "** for **" + task_name + "**."));
}

void doUncomplete(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
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

  if(maybe_series->status != SeriesStatus::active) {
    event.edit_original_response(dpp::message(
        "Series **" + series_name + "** is not active. Set its status to Active before modifying assignments."));
    return;
  }

  const auto maybe_chapter = chapters_repo.findByDisplayKey(session.wtx(), maybe_series->id, chapter_name);
  if(!maybe_chapter) {
    event.edit_original_response(dpp::message("Chapter **" + chapter_name + "** does not exist in **" + series_name + "**."));
    return;
  }

  if(maybe_chapter->status != ChapterStatus::in_progress && maybe_chapter->status != ChapterStatus::queued) {
    event.edit_original_response(dpp::message(
        "Chapter **" + chapter_name + "** is not in progress or queued. Set its status to In Progress or Queued before uncompleting assignments."));
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
  SheetSync::syncSeries(bot, series_name);
  SheetSync::syncTodo(bot);

  event.edit_original_response(dpp::message(
      "**" + task_name + "** marked as outstanding for <@" + std::to_string(target_discord_id) + "> on **" + chapter_name + "**."));
}

void doRemove(Bot &bot, const dpp::slashcommand_t &event, DbSession &session, Permission user_perm) {
  SeriesRepository series_repo;
  ChaptersRepository chapters_repo;
  ChapterAssignmentsRepository assignments_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("series"));
  const std::string chapter_name = std::get<std::string>(event.get_parameter("chapter"));

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  if(maybe_series->status != SeriesStatus::active) {
    event.edit_original_response(dpp::message(
        "Series **" + series_name + "** is not active. Set its status to Active before modifying chapters."));
    return;
  }

  const auto maybe_chapter = chapters_repo.findByDisplayKey(session.wtx(), maybe_series->id, chapter_name);
  if(!maybe_chapter) {
    event.edit_original_response(dpp::message("Chapter **" + chapter_name + "** does not exist in **" + series_name + "**."));
    return;
  }

  if(user_perm < Permission::supermanager && assignments_repo.hasCompletedByChapter(session.wtx(), maybe_chapter->id)) {
    event.edit_original_response(dpp::message(
        "Chapter **" + chapter_name + "** has completed assignments. Only a supermanager can delete it."));
    return;
  }

  if(user_perm >= Permission::supermanager) {
    // Clear completed_at before the delete so the immutability trigger
    // doesn't fire on the cascaded chapter_assignments removal.
    assignments_repo.clearAllCompletedByChapter(session.wtx(), maybe_chapter->id);
  }

  // Cascades to chapter_assignments via ON DELETE CASCADE.
  chapters_repo.remove(session.wtx(), maybe_chapter->id);
  session.commit();
  SheetSync::syncSeries(bot, series_name);
  SheetSync::syncTodo(bot);

  event.edit_original_response(dpp::message(
      "Chapter **" + chapter_name + "** deleted from **" + series_name + "**."));
}

void doMoveAssignment(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  ChaptersRepository chapters_repo;
  ChapterAssignmentsRepository assignments_repo;
  TasksRepository tasks_repo;
  RoleTasksRepository role_tasks_repo;
  DiscordIdentityRepository identity_repo;
  UserRolesRepository user_roles_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("series"));
  const std::string chapter_name = std::get<std::string>(event.get_parameter("chapter"));
  const dpp::snowflake from_discord_id = std::get<dpp::snowflake>(event.get_parameter("from_user"));
  const dpp::snowflake to_discord_id = std::get<dpp::snowflake>(event.get_parameter("to_user"));
  const std::string task_name = std::get<std::string>(event.get_parameter("task"));

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  if(maybe_series->status != SeriesStatus::active) {
    event.edit_original_response(dpp::message(
        "Series **" + series_name + "** is not active. Set its status to Active before modifying assignments."));
    return;
  }

  const auto maybe_chapter = chapters_repo.findByDisplayKey(session.wtx(), maybe_series->id, chapter_name);
  if(!maybe_chapter) {
    event.edit_original_response(dpp::message("Chapter **" + chapter_name + "** does not exist in **" + series_name + "**."));
    return;
  }

  if(maybe_chapter->status != ChapterStatus::in_progress && maybe_chapter->status != ChapterStatus::queued) {
    event.edit_original_response(dpp::message(
        "Chapter **" + chapter_name + "** is not in progress or queued. Set its status to In Progress or Queued before modifying assignments."));
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

  BotSettingsRepository settings_repo;
  if(settings_repo.get(session.wtx(), "role_check_enabled")) {
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
  }

  if(!assignments_repo.exists(session.wtx(), *maybe_from_id, maybe_chapter->id, maybe_task->id)) {
    event.edit_original_response(dpp::message("<@" + std::to_string(from_discord_id) + "> is not assigned to **" + chapter_name + "** for **" + task_name + "**."));
    return;
  }

  if(assignments_repo.exists(session.wtx(), *maybe_from_id, maybe_chapter->id, maybe_task->id, true)) {
    event.edit_original_response(dpp::message("That assignment is already completed and cannot be moved."));
    return;
  }

  if(assignments_repo.exists(session.wtx(), *maybe_to_id, maybe_chapter->id, maybe_task->id)) {
    event.edit_original_response(dpp::message("<@" + std::to_string(to_discord_id) + "> is already assigned to **" + chapter_name + "** for **" + task_name + "**."));
    return;
  }

  assignments_repo.remove(session.wtx(), *maybe_from_id, maybe_chapter->id, maybe_task->id);
  assignments_repo.create(session.wtx(), *maybe_to_id, maybe_chapter->id, maybe_task->id);
  session.commit();
  SheetSync::syncSeries(bot, series_name);
  SheetSync::syncTodo(bot);

  event.edit_original_response(dpp::message(
      "Moved **" + chapter_name + "** / **" + task_name + "** assignment from <@" + std::to_string(from_discord_id) + "> to <@" + std::to_string(to_discord_id) + ">."));
}

void doBulkAdd(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  ChaptersRepository chapters_repo;
  SeriesAssignmentsRepository series_assignments_repo;
  ChapterAssignmentsRepository chapter_assignments_repo;
  SeriesAssignmentPlaceholdersRepository series_placeholder_repo;
  ChapterAssignmentPlaceholdersRepository chapter_placeholder_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("series"));
  const std::string chapters_input = std::get<std::string>(event.get_parameter("chapters"));

  std::string parse_error;
  const auto chapter_numbers = BotUtils::parseChapterNumbers(chapters_input, parse_error);
  if(chapter_numbers.empty()) {
    event.edit_original_response(dpp::message(parse_error));
    return;
  }

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  if(maybe_series->status != SeriesStatus::active) {
    event.edit_original_response(dpp::message(
        "Series **" + series_name + "** is not active. Set its status to Active before adding chapters."));
    return;
  }

  const auto default_assignments = series_assignments_repo.listBySeries(session.wtx(), maybe_series->id);
  const auto placeholder_task_ids = series_placeholder_repo.listTaskIdsBySeries(session.wtx(), maybe_series->id);

  // Determine initial statuses. If the series already has an in_progress chapter,
  // all new chapters are queued. Otherwise, the first (lowest-numbered) new chapter
  // gets in_progress and the rest get queued.
  const bool series_has_in_progress = !chapters_repo.listBySeries(session.wtx(), maybe_series->id, ChapterStatus::in_progress).empty();
  bool first_chapter_assigned = series_has_in_progress;

  std::vector<double> created_nums;
  std::vector<double> skipped_nums;

  for(const double num : chapter_numbers) {
    if(chapters_repo.findByNumber(session.wtx(), maybe_series->id, num)) {
      skipped_nums.push_back(num);
      continue;
    }
    const ChapterStatus ch_status = first_chapter_assigned ? ChapterStatus::queued : ChapterStatus::in_progress;
    first_chapter_assigned = true;
    const int chapter_id = chapters_repo.create(session.wtx(), maybe_series->id, num, std::nullopt, std::nullopt, ch_status);
    for(const auto &assignment : default_assignments) {
      chapter_assignments_repo.create(session.wtx(), assignment.user_id, chapter_id, assignment.task_id);
    }
    // Cascade series-level placeholders to the new chapter.
    for(const int task_id : placeholder_task_ids) {
      chapter_placeholder_repo.create(session.wtx(), chapter_id, task_id);
    }
    created_nums.push_back(num);
  }

  if(!created_nums.empty()) {
    session.commit();
    SheetSync::syncSeries(bot, series_name);
    SheetSync::syncTodo(bot);
  }

  std::string msg;
  if(!created_nums.empty()) {
    std::string list;
    for(size_t i = 0; i < created_nums.size(); ++i) {
      if(i > 0) {
        list += ", ";
}
      list += "Ch." + fmtChapterNumber(created_nums[i]);
    }
    msg += "Added " + std::to_string(created_nums.size()) + " chapter(s) to **" + series_name + "**: " + list + ".";
  } else {
    msg += "No new chapters added.";
  }
  if(!skipped_nums.empty()) {
    std::string skip_list;
    for(size_t i = 0; i < skipped_nums.size(); ++i) {
      if(i > 0) {
        skip_list += ", ";
}
      skip_list += "Ch." + fmtChapterNumber(skipped_nums[i]);
    }
    msg += "\nSkipped (already exist): " + skip_list + ".";
  }
  event.edit_original_response(dpp::message(msg));
}

void doAddPlaceholder(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  ChaptersRepository chapters_repo;
  TasksRepository tasks_repo;
  ChapterAssignmentPlaceholdersRepository placeholder_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("series"));
  const std::string chapter_name = std::get<std::string>(event.get_parameter("chapter"));
  const std::string task_name = std::get<std::string>(event.get_parameter("task"));

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  if(maybe_series->status != SeriesStatus::active) {
    event.edit_original_response(dpp::message(
        "Series **" + series_name + "** is not active. Set its status to Active before modifying assignments."));
    return;
  }

  const auto maybe_chapter = chapters_repo.findByDisplayKey(session.wtx(), maybe_series->id, chapter_name);
  if(!maybe_chapter) {
    event.edit_original_response(dpp::message("Chapter **" + chapter_name + "** does not exist in **" + series_name + "**."));
    return;
  }

  if(maybe_chapter->status != ChapterStatus::in_progress && maybe_chapter->status != ChapterStatus::queued) {
    event.edit_original_response(dpp::message(
        "Chapter **" + chapter_name + "** is not in progress or queued. Set its status to In Progress or Queued before modifying assignments."));
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

  placeholder_repo.create(session.wtx(), maybe_chapter->id, maybe_task->id);
  const int total = placeholder_repo.count(session.wtx(), maybe_chapter->id, maybe_task->id);
  session.commit();
  SheetSync::syncSeries(bot, series_name);
  SheetSync::syncTodo(bot);

  event.edit_original_response(dpp::message(
      "Added placeholder for **" + task_name + "** on **" + chapter_name + "** (" + std::to_string(total) + " total)."));
}

void doRemovePlaceholder(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  ChaptersRepository chapters_repo;
  TasksRepository tasks_repo;
  ChapterAssignmentPlaceholdersRepository placeholder_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("series"));
  const std::string chapter_name = std::get<std::string>(event.get_parameter("chapter"));
  const std::string task_name = std::get<std::string>(event.get_parameter("task"));

  const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  if(maybe_series->status != SeriesStatus::active) {
    event.edit_original_response(dpp::message(
        "Series **" + series_name + "** is not active. Set its status to Active before modifying assignments."));
    return;
  }

  const auto maybe_chapter = chapters_repo.findByDisplayKey(session.wtx(), maybe_series->id, chapter_name);
  if(!maybe_chapter) {
    event.edit_original_response(dpp::message("Chapter **" + chapter_name + "** does not exist in **" + series_name + "**."));
    return;
  }

  if(maybe_chapter->status != ChapterStatus::in_progress && maybe_chapter->status != ChapterStatus::queued) {
    event.edit_original_response(dpp::message(
        "Chapter **" + chapter_name + "** is not in progress or queued. Set its status to In Progress or Queued before modifying assignments."));
    return;
  }

  const auto maybe_task = tasks_repo.findByName(session.wtx(), task_name);
  if(!maybe_task) {
    event.edit_original_response(dpp::message("Task **" + task_name + "** does not exist."));
    return;
  }

  if(placeholder_repo.count(session.wtx(), maybe_chapter->id, maybe_task->id) == 0) {
    event.edit_original_response(dpp::message("No placeholder exists for **" + task_name + "** on **" + chapter_name + "**."));
    return;
  }

  placeholder_repo.removeAll(session.wtx(), maybe_chapter->id, maybe_task->id);
  session.commit();
  SheetSync::syncSeries(bot, series_name);
  SheetSync::syncTodo(bot);

  event.edit_original_response(dpp::message(
      "Removed all placeholders for **" + task_name + "** on **" + chapter_name + "**."));
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
    } else if(sub == "uncomplete") {
      doUncomplete(bot, event, session);
    } else if(sub == "remove") {
      doRemove(bot, event, session, user_perm);
    } else if(sub == "bulk-add") {
      doBulkAdd(bot, event, session);
    } else if(sub == "move-assignment") {
      doMoveAssignment(bot, event, session);
    } else if(sub == "add-placeholder") {
      doAddPlaceholder(bot, event, session);
    } else if(sub == "remove-placeholder") {
      doRemovePlaceholder(bot, event, session);
    }
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
    if(key == "add/series" || key == "set-status/series" || key == "assign/series" || key == "unassign/series" || key == "uncomplete/series" || key == "remove/series" || key == "bulk-add/series" || key == "move-assignment/series" || key == "add-placeholder/series" || key == "remove-placeholder/series") {
      SeriesRepository series_repo;
      for(const auto &s : series_repo.list(session.wtx())) {
        if(lower_input.empty() || to_lower(s.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(s.name, s.name));
        }
      }
    }
    // Chapter name options — filters based on the already-typed series
    else if(key == "set-status/chapter" || key == "assign/chapter" || key == "unassign/chapter" || key == "uncomplete/chapter" || key == "remove/chapter" || key == "move-assignment/chapter" || key == "add-placeholder/chapter" || key == "remove-placeholder/chapter") {
      const std::string series_ctx = BotUtils::getAutoCompleteContext(event, "series");
      if(!series_ctx.empty()) {
        SeriesRepository series_repo;
        ChaptersRepository chapters_repo;
        const auto maybe_series = series_repo.findByName(session.wtx(), series_ctx);
        if(maybe_series) {
          for(const auto &c : chapters_repo.listBySeries(session.wtx(), maybe_series->id)) {
            const std::string display = c.name ? *c.name : "Ch." + fmtChapterNumber(c.number);
            if(lower_input.empty() || to_lower(display).find(lower_input) != std::string::npos) {
              r.add_autocomplete_choice(dpp::command_option_choice(display, display));
            }
          }
        }
      }
    }
    // Task name options
    else if(key == "assign/task" || key == "unassign/task" || key == "uncomplete/task" || key == "move-assignment/task" || key == "add-placeholder/task" || key == "remove-placeholder/task") {
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
