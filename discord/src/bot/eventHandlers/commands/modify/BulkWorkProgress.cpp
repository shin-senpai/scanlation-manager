// Associated Header Include
#include "bot/eventHandlers/commands/modify/BulkWorkProgress.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "bot/utils/GetAutoCompleteContext.hpp"
#include "bot/utils/ParseChapterNumbers.hpp"
#include "bot/utils/SheetSync.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/ChapterAssignmentPlaceholders.hpp"
#include "db/repositories/ChapterAssignments.hpp"
#include "db/repositories/Chapters.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/Series.hpp"
#include "db/repositories/TaskDependencies.hpp"
#include "db/repositories/Tasks.hpp"
#include "db/repositories/User.hpp"
#include "types/ChapterStatus.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// Third Party Includes
#include <dpp/dispatcher.h>
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

std::string joinChapterList(const std::vector<double> &numbers) {
  std::string out;
  for(size_t i = 0; i < numbers.size(); ++i) {
    if(i > 0) {
      out += ", ";
    }
    out += "Ch." + fmtChapterNumber(numbers[i]);
  }
  return out;
}

} // namespace

void Commands::bulkWorkProgress(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository identity_repo;
    UserRepository user_repo;
    SeriesRepository series_repo;
    ChaptersRepository chapters_repo;
    TasksRepository tasks_repo;
    TaskDependenciesRepository task_deps_repo;
    ChapterAssignmentsRepository assignments_repo;

    const auto maybe_user_id = identity_repo.findUserIdByDiscordId(session.wtx(), discord_id);
    if(!maybe_user_id) {
      event.edit_original_response(dpp::message("You are not registered. Please run /register first."));
      return;
    }

    int resolved_user_id;
    dpp::snowflake target_discord_id{};
    const auto &user_param = event.get_parameter("user");
    if(const auto *id = std::get_if<dpp::snowflake>(&user_param)) {
      target_discord_id = *id;
    }
    if(!target_discord_id.empty()) {
      if(user_repo.getPermissionLevel(session.wtx(), *maybe_user_id) < Permission::manager) {
        event.edit_original_response(dpp::message("You lack the permission to mark progress for other users."));
        return;
      }
      const auto maybe_target_id = identity_repo.findUserIdByDiscordId(session.wtx(), static_cast<int64_t>(target_discord_id));
      if(!maybe_target_id) {
        event.edit_original_response(dpp::message("The target user is not registered."));
        return;
      }
      resolved_user_id = *maybe_target_id;
    } else {
      resolved_user_id = *maybe_user_id;
      target_discord_id = discord_id;
    }

    const std::string series_name = std::get<std::string>(event.get_parameter("series"));
    const std::string task_name = std::get<std::string>(event.get_parameter("task"));
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

    const auto maybe_task = tasks_repo.findByName(session.wtx(), task_name);
    if(!maybe_task) {
      event.edit_original_response(dpp::message("Task **" + task_name + "** does not exist."));
      return;
    }

    // Phase 1: validate all chapters before writing anything
    std::vector<std::string> errors;
    struct ChapterEntry {
      int id;
      double number;
    };
    std::vector<ChapterEntry> valid_chapters;
    valid_chapters.reserve(chapter_numbers.size());

    for(const double num : chapter_numbers) {
      const std::string label = "Ch." + fmtChapterNumber(num);
      const auto maybe_chapter = chapters_repo.findByNumber(session.wtx(), maybe_series->id, num);
      if(!maybe_chapter) {
        errors.push_back(label + ": chapter not found");
        continue;
      }
      if(!assignments_repo.exists(session.wtx(), resolved_user_id, maybe_chapter->id, maybe_task->id)) {
        errors.push_back(label + ": not assigned to **" + task_name + "**");
        continue;
      }
      if(assignments_repo.exists(session.wtx(), resolved_user_id, maybe_chapter->id, maybe_task->id, true)) {
        errors.push_back(label + ": **" + task_name + "** already completed");
        continue;
      }
      const auto blocking = task_deps_repo.findFirstBlockingDependency(session.wtx(), maybe_chapter->id, maybe_task->id);
      if(blocking) {
        errors.push_back(label + ": blocked by incomplete **" + *blocking + "**");
        continue;
      }
      valid_chapters.push_back({maybe_chapter->id, num});
    }

    if(!errors.empty()) {
      std::string msg = "Cannot complete — please fix the following:";
      for(const auto &e : errors) {
        msg += "\n- " + e;
      }
      event.edit_original_response(dpp::message(msg));
      return;
    }

    // Phase 2: mark all complete and collect pings
    // task_name → set of discord IDs to ping (deduplicated across chapters)
    std::map<std::string, std::set<int64_t>> pings_by_task;
    std::vector<ChapterEntry> chapters_to_release;

    for(const auto &ch : valid_chapters) {
      assignments_repo.setCompleted(session.wtx(), resolved_user_id, ch.id, maybe_task->id);
      const auto dependents = task_deps_repo.findDependentAssignees(session.wtx(), ch.id, maybe_task->id);
      for(const auto &[did, dep_task] : dependents) {
        pings_by_task[dep_task].insert(did);
      }
      // Auto-release only when no incomplete assignments remain and no placeholder
      // vacancies exist (a placeholder indicates someone still needs to be found).
      ChapterAssignmentPlaceholdersRepository placeholder_repo;
      if(assignments_repo.listByChapter(session.wtx(), ch.id, std::nullopt, false).empty()
         && !placeholder_repo.existsForChapter(session.wtx(), ch.id)) {
        chapters_to_release.push_back(ch);
      }
    }

    for(const auto &released_ch : chapters_to_release) {
      chapters_repo.updateStatus(session.wtx(), released_ch.id, ChapterStatus::released);
      const auto next_queued_id = chapters_repo.findNextQueuedId(session.wtx(), maybe_series->id, released_ch.number);
      if(next_queued_id) {
        chapters_repo.updateStatus(session.wtx(), *next_queued_id, ChapterStatus::in_progress);
      }
    }

    session.commit();
    SheetSync::syncSeries(bot, series_name);
    SheetSync::syncTodo(bot);

    std::vector<double> completed_nums;
    completed_nums.reserve(valid_chapters.size());
    for(const auto &ch : valid_chapters) {
      completed_nums.push_back(ch.number);
    }

    std::string msg = "<@" + std::to_string(target_discord_id) + "> Marked **" + task_name + "** complete for " + joinChapterList(completed_nums) + " (" + series_name + ").";

    for(const auto &[dep_task, ids] : pings_by_task) {
      std::string ping_str;
      for(const int64_t id : ids) {
        ping_str += "<@" + std::to_string(id) + "> ";
      }
      msg += "\n" + ping_str + "— **" + task_name + "** is done, you can now proceed with **" + dep_task + "**.";
    }

    dpp::message response(msg);
    response.allowed_mentions.parse_users = true;
    event.edit_original_response(response);
  } catch(const std::exception &e) {
    std::cerr << "bulkWorkProgress failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to record progress. Contact the administrator to resolve this issue."));
  }
}

void Commands::bulkWorkProgressAutocomplete(Bot &bot, const std::string &key, const std::string &input, const dpp::autocomplete_t &event) {
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
      for(const auto &s : series_repo.list(session.wtx())) {
        if(lower_input.empty() || to_lower(s.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(s.name, s.name));
        }
      }
    } else if(key == "task") {
      const std::string series_name = BotUtils::getAutoCompleteContext(event, "series");
      const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);
      DiscordIdentityRepository identity_repo;
      const auto maybe_user_id = identity_repo.findUserIdByDiscordId(session.wtx(), discord_id);

      if(!series_name.empty() && maybe_user_id) {
        SeriesRepository series_repo;
        ChapterAssignmentsRepository assignments_repo;
        TasksRepository tasks_repo;
        const auto maybe_series = series_repo.findByName(session.wtx(), series_name);
        if(maybe_series) {
          // Find distinct tasks the user has incomplete assignments for in this series
          const auto all = assignments_repo.listBySeries(session.wtx(), {maybe_series->id}, std::nullopt, false);
          std::vector<int> seen_task_ids;
          for(const auto &a : all) {
            if(a.user_id != static_cast<int>(*maybe_user_id)) {
              continue;
            }
            if(std::find(seen_task_ids.begin(), seen_task_ids.end(), a.task_id) != seen_task_ids.end()) {
              continue;
            }
            seen_task_ids.push_back(a.task_id);
            const auto maybe_task = tasks_repo.findById(session.wtx(), a.task_id);
            if(!maybe_task) {
              continue;
            }
            if(lower_input.empty() || to_lower(maybe_task->name).find(lower_input) != std::string::npos) {
              r.add_autocomplete_choice(dpp::command_option_choice(maybe_task->name, maybe_task->name));
            }
          }
        }
      } else {
        // Fallback: all active tasks
        TasksRepository tasks_repo;
        for(const auto &t : tasks_repo.listAll(session.wtx())) {
          if(lower_input.empty() || to_lower(t.name).find(lower_input) != std::string::npos) {
            r.add_autocomplete_choice(dpp::command_option_choice(t.name, t.name));
          }
        }
      }
    }
  } catch(const std::exception &e) {
    std::cerr << "bulkWorkProgressAutocomplete failed for key=" << key << ": " << e.what() << std::endl;
  }

  bot.getCore().interaction_response_create(event.command.id, event.command.token, r);
}
