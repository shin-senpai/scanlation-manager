// Associated Header Include
#include "bot/eventHandlers/commands/modify/WorkProgress.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "bot/utils/GetAutoCompleteContext.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/ChapterAssignments.hpp"
#include "db/repositories/Chapters.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/Series.hpp"
#include "db/repositories/TaskDependencies.hpp"
#include "db/repositories/Tasks.hpp"

// Standard Includes
#include <algorithm>
#include <iostream>
#include <map>
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

void Commands::workProgress(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository identity_repo;
    SeriesRepository series_repo;
    ChaptersRepository chapters_repo;
    TasksRepository tasks_repo;
    TaskDependenciesRepository task_deps_repo;
    ChapterAssignmentsRepository assignments_repo;

    const auto maybe_user_id = identity_repo.findUserIdByDiscordId(session.rtx(), discord_id);
    if(!maybe_user_id) {
      event.edit_original_response(dpp::message("You are not registered. Please run /register first."));
      return;
    }
    const int user_id = static_cast<int>(*maybe_user_id);

    const std::string series_name = std::get<std::string>(event.get_parameter("series"));
    const std::string chapter_name = std::get<std::string>(event.get_parameter("chapter"));
    const std::string task_name = std::get<std::string>(event.get_parameter("task"));

    const auto maybe_series = series_repo.findByName(session.rtx(), series_name);
    if(!maybe_series) {
      event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
      return;
    }

    const auto maybe_chapter = chapters_repo.findByName(session.rtx(), maybe_series->id, chapter_name);
    if(!maybe_chapter) {
      event.edit_original_response(dpp::message("Chapter **" + chapter_name + "** not found in **" + series_name + "**."));
      return;
    }

    const auto maybe_task = tasks_repo.findByName(session.rtx(), task_name);
    if(!maybe_task) {
      event.edit_original_response(dpp::message("Task **" + task_name + "** does not exist."));
      return;
    }

    if(!assignments_repo.exists(session.rtx(), user_id, maybe_chapter->id, maybe_task->id)) {
      event.edit_original_response(dpp::message("You are not assigned to **" + task_name + "** for **" + chapter_name + "**."));
      return;
    }

    if(assignments_repo.exists(session.rtx(), user_id, maybe_chapter->id, maybe_task->id, true)) {
      event.edit_original_response(dpp::message("You have already completed **" + task_name + "** for **" + chapter_name + "**."));
      return;
    }

    if(task_deps_repo.findFirstBlockingDependency(session.rtx(), maybe_chapter->id, maybe_task->id)) {
      event.edit_original_response(dpp::message(
          "Cannot complete **" + task_name + "**: still has incomplete dependencies.**"));
      return;
    }

    assignments_repo.setCompleted(session.wtx(), user_id, maybe_chapter->id, maybe_task->id);
    session.commit();

    std::string msg = "Marked **" + task_name + "** complete for **" + chapter_name + "** (" + series_name + ").";

    const auto dependents = task_deps_repo.findDependentAssignees(session.rtx(), maybe_chapter->id, maybe_task->id);
    if(!dependents.empty()) {
      std::map<std::string, std::string> task_pings;
      for(const auto &[did, dep_task] : dependents) {
        task_pings[dep_task] += "<@" + std::to_string(did) + "> ";
      }
      for(const auto &[dep_task, pings] : task_pings) {
        msg += "\n" + pings + "— **" + task_name + "** is done, you can now proceed with **" + dep_task + "**.";
      }
    }

    event.edit_original_response(dpp::message(msg));
  } catch(const std::exception &e) {
    std::cerr << "workProgress failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to record progress. Contact the administrator to resolve this issue."));
  }
}

void Commands::workProgressAutocomplete(Bot &bot, const std::string &key, const std::string &input, const dpp::autocomplete_t &event) {
  dpp::interaction_response r(dpp::ir_autocomplete_reply);

  try {
    DbSession session(bot.getPool());

    auto to_lower = [](std::string s) {
      std::transform(s.begin(), s.end(), s.begin(), ::tolower);
      return s;
    };
    const std::string lower_input = to_lower(input);

    if(key == "series") {
      SeriesRepository series_repo;
      for(const auto &s : series_repo.list(session.rtx())) {
        if(lower_input.empty() || to_lower(s.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(s.name, s.name));
        }
      }
    } else if(key == "chapter") {
      const std::string series_name = BotUtils::getAutoCompleteContext(event, "series");

      if(!series_name.empty()) {
        SeriesRepository series_repo;
        ChaptersRepository chapters_repo;
        const auto maybe_series = series_repo.findByName(session.rtx(), series_name);
        if(maybe_series) {
          for(const auto &c : chapters_repo.listBySeries(session.rtx(), maybe_series->id)) {
            if(lower_input.empty() || to_lower(c.name).find(lower_input) != std::string::npos) {
              r.add_autocomplete_choice(dpp::command_option_choice(c.name, c.name));
            }
          }
        }
      }
    } else if(key == "task") {
      const std::string series_name = BotUtils::getAutoCompleteContext(event, "series");
      const std::string chapter_name = BotUtils::getAutoCompleteContext(event, "chapter");
      if(!series_name.empty() && !chapter_name.empty()) {
        const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);
        DiscordIdentityRepository identity_repo;
        SeriesRepository series_repo;
        ChaptersRepository chapters_repo;
        ChapterAssignmentsRepository assignments_repo;
        TasksRepository tasks_repo;

        const auto maybe_user_id = identity_repo.findUserIdByDiscordId(session.rtx(), discord_id);
        const auto maybe_series = series_repo.findByName(session.rtx(), series_name);
        if(maybe_user_id && maybe_series) {
          const auto maybe_chapter = chapters_repo.findByName(session.rtx(), maybe_series->id, chapter_name);
          if(maybe_chapter) {
            for(const auto &a : assignments_repo.listByChapter(session.rtx(), maybe_chapter->id, std::nullopt, false)) {
              if(a.user_id != static_cast<int>(*maybe_user_id)) {
                continue;
}
              const auto maybe_task = tasks_repo.findById(session.rtx(), a.task_id);
              if(!maybe_task) {
                continue;
}
              if(lower_input.empty() || to_lower(maybe_task->name).find(lower_input) != std::string::npos) {
                r.add_autocomplete_choice(dpp::command_option_choice(maybe_task->name, maybe_task->name));
              }
            }
          }
        }
      }
    }
  } catch(const std::exception &e) {
    std::cerr << "workProgressAutocomplete failed for key=" << key << ": " << e.what() << std::endl;
  }

  bot.getCore().interaction_response_create(event.command.id, event.command.token, r);
}
