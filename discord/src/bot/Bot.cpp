// Associated Header Include
#include "bot/Bot.hpp"

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

// Triggers
#include "bot/eventHandlers/triggers/WorkProgress.hpp"

// Commands
#include "bot/eventHandlers/commands/Ping.hpp"
#include "bot/eventHandlers/commands/RegisterUser.hpp"
#include "bot/eventHandlers/commands/SetProgressChannel.hpp"
#include "bot/eventHandlers/commands/WorkProgress.hpp"

void Bot::fillCommandMap() {
  m_commands["ping"] = {
      "Ping Pong",
      [](const dpp::slashcommand_t &e) { Commands::ping(e); }};

  m_commands["set-progress-channel"] = {
      "Sets the channel where progress should be posted",
      [this](const dpp::slashcommand_t &e) { Commands::setProgressChannel(*this, e); },
      {dpp::command_option(dpp::co_channel, "channel", "Work Progress Channel", true).add_channel_type(dpp::CHANNEL_TEXT)}};

  m_commands["work-update"] = {
      "Update your progress on a Chapter",
      [this](const dpp::slashcommand_t &e) { Commands::workProgress(e); },
      {
          dpp::command_option(dpp::co_string, "series", "Choose Series", true).set_auto_complete(true),
          dpp::command_option(dpp::co_string, "chapter", "Choose Chapter", true).set_auto_complete(true),
          dpp::command_option(dpp::co_string, "task", "Choose Task", true).set_auto_complete(true),
      },
      [this](const std::string &option_name, const std::string &input, const dpp::autocomplete_t &e) { Commands::workProgressAutocomplete(*this, option_name, input, e); }};

  m_commands["register"] = {
      "Register yourself as a scanlation team member",
      [this](const dpp::slashcommand_t &e) { Commands::registerUser(*this, e); }};
}

void Bot::fillTriggerList() {
  m_triggers.emplace_back(
      "Work Progress",
      [this](const dpp::message_create_t &e) { return e.msg.channel_id == m_work_progress_channel; },
      [this](const dpp::message_create_t &e) { Triggers::workProgress(e); });
}

void Bot::setWorkProgressChannel(dpp::snowflake channel_id) {
  m_config.set("work_progress_channel", static_cast<uint64_t>(channel_id));
  m_work_progress_channel = channel_id;
}

dpp::cluster &Bot::getCore() {
  return m_core;
}

const dpp::cluster &Bot::getCore() const {
  return m_core;
}

Db &Bot::getDb() {
  return m_db;
}

Bot::Bot(ConfigManager &cfg, Db &db)
    : m_core(cfg.getRequired<std::string>("discord_bot_token"), dpp::i_default_intents | dpp::i_message_content),
      m_work_progress_channel(static_cast<dpp::snowflake>(cfg.getOptional<uint64_t>("work_progress_channel"))),
      m_guild_id(static_cast<dpp::snowflake>(cfg.getRequired<uint64_t>("guild_id"))),
      m_config(cfg),
      m_db(db) {

  m_core.on_log(dpp::utility::cout_logger());

  fillCommandMap();
  fillTriggerList();

  m_core.on_ready([this](const dpp::ready_t &event) {
    if(dpp::run_once<struct register_bot_commands>()) {
      std::vector<dpp::slashcommand> slash_cmds;

      // Automatically adds all commands defined in the commands map to the
      // slash_cmds vector to be registered in bulk
      for(const auto &[name, cmd_info] : m_commands) {
        dpp::slashcommand cmd(name, cmd_info.description, m_core.me.id);
        for(const auto &opt : cmd_info.options) {
          cmd.add_option(opt);
        }
        slash_cmds.push_back(cmd);
      }

      m_core.guild_bulk_command_create(slash_cmds, m_guild_id);
    }
  });

  m_core.on_slashcommand([this](const dpp::slashcommand_t &event) {
    auto it = this->m_commands.find(event.command.get_command_name());
    if(it != this->m_commands.end()) {
      it->second.handler(event);
    }
  });

  m_core.on_autocomplete([this](const dpp::autocomplete_t &event) {
    auto it = this->m_commands.find(event.name);
    if(it == m_commands.end() || !it->second.autocomplete_handler) {
      return;
    }

    for(const auto &option : event.options) {
      if(option.focused) {
        std::string input = std::get<std::string>(option.value);
        it->second.autocomplete_handler(option.name, input, event);
        return;
      }
    }
  });

  m_core.on_message_create([this](const dpp::message_create_t &event) {
    if(event.msg.author.is_bot())
      return;

    for(const auto &trigger : m_triggers) {
      if(trigger.should_trigger(event)) {
        trigger.handler(event);
      }
    }
  });
}

void Bot::start() { m_core.start(dpp::st_wait); }