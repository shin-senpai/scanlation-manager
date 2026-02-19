// Associated Header Include

// User Defined Includes
#include "discord/DiscordUtils.h"

// Standard Includes

// Third Party Includes

void Bot::commandPing(const dpp::slashcommand_t &event) {
  event.reply("Pong! 🏓");
}