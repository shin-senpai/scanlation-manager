#pragma once

// Standard Includes
#include <functional>
#include <string>
#include <unordered_map>

// Third Party Includes
#include <dpp/cluster.h>
#include <dpp/dispatcher.h>
#include <dpp/snowflake.h>

class ConfigManager;
class Bot {
private:
  dpp::cluster m_core;
  ConfigManager &m_config;
  const dpp::snowflake m_guild_id;
  dpp::snowflake m_work_progress_channel;

  static dpp::snowflake fetchGuildId(ConfigManager &cfg);

  struct CommandInfo {
    std::string description;
    std::function<void(const dpp::slashcommand_t &)> handler;
    std::vector<dpp::command_option> options = {};
  };

  struct TriggerInfo {
    std::string description;
    std::function<bool(const dpp::message_create_t &)> should_trigger;
    std::function<void(const dpp::message_create_t &)> handler;
  };

  std::unordered_map<std::string, CommandInfo> m_commands;
  std::vector<TriggerInfo> m_triggers;

  void fillCommandMap();
  void fillTriggerList();

  // Slash Commands
  void commandPing(const dpp::slashcommand_t &event);
  void commandSetProgressChannel(const dpp::slashcommand_t &event);

  // Triggers
  void triggerWorkUpdate(const dpp::message_create_t &event);

public:
  Bot(ConfigManager &cfg);

  void start();
};