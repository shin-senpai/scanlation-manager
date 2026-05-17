// Associated Header Include
#include "bot/eventHandlers/commands/manage/Gsheet.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "db/DbSession.hpp"
#include "db/repositories/BotSettings.hpp"
#include "db/repositories/DiscordIdentities.hpp"
#include "db/repositories/User.hpp"
#include "types/Permission.hpp"
#include "utils/HttpUtils.hpp"

// Standard Includes
#include <iostream>
#include <string>

// Third Party Includes
#include <dpp/appcommand.h>
#include <dpp/dispatcher.h>
#include <pqxx/pqxx>

namespace {

void doEnable(Bot &bot, const dpp::slashcommand_t &event, DbSession &session) {
  if(bot.getBackendUrl().empty() || bot.getApiToken().empty()) {
    event.edit_original_response(dpp::message(
        "backend_url and api_token must be set in config.json before enabling Google Sheets."));
    return;
  }

  const std::string auth_header = "Authorization: Bearer " + bot.getApiToken();
  std::string response_body;

  // Step 1: generic backend connectivity + auth check
  const int backend_status = httpGet(bot.getBackendUrl() + "/health", {auth_header}, response_body);
  if(backend_status != 200) {
    event.edit_original_response(dpp::message(
        "Backend health check failed (HTTP " + std::to_string(backend_status) + "). "
        "Ensure the backend is running and the API token is correct."));
    return;
  }

  // Step 2: Google Sheets-specific check
  const int sheets_status = httpGet(bot.getBackendUrl() + "/sheets/health", {auth_header}, response_body);
  if(sheets_status != 200) {
    event.edit_original_response(dpp::message(
        "Backend is reachable but Google Sheets is not configured (HTTP " + std::to_string(sheets_status) + "). "
        "Ensure gsheet_spreadsheet_id and gdrive_credentials_file are set in the backend config."));
    return;
  }

  BotSettingsRepository settings_repo;
  settings_repo.set(session.wtx(), "gsheet_enabled", "1");
  session.commit();

  event.edit_original_response(dpp::message("Google Sheets integration enabled."));
}

void doDisable(const dpp::slashcommand_t &event, DbSession &session) {
  BotSettingsRepository settings_repo;

  if(!settings_repo.get(session.wtx(), "gsheet_enabled")) {
    event.edit_original_response(dpp::message("Google Sheets integration is not currently enabled."));
    return;
  }

  settings_repo.remove(session.wtx(), "gsheet_enabled");
  session.commit();

  event.edit_original_response(dpp::message("Google Sheets integration disabled."));
}

} // namespace

void Commands::gsheet(Bot &bot, const dpp::slashcommand_t &event) {
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

    if(user_repo.getPermissionLevel(session.wtx(), *maybe_user_id) < Permission::supermanager) {
      event.edit_original_response(dpp::message("You lack the permission to perform this action."));
      return;
    }

    if(sub == "enable") {
      doEnable(bot, event, session);
    } else if(sub == "disable") {
      doDisable(event, session);
    }

  } catch(const std::exception &e) {
    std::cerr << "gsheet/" << sub << " failed for user (" << discord_id << "): " << e.what() << std::endl;
    event.edit_original_response(dpp::message("An error occurred. Contact the administrator to resolve this issue."));
  }
}
