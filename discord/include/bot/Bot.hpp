#pragma once

// User Defined Includes
#include "db/ConnectionPool.hpp"

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
  dpp::snowflake m_work_progress_channel;
  dpp::snowflake m_staff_role_id;
  const dpp::snowflake m_guild_id;
  ConfigManager &m_config;
  ConnectionPool m_pool;

  struct CommandInfo {
    std::string description;
    std::function<void(const dpp::slashcommand_t &)> handler;
    std::vector<dpp::command_option> options = {};
    std::function<void(const std::string &option_name, const std::string &, const dpp::autocomplete_t &)> autocomplete_handler = {};
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

public:
  Bot(ConfigManager &cfg);

  dpp::cluster &getCore();
  const dpp::cluster &getCore() const;
  ConnectionPool &getPool();

  void setWorkProgressChannel(dpp::snowflake channel_id);
  void setStaffRole(dpp::snowflake role_id);

  void start();
};