// Associated Header Include
#include "bot/eventHandlers/commands/list/ListChapters.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "bot/utils/DateUtils.hpp"
#include "bot/utils/ListUtils.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/Chapters.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/Series.hpp"
#include "db/repositories/User.hpp"
#include "types/ChapterStatus.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <algorithm>
#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

void Commands::listChapters(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository identity_repo;
    UserRepository user_repo;
    SeriesRepository series_repo;
    ChaptersRepository chapters_repo;

    const auto maybe_user_id = identity_repo.findUserIdByDiscordId(session.rtx(), discord_id);
    if(!maybe_user_id) {
      event.edit_original_response(dpp::message("You are not registered. Please run /register first."));
      return;
    }

    if(user_repo.getPermissionLevel(session.rtx(), *maybe_user_id) < Permission::manager) {
      event.edit_original_response(dpp::message("You lack the permission to perform this action."));
      return;
    }

    // Parse optional series filter
    std::optional<int> series_id;
    std::string series_label;
    const auto series_param = event.get_parameter("series");
    if(std::holds_alternative<std::string>(series_param)) {
      const std::string series_name = std::get<std::string>(series_param);
      const auto maybe_series = series_repo.findByName(session.rtx(), series_name);
      if(!maybe_series) {
        event.edit_original_response(dpp::message("Series **" + series_name + "** does not exist."));
        return;
      }
      series_id = maybe_series->id;
      series_label = maybe_series->name;
    }

    // Parse optional chapter status filter
    std::optional<ChapterStatus> status_filter;
    const auto status_param = event.get_parameter("status");
    if(std::holds_alternative<std::string>(status_param)) {
      status_filter = chapterStatusFromString(std::get<std::string>(status_param));
    }

    // Parse optional sort order
    bool sort_chronological = false;
    const auto sort_param = event.get_parameter("sort");
    if(std::holds_alternative<std::string>(sort_param)) {
      sort_chronological = (std::get<std::string>(sort_param) == "chronological");
    }

    const auto chapter_list = chapters_repo.listWithStats(session.rtx(), series_id, status_filter, sort_chronological);

    const bool show_series = !series_id.has_value();

    std::vector<std::string> lines;
    lines.reserve(chapter_list.size());
    for(const auto &c : chapter_list) {
      const std::string date = BotUtils::toDiscordTimestamp(c.closed_at ? *c.closed_at : c.added_at);
      const std::string task_str = std::to_string(c.completed_tasks) + "/" + std::to_string(c.total_tasks) + " tasks";
      const std::string status_str = chapterStatusToString(c.status);

      std::string line = "- ";
      if(show_series) {
        line += "[" + c.series_name + "] ";
      }
      line += "**" + c.name + "** (" + status_str + ") — " + task_str + " | " + date;
      lines.push_back(std::move(line));
    }

    std::string title = "**Chapter List**";
    if(!series_label.empty()) {
      title += " — " + series_label;
    }
    if(status_filter) {
      title += " (" + chapterStatusToString(*status_filter) + ")";
    }

    auto pages = BotUtils::makePages(title, lines);

    if(pages.size() == 1) {
      event.edit_original_response(dpp::message(pages[0]));
      return;
    }

    const std::string token = event.command.token;
    const dpp::snowflake user_id = static_cast<dpp::snowflake>(event.command.usr.id);
    const dpp::snowflake chan_id = event.command.channel_id;

    event.edit_original_response(
        dpp::message(pages[0]),
        [&bot, pages = std::move(pages), token, user_id, chan_id](const dpp::confirmation_callback_t &cb) {
          if(cb.is_error()){
            return;
          }
          const auto &msg = cb.get<dpp::message>();
          bot.registerPagination(msg.id, Bot::PaginationState{
                                             pages,
                                             0,
                                             user_id,
                                             chan_id,
                                             token,
                                             std::chrono::steady_clock::now() + std::chrono::minutes(5)});
          bot.getCore().message_add_reaction(msg.id, chan_id, "◀️");
          bot.getCore().message_add_reaction(msg.id, chan_id, "▶️");
        });
  } catch(const std::exception &e) {
    std::cerr << "listChapters failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to list chapters. Contact the administrator to resolve this issue."));
  }
}

void Commands::listChaptersAutocomplete(Bot &bot, const std::string &key, const std::string &input, const dpp::autocomplete_t &event) {
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
    std::cerr << "listChaptersAutocomplete failed for key=" << key << ": " << e.what() << std::endl;
  }

  bot.getCore().interaction_response_create(event.command.id, event.command.token, r);
}
