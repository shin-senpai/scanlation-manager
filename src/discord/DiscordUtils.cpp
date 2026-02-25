// Associated Header Include
#include "discord/DiscordUtils.h"

// User Defined Includes
#include "ConfigManager.h"

// Standard Includes
#include <cstdint>
#include <string>
#include <vector>

// Third Party Includes
#include <dpp/appcommand.h>
#include <dpp/cluster.h>
#include <dpp/dispatcher.h>
#include <dpp/dpp.h>
#include <dpp/snowflake.h>

Bot::Bot(const std::string &DISCORD_BOT_TOKEN, ConfigManager &cfg)
    : core(DISCORD_BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content),
      config(cfg) {

  core.on_log(dpp::utility::cout_logger());

  //TODO 
  //Switch to a more robust way of loading then using a catch-all block
  try {
      work_progress_channel = static_cast<dpp::snowflake>(
          config.get<uint64_t>("work_progress_channel"));
  } catch (...) {
      work_progress_channel = 0; 
  }

  fillCommandMap();
  fillTriggerList();

  core.on_ready([this](const dpp::ready_t &event) {
    if (dpp::run_once<struct register_bot_commands>()) {
      std::vector<dpp::slashcommand> slash_cmds;

      // Automatically adds all commands defined in the commands map to the
      // slash_cmds vector to be registered in bulk
      for (const auto &[name, cmd_info] : commands) {
        dpp::slashcommand cmd(name, cmd_info.description, core.me.id);
        for (const auto &opt : cmd_info.options) {
          cmd.add_option(opt);
        }
        slash_cmds.push_back(cmd);
      }

      //TODO
      // CHANGE .guild_ to .global_
      core.guild_bulk_command_create(slash_cmds, GUILD_ID);
    }
  });

  // The following is to clean up command registration from guild and global
  /*
  core.on_ready([this](const dpp::ready_t& event) {
  if (dpp::run_once<struct cleanup_commands>()) {
      // 1. Clear ALL Global Commands (These take up to 1 hour to vanish)
      core.global_bulk_command_delete();

      // 2. Clear ALL Guild Commands for your specific server (Instant)
      core.guild_bulk_command_delete(GUILD_ID);

      // Optional: Log to console so you know it triggered
      core.log(dpp::ll_info, "Cleaned up all existing commands.");
  }
  */

  // Keep your registration logic BELOW the cleanup or comment it out
  // for one run to ensure a totally fresh start.
  //});

  core.on_slashcommand([this](const dpp::slashcommand_t &event) {
    auto it = this->commands.find(event.command.get_command_name());
    if (it != this->commands.end()) {
      it->second.handler(event);
    }
  });

  core.on_message_create([this](const dpp::message_create_t &event) {
    if (event.msg.author.is_bot()) return;
    
    for(const auto& trigger : triggers){
      if(trigger.should_trigger(event)){
        trigger.handler(event);
      }
    }

  });
}

void Bot::start() { core.start(dpp::st_wait); }

void Bot::fillCommandMap() {
  commands["ping"] = {
      "Ping Pong",
      [this](const dpp::slashcommand_t &e) { commandPing(e); }};

  commands["set-progress-channel"] = {
      "Sets the channel where progress should be posted",
      [this](const dpp::slashcommand_t &e) { commandSetProgressChannel(e); },
      {dpp::command_option(dpp::co_channel, "channel", "Work Progress Channel",true).add_channel_type(dpp::CHANNEL_TEXT)}};
}

void Bot::fillTriggerList() {
  triggers.emplace_back(
    "Work Progress",
    [this](const dpp::message_create_t &e){return e.msg.channel_id == work_progress_channel;},
    [this](const dpp::message_create_t &e){triggerWorkUpdate(e);}
);
}