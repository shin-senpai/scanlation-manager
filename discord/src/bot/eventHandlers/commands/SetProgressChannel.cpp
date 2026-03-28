// Associated Header Include
#include "bot/eventHandlers/commands/SetProgressChannel.hpp"

// User Defined Includes
#include "bot/Bot.hpp"

// Standard Includes

// Third Party Includes
#include <dpp/dispatcher.h>

void Commands::setProgressChannel(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  dpp::snowflake channel_id = std::get<dpp::snowflake>(event.get_parameter("channel"));
  bot.setWorkProgressChannel(channel_id);
  event.edit_original_response(dpp::message("Work progress channel updated!"));
}