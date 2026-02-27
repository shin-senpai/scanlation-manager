// Associated Header Include
#include "discord/eventHandlers/commands/Ping.hpp"

// User Defined Includes

// Standard Includes

// Third Party Includes
#include <dpp/dispatcher.h>

void Commands::ping(const dpp::slashcommand_t &event) {
  event.reply("Pong! 🏓");
}