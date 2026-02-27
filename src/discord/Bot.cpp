// Associated Header Include
#include "discord/Bot.hpp"

// User Defined Includes
#include "utils/ConfigManager.hpp"

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

void Bot::fillCommandMap() {
  m_commands["ping"] = {
      "Ping Pong",
      [this](const dpp::slashcommand_t &e) { commandPing(e); }
    };

  m_commands["set-progress-channel"] = {
      "Sets the channel where progress should be posted",
      [this](const dpp::slashcommand_t &e) { commandSetProgressChannel(e); },
      {dpp::command_option(dpp::co_channel, "channel", "Work Progress Channel",true).add_channel_type(dpp::CHANNEL_TEXT)}
    };
}

void Bot::fillTriggerList() {
  m_triggers.emplace_back(
    "Work Progress",
    [this](const dpp::message_create_t &e){return e.msg.channel_id == m_work_progress_channel;},
    [this](const dpp::message_create_t &e){triggerWorkUpdate(e);}
  );
}

dpp::snowflake Bot::fetchGuildId(ConfigManager &cfg) {
    try{
      return static_cast<dpp::snowflake>(cfg.get<uint64_t>("guild_id"));
    } catch(...) {
      throw std::runtime_error("guild_id is missing");
    }
  }

void Bot::start() { m_core.start(dpp::st_wait); }

Bot::Bot(ConfigManager &cfg)
    : m_core(cfg.get<std::string>("discord_bot_token"), dpp::i_default_intents | dpp::i_message_content),
      m_config(cfg),
      m_guild_id(fetchGuildId(cfg)) {

  m_core.on_log(dpp::utility::cout_logger());

  //TODO 
  //Switch to a more robust way of loading
  try{
      m_work_progress_channel = static_cast<dpp::snowflake>(
          m_config.get<uint64_t>("work_progress_channel"));
  } catch(...) {
      m_work_progress_channel = 0; 
  }

  fillCommandMap();
  fillTriggerList();

  m_core.on_ready([this](const dpp::ready_t &event) {
    if(dpp::run_once<struct register_bot_commands>()) {
      std::vector<dpp::slashcommand> slash_cmds;

      // Automatically adds all commands defined in the commands map to the
      // slash_cmds vector to be registered in bulk
      for(const auto &[name, cmd_info] : m_commands) {
        dpp::slashcommand cmd(name, cmd_info.description, m_core.me.id);
        for (const auto &opt : cmd_info.options) {
          cmd.add_option(opt);
        }
        slash_cmds.push_back(cmd);
      }

      //TODO
      // CHANGE .guild_ to .global_
      m_core.guild_bulk_command_create(slash_cmds, m_guild_id);
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

  m_core.on_slashcommand([this](const dpp::slashcommand_t &event) {
    auto it = this->m_commands.find(event.command.get_command_name());
    if(it != this->m_commands.end()) {
      it->second.handler(event);
    }
  });

  m_core.on_message_create([this](const dpp::message_create_t &event) {
    if(event.msg.author.is_bot()) return;
    
    for(const auto& trigger : m_triggers){
      if(trigger.should_trigger(event)){
        trigger.handler(event);
      }
    }

  });
}