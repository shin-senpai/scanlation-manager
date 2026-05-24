// Associated Header Include
#include "bot/eventHandlers/commands/modify/RetireTask.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "bot/utils/SheetSync.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/ChapterAssignmentPlaceholders.hpp"
#include "db/repositories/ChapterAssignments.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/SeriesAssignmentPlaceholders.hpp"
#include "db/repositories/SeriesAssignments.hpp"
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

void Commands::retireTask(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository identity_repo;
    UserRepository user_repo;
    TasksRepository tasks_repo;
    SeriesAssignmentsRepository series_assignments_repo;
    ChapterAssignmentsRepository chapter_assignments_repo;

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

    ChapterAssignmentPlaceholdersRepository placeholder_repo;
    SeriesAssignmentPlaceholdersRepository series_placeholder_repo;
    const auto affected_series = series_assignments_repo.listSeriesNamesByTask(session.wtx(), maybe_task->id);
    series_assignments_repo.removeAllByTask(session.wtx(), maybe_task->id);
    chapter_assignments_repo.removeOutstandingByTask(session.wtx(), maybe_task->id);
    series_placeholder_repo.removeAllByTask(session.wtx(), maybe_task->id);
    placeholder_repo.removeAllByTask(session.wtx(), maybe_task->id);
    tasks_repo.retire(session.wtx(), maybe_task->id);
    session.commit();

    for(const auto &series_name : affected_series) {
      SheetSync::syncSeries(bot, series_name);
    }
    SheetSync::syncTodo(bot);

    event.edit_original_response(dpp::message("Task **" + name + "** retired. Completion history is preserved."));
  } catch(const std::exception &e) {
    std::cerr << "retireTask failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to retire task. Contact the administrator to resolve this issue."));
  }
}

void Commands::retireTaskAutocomplete(Bot &bot, const std::string &key, const std::string &input, const dpp::autocomplete_t &event) {
  dpp::interaction_response r(dpp::ir_autocomplete_reply);
  try {
    DbSession session(bot.getPool());
    auto to_lower = [](std::string s) {
      std::transform(s.begin(), s.end(), s.begin(), ::tolower);
      return s;
    };
    const std::string lower_input = to_lower(input);
    if(key == "name") {
      TasksRepository tasks_repo;
      for(const auto &t : tasks_repo.listAll(session.rtx())) {
        if(lower_input.empty() || to_lower(t.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(t.name, t.name));
        }
      }
    }
  } catch(const std::exception &e) {
    std::cerr << "retireTaskAutocomplete failed for key=" << key << ": " << e.what() << std::endl;
  }
  bot.getCore().interaction_response_create(event.command.id, event.command.token, r);
}
