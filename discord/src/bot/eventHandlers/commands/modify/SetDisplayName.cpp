// Associated Header Include
#include "bot/eventHandlers/commands/modify/SetDisplayName.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "bot/utils/SheetSync.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/SeriesAssignments.hpp"
#include "db/repositories/User.hpp"
#include "types/Permission.hpp"

// Standard Includes
#include <iostream>
#include <string>

// Third Party Includes
#include <dpp/dispatcher.h>
#include <dpp/snowflake.h>

namespace {

// Keeps only printable non-space ASCII (codepoints 33–126). Strips spaces,
// control characters, DEL, and anything outside ASCII.
std::string sanitizeDisplayName(const std::string &input) {
  std::string out;
  out.reserve(input.size());
  for(unsigned char c : input) {
    if(c >= 33 && c <= 126) {
      out += static_cast<char>(c);
    }
  }
  return out;
}

} // namespace

void Commands::setDisplayName(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  const int64_t discord_id = static_cast<int64_t>(event.command.usr.id);

  try {
    DbSession session(bot.getPool());
    DiscordIdentityRepository identity_repo;
    UserRepository user_repo;
    SeriesAssignmentsRepository series_assignments_repo;

    const auto maybe_caller_id = identity_repo.findUserIdByDiscordId(session.wtx(), discord_id);
    if(!maybe_caller_id) {
      event.edit_original_response(dpp::message("You are not registered. Please run /register first."));
      return;
    }

    // Resolve target user — optional user parameter for managers.
    int target_user_id = *maybe_caller_id;
    const auto &target_param = event.get_parameter("user");
    dpp::snowflake target_snowflake{};
    if(const auto *p = std::get_if<dpp::snowflake>(&target_param)) {
      target_snowflake = *p;
    }
    if(!target_snowflake.empty()) {
      if(user_repo.getPermissionLevel(session.wtx(), *maybe_caller_id) < Permission::manager) {
        event.edit_original_response(dpp::message("You lack the permission to change another user's display name."));
        return;
      }
      const auto maybe_target = identity_repo.findUserIdByDiscordId(session.wtx(), static_cast<int64_t>(target_snowflake));
      if(!maybe_target) {
        event.edit_original_response(dpp::message("That user is not registered."));
        return;
      }
      target_user_id = *maybe_target;
    }

    const auto affected_series = series_assignments_repo.listSeriesNamesByUser(session.wtx(), target_user_id);

    const std::string raw_name = std::get<std::string>(event.get_parameter("name"));
    const std::string new_name = sanitizeDisplayName(raw_name);
    if(new_name.empty()) {
      event.edit_original_response(dpp::message(
          "Display name cannot be empty after removing spaces and non-ASCII characters."));
      return;
    }

    user_repo.updateDisplayName(session.wtx(), target_user_id, new_name);
    session.commit();

    for(const auto &series_name : affected_series) {
      SheetSync::syncSeries(bot, series_name);
    }
    SheetSync::syncTodo(bot);

    const bool is_self = target_snowflake.empty();
    if(is_self) {
      event.edit_original_response(dpp::message("Your display name has been updated to **" + new_name + "**."));
    } else {
      event.edit_original_response(dpp::message(
          "Display name for <@" + std::to_string(target_snowflake) + "> updated to **" + new_name + "**."));
    }
  } catch(const std::exception &e) {
    std::cerr << "set-display-name failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("An error occurred. Contact the administrator to resolve this issue."));
  }
}
