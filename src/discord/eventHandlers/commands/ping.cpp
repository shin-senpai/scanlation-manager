// Associated Header Include

// User Defined Includes
#include "discord/Bot.hpp"

// Standard Includes

// Third Party Includes

void Bot::commandPing(const dpp::slashcommand_t &event) {
  event.reply("Pong! 🏓");
}