// Associated Header Include
#include "bot/eventHandlers/commands/list/Info.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "bot/utils/DateUtils.hpp"
#include "bot/utils/GetAutoCompleteContext.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/ChapterAssignments.hpp"
#include "db/repositories/Chapters.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/Series.hpp"
#include "db/repositories/SeriesAssignments.hpp"
#include "db/repositories/User.hpp"
#include "db/repositories/UserAliases.hpp"
#include "types/ChapterStatus.hpp"
#include "types/Permission.hpp"
#include "types/SeriesStatus.hpp"

// Standard Includes
#include <algorithm>
#include <cmath>
#include <iostream>
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

void doSeriesInfo(const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  SeriesAssignmentsRepository series_assignments_repo;
  ChaptersRepository chapters_repo;

  const std::string name = std::get<std::string>(event.get_parameter("name"));

  const auto maybe_series = series_repo.findByName(session.rtx(), name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + name + "** does not exist."));
    return;
  }

  const auto &series = *maybe_series;
  const auto crew = series_assignments_repo.listBySeriesWithDetails(session.rtx(), series.id);
  const auto chapters = chapters_repo.listWithStats(session.rtx(), series.id);

  // Build response
  std::string out;

  // Header
  out += "**" + series.name + "** (" + seriesStatusToString(series.status) + ")\n";
  out += "Added: " + BotUtils::toDiscordTimestamp(series.added_at);
  if(series.closed_at) {
    out += " | Closed: " + BotUtils::toDiscordTimestamp(*series.closed_at);
  }
  out += "\n";

  // Default crew — rows already sorted by task then user from SQL
  out += "\n**Default Crew**\n";
  if(crew.empty()) {
    out += "(none)\n";
  } else {
    std::string cur_task;
    for(const auto &c : crew) {
      if(c.task_name != cur_task) {
        if(!cur_task.empty()) {
          out += "\n";
        }
        out += "- " + c.task_name + ": ";
        cur_task = c.task_name;
      } else {
        out += ", ";
      }
      out += c.user_display;
    }
    out += "\n";
  }

  // Chapters
  out += "\n**Chapters (" + std::to_string(chapters.size()) + ")**\n";
  if(chapters.empty()) {
    out += "(none)\n";
  } else {
    constexpr size_t max_shown = 15;
    const size_t shown = std::min(chapters.size(), max_shown);
    for(size_t i = 0; i < shown; ++i) {
      const auto &c = chapters[i];
      std::string line = "- ";
      if(c.volume) {
        line += "Vol." + std::to_string(*c.volume) + " ";
      }
      line += "Ch." + fmtChapterNumber(c.number);
      if(c.name) {
        line += " " + *c.name;
      }
      line += " (" + chapterStatusToString(c.status) + ")";
      line += " — " + std::to_string(c.completed_tasks) + "/" + std::to_string(c.total_tasks) + " tasks";
      out += line + "\n";
    }
    if(chapters.size() > max_shown) {
      out += "… and " + std::to_string(chapters.size() - max_shown) +
             " more — use /list-chapters to see all\n";
    }
  }

  event.edit_original_response(dpp::message(out));
}

void doChapterInfo(const dpp::slashcommand_t &event, DbSession &session) {
  SeriesRepository series_repo;
  ChaptersRepository chapters_repo;
  ChapterAssignmentsRepository chapter_assignments_repo;

  const std::string series_name = std::get<std::string>(event.get_parameter("series"));
  const std::string chapter_name = std::get<std::string>(event.get_parameter("chapter"));

  const auto maybe_series = series_repo.findByName(session.rtx(), series_name);
  if(!maybe_series) {
    event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
    return;
  }

  const auto maybe_chapter = chapters_repo.findByDisplayKey(session.rtx(), maybe_series->id, chapter_name);
  if(!maybe_chapter) {
    event.edit_original_response(dpp::message("Chapter **" + chapter_name + "** not found in series **" + series_name + "**."));
    return;
  }

  const auto &ch = *maybe_chapter;
  const auto assignments = chapter_assignments_repo.listByChapterWithDetails(session.rtx(), ch.id);

  // Build response
  std::string out;

  // Header
  out += "**" + ch.name.value_or("Ch." + fmtChapterNumber(ch.number)) + "** · " + maybe_series->name + "\n";
  std::string meta;
  if(ch.volume) {
    meta += "Vol." + std::to_string(*ch.volume) + " · ";
  }
  meta += "Ch." + fmtChapterNumber(ch.number) + " · " + chapterStatusToString(ch.status);
  out += meta + "\n";
  out += "Added: " + BotUtils::toDiscordTimestamp(ch.added_at);
  if(ch.closed_at) {
    out += " | Closed: " + BotUtils::toDiscordTimestamp(*ch.closed_at);
  }
  out += "\n";

  // Assignments — group users by task name
  out += "\n**Assignments (" + std::to_string(assignments.size()) + ")**\n";
  if(assignments.empty()) {
    out += "(none)\n";
  } else {
    std::string cur_task;
    for(const auto &a : assignments) {
      if(a.task_name != cur_task) {
        out += a.task_name + ":\n";
        cur_task = a.task_name;
      }
      out += "  - " + a.user_display;
      if(a.completed_at) {
        out += " ✅ " + BotUtils::toDiscordTimestamp(*a.completed_at);
      } else {
        out += " ⬜";
      }
      out += "\n";
    }
  }

  event.edit_original_response(dpp::message(out));
}

void doUserInfo(const dpp::slashcommand_t &event, DbSession &session, int caller_user_id, Permission caller_permission) {
  DiscordIdentityRepository identity_repo;
  UserRepository user_repo;
  UserAliasesRepository alias_repo;
  ChapterAssignmentsRepository chapter_assignments_repo;

  int resolved_user_id;
  const auto &user_param = event.get_parameter("user");
  dpp::snowflake target_snowflake{};
  if(const auto *p = std::get_if<dpp::snowflake>(&user_param)) {
    target_snowflake = *p;
  }
  if(!target_snowflake.empty()) {
    if(caller_permission < Permission::manager) {
      event.edit_original_response(dpp::message("You lack the permission to view other users info."));
      return;
    }
    const auto maybe_target = identity_repo.findUserIdByDiscordId(session.rtx(), static_cast<int64_t>(target_snowflake));
    if(!maybe_target) {
      event.edit_original_response(dpp::message("That user is not registered."));
      return;
    }
    resolved_user_id = *maybe_target;
  } else {
    resolved_user_id = caller_user_id;
  }

  const auto maybe_user = user_repo.findById(session.rtx(), resolved_user_id);
  if(!maybe_user) {
    event.edit_original_response(dpp::message("User not found."));
    return;
  }
  const auto &u = *maybe_user;

  const auto maybe_alias = alias_repo.read(session.rtx(), resolved_user_id);
  const auto maybe_identity = identity_repo.findActiveByUserId(session.rtx(), resolved_user_id);
  const auto active_series = chapter_assignments_repo.listActiveSeriesForUser(session.rtx(), resolved_user_id);
  const int total_series = chapter_assignments_repo.countTotalSeriesForUser(session.rtx(), resolved_user_id);

  std::string permission_str;
  switch(u.permission_level) {
    case Permission::standard:
      permission_str = "Standard";
      break;
    case Permission::manager:
      permission_str = "Manager";
      break;
    case Permission::supermanager:
      permission_str = "Supermanager";
      break;
  }

  std::string out;
  out += "**" + u.display_name + "**\n";
  out += "Username: " + (u.name ? "**" + *u.name + "**" : "N/A") + "\n";
  out += "Alias: " + (maybe_alias ? "**" + *maybe_alias + "**" : "N/A") + "\n";
  out += "Permission: " + permission_str + "\n";
  out += "Joined: " + BotUtils::toDiscordTimestamp(u.joined_at) + "\n";
  if(u.left_at) {
    out += "Left: " + BotUtils::toDiscordTimestamp(*u.left_at) + "\n";
  }

  if(maybe_identity) {
    out += "Discord: <@" + std::to_string(maybe_identity->discord_id) + ">" + " (linked " + BotUtils::toDiscordTimestamp(maybe_identity->linked_at) + ")\n";
  } else {
    out += "Discord: N/A\n";
  }

  out += "\n**Currently Working On (" + std::to_string(active_series.size()) + ")**\n";
  if(active_series.empty()) {
    out += "(none)\n";
  } else {
    for(const auto &[sid, sname] : active_series) {
      out += "• " + sname + "\n";
    }
  }
  out += "\n**Total Series Worked On:** " + std::to_string(total_series);

  event.edit_original_response(dpp::message(out));
}

} // namespace

void Commands::info(Bot &bot, const dpp::slashcommand_t &event) {
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

    const auto maybe_user_id = identity_repo.findUserIdByDiscordId(session.rtx(), discord_id);
    if(!maybe_user_id) {
      event.edit_original_response(dpp::message("You are not registered. Please run /register first."));
      return;
    }
    const Permission perm = user_repo.getPermissionLevel(session.rtx(), *maybe_user_id);

    if(sub == "series" || sub == "chapter") {
      if(perm < Permission::manager) {
        event.edit_original_response(dpp::message("You lack the permission to perform this action."));
        return;
      }
      if(sub == "series") {
        doSeriesInfo(event, session);
      } else {
        doChapterInfo(event, session);
      }
    } else if(sub == "user") {
      doUserInfo(event, session, *maybe_user_id, perm);
    }
  } catch(const std::exception &e) {
    std::cerr << "info failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to retrieve info. Contact the administrator to resolve this issue."));
  }
}

void Commands::infoAutocomplete(Bot &bot, const std::string &key, const std::string &input, const dpp::autocomplete_t &event) {
  dpp::interaction_response r(dpp::ir_autocomplete_reply);

  try {
    DbSession session(bot.getPool());

    auto to_lower = [](std::string s) {
      std::transform(s.begin(), s.end(), s.begin(), ::tolower);
      return s;
    };
    const std::string lower_input = to_lower(input);

    if(key == "series/name" || key == "chapter/series") {
      SeriesRepository series_repo;
      for(const auto &s : series_repo.list(session.rtx())) {
        if(lower_input.empty() || to_lower(s.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(s.name, s.name));
        }
      }
    } else if(key == "chapter/chapter") {
      // Read the already-filled series option from the autocomplete context
      std::string series_ctx = BotUtils::getAutoCompleteContext(event, "series");
      if(!series_ctx.empty()) {
        SeriesRepository series_repo;
        ChaptersRepository chapters_repo;
        const auto maybe_series = series_repo.findByName(session.rtx(), series_ctx);
        if(maybe_series) {
          for(const auto &c : chapters_repo.listBySeries(session.rtx(), maybe_series->id)) {
            const std::string display = c.name ? *c.name : "Ch." + fmtChapterNumber(c.number);
            if(lower_input.empty() || to_lower(display).find(lower_input) != std::string::npos) {
              r.add_autocomplete_choice(dpp::command_option_choice(display, display));
            }
          }
        }
      }
    }
  } catch(const std::exception &e) {
    std::cerr << "infoAutocomplete failed for key=" << key << ": " << e.what() << std::endl;
  }

  bot.getCore().interaction_response_create(event.command.id, event.command.token, r);
}
