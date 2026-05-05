// Associated Header Include
#include "bot/eventHandlers/commands/list/ListSeries.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "bot/utils/ListUtils.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/Series.hpp"
#include "db/repositories/User.hpp"
#include "types/Permission.hpp"
#include "types/SeriesStatus.hpp"

// Standard Includes
#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

void Commands::listSeries(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository identity_repo;
    UserRepository user_repo;
    SeriesRepository series_repo;

    const auto maybe_user_id = identity_repo.findUserIdByDiscordId(session.rtx(), discord_id);
    if(!maybe_user_id) {
      event.edit_original_response(dpp::message("You are not registered. Please run /register first."));
      return;
    }

    if(user_repo.getPermissionLevel(session.rtx(), *maybe_user_id) < Permission::manager) {
      event.edit_original_response(dpp::message("You lack the permission to perform this action."));
      return;
    }

    // Parse optional status filter and derive the matching chapter status for the timestamp
    std::optional<SeriesStatus> series_filter;
    std::optional<std::string> chapter_status_for_ts;
    std::string label = "latest";

    const auto status_param = event.get_parameter("status");
    if(std::holds_alternative<std::string>(status_param)) {
      const std::string status_str = std::get<std::string>(status_param);
      series_filter = seriesStatusFromString(status_str);
      switch(*series_filter) {
        case SeriesStatus::completed:
          chapter_status_for_ts = "released";
          label = "latest released";
          break;
        case SeriesStatus::dropped:
          chapter_status_for_ts = "dropped";
          label = "latest dropped";
          break;
        case SeriesStatus::hiatus:
          chapter_status_for_ts = "hiatus";
          label = "latest hiatus";
          break;
        default: // active: no chapter-status restriction
          break;
      }
    }

    const auto series_list = series_repo.listWithStats(session.rtx(), series_filter, chapter_status_for_ts);

    std::vector<std::string> lines;
    lines.reserve(series_list.size());
    for(const auto &s : series_list) {
      const std::string date = s.latest_chapter_at ? *s.latest_chapter_at : "—";
      lines.push_back(
          "- **" + s.name + "** (" + seriesStatusToString(s.status) + ") — " +
          std::to_string(s.chapter_count) + " ch | " + label + ": " + date);
    }

    const std::string title = series_filter
                                  ? "**Series List** (" + seriesStatusToString(*series_filter) + ")"
                                  : "**Series List**";
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
          if(cb.is_error()) {
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
    std::cerr << "listSeries failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("Failed to list series. Contact the administrator to resolve this issue."));
  }
}
