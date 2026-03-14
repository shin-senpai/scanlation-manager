// Associated Header Include
#include "bot/eventHandlers/commands/WorkProgress.hpp"

// User Defined Includes
#include "bot/Bot.hpp"
#include "models/ModelWorkProgress.hpp"

// Standard Includes
#include <dpp/snowflake.h>
#include <string>

// Third Party Includes
#include <dpp/appcommand.h>
#include <dpp/cache.h>
#include <dpp/discordclient.h>
#include <dpp/dispatcher.h>

void Commands::workProgress(const dpp::slashcommand_t &event) {
  WorkProgress update;
  dpp::user user;

  update.staff_name = event.command.usr.username;
}

void Commands::workProgressAutocomplete(Bot &bot, const std::string &option_name, const std::string &input, const dpp::autocomplete_t &event) {
  dpp::interaction_response r(dpp::ir_autocomplete_reply);
  std::vector<std::string> series = {"One", "Two"};
  std::vector<std::string> chapters = {"1", "2"};
  std::vector<std::string> tasks = {"TL", "CLRD"};

  if(option_name == "series") {
    for(const auto &s : series) {
      if(s.starts_with(input)) {
        r.add_autocomplete_choice(dpp::command_option_choice(s, s));
      }
    }
  }

  if(option_name == "chapter") {
    for(const auto &c : chapters) {
      if(c.starts_with(input)) {
        r.add_autocomplete_choice(dpp::command_option_choice(c, c));
      }
    }
  }

  if(option_name == "task") {
    for(const auto &t : tasks) {
      if(t.starts_with(input)) {
        r.add_autocomplete_choice(dpp::command_option_choice(t, t));
      }
    }
  }

  bot.getCore().interaction_response_create(
      event.command.id,
      event.command.token,
      r);
}