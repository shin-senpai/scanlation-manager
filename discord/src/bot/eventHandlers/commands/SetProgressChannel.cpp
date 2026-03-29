// Associated Header Include
#include "bot/eventHandlers/commands/SetProgressChannel.hpp"

// User Defined Includes
#include "bot/Bot.hpp"

// Standard Includes
#include <exception>

// Third Party Includes
#include <dpp/dispatcher.h>


void Commands::setProgressChannel(Bot &bot, const dpp::slashcommand_t &event) {
  event.thinking(true);
  try {
    dpp::snowflake channel_id = std::get<dpp::snowflake>(event.get_parameter("channel"));
    bot.setWorkProgressChannel(channel_id);
    event.edit_original_response(dpp::message("Work progress channel updated!"));
  } catch(std::exception &e) {
    event.edit_original_response(dpp::message("Failed to set channel for Work-Progess. Contact the administrator to resolve this issue"));
    std::cerr << "work progress channel was failed to be set due to exception: " << e.what() << std::endl;
  }
}