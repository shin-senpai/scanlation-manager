// Associated Header Include
#include "bot/Bot.hpp"

// User Defined Includes
#include "db/ConnectionPool.hpp"
#include "utils/ConfigManager.hpp"

// Standard Includes
#include <cstdint>
#include <exception>
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
#include "bot/eventHandlers/commands/AddRole.hpp"
#include "bot/eventHandlers/commands/AddSeries.hpp"
#include "bot/eventHandlers/commands/AddTask.hpp"
#include "bot/eventHandlers/commands/AssignRole.hpp"
#include "bot/eventHandlers/commands/DeleteRole.hpp"
#include "bot/eventHandlers/commands/DeleteTask.hpp"
#include "bot/eventHandlers/commands/ListRoles.hpp"
#include "bot/eventHandlers/commands/ListTasks.hpp"
#include "bot/eventHandlers/commands/MapRoleTask.hpp"
#include "bot/eventHandlers/commands/Ping.hpp"
#include "bot/eventHandlers/commands/RemoveRole.hpp"
#include "bot/eventHandlers/commands/RemoveRoleTask.hpp"
#include "bot/eventHandlers/commands/RetireTask.hpp"
#include "bot/eventHandlers/commands/UnretireTask.hpp"
#include "bot/eventHandlers/commands/RegisterUser.hpp"
#include "bot/eventHandlers/commands/SetAlias.hpp"
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

  m_commands["set-alias"] = {
      "Set the alias that you want to use for credit",
      [this](const dpp::slashcommand_t &e) { Commands::setAlias(*this, e); },
      {dpp::command_option(dpp::co_string, "alias", "The Credit Name you want to use", true)}};

  m_commands["add-role"] = {
      "Create a new scanlation role",
      [this](const dpp::slashcommand_t &e) { Commands::addRole(*this, e); },
      {dpp::command_option(dpp::co_string, "name", "Name of the role", true)}};

  m_commands["add-task"] = {
      "Create a new task type",
      [this](const dpp::slashcommand_t &e) { Commands::addTask(*this, e); },
      {dpp::command_option(dpp::co_string, "name", "Name of the task", true)}};

  m_commands["add-series"] = {
      "Add a new series",
      [this](const dpp::slashcommand_t &e) { Commands::addSeries(*this, e); },
      {dpp::command_option(dpp::co_string, "name", "Name of the series", true)}};

  m_commands["assign-role"] = {
      "Assign a role to a user",
      [this](const dpp::slashcommand_t &e) { Commands::assignRole(*this, e); },
      {
          dpp::command_option(dpp::co_user, "user", "The user to assign the role to", true),
          dpp::command_option(dpp::co_string, "role", "Name of the role", true),
      }};

  m_commands["remove-role"] = {
      "Remove a role from a user",
      [this](const dpp::slashcommand_t &e) { Commands::removeRole(*this, e); },
      {
          dpp::command_option(dpp::co_user, "user", "The user to remove the role from", true),
          dpp::command_option(dpp::co_string, "role", "Name of the role", true),
      }};

  m_commands["delete-role"] = {
      "Delete a role and all its mappings",
      [this](const dpp::slashcommand_t &e) { Commands::deleteRole(*this, e); },
      {dpp::command_option(dpp::co_string, "name", "Name of the role to delete", true)}};

  m_commands["delete-task"] = {
      "Delete a task and all its mappings",
      [this](const dpp::slashcommand_t &e) { Commands::deleteTask(*this, e); },
      {dpp::command_option(dpp::co_string, "name", "Name of the task to delete", true)}};

  m_commands["retire-task"] = {
      "Retire a task, preserving its completion history",
      [this](const dpp::slashcommand_t &e) { Commands::retireTask(*this, e); },
      {dpp::command_option(dpp::co_string, "name", "Name of the task to retire", true)}};

  m_commands["unretire-task"] = {
      "Restore a retired task to active status",
      [this](const dpp::slashcommand_t &e) { Commands::unretireTask(*this, e); },
      {dpp::command_option(dpp::co_string, "name", "Name of the task to unretire", true)}};

  m_commands["list-roles"] = {
      "List all roles",
      [this](const dpp::slashcommand_t &e) { Commands::listRoles(*this, e); }};

  m_commands["list-tasks"] = {
      "List all tasks",
      [this](const dpp::slashcommand_t &e) { Commands::listTasks(*this, e); }};

  m_commands["remove-role-task"] = {
      "Remove a role-task mapping",
      [this](const dpp::slashcommand_t &e) { Commands::removeRoleTask(*this, e); },
      {
          dpp::command_option(dpp::co_string, "role", "Name of the role", true),
          dpp::command_option(dpp::co_string, "task", "Name of the task", true),
      }};

  m_commands["map-role-task"] = {
      "Map a role to a task it is responsible for",
      [this](const dpp::slashcommand_t &e) { Commands::mapRoleTask(*this, e); },
      {
          dpp::command_option(dpp::co_string, "role", "Name of the role", true),
          dpp::command_option(dpp::co_string, "task", "Name of the task", true),
      }};
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

ConnectionPool &Bot::getPool() {
  return m_pool;
}

Bot::Bot(ConfigManager &cfg)
    : m_core(cfg.getRequired<std::string>("discord_bot_token"), dpp::i_default_intents | dpp::i_message_content),
      m_work_progress_channel(static_cast<dpp::snowflake>(cfg.getOptional<uint64_t>("work_progress_channel"))),
      m_guild_id(static_cast<dpp::snowflake>(cfg.getRequired<uint64_t>("guild_id"))),
      m_config(cfg),
      m_pool(cfg.getRequired<std::string>("db_connection_string"), cfg.getRequired<size_t>("db_pool_size")) {

  m_core.on_log(dpp::utility::cout_logger());

  fillCommandMap();
  fillTriggerList();

  m_core.on_ready([this](const dpp::ready_t &event) {
    if(dpp::run_once<struct register_bot_commands>()) {
      std::vector<dpp::slashcommand> slash_cmds;

      // Automatically adds all commands defined in the commands map to the
      // slash_cmds vector to be registered in bulk
      for(const auto &[cmd_name, cmd_info] : m_commands) {
        dpp::slashcommand cmd(cmd_name, cmd_info.description, m_core.me.id);
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
        std::string input{};
        try {
          input = std::get<std::string>(option.value);
        } catch(std::exception &e) {
          std::cerr << "Failed to get option's value for autocomplete event: " << event.name << " due to exception: " << e.what() << std::endl;
        }
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