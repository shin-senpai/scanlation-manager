// Associated Header Include
#include "bot/eventHandlers/commands/SetProgressChannel.hpp"

// User Defined Includes
#include "bot/Bot.hpp"

// Standard Includes

// Third Party Includes
#include <dpp/dispatcher.h>

void Commands::setProgressChannel(Bot &bot, const dpp::slashcommand_t &event) {
  dpp::snowflake channel_id = std::get<dpp::snowflake>(event.get_parameter("channel"));
  bot.setWorkProgressChannel(channel_id);
  event.reply("Work progress channel updated!");
}