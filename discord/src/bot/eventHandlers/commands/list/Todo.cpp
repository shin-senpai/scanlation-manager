// Associated Header Include
#include "bot/eventHandlers/commands/list/Todo.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/ChapterAssignments.hpp"
#include "db/repositories/Chapters.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/Series.hpp"
#include "db/repositories/SeriesAssignments.hpp"
#include "db/repositories/TaskDependencies.hpp"
#include "db/repositories/Tasks.hpp"
#include "db/repositories/User.hpp"
#include "models/ModelChapterAssignment.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

namespace {
uint64_t packU32ToU64(uint32_t m, uint32_t n) {
  return ((static_cast<uint64_t>(m) << 32) | n);
}

std::vector<ChapterAssignment> resolveNextInLineAssignments(
    std::vector<ChapterAssignment> user_incomplete_assignments,
    const std::unordered_map<int, std::vector<int>> &task_deps_map,
    const std::vector<ChapterAssignment> &all_incomplete_assignments) {

  std::vector<ChapterAssignment> next_in_line_assignments;

  if(task_deps_map.empty()) {
    return user_incomplete_assignments;
  }

  std::unordered_set<uint64_t> incomplete_assignment_set;
  incomplete_assignment_set.reserve(all_incomplete_assignments.size());
  for(const auto &assignment : all_incomplete_assignments) {
    incomplete_assignment_set.insert(packU32ToU64(assignment.chapter_id, assignment.task_id));
  }

  for(const auto &assignment : user_incomplete_assignments) {
    auto it = task_deps_map.find(assignment.task_id);
    if(it == task_deps_map.end()) {
      next_in_line_assignments.emplace_back(assignment);
      continue;
    }

    bool dep_unmet = false;
    for(const auto &dep : it->second) {
      if(incomplete_assignment_set.contains(packU32ToU64(assignment.chapter_id, dep))) {
        dep_unmet = true;
        break;
      }
    }

    if(!dep_unmet) {
      next_in_line_assignments.emplace_back(assignment);
    }
  }

  return next_in_line_assignments;
}
} // namespace

void Commands::todo(Bot &bot, const dpp::slashcommand_t event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository identity_repo;
    UserRepository user_repo;
    SeriesAssignmentsRepository series_assignments_repo;
    ChapterAssignmentsRepository chapter_assignments_repo;
    TaskDependenciesRepository task_deps_repo;
    ChaptersRepository chapters_repo;
    SeriesRepository series_repo;
    TasksRepository tasks_repo;

    int resolved_user_id;
    const auto maybe_user_id = identity_repo.findUserIdByDiscordId(session.wtx(), discord_id);
    if(!maybe_user_id) {
      event.edit_original_response(dpp::message("You are not registered. Please run /register first."));
      return;
    }

    const dpp::snowflake target_discord_id = std::get<dpp::snowflake>(event.get_parameter("user"));
    if(!target_discord_id.empty()) {
      Permission permission_level = user_repo.getPermissionLevel(session.wtx(), *maybe_user_id);
      if(permission_level < Permission::manager) {
        event.edit_original_response(dpp::message("You lack the permission to check the todo list of other users."));
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
    }

    const auto series = chapter_assignments_repo.listDistinctSeriesByUser(session.rtx(), resolved_user_id, false);
    auto user_incomplete_assignments = chapter_assignments_repo.listByUser(session.rtx(), resolved_user_id, std::nullopt, false);

    std::vector<int> chapters;
    for(const auto &a : user_incomplete_assignments) {
      chapters.emplace_back(a.chapter_id);
    }

    const auto task_deps_map = task_deps_repo.listAll(session.rtx());
    const auto all_incomplete_assignments = chapter_assignments_repo.listBySeries(session.rtx(), series, chapters, false);

    auto next_in_line = resolveNextInLineAssignments(std::move(user_incomplete_assignments), task_deps_map, all_incomplete_assignments);

    if(next_in_line.empty()) {
      const bool is_self = !target_discord_id || target_discord_id == event.command.usr.id;
      event.edit_original_response(dpp::message(is_self ? "You have no pending tasks." : "That user has no pending tasks."));
      return;
    }

    std::unordered_map<int, std::string> series_name_map;
    for(const auto &s : series_repo.list(session.rtx())) {
      series_name_map[s.id] = s.name;
    }

    std::unordered_map<int, Chapter> chapter_map;
    for(auto &c : chapters_repo.listBySeriesIds(session.rtx(), series)) {
      chapter_map.emplace(c.id, std::move(c));
    }

    std::unordered_map<int, std::string> task_name_map;
    for(const auto &t : tasks_repo.listAll(session.rtx())) {
      task_name_map[t.id] = t.name;
    }

    // Group: series name → chapter name → [task names]
    std::map<std::string, std::map<std::string, std::vector<std::string>>> grouped;
    for(const auto &a : next_in_line) {
      const auto ch = chapter_map.at(a.chapter_id);
      const std::string &sname = series_name_map.at(ch.series_id);
      const std::string tname = task_name_map.at(a.task_id);
      grouped[sname][ch.name].push_back(tname);
    }

    std::string msg = "**To-do list:**\n";
    for(const auto &[sname, chapter_tasks] : grouped) {
      msg += "\n**" + sname + "**\n";
      for(const auto &[cname, tasks] : chapter_tasks) {
        msg += "• **" + cname + "** — ";
        for(size_t i = 0; i < tasks.size(); ++i) {
          if(i > 0) {
            msg += ", ";
}
          msg += tasks[i];
        }
        msg += "\n";
      }
    }

    event.edit_original_response(dpp::message(msg));
  } catch(const std::exception &e) {
    std::cerr << "todo failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to fetch to-do list. Contact the administrator to resolve this issue."));
  }
}