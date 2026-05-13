// Associated Header Include
#include "bot/eventHandlers/commands/list/UserHistory.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "bot/utils/DateUtils.hpp"
#include "bot/utils/ListUtils.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/ChapterAssignments.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/Series.hpp"
#include "db/repositories/User.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <algorithm>
#include <chrono>
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

} // namespace

void Commands::userHistory(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository identity_repo;
    UserRepository user_repo;
    SeriesRepository series_repo;
    ChapterAssignmentsRepository assignments_repo;

    const auto maybe_caller_id = identity_repo.findUserIdByDiscordId(session.rtx(), discord_id);
    if(!maybe_caller_id) {
      event.edit_original_response(dpp::message("You are not registered. Please run /register first."));
      return;
    }
    const Permission caller_perm = user_repo.getPermissionLevel(session.rtx(), *maybe_caller_id);

    // Resolve target user
    int resolved_user_id;
    std::string target_display;
    const auto &user_param = event.get_parameter("user");
    dpp::snowflake target_snowflake{};
    if(const auto p = std::get_if<dpp::snowflake>(&user_param)) {
      target_snowflake = *p;
    }
    if(!target_snowflake.empty()) {
      if(caller_perm < Permission::manager) {
        event.edit_original_response(dpp::message("You lack the permission to view other users' history."));
        return;
      }
      const auto maybe_target = identity_repo.findUserIdByDiscordId(session.rtx(), static_cast<int64_t>(target_snowflake));
      if(!maybe_target) {
        event.edit_original_response(dpp::message("That user is not registered."));
        return;
      }
      resolved_user_id = *maybe_target;
      target_display = event.command.get_resolved_user(target_snowflake).username;
    } else {
      resolved_user_id = *maybe_caller_id;
      target_display = "You";
    }

    // Resolve optional series filter
    std::optional<int> series_id;
    std::string series_label;
    const auto &series_param = event.get_parameter("series");
    std::string series_name;
    if(const auto *p = std::get_if<std::string>(&series_param)) {
      series_name = *p;
    }
    if(!series_name.empty()) {
      const auto maybe_series = series_repo.findByName(session.rtx(), series_name);
      if(!maybe_series) {
        event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
        return;
      }
      series_id = maybe_series->id;
      series_label = maybe_series->name;
    }

    const auto entries = assignments_repo.listCompletedByUserWithDetails(session.rtx(), resolved_user_id, series_id);

    std::vector<std::string> lines;
    lines.reserve(entries.size());
    for(const auto &e : entries) {
      std::string line = "- [**" + e.series_name + "**] ";
      if(e.volume) {
        line += "Vol." + std::to_string(*e.volume) + " ";
      }
      line += "Ch." + fmtChapterNumber(e.chapter_number);
      if(e.chapter_name) {
        line += " \"" + *e.chapter_name + "\"";
      }
      line += " — " + e.task_name;
      line += " | " + BotUtils::toDiscordTimestamp(e.completed_at);
      lines.push_back(std::move(line));
    }

    std::string title = "**" + (target_display == "You" ? "Your" : target_display + "'s") + " Completed Assignments**";
    if(!series_label.empty()) {
      title += " — " + series_label;
    }

    const auto pages = BotUtils::makePages(title, lines);

    if(pages.size() == 1) {
      event.edit_original_response(dpp::message(pages[0]));
      return;
    }

    const std::string token = event.command.token;
    const dpp::snowflake caller_snowflake = static_cast<dpp::snowflake>(event.command.usr.id);
    const dpp::snowflake chan_id = event.command.channel_id;

    event.edit_original_response(
        dpp::message(pages[0]),
        [&bot, pages = std::move(pages), token, caller_snowflake, chan_id](const dpp::confirmation_callback_t &cb) {
          if(cb.is_error()) {
            return;
          }
          const auto &msg = cb.get<dpp::message>();
          bot.registerPagination(msg.id, Bot::PaginationState{
                                             pages,
                                             0,
                                             caller_snowflake,
                                             chan_id,
                                             token,
                                             std::chrono::steady_clock::now() + std::chrono::minutes(5)});
          bot.getCore().message_add_reaction(msg.id, chan_id, "◀️");
          bot.getCore().message_add_reaction(msg.id, chan_id, "▶️");
        });
  } catch(const std::exception &e) {
    std::cerr << "userHistory failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to retrieve history. Contact the administrator to resolve this issue."));
  }
}

void Commands::userHistoryAutocomplete(Bot &bot, const std::string &key, const std::string &input, const dpp::autocomplete_t &event) {
  dpp::interaction_response r(dpp::ir_autocomplete_reply);

  try {
    if(key == "series") {
      DbSession session(bot.getPool());
      SeriesRepository series_repo;

      auto to_lower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
      };
      const std::string lower_input = to_lower(input);

      for(const auto &s : series_repo.list(session.rtx())) {
        if(lower_input.empty() || to_lower(s.name).find(lower_input) != std::string::npos) {
          r.add_autocomplete_choice(dpp::command_option_choice(s.name, s.name));
        }
      }
    }
  } catch(const std::exception &e) {
    std::cerr << "userHistoryAutocomplete failed for key=" << key << ": " << e.what() << std::endl;
  }

  bot.getCore().interaction_response_create(event.command.id, event.command.token, r);
}
