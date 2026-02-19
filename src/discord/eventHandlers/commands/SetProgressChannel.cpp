// Associated Header Include

// User Defined Includes
#include "ConfigManager.h"
#include "discord/DiscordUtils.h"

// Standard Includes

// Third Party Includes

void Bot::commandSetProgressChannel(const dpp::slashcommand_t &event) {
  dpp::snowflake channel_id = std::get<dpp::snowflake>(event.get_parameter("channel"));

  config.set("work_progress_channel", static_cast<uint64_t>(channel_id));
  work_progress_channel = channel_id;
  event.reply("Work progress channel updated!");
}