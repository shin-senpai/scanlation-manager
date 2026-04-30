// Associated Header Include
#include "bot/Bot.hpp"

// User Defined Includes
#include "db/ConnectionPool.hpp"
#include "utils/ConfigManager.hpp"

// Standard Includes
#include <algorithm>
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
#include "bot/eventHandlers/commands/add/AddRole.hpp"
#include "bot/eventHandlers/commands/add/AddTask.hpp"
#include "bot/eventHandlers/commands/add/RegisterUser.hpp"
#include "bot/eventHandlers/commands/list/ListRoleTasks.hpp"
#include "bot/eventHandlers/commands/list/ListRoles.hpp"
#include "bot/eventHandlers/commands/list/ListTasks.hpp"
#include "bot/eventHandlers/commands/list/Ping.hpp"
#include "bot/eventHandlers/commands/manage/Chapter.hpp"
#include "bot/eventHandlers/commands/manage/Series.hpp"
#include "bot/eventHandlers/commands/modify/AssignRole.hpp"
#include "bot/eventHandlers/commands/modify/MapRoleTask.hpp"
#include "bot/eventHandlers/commands/modify/RetireTask.hpp"
#include "bot/eventHandlers/commands/modify/SetAlias.hpp"
#include "bot/eventHandlers/commands/modify/SetProgressChannel.hpp"
#include "bot/eventHandlers/commands/modify/SetStaffRole.hpp"
#include "bot/eventHandlers/commands/modify/UnretireTask.hpp"
#include "bot/eventHandlers/commands/modify/WorkProgress.hpp"
#include "bot/eventHandlers/commands/remove/DeleteRole.hpp"
#include "bot/eventHandlers/commands/remove/DeleteTask.hpp"
#include "bot/eventHandlers/commands/remove/RemoveRole.hpp"
#include "bot/eventHandlers/commands/remove/RemoveRoleTask.hpp"

void Bot::fillCommandMap() {
  m_commands["ping"] = {
      "Ping Pong",
      [](const dpp::slashcommand_t &e) { Commands::ping(e); }};

  m_commands["set-progress-channel"] = {
      "Sets the channel where progress should be posted",
      [this](const dpp::slashcommand_t &e) { Commands::setProgressChannel(*this, e); },
      {dpp::command_option(dpp::co_channel, "channel", "Work Progress Channel", true).add_channel_type(dpp::CHANNEL_TEXT)}};

  m_commands["set-staff-role"] = {
      "Sets the staff role",
      [this](const dpp::slashcommand_t &e) { Commands::setStaffRole(*this, e); },
      {dpp::command_option(dpp::co_role, "role", "Staff Roles", true)}};

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

  m_commands["series"] = {
      "Manage a series",
      [this](const dpp::slashcommand_t &e) { Commands::series(*this, e); },
      {
          dpp::command_option(dpp::co_sub_command, "add", "Add a new series")
              .add_option(dpp::command_option(dpp::co_string, "name", "Name of the series", true)),
          dpp::command_option(dpp::co_sub_command, "set-status", "Update series status")
              .add_option(dpp::command_option(dpp::co_string, "name", "Series name", true).set_auto_complete(true))
              .add_option(dpp::command_option(dpp::co_string, "status", "New status", true)
                              .add_choice(dpp::command_option_choice("Active", std::string("active")))
                              .add_choice(dpp::command_option_choice("Hiatus", std::string("hiatus")))
                              .add_choice(dpp::command_option_choice("Completed", std::string("completed")))
                              .add_choice(dpp::command_option_choice("Dropped", std::string("dropped")))),
          dpp::command_option(dpp::co_sub_command, "assign", "Add a user to this series' default crew")
              .add_option(dpp::command_option(dpp::co_string, "name", "Series name", true).set_auto_complete(true))
              .add_option(dpp::command_option(dpp::co_user, "user", "User to assign", true))
              .add_option(dpp::command_option(dpp::co_string, "task", "Task name", true).set_auto_complete(true)),
          dpp::command_option(dpp::co_sub_command, "unassign", "Remove a user from this series' default crew")
              .add_option(dpp::command_option(dpp::co_string, "name", "Series name", true).set_auto_complete(true))
              .add_option(dpp::command_option(dpp::co_user, "user", "User to unassign", true))
              .add_option(dpp::command_option(dpp::co_string, "task", "Task name", true).set_auto_complete(true)),
      },
      [this](const std::string &key, const std::string &input, const dpp::autocomplete_t &e) {
        Commands::seriesAutocomplete(*this, key, input, e);
      }};

  m_commands["chapter"] = {
      "Manage a chapter",
      [this](const dpp::slashcommand_t &e) { Commands::chapter(*this, e); },
      {
          dpp::command_option(dpp::co_sub_command, "add", "Add a chapter to a series")
              .add_option(dpp::command_option(dpp::co_string, "series", "Series name", true).set_auto_complete(true))
              .add_option(dpp::command_option(dpp::co_number, "number", "Chapter number (e.g. 51 or 51.1)", true))
              .add_option(dpp::command_option(dpp::co_string, "name", "Display name (e.g. Ch 51)", true))
              .add_option(dpp::command_option(dpp::co_integer, "volume", "Volume number", false)),
          dpp::command_option(dpp::co_sub_command, "set-status", "Update chapter status")
              .add_option(dpp::command_option(dpp::co_string, "series", "Series name", true).set_auto_complete(true))
              .add_option(dpp::command_option(dpp::co_string, "chapter", "Chapter name", true).set_auto_complete(true))
              .add_option(dpp::command_option(dpp::co_string, "status", "New status", true)
                              .add_choice(dpp::command_option_choice("In Progress", std::string("in_progress")))
                              .add_choice(dpp::command_option_choice("Released", std::string("released")))
                              .add_choice(dpp::command_option_choice("Hiatus", std::string("hiatus")))
                              .add_choice(dpp::command_option_choice("Dropped", std::string("dropped")))),
          dpp::command_option(dpp::co_sub_command, "assign", "Assign a user to this chapter for a task")
              .add_option(dpp::command_option(dpp::co_string, "series", "Series name", true).set_auto_complete(true))
              .add_option(dpp::command_option(dpp::co_string, "chapter", "Chapter name", true).set_auto_complete(true))
              .add_option(dpp::command_option(dpp::co_user, "user", "User to assign", true))
              .add_option(dpp::command_option(dpp::co_string, "task", "Task name", true).set_auto_complete(true)),
          dpp::command_option(dpp::co_sub_command, "unassign", "Remove a user from this chapter for a task")
              .add_option(dpp::command_option(dpp::co_string, "series", "Series name", true).set_auto_complete(true))
              .add_option(dpp::command_option(dpp::co_string, "chapter", "Chapter name", true).set_auto_complete(true))
              .add_option(dpp::command_option(dpp::co_user, "user", "User to unassign", true))
              .add_option(dpp::command_option(dpp::co_string, "task", "Task name", true).set_auto_complete(true)),
          dpp::command_option(dpp::co_sub_command, "uncomplete", "Mark a completed task assignment as outstanding again")
              .add_option(dpp::command_option(dpp::co_string, "series", "Series name", true).set_auto_complete(true))
              .add_option(dpp::command_option(dpp::co_string, "chapter", "Chapter name", true).set_auto_complete(true))
              .add_option(dpp::command_option(dpp::co_user, "user", "User whose assignment to reset", true))
              .add_option(dpp::command_option(dpp::co_string, "task", "Task name", true).set_auto_complete(true)),
      },
      [this](const std::string &key, const std::string &input, const dpp::autocomplete_t &e) {
        Commands::chapterAutocomplete(*this, key, input, e);
      }};

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

  m_commands["list-role-tasks"] = {
      "List all Role to Task mappings",
      [this](const dpp::slashcommand_t &e) { Commands::listRoleTasks(*this, e); }};
}

void Bot::fillTriggerList() {
  m_triggers.emplace_back(
      "Work Progress",
      [this](const dpp::message_create_t &e) { return e.msg.channel_id == m_work_progress_channel; },
      [this](const dpp::message_create_t &e) { Triggers::workProgress(e); });
}

static bool hasRole(const std::vector<dpp::snowflake> &roles, dpp::snowflake role_id) {
  return std::find(roles.begin(), roles.end(), role_id) != roles.end();
}

static bool isAdmin(const dpp::snowflake guild_id, const dpp::snowflake user_id, const std::vector<dpp::snowflake> &roles) {
  dpp::guild *guild = dpp::find_guild(guild_id);
  if(!guild)
    return false;

  if(user_id == guild->owner_id)
    return true;

  for(const auto &role_id : roles) {
    dpp::role *role = dpp::find_role(role_id);
    if(!role) {
      continue;
    }
    if(role->has_manage_guild() || role->has_administrator()) {
      return true;
    }
  }
  return false;
}

void Bot::setWorkProgressChannel(dpp::snowflake channel_id) {
  m_config.set("work_progress_channel", static_cast<uint64_t>(channel_id));
  m_work_progress_channel = channel_id;
}

void Bot::setStaffRole(dpp::snowflake role_id) {
  m_config.set("staff_role_id", static_cast<uint64_t>(role_id));
  m_staff_role_id = role_id;
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
      m_staff_role_id(static_cast<dpp::snowflake>(cfg.getOptional<uint64_t>("staff_role_id"))),
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
    const auto &roles = event.command.member.get_roles();
    if(!hasRole(roles, m_staff_role_id) && !isAdmin(event.command.guild_id, event.command.member.user_id, roles)) {
      event.reply(dpp::message("You are not allowed to use this bot").set_flags(dpp::m_ephemeral));
      return;
    }
    auto it = this->m_commands.find(event.command.get_command_name());
    if(it != this->m_commands.end()) {
      it->second.handler(event);
    }
  });

  m_core.on_autocomplete([this](const dpp::autocomplete_t &event) {
    const auto &roles = event.command.member.get_roles();
    if(!hasRole(roles, m_staff_role_id) && !isAdmin(event.command.guild_id, event.command.member.user_id, roles)) {
      dpp::interaction_response r(dpp::ir_autocomplete_reply);
      m_core.interaction_response_create(event.command.id, event.command.token, r);
      return;
    }

    auto it = this->m_commands.find(event.name);
    if(it == m_commands.end() || !it->second.autocomplete_handler) {
      return;
    }

    auto dispatch = [&](const std::string &key, const dpp::command_option &opt) {
      std::string input{};
      try {
        input = std::get<std::string>(opt.value);
      } catch(std::exception &e) {
        std::cerr << "Failed to get autocomplete value for " << event.name << "/" << key << ": " << e.what() << std::endl;
      }
      it->second.autocomplete_handler(key, input, event);
    };

    for(const auto &option : event.options) {
      if(option.type == dpp::co_sub_command) {
        // Focused option is one level deeper, inside the subcommand
        for(const auto &sub_opt : option.options) {
          if(sub_opt.focused) {
            dispatch(option.name + "/" + sub_opt.name, sub_opt);
            return;
          }
        }
        return;
      }
      if(option.focused) {
        dispatch(option.name, option);
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